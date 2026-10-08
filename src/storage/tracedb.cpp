// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "tracedb.h"

#include <boost/bind.hpp>

#include "block.h"

using namespace std;
using namespace hnbase;

namespace hashahead
{
namespace storage
{

const uint8 DB_TRACE_KEY_TYPE_PREVROOT = 0x01;
const uint8 DB_TRACE_KEY_TYPE_TRIEROOT = 0x02;

const uint8 DB_TRACE_KEY_TYPE_TRIEROOT_CONTRACT_KV = 0x11;

// const uint8 DB_TRACE_KEY_NAME_TXPOS = 0x21;
const uint8 DB_TRACE_KEY_NAME_CONTRACT_RECEIPT = 0x22;
const uint8 DB_TRACE_KEY_NAME_CONTRACT_PREV_STATE = 0x23;
const uint8 DB_TRACE_KEY_NAME_CONTRACT_ADDRESS_KV_PAIR = 0x24;

#define DB_TRACE_KEY_ID_PREVROOT string("prevroot")

//////////////////////////////
// CCacheBlockContractReceipts

CCacheBlockContractReceipts::CCacheBlockContractReceipts(const BlockContractReceipts& bcr)
  : bcReceipts(bcr)
{
    for (uint32 i = 0; i < (uint32)(bcReceipts.size()); i++)
    {
        mapTxCr.insert(std::make_pair(bcReceipts[i].first, i));
    }
}

bool CCacheBlockContractReceipts::GetTxContractReceipts(const uint256& txid, TxContractReceipts& tcrReceipt) const
{
    auto it = mapTxCr.find(txid);
    if (it != mapTxCr.end() && it->second < (uint32)(bcReceipts.size()))
    {
        tcrReceipt = bcReceipts[it->second].second;
        return true;
    }
    return false;
}

//////////////////////////////
// CCacheBlockContractPrevState

CCacheBlockContractPrevState::CCacheBlockContractPrevState(const BlockContractPrevState& bcps)
  : bcPrevState(bcps)
{
    for (uint32 i = 0; i < (uint32)(bcPrevState.size()); i++)
    {
        mapTxPs.insert(std::make_pair(bcPrevState[i].first, i));
    }
}

bool CCacheBlockContractPrevState::GetTxContractPrevState(const uint256& txid, MapContractPrevState& prevState) const
{
    auto it = mapTxPs.find(txid);
    if (it != mapTxPs.end() && it->second < (uint32)(bcPrevState.size()))
    {
        prevState = bcPrevState[it->second].second;
        return true;
    }
    return false;
}

//////////////////////////////
// CCacheTraceData

void CCacheTraceData::AddCacheBlockContractTraceData(const uint256& hashBlock, const BlockContractReceipts& vContractReceipts, const BlockContractPrevState& vContractPrevAddressState)
{
    bool fAdd = false;
    if (mapBlockContractReceipts.find(hashBlock) == mapBlockContractReceipts.end())
    {
        mapBlockContractReceipts.insert(std::make_pair(hashBlock, CCacheBlockContractReceipts(vContractReceipts)));
        fAdd = true;
    }
    if (mapBlockContractPrevState.find(hashBlock) == mapBlockContractPrevState.end())
    {
        mapBlockContractPrevState.insert(std::make_pair(hashBlock, CCacheBlockContractPrevState(vContractPrevAddressState)));
        fAdd = true;
    }
    if (fAdd)
    {
        qBlockHash.push(hashBlock);
        if (qBlockHash.size() > MAX_CACHE_BLOCK_COUNT)
        {
            const uint256 hash = qBlockHash.front();
            qBlockHash.pop();
            mapBlockContractReceipts.erase(hash);
            mapBlockContractPrevState.erase(hash);
        }
    }
}

bool CCacheTraceData::GetBlockContractReceipt(const uint256& hashBlock, BlockContractReceipts& vContractReceipts) const
{
    auto it = mapBlockContractReceipts.find(hashBlock);
    if (it != mapBlockContractReceipts.end())
    {
        vContractReceipts = it->second.GetBlockContractReceipts();
        return true;
    }
    return false;
}

bool CCacheTraceData::GetBlockContractPrevState(const uint256& hashBlock, BlockContractPrevState& vBlockContractPrevState) const
{
    auto it = mapBlockContractPrevState.find(hashBlock);
    if (it != mapBlockContractPrevState.end())
    {
        vBlockContractPrevState = it->second.GetBlockContractPrevState();
        return true;
    }
    return false;
}

bool CCacheTraceData::GetTxContractReceipt(const uint256& hashBlock, const uint256& txid, TxContractReceipts& tcrReceipt) const
{
    auto it = mapBlockContractReceipts.find(hashBlock);
    if (it != mapBlockContractReceipts.end())
    {
        return it->second.GetTxContractReceipts(txid, tcrReceipt);
    }
    return false;
}

bool CCacheTraceData::GetTxContractPrevState(const uint256& hashBlock, const uint256& txid, MapContractPrevState& mapContractPrevState) const
{
    auto it = mapBlockContractPrevState.find(hashBlock);
    if (it != mapBlockContractPrevState.end())
    {
        return it->second.GetTxContractPrevState(txid, mapContractPrevState);
    }
    return false;
}

//////////////////////////////
// CForkTraceDB

CForkTraceDB::CForkTraceDB()
  : fUseCacheData(false), fPrune(false)
{
}

CForkTraceDB::~CForkTraceDB()
{
    dbTrie.Deinitialize();
}

bool CForkTraceDB::Initialize(const uint256& hashForkIn, const boost::filesystem::path& pathData, const bool fUseCacheDataIn, const bool fPruneIn)
{
    if (!dbTrie.Initialize(pathData))
    {
        return false;
    }
    hashFork = hashForkIn;
    fUseCacheData = fUseCacheDataIn;
    fPrune = fPruneIn;
    return true;
}

void CForkTraceDB::Deinitialize()
{
    dbTrie.Deinitialize();
}

bool CForkTraceDB::RemoveAll()
{
    dbTrie.RemoveAll();
    return true;
}

bool CForkTraceDB::AddBlockContractTraceData(const uint256& hashBlock, const BlockContractReceipts& vContractReceipts, const BlockContractPrevState& vContractPrevAddressState)
{
    CWriteLock wlock(rwAccess);

    if (!fUseCacheData)
    {
        for (const auto& vd : vContractReceipts)
        {
            hnbase::CBufStream ssKey, ssValue;
            ssKey << DB_TRACE_KEY_NAME_CONTRACT_RECEIPT << hashBlock << vd.first;
            ssValue << vd.second;

            if (!dbTrie.WriteExtKv(ssKey, ssValue))
            {
                StdLog("CForkTraceDB", "Add block contract trace data: Write contract receipt fail, block: %s, txid: %s", hashBlock.GetBhString().c_str(), vd.first.ToString().c_str());
                return false;
            }
        }
        for (const auto& vd : vContractPrevAddressState)
        {
            hnbase::CBufStream ssKey, ssValue;
            ssKey << DB_TRACE_KEY_NAME_CONTRACT_PREV_STATE << hashBlock << vd.first;
            ssValue << vd.second;

            if (!dbTrie.WriteExtKv(ssKey, ssValue))
            {
                StdLog("CForkTraceDB", "Add block contract trace data: Write contract prev state fail, block: %s, txid: %s", hashBlock.GetBhString().c_str(), vd.first.ToString().c_str());
                return false;
            }
        }
    }

    cacheTraceData.AddCacheBlockContractTraceData(hashBlock, vContractReceipts, vContractPrevAddressState);
    return true;
}

bool CForkTraceDB::AddBlockContractKvData(const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDestination, std::map<uint256, bytes>>& mapTraceContractKvData)
{
    CWriteLock wlock(rwAccess);

    uint256 hashPrevRoot;
    if (hashBlock != hashFork)
    {
        if (!ReadTrieRoot(DB_TRACE_KEY_TYPE_TRIEROOT_CONTRACT_KV, hashPrevBlock, hashPrevRoot))
        {
            StdLog("CForkTraceDB", "Add block contract kv data: Read trie root fail, prev block: %s", hashPrevBlock.GetBhString().c_str());
            return false;
        }
    }

    bytesmap mapKv;
    for (const auto& kv : mapTraceContractKvData)
    {
        for (const auto& kv2 : kv.second)
        {
            CBufStream ss;
            ss.Write((char*)(kv.first.begin()), kv.first.size());
            ss.Write((char*)(kv2.first.begin()), kv2.first.size());
            const uint256 hash = crypto::CryptoHash(ss.GetData(), ss.GetSize());

            hnbase::CBufStream ssKey, ssValue;
            bytes btKey, btValue;

            ssKey << DB_TRACE_KEY_NAME_CONTRACT_ADDRESS_KV_PAIR << kv.first << kv2.first;
            ssKey.GetData(btKey);

            ssValue << hash;
            ssValue.GetData(btValue);

            mapKv.insert(make_pair(btKey, btValue));
        }
    }
    if (hashPrevRoot == 0)
    {
        AddPrevRoot(hashPrevRoot, hashBlock, mapKv);
    }

    uint256 hashBlockRoot;
    if (!dbTrie.AddNewTrie(hashPrevRoot, mapKv, hashBlockRoot))
    {
        StdLog("CForkTraceDB", "Add block contract kv data: Add new trie fail, block: %s", hashBlock.GetBhString().c_str());
        return false;
    }

    if (!WriteTrieRoot(DB_TRACE_KEY_TYPE_TRIEROOT_CONTRACT_KV, hashBlock, hashBlockRoot))
    {
        StdLog("CForkTraceDB", "Add block contract kv data: Write trie root fail, block: %s", hashBlock.GetBhString().c_str());
        return false;
    }
    return true;
}

bool CForkTraceDB::RetrieveTxContractReceipt(const uint256& hashBlock, const uint256& txid, TxContractReceipts& tcrReceipt)
{
    CReadLock rlock(rwAccess);

    if (cacheTraceData.GetTxContractReceipt(hashBlock, txid, tcrReceipt))
    {
        return true;
    }

    if (!fUseCacheData)
    {
        try
        {
            hnbase::CBufStream ssKey, ssValue;
            ssKey << DB_TRACE_KEY_NAME_CONTRACT_RECEIPT << hashBlock << txid;
            if (dbTrie.ReadExtKv(ssKey, ssValue))
            {
                ssValue >> tcrReceipt;
                return true;
            }
        }
        catch (std::exception& e)
        {
            hnbase::StdError(__PRETTY_FUNCTION__, e.what());
            return false;
        }
    }
    return false;
}

bool CForkTraceDB::ListBlockContractReceipt(const uint256& hashBlock, BlockContractReceipts& vContractReceipts)
{
    CReadLock rlock(rwAccess);

    if (cacheTraceData.GetBlockContractReceipt(hashBlock, vContractReceipts))
    {
        return true;
    }

    if (!fUseCacheData)
    {
        auto funcWalker = [&](CBufStream& ssKey, CBufStream& ssValue) -> bool {
            try
            {
                uint8 nExtKey;
                uint8 nKeyType;
                ssKey >> nExtKey >> nKeyType;
                if (nKeyType == DB_TRACE_KEY_NAME_CONTRACT_RECEIPT)
                {
                    uint256 hashBlockDb;
                    uint256 txid;
                    ssKey >> hashBlockDb >> txid;

                    TxContractReceipts tcrReceipt;
                    ssValue >> tcrReceipt;

                    vContractReceipts.push_back(std::make_pair(txid, tcrReceipt));
                }
                return true;
            }
            catch (std::exception& e)
            {
                hnbase::StdError(__PRETTY_FUNCTION__, e.what());
            }
            return false;
        };

        CBufStream ssKeyBegin, ssKeyPrefix;
        ssKeyBegin << DB_TRACE_KEY_NAME_CONTRACT_RECEIPT << hashBlock;
        ssKeyPrefix << DB_TRACE_KEY_NAME_CONTRACT_RECEIPT << hashBlock;

        return dbTrie.WalkThroughExtKv(ssKeyBegin, ssKeyPrefix, funcWalker);
    }
    return false;
}

bool CForkTraceDB::RetrieveTxContractPrevState(const uint256& hashBlock, const uint256& txid, MapContractPrevState& mapContractPrevState)
{
    CReadLock rlock(rwAccess);

    if (cacheTraceData.GetTxContractPrevState(hashBlock, txid, mapContractPrevState))
    {
        return true;
    }

    if (!fUseCacheData)
    {
        try
        {
            hnbase::CBufStream ssKey, ssValue;
            ssKey << DB_TRACE_KEY_NAME_CONTRACT_PREV_STATE << hashBlock << txid;
            if (dbTrie.ReadExtKv(ssKey, ssValue))
            {
                ssValue >> mapContractPrevState;
                return true;
            }
        }
        catch (std::exception& e)
        {
            hnbase::StdError(__PRETTY_FUNCTION__, e.what());
            return false;
        }
    }
    return false;
}

bool CForkTraceDB::ListBlockContractPrevState(const uint256& hashBlock, BlockContractPrevState& vBlockContractPrevState)
{
    CReadLock rlock(rwAccess);

    if (cacheTraceData.GetBlockContractPrevState(hashBlock, vBlockContractPrevState))
    {
        return true;
    }

    if (!fUseCacheData)
    {
        auto funcWalker = [&](CBufStream& ssKey, CBufStream& ssValue) -> bool {
            try
            {
                uint8 nExtKey;
                uint8 nKeyType;
                ssKey >> nExtKey >> nKeyType;
                if (nKeyType == DB_TRACE_KEY_NAME_CONTRACT_PREV_STATE)
                {
                    uint256 hashBlockDb;
                    uint256 txid;
                    ssKey >> hashBlockDb >> txid;

                    MapContractPrevState mapConPrevState;
                    ssValue >> mapConPrevState;

                    vBlockContractPrevState.push_back(std::make_pair(txid, mapConPrevState));
                }
                return true;
            }
            catch (std::exception& e)
            {
                hnbase::StdError(__PRETTY_FUNCTION__, e.what());
            }
            return false;
        };

        CBufStream ssKeyBegin, ssKeyPrefix;
        ssKeyBegin << DB_TRACE_KEY_NAME_CONTRACT_PREV_STATE << hashBlock;
        ssKeyPrefix << DB_TRACE_KEY_NAME_CONTRACT_PREV_STATE << hashBlock;

        return dbTrie.WalkThroughExtKv(ssKeyBegin, ssKeyPrefix, funcWalker);
    }
    return false;
}

bool CForkTraceDB::GetContractKvPairList(const uint256& hashBlock, const CDestination& destContract, const uint256& keyStart, const uint32 nLimit, std::vector<std::pair<uint256, uint256>>& vContractKvPair, uint256& keyNext)
{
    CReadLock rlock(rwAccess);

    class CListTrieDBWalker : public CTrieDBWalker
    {
    public:
        CListTrieDBWalker(const uint32 nLimitIn, const CDestination destContractIn, std::vector<std::pair<uint256, uint256>>& vContractKvPairOut, uint256& keyNextOut)
          : nLimit(nLimitIn), destContract(destContractIn), vContractKvPair(vContractKvPairOut), keyNext(keyNextOut) {}

        bool Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver) override
        {
            if (btKey.size() == 0 || btValue.size() == 0)
            {
                StdError("CListTrieDBWalker", "btKey.size() = %ld, btValue.size() = %ld", btKey.size(), btValue.size());
                return false;
            }

            try
            {
                hnbase::CBufStream ssKey(btKey);
                uint8 nKeyType;
                ssKey >> nKeyType;
                if (nKeyType == DB_TRACE_KEY_NAME_CONTRACT_ADDRESS_KV_PAIR)
                {
                    CDestination destContractDb;
                    uint256 keyDb;
                    uint256 hashValueDb;
                    hnbase::CBufStream ssValue(btValue);
                    ssKey >> destContractDb >> keyDb;
                    ssValue >> hashValueDb;
                    if (destContractDb != destContract)
                    {
                        fWalkOver = true;
                        return true;
                    }
                    if (vContractKvPair.size() >= nLimit)
                    {
                        keyNext = keyDb;
                        fWalkOver = true;
                        return true;
                    }
                    vContractKvPair.push_back(std::make_pair(keyDb, hashValueDb));
                }
                else
                {
                    fWalkOver = true;
                    return true;
                }
            }
            catch (std::exception& e)
            {
                hnbase::StdError(__PRETTY_FUNCTION__, e.what());
                return false;
            }
            return true;
        }

    public:
        const uint32 nLimit;
        const CDestination destContract;
        std::vector<std::pair<uint256, uint256>>& vContractKvPair;
        uint256& keyNext;
    };

    uint256 hashRoot;
    if (!ReadTrieRoot(DB_TRACE_KEY_TYPE_TRIEROOT_CONTRACT_KV, hashBlock, hashRoot))
    {
        StdLog("CForkTraceDB", "Get contract kv pair list: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }

    bytes btKeyPrefix, btBeginKeyTail;

    hnbase::CBufStream ssKeyPrefix;
    ssKeyPrefix << DB_TRACE_KEY_NAME_CONTRACT_ADDRESS_KV_PAIR << destContract;
    ssKeyPrefix.GetData(btKeyPrefix);

    if (keyStart != 0)
    {
        hnbase::CBufStream ssBeginKeyTail;
        ssBeginKeyTail << keyStart;
        ssBeginKeyTail.GetData(btBeginKeyTail);
    }

    CListTrieDBWalker walker(nLimit, destContract, vContractKvPair, keyNext);
    if (!dbTrie.WalkThroughTrie(hashRoot, walker, btKeyPrefix, btBeginKeyTail))
    {
        StdLog("CForkTraceDB", "Get contract kv pair list: Walk through trie fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CForkTraceDB::ClearTraceUnavailableNode(const uint32 nClearRefHeight)
{
    if (!fPrune)
    {
        return false;
    }

    if (!ClearHeightTrieRoot(nClearRefHeight))
    {
        StdLog("CForkTraceDB", "Clear trace unavailable node: Clear height trie root failed, height: %d", nClearRefHeight);
        return false;
    }
    return true;
}

bool CForkTraceDB::GetSnapshotTraceData(const std::vector<uint256>& vBlockHash, bytes& btSnapData)
{
    CForkTraceRootKv traceRootKv(hashFork, vBlockHash);

    traceRootKv.vKv.reserve(vBlockHash.size());
    for (const auto& hashBlock : vBlockHash)
    {
        uint256 hashRoot;
        if (!ReadTrieRoot(DB_TRACE_KEY_TYPE_TRIEROOT_CONTRACT_KV, hashBlock, hashRoot))
        {
            StdLog("CForkTraceDB", "Get snapshot trace data: Read trie root failed, block: %s", hashBlock.GetBhString().c_str());
            return false;
        }
        traceRootKv.vKv.push_back(std::make_pair(hashRoot, bytesmap()));
    }

    CBufStream ss;
    ss << traceRootKv;
    ss.GetData(btSnapData);
    return true;
}

bool CForkTraceDB::RecoveryTraceData(const CForkTraceRootKv& traceRootKv)
{
    return true;
}

///////////////////////////////////
bool CForkTraceDB::WriteTrieRoot(const uint8 nTrieType, const uint256& hashBlock, const uint256& hashTrieRoot)
{
    hnbase::CBufStream ssKey, ssValue;
    ssKey << DB_TRACE_KEY_TYPE_TRIEROOT << nTrieType << hashBlock;
    ssValue << hashTrieRoot;
    return dbTrie.WriteExtKv(ssKey, ssValue);
}

bool CForkTraceDB::ReadTrieRoot(const uint8 nTrieType, const uint256& hashBlock, uint256& hashTrieRoot)
{
    if (hashBlock == 0)
    {
        hashTrieRoot = 0;
        return true;
    }

    hnbase::CBufStream ssKey, ssValue;
    ssKey << DB_TRACE_KEY_TYPE_TRIEROOT << nTrieType << hashBlock;
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

bool CForkTraceDB::RemoveTrieRoot(const uint8 nTrieType, const uint256& hashBlock)
{
    hnbase::CBufStream ssKey;
    ssKey << DB_TRACE_KEY_TYPE_TRIEROOT << nTrieType << hashBlock;
    return dbTrie.RemoveExtKv(ssKey);
}

void CForkTraceDB::AddPrevRoot(const uint256& hashPrevRoot, const uint256& hashBlock, bytesmap& mapKv)
{
    hnbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;

    ssKey << DB_TRACE_KEY_TYPE_PREVROOT << DB_TRACE_KEY_ID_PREVROOT;
    ssKey.GetData(btKey);

    ssValue << hashPrevRoot << hashBlock;
    ssValue.GetData(btValue);

    mapKv.insert(make_pair(btKey, btValue));
}

bool CForkTraceDB::GetPrevRoot(const uint256& hashRoot, uint256& hashPrevRoot, uint256& hashBlock)
{
    hnbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_TRACE_KEY_TYPE_PREVROOT << DB_TRACE_KEY_ID_PREVROOT;
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

bool CForkTraceDB::ClearHeightTrieRoot(const uint32 nLastHeight)
{
    std::vector<std::pair<uint8, uint256>> vBlockTrieType;

    auto funcWalker = [&](CBufStream& ssKey, CBufStream& ssValue) -> bool {
        try
        {
            uint8 nExtKey;
            uint8 nKeyType;
            ssKey >> nExtKey >> nKeyType;
            if (nKeyType == DB_TRACE_KEY_TYPE_TRIEROOT)
            {
                uint8 nTrieType;
                uint256 hashBlock;
                ssKey >> nTrieType >> hashBlock;
                if (CBlock::GetBlockHeightByHash(hashBlock) < nLastHeight)
                {
                    vBlockTrieType.push_back(std::make_pair(nTrieType, hashBlock));
                }
            }
            return true;
        }
        catch (std::exception& e)
        {
            hnbase::StdError(__PRETTY_FUNCTION__, e.what());
        }
        return false;
    };

    CBufStream ssKeyBegin, ssKeyPrefix;
    ssKeyPrefix << DB_TRACE_KEY_TYPE_TRIEROOT;

    if (!dbTrie.WalkThroughExtKv(ssKeyBegin, ssKeyPrefix, funcWalker))
    {
        StdLog("CForkTraceDB", "Clear height trie root: Walk through ext kv failed, last height: %d", nLastHeight);
        return false;
    }

    for (auto& vd : vBlockTrieType)
    {
        if (!RemoveTrieRoot(vd.first, vd.second))
        {
            StdLog("CForkTraceDB", "Clear height trie root: Remove trie root failed, trie type: %d, block: %s, last height: %d", vd.first, vd.second.ToString().c_str(), nLastHeight);
            return false;
        }
    }

    StdDebug("CForkTraceDB", "Clear height trie root: Remove trie root success, remove block count: %lu, last height: %d", vBlockTrieType.size(), nLastHeight);
    return true;
}

//////////////////////////////
// CTraceDB

bool CTraceDB::Initialize(const boost::filesystem::path& pathData, const bool fUseCacheDataIn, const bool fPruneIn)
{
    pathTrace = pathData / "trace";
    fUseCacheData = fUseCacheDataIn;
    fPrune = fPruneIn;

    if (!boost::filesystem::exists(pathTrace))
    {
        boost::filesystem::create_directories(pathTrace);
    }

    if (!boost::filesystem::is_directory(pathTrace))
    {
        return false;
    }
    return true;
}

void CTraceDB::Deinitialize()
{
    CWriteLock wlock(rwAccess);
    mapTraceDB.clear();
}

bool CTraceDB::ExistFork(const uint256& hashFork)
{
    CReadLock rlock(rwAccess);
    return (mapTraceDB.find(hashFork) != mapTraceDB.end());
}

bool CTraceDB::LoadFork(const uint256& hashFork)
{
    CWriteLock wlock(rwAccess);

    auto it = mapTraceDB.find(hashFork);
    if (it != mapTraceDB.end())
    {
        return true;
    }

    std::shared_ptr<CForkTraceDB> spDb(new CForkTraceDB());
    if (spDb == nullptr)
    {
        return false;
    }
    if (!spDb->Initialize(hashFork, pathTrace / hashFork.GetHex(), fUseCacheData, fPrune))
    {
        return false;
    }
    mapTraceDB.insert(make_pair(hashFork, spDb));
    return true;
}

void CTraceDB::RemoveFork(const uint256& hashFork)
{
    CWriteLock wlock(rwAccess);

    auto it = mapTraceDB.find(hashFork);
    if (it != mapTraceDB.end())
    {
        it->second->RemoveAll();
        mapTraceDB.erase(it);
    }

    boost::filesystem::path forkPath = pathTrace / hashFork.GetHex();
    if (boost::filesystem::exists(forkPath))
    {
        boost::filesystem::remove_all(forkPath);
    }
}

bool CTraceDB::AddNewFork(const uint256& hashFork)
{
    RemoveFork(hashFork);
    return LoadFork(hashFork);
}

void CTraceDB::Clear()
{
    CWriteLock wlock(rwAccess);

    auto it = mapTraceDB.begin();
    while (it != mapTraceDB.end())
    {
        it->second->RemoveAll();
        mapTraceDB.erase(it++);
    }
}

bool CTraceDB::AddBlockContractTraceData(const uint256& hashFork, const uint256& hashBlock, const BlockContractReceipts& vContractReceipts, const BlockContractPrevState& vContractPrevAddressState)
{
    CReadLock rlock(rwAccess);

    auto it = mapTraceDB.find(hashFork);
    if (it != mapTraceDB.end())
    {
        return it->second->AddBlockContractTraceData(hashBlock, vContractReceipts, vContractPrevAddressState);
    }
    return false;
}

bool CTraceDB::AddBlockContractKvData(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDestination, std::map<uint256, bytes>>& mapTraceContractKvData)
{
    CReadLock rlock(rwAccess);

    auto it = mapTraceDB.find(hashFork);
    if (it != mapTraceDB.end())
    {
        return it->second->AddBlockContractKvData(hashPrevBlock, hashBlock, mapTraceContractKvData);
    }
    return false;
}

bool CTraceDB::RetrieveTxContractReceipt(const uint256& hashFork, const uint256& hashBlock, const uint256& txid, TxContractReceipts& tcrReceipt)
{
    CReadLock rlock(rwAccess);

    auto it = mapTraceDB.find(hashFork);
    if (it != mapTraceDB.end())
    {
        return it->second->RetrieveTxContractReceipt(hashBlock, txid, tcrReceipt);
    }
    return false;
}

bool CTraceDB::ListBlockContractReceipt(const uint256& hashFork, const uint256& hashBlock, BlockContractReceipts& vContractReceipts)
{
    CReadLock rlock(rwAccess);

    auto it = mapTraceDB.find(hashFork);
    if (it != mapTraceDB.end())
    {
        return it->second->ListBlockContractReceipt(hashBlock, vContractReceipts);
    }
    return false;
}

bool CTraceDB::RetrieveTxContractPrevState(const uint256& hashFork, const uint256& hashBlock, const uint256& txid, MapContractPrevState& mapContractPrevState)
{
    CReadLock rlock(rwAccess);

    auto it = mapTraceDB.find(hashFork);
    if (it != mapTraceDB.end())
    {
        return it->second->RetrieveTxContractPrevState(hashBlock, txid, mapContractPrevState);
    }
    return false;
}

bool CTraceDB::ListBlockContractPrevState(const uint256& hashFork, const uint256& hashBlock, BlockContractPrevState& vBlockContractPrevState)
{
    CReadLock rlock(rwAccess);

    auto it = mapTraceDB.find(hashFork);
    if (it != mapTraceDB.end())
    {
        return it->second->ListBlockContractPrevState(hashBlock, vBlockContractPrevState);
    }
    return false;
}

bool CTraceDB::GetContractKvPairList(const uint256& hashFork, const uint256& hashBlock, const CDestination& destContract, const uint256& keyStart, const uint32 nLimit, std::vector<std::pair<uint256, uint256>>& vContractKvPair, uint256& keyNext)
{
    CReadLock rlock(rwAccess);

    auto it = mapTraceDB.find(hashFork);
    if (it != mapTraceDB.end())
    {
        return it->second->GetContractKvPairList(hashBlock, destContract, keyStart, nLimit, vContractKvPair, keyNext);
    }
    return false;
}

bool CTraceDB::ClearTraceUnavailableNode(const uint256& hashFork, const uint32 nClearRefHeight)
{
    CReadLock rlock(rwAccess);

    auto it = mapTraceDB.find(hashFork);
    if (it != mapTraceDB.end())
    {
        return it->second->ClearTraceUnavailableNode(nClearRefHeight);
    }
    return false;
}

bool CTraceDB::GetSnapshotTraceData(const uint256& hashFork, const std::vector<uint256>& vBlockHash, bytes& btSnapData)
{
    CReadLock rlock(rwAccess);

    auto it = mapTraceDB.find(hashFork);
    if (it != mapTraceDB.end())
    {
        return it->second->GetSnapshotTraceData(vBlockHash, btSnapData);
    }
    return false;
}

bool CTraceDB::RecoveryTraceData(const bytes& btSnapData)
{
    CForkTraceRootKv traceRootKv;
    try
    {
        CBufStream ss(btSnapData);
        ss >> traceRootKv;
    }
    catch (std::exception& e)
    {
        hnbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }

    CReadLock rlock(rwAccess);

    auto it = mapTraceDB.find(traceRootKv.hashFork);
    if (it != mapTraceDB.end())
    {
        return it->second->RecoveryTraceData(traceRootKv);
    }
    return false;
}

} // namespace storage
} // namespace hashahead
