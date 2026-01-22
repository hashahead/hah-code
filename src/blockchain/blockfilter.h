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
