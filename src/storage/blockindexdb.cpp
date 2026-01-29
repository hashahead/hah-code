// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blockindexdb.h"

#include <boost/range/adaptor/reversed.hpp>

#include "leveldbeng.h"

#include "dbstruct.h"

using namespace std;
using namespace hnbase;

namespace hashahead
{
namespace storage
{

const uint8 DB_BLOCKINDEX_ROOT_TYPE_BLOCK_INDEX = 0x10;
const uint8 DB_BLOCKINDEX_ROOT_TYPE_BLOCK_NUMBER = 0x20;
const uint8 DB_BLOCKINDEX_ROOT_TYPE_BLOCK_VOTE_RESULT = 0x30;

const uint8 DB_BLOCKINDEX_KEY_TYPE_BLOCK_INDEX = DB_BLOCKINDEX_ROOT_TYPE_BLOCK_INDEX | 0x01;
const uint8 DB_BLOCKINDEX_KEY_TYPE_BLOCK_HEIGHT = DB_BLOCKINDEX_ROOT_TYPE_BLOCK_INDEX | 0x02;
const uint8 DB_BLOCKINDEX_KEY_TYPE_BLOCK_MAX_HEIGHT = DB_BLOCKINDEX_ROOT_TYPE_BLOCK_INDEX | 0x03;

const uint8 DB_BLOCKINDEX_KEY_TYPE_BLOCK_NUMBER = DB_BLOCKINDEX_ROOT_TYPE_BLOCK_NUMBER | 0x01;
const uint8 DB_BLOCKINDEX_KEY_TYPE_LAST_BLOCK_NUMBER = DB_BLOCKINDEX_ROOT_TYPE_BLOCK_NUMBER | 0x02;

const uint8 DB_BLOCKINDEX_KEY_TYPE_BLOCK_VOTE_RESULT = DB_BLOCKINDEX_ROOT_TYPE_BLOCK_VOTE_RESULT | 0x01;
const uint8 DB_BLOCKINDEX_KEY_TYPE_LAST_BLOCK_VOTE = DB_BLOCKINDEX_ROOT_TYPE_BLOCK_VOTE_RESULT | 0x02;
const uint8 DB_BLOCKINDEX_KEY_TYPE_BLOCK_LOCAL_SIGN_FLAG = DB_BLOCKINDEX_ROOT_TYPE_BLOCK_VOTE_RESULT | 0x03;

//////////////////////////////
// CBlockIndexDB

CBlockIndexDB::CBlockIndexDB()
{
}

CBlockIndexDB::~CBlockIndexDB()
{
}

bool CBlockIndexDB::Initialize(const boost::filesystem::path& pathData)
{
    CLevelDBArguments args;
    args.path = (pathData / "blockindex").string();
    args.syncwrite = false;
    CLevelDBEngine* engine = new CLevelDBEngine(args);
    if (!Open(engine))
    {
        StdLog("CBlockIndexDB", "Open db fail");
        delete engine;
        return false;
    }
    return true;
}

void CBlockIndexDB::Deinitialize()
{
    Close();
}

bool CBlockIndexDB::Clear()
{
    return RemoveAll();
}

//-----------------------------------------------------
// block index

bool CBlockIndexDB::AddNewBlockIndex(const CBlockIndex& outline)
{
    CWriteLock wlock(rwAccess);

    if (!TxnBegin())
    {
        StdLog("CBlockIndexDB", "Add new block index: Txn begin fail");
        return false;
    }

    const uint256 hashBlock = outline.GetBlockHash();
    const CChainId nChainId = outline.GetBlockChainId();
    const uint32 nHeight = outline.GetBlockHeight();

    {
        CBufStream ssKey, ssValue;
        ssKey << DB_BLOCKINDEX_KEY_TYPE_BLOCK_INDEX << hashBlock;
        ssValue << outline;
        if (!Write(ssKey, ssValue))
        {
            StdLog("CBlockIndexDB", "Add new block index: Write fail, block: %s", hashBlock.GetHex().c_str());
            return false;
        }
    }

    {
        CBufStream ssKey, ssValue;
        ssKey << DB_BLOCKINDEX_KEY_TYPE_BLOCK_HEIGHT << BSwap32(nChainId) << BSwap32(nHeight) << hashBlock;
        ssValue << outline.GetBlockNumber();
        if (!Write(ssKey, ssValue))
        {
            StdLog("CBlockIndexDB", "Add new block index: Write block height fail, height: %d, block: %s", nHeight, hashBlock.ToString().c_str());
            TxnAbort();
            return false;
        }
    }

    uint32 nOldMaxHeight = 0;
    GetForkMaxHeight(outline.GetOriginHash(), nOldMaxHeight);
    if (nHeight > nOldMaxHeight)
    {
        CBufStream ssKey, ssValue;
        ssKey << DB_BLOCKINDEX_KEY_TYPE_BLOCK_MAX_HEIGHT << BSwap32(nChainId);
        ssValue << nHeight;
        if (!Write(ssKey, ssValue))
        {
            StdLog("CBlockIndexDB", "Add new block index: Write block max height fail, height: %d, block: %s", nHeight, hashBlock.ToString().c_str());
            TxnAbort();
            return false;
        }
    }

    if (!TxnCommit())
    {
        StdLog("CBlockIndexDB", "Add new block index: Txn commit fail");
        TxnAbort();
        return false;
    }
    return true;
}

bool CBlockIndexDB::RemoveBlockIndex(const uint256& hashBlock)
{
    CBufStream ssKey;
    ssKey << DB_BLOCKINDEX_KEY_TYPE_BLOCK_INDEX << hashBlock;
    return Erase(ssKey);
}

bool CBlockIndexDB::RetrieveBlockIndex(const uint256& hashBlock, CBlockIndex& outline)
{
    CBufStream ssKey, ssValue;
    ssKey << DB_BLOCKINDEX_KEY_TYPE_BLOCK_INDEX << hashBlock;
    if (!Read(ssKey, ssValue))
    {
        return false;
    }
    try
    {
        ssValue >> outline;
    }
    catch (std::exception& e)
    {
        hnbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CBlockIndexDB::WalkThroughBlockIndex(CBlockDBWalker& walker)
{
    auto funcWalker = [&](CBufStream& ssKey, CBufStream& ssValue) -> bool {
        try
        {
            CBlockIndex outline;
            ssValue >> outline;
            return walker.Walk(outline);
        }
        catch (std::exception& e)
        {
            hnbase::StdError(__PRETTY_FUNCTION__, e.what());
        }
        return false;
    };

    CBufStream ssKeyBegin, ssKeyPrefix;
    ssKeyBegin << DB_BLOCKINDEX_KEY_TYPE_BLOCK_INDEX;
    ssKeyPrefix << DB_BLOCKINDEX_KEY_TYPE_BLOCK_INDEX;

    return WalkThroughOfPrefix(ssKeyBegin, ssKeyPrefix, funcWalker);
}

bool CBlockIndexDB::RetrieveBlockHashByHeight(const uint256& hashFork, const uint32 nBlockHeight, std::vector<uint256>& vBlockHash)
{
    const CChainId nChainId = CBlock::GetBlockChainIdByHash(hashFork);

    auto funcWalker = [&](CBufStream& ssKey, CBufStream& ssValue) -> bool {
        try
        {
            uint8 nKeyTypeDb;
            CChainId nChainIdDb;
            uint32 nHeightDb;
            uint256 hashBlockDb;
            ssKey >> nKeyTypeDb >> nChainIdDb >> nHeightDb >> hashBlockDb;
            nChainIdDb = BSwap32(nChainIdDb);
            nHeightDb = BSwap32(nHeightDb);
            if (nKeyTypeDb == DB_BLOCKINDEX_KEY_TYPE_BLOCK_HEIGHT && nChainIdDb == nChainId && nHeightDb == nBlockHeight)
            {
                vBlockHash.push_back(hashBlockDb);
                return true;
            }
        }
        catch (std::exception& e)
        {
            hnbase::StdError(__PRETTY_FUNCTION__, e.what());
        }
        return false;
    };

    CBufStream ssKeyBegin, ssKeyPrefix;
    ssKeyBegin << DB_BLOCKINDEX_KEY_TYPE_BLOCK_HEIGHT << BSwap32(nChainId) << BSwap32(nBlockHeight);
    ssKeyPrefix << DB_BLOCKINDEX_KEY_TYPE_BLOCK_HEIGHT << BSwap32(nChainId) << BSwap32(nBlockHeight);

    return WalkThroughOfPrefix(ssKeyBegin, ssKeyPrefix, funcWalker);
}

bool CBlockIndexDB::GetForkMaxHeight(const uint256& hashFork, uint32& nMaxHeight)
{
    const CChainId nChainId = CBlock::GetBlockChainIdByHash(hashFork);

    CBufStream ssKey, ssValue;
    ssKey << DB_BLOCKINDEX_KEY_TYPE_BLOCK_MAX_HEIGHT << BSwap32(nChainId);
    if (!Read(ssKey, ssValue))
    {
        nMaxHeight = 0;
    }
    else
    {
        try
        {
            ssValue >> nMaxHeight;
        }
        catch (std::exception& e)
        {
            hnbase::StdError(__PRETTY_FUNCTION__, e.what());
            nMaxHeight = 0;
        }
    }
    return true;
}

//-----------------------------------------------------
// block number

bool CBlockIndexDB::UpdateBlockNumberBlockLongChain(const uint256& hashFork, const std::vector<std::pair<uint64, uint256>>& vRemoveNumberBlock, const std::vector<std::pair<uint64, uint256>>& mapNewNumberBlock)
{
    CWriteLock wlock(rwAccess);

    if (!TxnBegin())
    {
        StdLog("CBlockIndexDB", "Add block number: Txn begin fail");
        return false;
    }

    const CChainId nChainId = CBlock::GetBlockChainIdByHash(hashFork);

    for (auto& vd : vRemoveNumberBlock)
    {
        const uint64 nBlockNumber = vd.first;
        const uint256& hashBlock = vd.second;

        CBufStream ssKey;
        ssKey << DB_BLOCKINDEX_KEY_TYPE_BLOCK_NUMBER << BSwap32(nChainId) << BSwap64(nBlockNumber);
        if (!Erase(ssKey))
        {
            StdLog("CBlockIndexDB", "Add block number: Remove block number fail, number: %lu, block: %s", nBlockNumber, hashBlock.ToString().c_str());
            TxnAbort();
            return false;
        }
    }

    for (auto& vd : mapNewNumberBlock)
    {
        const uint64 nBlockNumber = vd.first;
        const uint256& hashBlock = vd.second;

        CBufStream ssKey, ssValue;
        ssKey << DB_BLOCKINDEX_KEY_TYPE_BLOCK_NUMBER << BSwap32(nChainId) << BSwap64(nBlockNumber);
        ssValue << hashBlock;
        if (!Write(ssKey, ssValue))
        {
            StdLog("CBlockIndexDB", "Add block number: Write block number fail, number: %lu, block: %s", nBlockNumber, hashBlock.ToString().c_str());
            TxnAbort();
            return false;
        }
    }

    if (mapNewNumberBlock.size() > 0)
    {
        auto& lastBlock = mapNewNumberBlock[mapNewNumberBlock.size() - 1];
        const uint64 nLastNumber = lastBlock.first;
        const uint256& hashLastBlock = lastBlock.second;
        CBufStream ssKey, ssValue;
        ssKey << DB_BLOCKINDEX_KEY_TYPE_LAST_BLOCK_NUMBER << nChainId;
        ssValue << nLastNumber << hashLastBlock;
        if (!Write(ssKey, ssValue))
        {
            StdLog("CBlockIndexDB", "Add block number: Write last block number fail, last number: %lu, last block: %s", nLastNumber, hashLastBlock.ToString().c_str());
            TxnAbort();
            return false;
        }
    }

    if (!TxnCommit())
    {
        StdLog("CBlockIndexDB", "Add block number: Txn commit fail");
        TxnAbort();
        return false;
    }
    return true;
}

bool CBlockIndexDB::RetrieveBlockHashByNumber(const uint256& hashFork, const uint64 nBlockNumber, uint256& hashBlock)
{
    CReadLock rlock(rwAccess);

    const CChainId nChainId = CBlock::GetBlockChainIdByHash(hashFork);

    CBufStream ssKey, ssValue;
    ssKey << DB_BLOCKINDEX_KEY_TYPE_BLOCK_NUMBER << BSwap32(nChainId) << BSwap64(nBlockNumber);
    if (!Read(ssKey, ssValue))
    {
        return false;
    }
    try
    {
        ssValue >> hashBlock;
    }
    catch (std::exception& e)
    {
        hnbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

///////////////////////////////////
// block vote result

bool CBlockIndexDB::AddBlockVoteResult(const uint256& hashBlock, const bool fLongChain, const bytes& btBitmap, const bytes& btAggSig, const bool fAtChain, const uint256& hashAtBlock)
{
    CWriteLock wlock(rwAccess);

    if (!TxnBegin())
    {
        StdLog("CBlockIndexDB", "Add block vote result: Txn begin fail");
        return false;
    }

    {
        CBufStream ssKey, ssValue;
        ssKey << DB_BLOCKINDEX_KEY_TYPE_BLOCK_VOTE_RESULT << hashBlock;
        ssValue << btBitmap << btAggSig << fAtChain << hashAtBlock;
        if (!Write(ssKey, ssValue))
        {
            StdLog("CBlockIndexDB", "Add block vote result: Write block vote result fail, block: %s", hashBlock.GetHex().c_str());
            TxnAbort();
            return false;
        }
    }

    if (fLongChain)
    {
        CBlockIndex outline;
        if (!RetrieveBlockIndex(hashBlock, outline))
        {
            StdLog("CBlockIndexDB", "Add block vote result: Retrieve block index fail, block: %s", hashBlock.GetHex().c_str());
            TxnAbort();
            return false;
        }

        uint256 hashLastBlock;
        uint64 nLastNumber;
        bool fLastAtChain = false;
        {
            CBufStream ssKey, ssValue;
            ssKey << DB_BLOCKINDEX_KEY_TYPE_LAST_BLOCK_VOTE << outline.hashOrigin;
            if (Read(ssKey, ssValue))
            {
                try
                {
                    bytes btTempBitmap;
                    bytes btTempAggSig;
                    uint256 hashLastAtBlock;
                    ssValue >> hashLastBlock >> nLastNumber >> btTempBitmap >> btTempAggSig >> fLastAtChain >> hashLastAtBlock;
                }
                catch (std::exception& e)
                {
                    hnbase::StdError(__PRETTY_FUNCTION__, e.what());
                    hashLastBlock = 0;
                }
            }
        }

        bool fWriteLast = false;
        do
        {
            if (hashLastBlock.IsNull())
            {
                fWriteLast = true;
                break;
            }
            if (!hashLastBlock.IsNull() && hashLastBlock == hashBlock && !fLastAtChain && fAtChain)
            {
                fWriteLast = true;
                break;
            }
            if (CBlock::GetBlockHeightByHash(hashLastBlock) <= CBlock::GetBlockHeightByHash(hashBlock) && nLastNumber < outline.GetBlockNumber())
            {
                fWriteLast = true;
                break;
            }
        } while (0);

        if (fWriteLast)
        {
            CBufStream ssKey, ssValue;
            ssKey << DB_BLOCKINDEX_KEY_TYPE_LAST_BLOCK_VOTE << outline.hashOrigin;
            ssValue << hashBlock << outline.GetBlockNumber() << btBitmap << btAggSig << fAtChain << hashAtBlock;
            if (!Write(ssKey, ssValue))
            {
                StdLog("CBlockIndexDB", "Add block vote result: Write last block vote fail, block: %s", hashBlock.GetHex().c_str());
                TxnAbort();
                return false;
            }
        }
    }

    if (!TxnCommit())
    {
        StdLog("CBlockIndexDB", "Add block vote result: Txn commit fail");
        TxnAbort();
        return false;
    }
    return true;
}

bool CBlockIndexDB::RemoveBlockVoteResult(const uint256& hashBlock)
{
    CWriteLock wlock(rwAccess);
    CBufStream ssKey;
    ssKey << DB_BLOCKINDEX_KEY_TYPE_BLOCK_VOTE_RESULT << hashBlock;
    return Erase(ssKey);
}

bool CBlockIndexDB::RetrieveBlockVoteResult(const uint256& hashBlock, bytes& btBitmap, bytes& btAggSig, bool& fAtChain, uint256& hashAtBlock)
{
    CReadLock rlock(rwAccess);
    CBufStream ssKey, ssValue;
    ssKey << DB_BLOCKINDEX_KEY_TYPE_BLOCK_VOTE_RESULT << hashBlock;
    if (!Read(ssKey, ssValue))
    {
        return false;
    }
    try
    {
        ssValue >> btBitmap >> btAggSig >> fAtChain >> hashAtBlock;
    }
    catch (std::exception& e)
    {
        hnbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CBlockIndexDB::VerifyBlockNumberContext(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode)
{
    if (!ReadTrieRoot(DB_BLOCKINDEX_ROOT_TYPE_BLOCK_NUMBER, hashBlock, hashRoot))
    {
        return false;
    }

    if (fVerifyAllNode)
    {
        std::map<uint256, CTrieValue> mapCacheNode;
        if (!dbTrie.CheckTrieNode(hashRoot, mapCacheNode))
        {
            return false;
        }
    }

    uint256 hashPrevRoot;
    if (hashBlock != hashFork)
    {
        if (!ReadTrieRoot(DB_BLOCKINDEX_ROOT_TYPE_BLOCK_NUMBER, hashPrevBlock, hashPrevRoot))
        {
            return false;
        }
    }

    uint256 hashRootPrevDb;
    uint256 hashBlockLocalDb;
    if (!GetPrevRoot(DB_BLOCKINDEX_ROOT_TYPE_BLOCK_NUMBER, hashRoot, hashRootPrevDb, hashBlockLocalDb))
    {
        return false;
    }
    if (hashRootPrevDb != hashPrevRoot || hashBlockLocalDb != hashBlock)
    {
        return false;
    }
    return true;
}

///////////////////////////////////
bool CBlockIndexDB::WriteTrieRoot(const uint8 nRootType, const uint256& hashBlock, const uint256& hashTrieRoot)
{
    hnbase::CBufStream ssKey, ssValue;
    ssKey << nRootType << DB_BLOCKINDEX_KEY_ID_TRIEROOT << hashBlock;
    ssValue << hashTrieRoot;
    return dbTrie.WriteExtKv(ssKey, ssValue);
}

bool CBlockIndexDB::ReadTrieRoot(const uint8 nRootType, const uint256& hashBlock, uint256& hashTrieRoot)
{
    if (hashBlock == 0)
    {
        hashTrieRoot = 0;
        return true;
    }

    hnbase::CBufStream ssKey, ssValue;
    ssKey << nRootType << DB_BLOCKINDEX_KEY_ID_TRIEROOT << hashBlock;
    if (!dbTrie.ReadExtKv(ssKey, ssValue))
    {
        return false;
    }

    try
    {
        ssValue >> hashTrieRoot;
    }
    catch (std::exception& e)
    {
        hnbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

void CBlockIndexDB::AddPrevRoot(const uint8 nRootType, const uint256& hashPrevRoot, const uint256& hashBlock, bytesmap& mapKv)
{
    hnbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;

    ssKey << nRootType << DB_BLOCKINDEX_KEY_ID_PREVROOT;
    ssKey.GetData(btKey);

    ssValue << hashPrevRoot << hashBlock;
    ssValue.GetData(btValue);

    mapKv.insert(make_pair(btKey, btValue));
}

bool CBlockIndexDB::GetPrevRoot(const uint8 nRootType, const uint256& hashRoot, uint256& hashPrevRoot, uint256& hashBlock)
{
    hnbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << nRootType << DB_BLOCKINDEX_KEY_ID_PREVROOT;
    ssKey.GetData(btKey);
    if (!dbTrie.Retrieve(hashRoot, btKey, btValue))
    {
        return false;
    }
    try
    {
        ssValue.Write((char*)(btValue.data()), btValue.size());
        ssValue >> hashPrevRoot >> hashBlock;
    }
    catch (std::exception& e)
    {
        hnbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

} // namespace storage
} // namespace hashahead
