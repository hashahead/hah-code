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
}

} // namespace storage
} // namespace hashahead
