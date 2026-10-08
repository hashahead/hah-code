// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_TRACEDB_H
#define STORAGE_TRACEDB_H

#include <boost/thread/thread.hpp>

#include "dbstruct.h"
#include "hnbase.h"
#include "transaction.h"
#include "triedb.h"

#define MAX_CACHE_BLOCK_COUNT 1024

namespace hashahead
{
namespace storage
{

class CCacheBlockContractReceipts
{
public:
    CCacheBlockContractReceipts(const BlockContractReceipts& bcr);

    const BlockContractReceipts& GetBlockContractReceipts() const
    {
        return bcReceipts;
    }

    bool GetTxContractReceipts(const uint256& txid, TxContractReceipts& tcrReceipt) const;

protected:
    const BlockContractReceipts bcReceipts;
    std::map<uint256, uint32> mapTxCr; // key: txid, value: index
};

class CCacheBlockContractPrevState
{
public:
    CCacheBlockContractPrevState(const BlockContractPrevState& bcps);

    const BlockContractPrevState& GetBlockContractPrevState() const
    {
        return bcPrevState;
    }

    bool GetTxContractPrevState(const uint256& txid, MapContractPrevState& prevState) const;

protected:
    const BlockContractPrevState bcPrevState;
    std::map<uint256, uint32> mapTxPs; // key: txid, value: index
};

class CCacheTraceData
{
public:
    CCacheTraceData() {}

    void AddCacheBlockContractTraceData(const uint256& hashBlock, const BlockContractReceipts& vContractReceipts, const BlockContractPrevState& vContractPrevAddressState);

    bool GetBlockContractReceipt(const uint256& hashBlock, BlockContractReceipts& vContractReceipts) const;
    bool GetBlockContractPrevState(const uint256& hashBlock, BlockContractPrevState& vBlockContractPrevState) const;

    bool GetTxContractReceipt(const uint256& hashBlock, const uint256& txid, TxContractReceipts& tcrReceipt) const;
    bool GetTxContractPrevState(const uint256& hashBlock, const uint256& txid, MapContractPrevState& mapContractPrevState) const;

protected:
    std::map<uint256, CCacheBlockContractReceipts> mapBlockContractReceipts;   // key: block hash
    std::map<uint256, CCacheBlockContractPrevState> mapBlockContractPrevState; // key: block hash
    std::queue<uint256> qBlockHash;
};

class CForkTraceDB
{
public:
    CForkTraceDB();
    ~CForkTraceDB();

    bool Initialize(const uint256& hashForkIn, const boost::filesystem::path& pathData, const bool fUseCacheDataIn, const bool fPruneIn = false);
    void Deinitialize();
    bool RemoveAll();

    bool AddBlockContractTraceData(const uint256& hashBlock, const BlockContractReceipts& vContractReceipts, const BlockContractPrevState& vContractPrevAddressState);
    bool AddBlockContractKvData(const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDestination, std::map<uint256, bytes>>& mapTraceContractKvData);

    bool RetrieveTxContractReceipt(const uint256& hashBlock, const uint256& txid, TxContractReceipts& tcrReceipt);
    bool ListBlockContractReceipt(const uint256& hashBlock, BlockContractReceipts& vContractReceipts);
    bool RetrieveTxContractPrevState(const uint256& hashBlock, const uint256& txid, MapContractPrevState& mapContractPrevState);
    bool ListBlockContractPrevState(const uint256& hashBlock, BlockContractPrevState& vBlockContractPrevState);
    bool GetContractKvPairList(const uint256& hashBlock, const CDestination& destContract, const uint256& keyStart, const uint32 nLimit, std::vector<std::pair<uint256, uint256>>& vContractKvPair, uint256& keyNext);

    bool ClearTraceUnavailableNode(const uint32 nClearRefHeight);
    bool GetSnapshotTraceData(const std::vector<uint256>& vBlockHash, bytes& btSnapData);
    bool RecoveryTraceData(const CForkTraceRootKv& traceRootKv);

protected:
    bool WriteTrieRoot(const uint8 nTrieType, const uint256& hashBlock, const uint256& hashTrieRoot);
    bool ReadTrieRoot(const uint8 nTrieType, const uint256& hashBlock, uint256& hashTrieRoot);
    bool RemoveTrieRoot(const uint8 nTrieType, const uint256& hashBlock);
    void AddPrevRoot(const uint256& hashPrevRoot, const uint256& hashBlock, bytesmap& mapKv);
    bool GetPrevRoot(const uint256& hashRoot, uint256& hashPrevRoot, uint256& hashBlock);
    bool ClearHeightTrieRoot(const uint32 nLastHeight);

protected:
    hnbase::CRWAccess rwAccess;
    uint256 hashFork;
    bool fPrune;
    CTrieDB dbTrie;
    bool fUseCacheData;
    CCacheTraceData cacheTraceData;
};

class CTraceDB
{
public:
    CTraceDB()
      : fUseCacheData(false), fPrune(false) {}
    bool Initialize(const boost::filesystem::path& pathData, const bool fUseCacheDataIn, const bool fPruneIn = false);
    void Deinitialize();

    bool ExistFork(const uint256& hashFork);
    bool LoadFork(const uint256& hashFork);
    void RemoveFork(const uint256& hashFork);
    bool AddNewFork(const uint256& hashFork);
    void Clear();

    bool AddBlockContractTraceData(const uint256& hashFork, const uint256& hashBlock, const BlockContractReceipts& vContractReceipts, const BlockContractPrevState& vContractPrevAddressState);
    bool AddBlockContractKvData(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDestination, std::map<uint256, bytes>>& mapTraceContractKvData);

    bool RetrieveTxContractReceipt(const uint256& hashFork, const uint256& hashBlock, const uint256& txid, TxContractReceipts& tcrReceipt);
    bool ListBlockContractReceipt(const uint256& hashFork, const uint256& hashBlock, BlockContractReceipts& vContractReceipts);
    bool RetrieveTxContractPrevState(const uint256& hashFork, const uint256& hashBlock, const uint256& txid, MapContractPrevState& mapContractPrevState);
    bool ListBlockContractPrevState(const uint256& hashFork, const uint256& hashBlock, BlockContractPrevState& vBlockContractPrevState);
    bool GetContractKvPairList(const uint256& hashFork, const uint256& hashBlock, const CDestination& destContract, const uint256& keyStart, const uint32 nLimit, std::vector<std::pair<uint256, uint256>>& vContractKvPair, uint256& keyNext);

    bool ClearTraceUnavailableNode(const uint256& hashFork, const uint32 nClearRefHeight);
protected:
    boost::filesystem::path pathTrace;
    hnbase::CRWAccess rwAccess;
    std::map<uint256, std::shared_ptr<CForkTraceDB>> mapTraceDB;
    bool fUseCacheData;
    bool fPrune;
};

} // namespace storage
} // namespace hashahead

#endif // STORAGE_TRACEDB_H
