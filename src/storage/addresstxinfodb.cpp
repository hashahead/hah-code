// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "addresstxinfodb.h"

#include <boost/range/adaptor/reversed.hpp>

#include "leveldbeng.h"

#include "block.h"

using namespace std;
using namespace hnbase;

namespace hashahead
{
namespace storage
{

#define MAX_FETCH_ADDRESS_TX_COUNT 1000

const uint8 DB_ADDRESS_TXINFO_KEY_TYPE_ADDRESS_TX_INFO = 0x10;
const uint8 DB_ADDRESS_TXINFO_KEY_TYPE_ADDRESS_TX_COUNT = 0x11;
const uint8 DB_ADDRESS_TXINFO_KEY_TYPE_BLOCK_ADDRESS_TX_DATA = 0x12;
const uint8 DB_ADDRESS_TXINFO_KEY_TYPE_BLOCK_ADDRESS_TX_RANGE = 0x13;
const uint8 DB_ADDRESS_TXINFO_KEY_TYPE_BLOCK_LASTBLOCK = 0x14;
const uint8 DB_ADDRESS_TXINFO_KEY_TYPE_BLOCK_PREVBLOCK = 0x15;

const uint8 DB_ADDRESS_TXINFO_KEY_TYPE_TOKEN_TX_INFO = 0x16;
const uint8 DB_ADDRESS_TXINFO_KEY_TYPE_TOKEN_ADDRESS_TX_COUNT = 0x17;
const uint8 DB_ADDRESS_TXINFO_KEY_TYPE_BLOCK_TOKEN_ADDRESS_TX_RANGE = 0x18;

#define DB_ADDRESS_TXINFO_KEY_ID_LAST_BLOCK string("lastblock")

//////////////////////////////
// CForkAddressTxInfoDB

CForkAddressTxInfoDB::CForkAddressTxInfoDB()
{
}

CForkAddressTxInfoDB::~CForkAddressTxInfoDB()
{
}

bool CForkAddressTxInfoDB::Initialize(const uint256& hashForkIn, const boost::filesystem::path& pathData)
{
    CLevelDBArguments args;
    args.path = pathData.string();
    args.syncwrite = false;
    CLevelDBEngine* engine = new CLevelDBEngine(args);
    if (!Open(engine))
    {
        StdLog("CForkAddressTxInfoDB", "Open db fail");
        delete engine;
        return false;
    }
    hashFork = hashForkIn;
    return true;
}

void CForkAddressTxInfoDB::Deinitialize()
{
    Close();
}

bool CForkAddressTxInfoDB::AddAddressTxInfo(const uint256& hashPrevBlock, const uint256& hashBlock, const uint64 nBlockNumber,
                                            const std::map<CDestination, std::vector<CDestTxInfo>>& mapAddressTxInfo,
                                            const std::map<CDestination, std::vector<CTokenTransRecord>>& mapTokenRecord)
{
    CWriteLock wlock(rwAccess);

    if (!TxnBegin())
    {
        StdLog("CForkAddressTxInfoDB", "Add address tx info: Txn begin fail, block: %s", hashBlock.ToString().c_str());
        return false;
    }

    {
        hnbase::CBufStream ssKey, ssValue;
        ssKey << DB_ADDRESS_TXINFO_KEY_TYPE_BLOCK_ADDRESS_TX_DATA << hashBlock;
        ssValue << mapAddressTxInfo << mapTokenRecord;
        if (!Write(ssKey, ssValue))
        {
            StdLog("CForkAddressTxInfoDB", "Add address tx info: Write tx info fail, block: %s", hashBlock.ToString().c_str());
            TxnAbort();
            return false;
        }
    }

    {
        hnbase::CBufStream ssKey, ssValue;
        ssKey << DB_ADDRESS_TXINFO_KEY_TYPE_BLOCK_PREVBLOCK << hashBlock;
        ssValue << hashPrevBlock << nBlockNumber;
        if (!Write(ssKey, ssValue))
        {
            StdLog("CForkAddressTxInfoDB", "Add address tx info: Write block prev fail, block: %s", hashBlock.ToString().c_str());
            TxnAbort();
            return false;
        }
    }

    if (!TxnCommit())
    {
        StdLog("CForkAddressTxInfoDB", "Add address tx info: Txn commit fail, block: %s", hashBlock.ToString().c_str());
        TxnAbort();
        return false;
    }

    if (setCacheBlockHash.empty())
    {
        WalkThroughAllBlockHash(setCacheBlockHash);
    }
    setCacheBlockHash.insert(hashBlock);
    return true;
}

bool CForkAddressTxInfoDB::UpdateAddressTxInfoBlockLongChain(const vector<uint256>& vRemoveBlock, const vector<uint256>& vAddBlock)
{
    CWriteLock wlock(rwAccess);

    if (!TxnBegin())
    {
        StdLog("CForkAddressTxInfoDB", "Update address tx info block longchain: Txn begin fail");
        return false;
    }

    for (auto& hashBlock : vRemoveBlock)
    {
        RemoveLongChainlock(hashBlock);
    }
    for (auto& hashBlock : vAddBlock)
    {
        if (!AddLongChainBlock(hashBlock))
        {
            StdLog("CForkAddressTxInfoDB", "Update address tx info block longchain: Add longchain fail");
            TxnAbort();
            return false;
        }
    }
    ClearOldBlockData();

    if (!TxnCommit())
    {
        StdLog("CForkAddressTxInfoDB", "Update address tx info block longchain: Txn commit fail");
        TxnAbort();
        return false;
    }
    return true;
}

bool CForkAddressTxInfoDB::GetAddressTxCount(const CDestination& address, uint64& nTxCount)
{
    CReadLock rlock(rwAccess);
    return ReadAddressTxCount(address, nTxCount);
}

bool CForkAddressTxInfoDB::RetrieveAddressTxInfo(const CDestination& address, const uint64 nTxIndex, CDestTxInfo& ctxAddressTxInfo)
{
    CReadLock rlock(rwAccess);
    try
    {
        hnbase::CBufStream ssKey, ssValue;
        ssKey << DB_ADDRESS_TXINFO_KEY_TYPE_ADDRESS_TX_INFO << address << BSwap64(nTxIndex);
        if (!Read(ssKey, ssValue))
        {
            return false;
        }
        ssValue >> ctxAddressTxInfo;
    }
    catch (exception& e)
    {
        hnbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CForkAddressTxInfoDB::ListAddressTxInfo(const CDestination& address, const uint64 nBeginTxIndex, const uint64 nGetTxCount, const bool fReverse, vector<CDestTxInfo>& vAddressTxInfo)
{
    CReadLock rlock(rwAccess);

    uint64 nGetBeginTxIndex = nBeginTxIndex;
    uint64 nGetTxCountInner = nGetTxCount;
    if (nGetTxCount == 0 || nGetTxCount > MAX_FETCH_ADDRESS_TX_COUNT)
    {
        nGetTxCountInner = MAX_FETCH_ADDRESS_TX_COUNT;
    }
    if (fReverse)
    {
        uint64 nTxCount = 0;
        if (!ReadAddressTxCount(address, nTxCount))
        {
            return false;
        }
        if (nGetBeginTxIndex >= nTxCount)
        {
            return true;
        }
        if (nGetBeginTxIndex + nGetTxCountInner > nTxCount)
        {
            nGetTxCountInner = nTxCount - nGetBeginTxIndex;
            nGetBeginTxIndex = 0;
        }
        else
        {
            nGetBeginTxIndex = nTxCount - (nGetBeginTxIndex + nGetTxCountInner);
        }
    }

    if (!WalkThroughAddressTxInfo(address, nGetBeginTxIndex, nGetTxCountInner, vAddressTxInfo))
    {
        StdLog("CForkAddressTxInfoDB", "List address tx info: Walk through fail, address: %s", address.ToString().c_str());
        return false;
    }

    if (fReverse)
    {
        reverse(vAddressTxInfo.begin(), vAddressTxInfo.end());
    }
    return true;
}

bool CForkAddressTxInfoDB::ListTokenTx(const CDestination& destContractAddress, const CDestination& destUserAddress, const uint64 nPageNumber, const uint64 nPageSize,
                                       const bool fReverse, uint64& nTotalRecordCount, uint64& nPageCount, std::vector<std::pair<uint64, CTokenTransRecord>>& vTokenTxRecord)
{
    CReadLock rlock(rwAccess);

    if (destContractAddress.IsNull() || nPageSize == 0)
    {
        return false;
    }

    CDestination destGetAddress;
    if (destUserAddress.IsNull())
    {
        destGetAddress = destContractAddress;
    }
    else
    {
        destGetAddress = destUserAddress;
    }

    if (!ReadTokenTxCount(destContractAddress, destGetAddress, nTotalRecordCount) || nTotalRecordCount == 0)
    {
        nTotalRecordCount = 0;
        nPageCount = 0;
        return true;
    }
    nPageCount = nTotalRecordCount / nPageSize;
    if (nTotalRecordCount % nPageSize != 0)
    {
        nPageCount++;
    }

    uint64 nBeginTxIndex = nPageNumber * nPageSize;
    uint64 nGetTxCountInner = nPageSize;
    if (nBeginTxIndex >= nTotalRecordCount)
    {
        return true;
    }
    if (fReverse)
    {
        if (nBeginTxIndex + nGetTxCountInner > nTotalRecordCount)
        {
            nGetTxCountInner = nTotalRecordCount - nBeginTxIndex;
            nBeginTxIndex = 0;
        }
        else
        {
            nBeginTxIndex = nTotalRecordCount - (nBeginTxIndex + nGetTxCountInner);
        }
    }

    if (!WalkThroughTokenTxInfo(destContractAddress, destUserAddress, nBeginTxIndex, nGetTxCountInner, vTokenTxRecord))
    {
        StdLog("CForkAddressTxInfoDB", "List token tx info: Walk through fail, contract address: %s, address: %s", destContractAddress.ToString().c_str(), destUserAddress.ToString().c_str());
        return false;
    }
    if (fReverse)
    {
        reverse(vTokenTxRecord.begin(), vTokenTxRecord.end());
    }
    return true;
}

bool CForkAddressTxInfoDB::WalkThroughSnapshotAddressTxKv(const uint64 nLastBlockNumber, WalkerAddressTxKvFunc fnWalker)
{
    auto funcWalker = [&](CBufStream& ssKey, CBufStream& ssValue) -> bool {
        try
        {
            bytes btKey, btValue;
            ssKey.GetData(btKey);
            ssValue.GetData(btValue);

            uint8 nKeyType;
            ssKey >> nKeyType;
            if (nKeyType == DB_ADDRESS_TXINFO_KEY_TYPE_ADDRESS_TX_INFO)
            {
                CDestination addressDb;
                uint64 nIndexDb;
                ssKey >> addressDb >> nIndexDb;
                nIndexDb = BSwap64(nIndexDb);

                CDestTxInfo txIndexDb;
                ssValue >> txIndexDb;

                if (txIndexDb.nBlockNumber <= nLastBlockNumber)
                {
                    return fnWalker(addressDb, nIndexDb, btKey, btValue);
                }
            }
            return true;
        }
        catch (exception& e)
        {
            hnbase::StdError(__PRETTY_FUNCTION__, e.what());
        }
        return false;
    };

    CBufStream ssKeyBegin, ssKeyPrefix;
    ssKeyBegin << DB_ADDRESS_TXINFO_KEY_TYPE_ADDRESS_TX_INFO;
    ssKeyPrefix << DB_ADDRESS_TXINFO_KEY_TYPE_ADDRESS_TX_INFO;

    return WalkThroughOfPrefix(ssKeyBegin, ssKeyPrefix, funcWalker);
}

bool CForkAddressTxInfoDB::WalkThroughSnapshotTokenTxKv(const uint64 nLastBlockNumber, WalkerTokenTxKvFunc fnWalker)
{
    auto funcWalker = [&](CBufStream& ssKey, CBufStream& ssValue) -> bool {
        try
        {
            bytes btKey, btValue;
            ssKey.GetData(btKey);
            ssValue.GetData(btValue);

            uint8 nKeyType;
            ssKey >> nKeyType;
            if (nKeyType == DB_ADDRESS_TXINFO_KEY_TYPE_TOKEN_TX_INFO)
            {
                CDestination destContractDb, destUserAddressDb;
                uint64 nIndexDb;
                ssKey >> destContractDb >> destUserAddressDb >> nIndexDb;
                nIndexDb = BSwap64(nIndexDb);

                CTokenTransRecord tokenTransRecord;
                ssValue >> tokenTransRecord;

                if (tokenTransRecord.nBlockNumber <= nLastBlockNumber)
                {
                    return fnWalker(destContractDb, destUserAddressDb, nIndexDb, btKey, btValue);
                }
            }
            return true;
        }
        catch (exception& e)
        {
            hnbase::StdError(__PRETTY_FUNCTION__, e.what());
        }
        return false;
    };

    CBufStream ssKeyBegin, ssKeyPrefix;
    ssKeyBegin << DB_ADDRESS_TXINFO_KEY_TYPE_TOKEN_TX_INFO;
    ssKeyPrefix << DB_ADDRESS_TXINFO_KEY_TYPE_TOKEN_TX_INFO;

    return WalkThroughOfPrefix(ssKeyBegin, ssKeyPrefix, funcWalker);
}

bool CForkAddressTxInfoDB::WriteSnapshotAddressTxKvData(const bytes& btKey, const bytes& btValue)
{
    CBufStream ssKey(btKey);
    CBufStream ssValue(btValue);
    return Write(ssKey, ssValue);
}

bool CForkAddressTxInfoDB::WriteSnapshotAddressTxCount(const uint256& hashLastBlock, const std::map<CDestination, uint64>& mapAddressTxCount)
{
    for (const auto& kv : mapAddressTxCount)
    {
        if (!WriteAddressTxCount(kv.first, kv.second))
        {
            StdLog("CForkAddressTxInfoDB", "Write snapshot address tx count: Write address tx count failed, address: %s", kv.first.ToString().c_str());
            return false;
        }
    }
    if (!WriteLastBlock(hashLastBlock))
    {
        StdLog("CForkAddressTxInfoDB", "Write snapshot address tx count: Write last block failed, last block: %s", hashLastBlock.ToString().c_str());
        return false;
    }
    return true;
}

bool CForkAddressTxInfoDB::WriteSnapshotTokenTxCount(const std::map<CDestination, std::map<CDestination, uint64>>& mapTokenTxCount)
{
    for (const auto& kv : mapTokenTxCount)
    {
        for (const auto& kv2 : kv.second)
        {
            if (!WriteTokenTxCount(kv.first, kv2.first, kv2.second))
            {
                StdLog("CForkAddressTxInfoDB", "Write snapshot token tx count: Write token tx count failed, contract address: %s, user address: %s", kv.first.ToString().c_str(), kv2.first.ToString().c_str());
                return false;
            }
        }
    }
    return true;
}

//------------------------------------------------------------------------------------------
bool CForkAddressTxInfoDB::GetBlockAddressTxInfo(const uint256& hashBlock, map<CDestination, vector<CDestTxInfo>>& mapAddressTxInfo, map<CDestination, vector<CTokenTransRecord>>& mapTokenRecord)
{
    try
    {
        hnbase::CBufStream ssKey, ssValue;
        ssKey << DB_ADDRESS_TXINFO_KEY_TYPE_BLOCK_ADDRESS_TX_DATA << hashBlock;
        if (!Read(ssKey, ssValue))
        {
            return false;
        }
        ssValue >> mapAddressTxInfo >> mapTokenRecord;
    }
    catch (exception& e)
    {
        hnbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CForkAddressTxInfoDB::RetrieveAddressTxInfo(const uint256& hashBlock, const CDestination& dest, const uint64 nTxIndex, CDestTxInfo& ctxtAddressTxInfo)
{
    uint256 hashRoot;
    if (!ReadTrieRoot(hashBlock, hashRoot))
    {
        StdLog("CForkAddressTxInfoDB", "Retrieve address tx info: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    hnbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_ADDRESS_TXINFO_KEY_TYPE_ADDRESS << dest << BSwap64(nTxIndex);
    ssKey.GetData(btKey);
    if (!dbTrie.Retrieve(hashRoot, btKey, btValue))
    {
        StdLog("CForkAddressTxInfoDB", "Retrieve address tx info: Trie retrieve kv fail, root: %s, block: %s",
               hashRoot.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    try
    {
        ssValue.Write((char*)(btValue.data()), btValue.size());
        ssValue >> ctxtAddressTxInfo;
    }
    catch (std::exception& e)
    {
        hnbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CForkAddressTxInfoDB::ListAddressTxInfo(const uint256& hashBlock, const CDestination& dest, const uint64 nBeginTxIndex, const uint64 nGetTxCount, const bool fReverse, std::vector<CDestTxInfo>& vAddressTxInfo)
{
    uint256 hashRoot;
    if (!ReadTrieRoot(hashBlock, hashRoot))
    {
        StdLog("CForkAddressTxInfoDB", "List address tx info: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }

    hnbase::CBufStream ssKeyPrefix;
    ssKeyPrefix << DB_ADDRESS_TXINFO_KEY_TYPE_ADDRESS << dest;
    bytes btKeyPrefix;
    ssKeyPrefix.GetData(btKeyPrefix);

    bytes btBeginKeyTail;
    if (nBeginTxIndex > 0)
    {
        hnbase::CBufStream ss;
        ss << BSwap64(nBeginTxIndex);
        ss.GetData(btBeginKeyTail);
    }
    uint64 nGetCountInner = 0;
    if (nGetTxCount == 0 || nGetTxCount > MAX_FETCH_ADDRESS_TX_COUNT)
    {
        nGetCountInner = MAX_FETCH_ADDRESS_TX_COUNT;
    }
    else
    {
        nGetCountInner = nGetTxCount;
    }

    CListAddressTxInfoTrieDBWalker walker(nGetCountInner, vAddressTxInfo);
    if (!dbTrie.WalkThroughTrie(hashRoot, walker, btKeyPrefix, btBeginKeyTail, fReverse))
    {
        StdLog("CForkAddressTxInfoDB", "List address tx info: Walk through trie fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CForkAddressTxInfoDB::VerifyAddressTxInfo(const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode)
{
    if (!ReadTrieRoot(hashBlock, hashRoot))
    {
        StdLog("CForkAddressTxInfoDB", "Verify address tx info: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }

    if (fVerifyAllNode)
    {
        std::map<uint256, CTrieValue> mapCacheNode;
        if (!dbTrie.CheckTrieNode(hashRoot, mapCacheNode))
        {
            StdLog("CForkAddressTxInfoDB", "Verify address tx info: Check trie node fail, root: %s, block: %s", hashRoot.GetHex().c_str(), hashBlock.GetHex().c_str());
            return false;
        }
    }

    uint256 hashPrevRoot;
    if (hashBlock != hashFork)
    {
        if (!ReadTrieRoot(hashPrevBlock, hashPrevRoot))
        {
            StdLog("CForkAddressTxInfoDB", "Verify address tx info: Read prev trie root fail, prev block: %s", hashPrevBlock.GetHex().c_str());
            return false;
        }
    }

    uint256 hashRootPrevDb;
    uint256 hashBlockLocalDb;
    if (!GetPrevRoot(hashRoot, hashRootPrevDb, hashBlockLocalDb))
    {
        StdLog("CForkAddressTxInfoDB", "Verify address tx info: Get prev root fail, hashRoot: %s, hashPrevRoot: %s, hashBlock: %s",
               hashRoot.GetHex().c_str(), hashPrevRoot.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    if (hashRootPrevDb != hashPrevRoot || hashBlockLocalDb != hashBlock)
    {
        StdLog("CForkAddressTxInfoDB", "Verify address tx info: Root error, hashRootPrevDb: %s, hashPrevRoot: %s, hashBlockLocalDb: %s, hashBlock: %s",
               hashRootPrevDb.GetHex().c_str(), hashPrevRoot.GetHex().c_str(),
               hashBlockLocalDb.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

///////////////////////////////////
bool CForkAddressTxInfoDB::WriteTrieRoot(const uint256& hashBlock, const uint256& hashTrieRoot)
{
    hnbase::CBufStream ssKey, ssValue;
    ssKey << DB_ADDRESS_TXINFO_KEY_TYPE_TRIEROOT << hashBlock;
    ssValue << hashTrieRoot;
    return dbTrie.WriteExtKv(ssKey, ssValue);
}

bool CForkAddressTxInfoDB::ReadTrieRoot(const uint256& hashBlock, uint256& hashTrieRoot)
{
    if (hashBlock == 0)
    {
        hashTrieRoot = 0;
        return true;
    }

    hnbase::CBufStream ssKey, ssValue;
    ssKey << DB_ADDRESS_TXINFO_KEY_TYPE_TRIEROOT << hashBlock;
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

void CForkAddressTxInfoDB::AddPrevRoot(const uint256& hashPrevRoot, const uint256& hashBlock, bytesmap& mapKv)
{
    hnbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;

    ssKey << DB_ADDRESS_TXINFO_KEY_TYPE_PREVROOT << DB_ADDRESS_TXINFO_KEY_ID_PREVROOT;
    ssKey.GetData(btKey);

    ssValue << hashPrevRoot << hashBlock;
    ssValue.GetData(btValue);

    mapKv.insert(make_pair(btKey, btValue));
}

bool CForkAddressTxInfoDB::GetPrevRoot(const uint256& hashRoot, uint256& hashPrevRoot, uint256& hashBlock)
{
    hnbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_ADDRESS_TXINFO_KEY_TYPE_PREVROOT << DB_ADDRESS_TXINFO_KEY_ID_PREVROOT;
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

void CForkAddressTxInfoDB::WriteAddressLast(const CDestination& dest, const uint64 nTxCount, bytesmap& mapKv)
{
    hnbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;

    ssKey << DB_ADDRESS_TXINFO_KEY_TYPE_LASTDEST << dest;
    ssKey.GetData(btKey);

    ssValue << nTxCount;
    ssValue.GetData(btValue);

    mapKv.insert(make_pair(btKey, btValue));
}

bool CForkAddressTxInfoDB::ReadAddressLast(const uint256& hashRoot, const CDestination& dest, uint64& nTxCount)
{
    hnbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_ADDRESS_TXINFO_KEY_TYPE_LASTDEST << dest;
    ssKey.GetData(btKey);
    if (!dbTrie.Retrieve(hashRoot, btKey, btValue))
    {
        return false;
    }
    try
    {
        ssValue.Write((char*)(btValue.data()), btValue.size());
        ssValue >> nTxCount;
    }
    catch (std::exception& e)
    {
        hnbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

//////////////////////////////
// CAddressTxInfoDB

bool CAddressTxInfoDB::Initialize(const boost::filesystem::path& pathData)
{
    pathAddress = pathData / "addresstx";

    if (!boost::filesystem::exists(pathAddress))
    {
        boost::filesystem::create_directories(pathAddress);
    }

    if (!boost::filesystem::is_directory(pathAddress))
    {
        return false;
    }
    return true;
}

void CAddressTxInfoDB::Deinitialize()
{
    CWriteLock wlock(rwAccess);
    mapAddressTxInfoDB.clear();
}

bool CAddressTxInfoDB::ExistFork(const uint256& hashFork)
{
    CReadLock rlock(rwAccess);
    return (mapAddressTxInfoDB.find(hashFork) != mapAddressTxInfoDB.end());
}

bool CAddressTxInfoDB::LoadFork(const uint256& hashFork)
{
    CWriteLock wlock(rwAccess);

    auto it = mapAddressTxInfoDB.find(hashFork);
    if (it != mapAddressTxInfoDB.end())
    {
        return true;
    }

    std::shared_ptr<CForkAddressTxInfoDB> spAddress(new CForkAddressTxInfoDB());
    if (spAddress == nullptr)
    {
        return false;
    }
    if (!spAddress->Initialize(hashFork, pathAddress / hashFork.GetHex()))
    {
        return false;
    }
    mapAddressTxInfoDB.insert(make_pair(hashFork, spAddress));
    return true;
}

void CAddressTxInfoDB::RemoveFork(const uint256& hashFork)
{
    CWriteLock wlock(rwAccess);

    auto it = mapAddressTxInfoDB.find(hashFork);
    if (it != mapAddressTxInfoDB.end())
    {
        it->second->RemoveAll();
        mapAddressTxInfoDB.erase(it);
    }

    boost::filesystem::path forkPath = pathAddress / hashFork.GetHex();
    if (boost::filesystem::exists(forkPath))
    {
        boost::filesystem::remove_all(forkPath);
    }
}

bool CAddressTxInfoDB::AddNewFork(const uint256& hashFork)
{
    RemoveFork(hashFork);
    return LoadFork(hashFork);
}

void CAddressTxInfoDB::Clear()
{
    CWriteLock wlock(rwAccess);

    auto it = mapAddressTxInfoDB.begin();
    while (it != mapAddressTxInfoDB.end())
    {
        it->second->RemoveAll();
        mapAddressTxInfoDB.erase(it++);
    }
}

bool CAddressTxInfoDB::AddAddressTxInfo(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, const uint64 nBlockNumber, const std::map<CDestination, std::vector<CDestTxInfo>>& mapAddressTxInfo, uint256& hashNewRoot)
{
    CReadLock rlock(rwAccess);

    auto it = mapAddressTxInfoDB.find(hashFork);
    if (it != mapAddressTxInfoDB.end())
    {
        return it->second->AddAddressTxInfo(hashPrevBlock, hashBlock, nBlockNumber, mapAddressTxInfo, hashNewRoot);
    }
    return false;
}

bool CAddressTxInfoDB::GetAddressTxCount(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, uint64& nTxCount)
{
    CReadLock rlock(rwAccess);

    auto it = mapAddressTxInfoDB.find(hashFork);
    if (it != mapAddressTxInfoDB.end())
    {
        return it->second->GetAddressTxCount(hashBlock, dest, nTxCount);
    }
    return false;
}

bool CAddressTxInfoDB::RetrieveAddressTxInfo(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, const uint64 nTxIndex, CDestTxInfo& ctxtAddressTxInfo)
{
    CReadLock rlock(rwAccess);

    auto it = mapAddressTxInfoDB.find(hashFork);
    if (it != mapAddressTxInfoDB.end())
    {
        return it->second->RetrieveAddressTxInfo(hashBlock, dest, nTxIndex, ctxtAddressTxInfo);
    }
    return false;
}

bool CAddressTxInfoDB::ListAddressTxInfo(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, const uint64 nBeginTxIndex, const uint64 nGetTxCount, const bool fReverse, std::vector<CDestTxInfo>& vAddressTxInfo)
{
    CReadLock rlock(rwAccess);

    auto it = mapAddressTxInfoDB.find(hashFork);
    if (it != mapAddressTxInfoDB.end())
    {
        return it->second->ListAddressTxInfo(hashBlock, dest, nBeginTxIndex, nGetTxCount, fReverse, vAddressTxInfo);
    }
    return false;
}

bool CAddressTxInfoDB::VerifyAddressTxInfo(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode)
{
    CReadLock rlock(rwAccess);

    auto it = mapAddressTxInfoDB.find(hashFork);
    if (it != mapAddressTxInfoDB.end())
    {
        return it->second->VerifyAddressTxInfo(hashPrevBlock, hashBlock, hashRoot, fVerifyAllNode);
    }
    return false;
}

} // namespace storage
} // namespace hashahead
