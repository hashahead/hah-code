// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HASHAHEAD_BLOCKFILTER_H
#define HASHAHEAD_BLOCKFILTER_H

#include "base.h"
#include "block.h"
#include "cmstruct.h"
#include "filterdata.h"
#include "hnbase.h"

namespace hashahead
{

//////////////////////////////
// CForkBlockStat

class CForkBlockStat
{
public:
    CForkBlockStat()
      : nPrevSendStatTime(0), nStartBlockNumber(0), nCurrBlockNumber(0), nMaxPeerBlockNumber(0) {}

public:
    int64 nPrevSendStatTime;

    uint64 nStartBlockNumber;
    uint64 nCurrBlockNumber;
    uint64 nMaxPeerBlockNumber;
};

/////////////////////////////
// CBlockFilter

class CBlockFilter : public IBlockFilter
{
public:
    CBlockFilter();
    ~CBlockFilter();

    void RemoveFilter(const uint256& nFilterId) override;

    uint256 AddLogsFilter(const uint256& hashClient, const uint256& hashFork, const CLogsFilter& logFilterIn, const std::map<uint256, std::vector<CTransactionReceipt>, CustomBlockHashCompare>& mapHisBlockReceiptsIn) override;
    void AddTxReceipt(const uint256& hashFork, const uint256& hashBlock, const CTransactionReceipt& receipt) override;
    bool GetTxReceiptLogsByFilterId(const uint256& nFilterId, const bool fAll, ReceiptLogsVec& receiptLogs) override;

    uint256 AddBlockFilter(const uint256& hashClient, const uint256& hashFork) override;
    void AddNewBlockInfo(const uint256& hashFork, const uint256& hashBlock, const CBlock& block) override;
    bool GetFilterBlockHashs(const uint256& nFilterId, const uint256& hashLastBlock, const bool fAll, std::vector<uint256>& vBlockHash) override;

    uint256 AddPendingTxFilter(const uint256& hashClient, const uint256& hashFork) override;
    void AddPendingTx(const uint256& hashFork, const uint256& txid) override;
protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    bool HandleInvoke() override;
    void HandleHalt() override;

    bool LoadFilter();
    void SaveFilter();

protected:
    IWsService* pWsService;

    storage::CFilterData datFilterData;

    boost::shared_mutex mutexFilter;
    CFilterId createFilterId;

    std::map<uint256, CBlockLogsFilter> mapLogFilter;     // key: filter id
    std::map<uint256, CBlockMakerFilter> mapBlockFilter;  // key: filter id
    std::map<uint256, CBlockPendingTxFilter> mapTxFilter; // key: filter id
};

} // namespace hashahead

#endif // HASHAHEAD_BLOCKFILTER_H
