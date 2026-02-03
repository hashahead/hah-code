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

void CBlockFilter::AddNewBlockInfo(const uint256& hashFork, const uint256& hashBlock, const CBlock& block)
{
    {
        boost::unique_lock<boost::shared_mutex> lock(mutexFilter);
        for (auto it = mapBlockFilter.begin(); it != mapBlockFilter.end();)
        {
            if (it->second.isTimeout())
            {
                mapBlockFilter.erase(it++);
            }
            else
            {
                if (!it->second.AddBlockHash(hashFork, hashBlock, block.hashPrev))
                {
                    mapBlockFilter.erase(it++);
                }
                else
                {
                    ++it;
                }
            }
        }
    }

    {
        CEventWsServicePushNewBlock* pPushEvent = new CEventWsServicePushNewBlock(0);
        if (pPushEvent != nullptr)
        {
            pPushEvent->data.hashFork = hashFork;
            pPushEvent->data.hashBlock = hashBlock;
            pPushEvent->data.block = block;

            pWsService->PostEvent(pPushEvent);
        }
    }

    {
        boost::unique_lock<boost::shared_mutex> lock(mutexFilter);

        CForkBlockStat& stat = mapForkBlockStat[hashFork];

        if (stat.nStartBlockNumber == 0)
        {
            stat.nStartBlockNumber = block.GetBlockNumber();
        }
        stat.nCurrBlockNumber = block.GetBlockNumber();
        if (stat.nMaxPeerBlockNumber < block.GetBlockNumber())
        {
            stat.nMaxPeerBlockNumber = block.GetBlockNumber();
        }

        if (GetTime() > stat.nPrevSendStatTime)
        {
            stat.nPrevSendStatTime = GetTime();

            CEventWsServicePushSyncing* pPushEvent = new CEventWsServicePushSyncing(0);
            if (pPushEvent != nullptr)
            {
                pPushEvent->data.hashFork = hashFork;
                pPushEvent->data.fSyncing = true;
                pPushEvent->data.nStartingBlockNumber = stat.nStartBlockNumber;
                pPushEvent->data.nCurrentBlockNumber = stat.nCurrBlockNumber;
                pPushEvent->data.nHighestBlockNumber = stat.nMaxPeerBlockNumber;
                pPushEvent->data.nPulledStates = 0;
                pPushEvent->data.nKnownStates = 0;

                pWsService->PostEvent(pPushEvent);
            }
        }
    }
}

bool CBlockFilter::GetFilterBlockHashs(const uint256& nFilterId, const uint256& hashLastBlock, const bool fAll, std::vector<uint256>& vBlockHash)
{
    boost::shared_lock<boost::shared_mutex> lock(mutexFilter);
    auto it = mapBlockFilter.find(nFilterId);
    if (it == mapBlockFilter.end())
    {
        return false;
    }
    it->second.GetFilterBlockHashs(hashLastBlock, fAll, vBlockHash);
    return true;
}

uint256 CBlockFilter::AddPendingTxFilter(const uint256& hashClient, const uint256& hashFork)
{
    boost::unique_lock<boost::shared_mutex> lock(mutexFilter);
    uint256 nFilterId = createFilterId.CreateTxFilterId(hashClient);
    while (mapTxFilter.count(nFilterId) > 0)
    {
        nFilterId = createFilterId.CreateTxFilterId(hashClient);
    }
    mapTxFilter.insert(make_pair(nFilterId, CBlockPendingTxFilter(hashFork)));
    return nFilterId;
}

void CBlockFilter::AddPendingTx(const uint256& hashFork, const uint256& txid)
{
    {
        boost::unique_lock<boost::shared_mutex> lock(mutexFilter);
        for (auto it = mapTxFilter.begin(); it != mapTxFilter.end();)
        {
            if (it->second.isTimeout())
            {
                mapTxFilter.erase(it++);
            }
            else
            {
                if (!it->second.AddPendingTx(hashFork, txid))
                {
                    mapTxFilter.erase(it++);
                }
                else
                {
                    ++it;
                }
            }
        }
    }

    {
        CEventWsServicePushNewPendingTx* pPushEvent = new CEventWsServicePushNewPendingTx(0);
        if (pPushEvent != nullptr)
        {
            pPushEvent->data.hashFork = hashFork;
            pPushEvent->data.txid = txid;

            pWsService->PostEvent(pPushEvent);
        }
    }
}

bool CBlockFilter::GetFilterTxids(const uint256& hashFork, const uint256& nFilterId, const bool fAll, std::vector<uint256>& vTxid)
{
    boost::shared_lock<boost::shared_mutex> lock(mutexFilter);
    auto it = mapTxFilter.find(nFilterId);
    if (it == mapTxFilter.end())
    {
        return false;
    }
    it->second.GetFilterTxids(hashFork, fAll, vTxid);
    return true;
}

} // namespace hashahead
