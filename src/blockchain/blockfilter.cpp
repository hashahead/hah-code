// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blockfilter.h"

#include "event.h"

using namespace std;
using namespace hnbase;
using namespace hashahead::crypto;

namespace hashahead
{

//////////////////////////////
// CBlockFilter

CBlockFilter::CBlockFilter()
{
    pWsService = nullptr;
}

CBlockFilter::~CBlockFilter()
{
}

bool CBlockFilter::HandleInitialize()
{
    if (!GetObject("wsservice", pWsService))
    {
        Error("Failed to request wsservice");
        return false;
    }
    return true;
}

void CBlockFilter::HandleDeinitialize()
{
    pWsService = nullptr;
}

bool CBlockFilter::HandleInvoke()
{
    if (!datFilterData.Initialize(Config()->pathData))
    {
        Error("Failed to initialize filter data");
        return false;
    }
    if (!LoadFilter())
    {
        Error("Failed to load filter data");
        return false;
    }
    return true;
}

void CBlockFilter::HandleHalt()
{
    SaveFilter();
}

bool CBlockFilter::LoadFilter()
{
    boost::unique_lock<boost::shared_mutex> lock(mutexFilter);

    if (!datFilterData.Load(mapLogFilter, mapBlockFilter, mapTxFilter))
    {
        return false;
    }
    return true;
}

void CBlockFilter::SaveFilter()
{
    boost::unique_lock<boost::shared_mutex> lock(mutexFilter);

    datFilterData.Save(mapLogFilter, mapBlockFilter, mapTxFilter);
}

void CBlockFilter::RemoveFilter(const uint256& nFilterId)
{
    boost::unique_lock<boost::shared_mutex> lock(mutexFilter);
    if (CFilterId::isLogsFilter(nFilterId))
    {
        mapLogFilter.erase(nFilterId);
    }
    else if (CFilterId::isBlockFilter(nFilterId))
    {
        mapBlockFilter.erase(nFilterId);
    }
    else if (CFilterId::isTxFilter(nFilterId))
    {
        mapTxFilter.erase(nFilterId);
    }
}

uint256 CBlockFilter::AddLogsFilter(const uint256& hashClient, const uint256& hashFork, const CLogsFilter& logFilterIn, const std::map<uint256, std::vector<CTransactionReceipt>, CustomBlockHashCompare>& mapHisBlockReceiptsIn)
{
    boost::unique_lock<boost::shared_mutex> lock(mutexFilter);
    uint256 nFilterId = createFilterId.CreateLogsFilterId(hashClient);
    while (mapLogFilter.count(nFilterId) > 0)
    {
        nFilterId = createFilterId.CreateLogsFilterId(hashClient);
    }
    auto it = mapLogFilter.insert(make_pair(nFilterId, CBlockLogsFilter(hashFork, logFilterIn))).first;
    if (it == mapLogFilter.end())
    {
        return 0;
    }
    CBlockLogsFilter& filter = it->second;
    for (const auto& kv : mapHisBlockReceiptsIn)
    {
        const uint256& hashBlock = kv.first;
        for (const auto& receipt : kv.second)
        {
            filter.AddTxReceipt(hashFork, hashBlock, receipt);
        }
    }
    return nFilterId;
}

void CBlockFilter::AddTxReceipt(const uint256& hashFork, const uint256& hashBlock, const CTransactionReceipt& receipt)
{
    {
        boost::unique_lock<boost::shared_mutex> lock(mutexFilter);
        for (auto it = mapLogFilter.begin(); it != mapLogFilter.end();)
        {
            if (it->second.isTimeout())
            {
                mapLogFilter.erase(it++);
            }
            else
            {
                if (!it->second.AddTxReceipt(hashFork, hashBlock, receipt))
                {
                    mapLogFilter.erase(it++);
                }
                else
                {
                    ++it;
                }
            }
        }
    }

    {
        CEventWsServicePushLogs* pPushEvent = new CEventWsServicePushLogs(0);
        if (pPushEvent != nullptr)
        {
            pPushEvent->data.hashFork = hashFork;
            pPushEvent->data.hashBlock = hashBlock;
            pPushEvent->data.receipt = receipt;

            pWsService->PostEvent(pPushEvent);
        }
    }
}

bool CBlockFilter::GetTxReceiptLogsByFilterId(const uint256& nFilterId, const bool fAll, ReceiptLogsVec& receiptLogs)
{
    boost::shared_lock<boost::shared_mutex> lock(mutexFilter);
    auto it = mapLogFilter.find(nFilterId);
    if (it == mapLogFilter.end())
    {
        return false;
    }
    it->second.GetTxReceiptLogs(fAll, receiptLogs);
    return true;
}

uint256 CBlockFilter::AddBlockFilter(const uint256& hashClient, const uint256& hashFork)
{
    boost::unique_lock<boost::shared_mutex> lock(mutexFilter);
    uint256 nFilterId = createFilterId.CreateBlockFilterId(hashClient);
    while (mapBlockFilter.count(nFilterId) > 0)
    {
        nFilterId = createFilterId.CreateBlockFilterId(hashClient);
    }
    mapBlockFilter.insert(make_pair(nFilterId, CBlockMakerFilter(hashFork)));
    return nFilterId;
}

} // namespace hashahead
