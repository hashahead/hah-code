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
protected:
    std::map<uint256, CCacheBlockContractReceipts> mapBlockContractReceipts;   // key: block hash
    std::map<uint256, CCacheBlockContractPrevState> mapBlockContractPrevState; // key: block hash
    std::queue<uint256> qBlockHash;
};

class CTraceDB
{
public:
    CTraceDB()
      : fUseCacheData(false), fPrune(false) {}
    bool Initialize(const boost::filesystem::path& pathData, const bool fUseCacheDataIn, const bool fPruneIn = false);
    void Deinitialize();

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
