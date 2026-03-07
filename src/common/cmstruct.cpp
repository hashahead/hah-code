// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cmstruct.h"

#include "util.h"

using namespace hnbase;

namespace hashahead
{

//////////////////////////////
// CBlockLogsFilter

bool CBlockLogsFilter::isTimeout()
{
    if (GetTime() - nPrevGetChangesTime >= FILTER_DEFAULT_TIMEOUT)
    {
        return true;
    }
    return false;
}

bool CBlockLogsFilter::AddTxReceipt(const uint256& hashFork, const uint256& hashBlock, const CTransactionReceipt& receipt)
{
    if (hashFork != hashFork)
    {
        return true;
    }
    if (logFilter.hashFromBlock != 0 && CBlock::GetBlockHeightByHash(hashBlock) < CBlock::GetBlockHeightByHash(logFilter.hashFromBlock))
    {
        return true;
    }
    if (logFilter.hashToBlock != 0 && CBlock::GetBlockHeightByHash(hashBlock) > CBlock::GetBlockHeightByHash(logFilter.hashToBlock))
    {
        return true;
    }

    MatchLogsVec vLogs;
    logFilter.matchesLogs(receipt, vLogs);
    if (!vLogs.empty())
    {
        CReceiptLogs r(vLogs);
        r.txid = receipt.txid;
        r.nTxIndex = receipt.nTxIndex;
        r.nBlockNumber = receipt.nBlockNumber;
        r.hashBlock = hashBlock;
        vReceiptLogs.push_back(r);

        nLogsCount += r.matchLogs.size();
        if (nLogsCount > MAX_FILTER_CACHE_COUNT * 2)
        {
            return false;
        }
    }
    return true;
}

void CBlockLogsFilter::GetTxReceiptLogs(const bool fAll, ReceiptLogsVec& receiptLogs)
{
    if (fAll)
    {
        for (auto& r : vHisReceiptLogs)
        {
            receiptLogs.push_back(r);
        }
        for (auto& r : vReceiptLogs)
        {
            receiptLogs.push_back(r);
        }
    }
    else
    {
        for (auto& r : vReceiptLogs)
        {
            receiptLogs.push_back(r);
            vHisReceiptLogs.push_back(r);
            nHisLogsCount += r.matchLogs.size();
        }

        vReceiptLogs.clear();
        nLogsCount = 0;
        nPrevGetChangesTime = GetTime();

        while (vHisReceiptLogs.size() > 0 && nHisLogsCount > MAX_FILTER_CACHE_COUNT * 2)
        {
            auto it = vHisReceiptLogs.begin();
            nHisLogsCount -= it->matchLogs.size();
            vHisReceiptLogs.erase(it);
        }
    }
}

//////////////////////////////
// CBlockMakerFilter

bool CBlockMakerFilter::isTimeout()
{
    if (GetTime() - nPrevGetChangesTime >= FILTER_DEFAULT_TIMEOUT)
    {
        return true;
    }
    return false;
}

bool CBlockMakerFilter::AddBlockHash(const uint256& hashFork, const uint256& hashBlock, const uint256& hashPrev)
{
    if (hashFork != hashFork)
    {
        return true;
    }
    if (mapBlockHash.size() >= MAX_FILTER_CACHE_COUNT * 2)
    {
        return false;
    }
    mapBlockHash[hashBlock] = hashPrev;
    return true;
}

bool CBlockMakerFilter::GetFilterBlockHashs(const uint256& hashLastBlock, const bool fAll, std::vector<uint256>& vBlockhash)
{
    if (fAll)
    {
        uint256 hash = hashLastBlock;
        while (true)
        {
            auto it = mapBlockHash.find(hash);
            if (it == mapBlockHash.end())
            {
                break;
            }
            vBlockhash.push_back(hash);
            hash = it->second;
        }
        while (true)
        {
            auto it = mapHisBlockHash.find(hash);
            if (it == mapHisBlockHash.end())
            {
                break;
            }
            vBlockhash.push_back(hash);
            hash = it->second;
        }
    }
    else
    {
        if (mapBlockHash.count(hashLastBlock) == 0)
        {
            return true;
        }
        uint256 hash = hashLastBlock;
        while (true)
        {
            auto it = mapBlockHash.find(hash);
            if (it == mapBlockHash.end())
            {
                break;
            }
            vBlockhash.push_back(hash);
            mapHisBlockHash.insert(*it);
            hash = it->second;
        }
        mapBlockHash.clear();
        nPrevGetChangesTime = GetTime();
        std::reverse(vBlockhash.begin(), vBlockhash.end());

        while (mapHisBlockHash.size() > MAX_FILTER_CACHE_COUNT * 2)
        {
            mapHisBlockHash.erase(mapHisBlockHash.begin());
        }
    }
    return true;
}

//////////////////////////////
// CBlockPendingTxFilter

bool CBlockPendingTxFilter::isTimeout()
{
    if (GetTime() - nPrevGetChangesTime >= FILTER_DEFAULT_TIMEOUT)
    {
        return true;
    }
    return false;
}

bool CBlockPendingTxFilter::AddPendingTx(const uint256& hashFork, const uint256& txid)
{
    if (hashFork != hashFork)
    {
        return true;
    }
    if (setTxid.size() >= MAX_FILTER_CACHE_COUNT * 2)
    {
        return false;
    }
    if (setTxid.find(txid) == setTxid.end())
    {
        setTxid.insert(txid);
        if (setHisTxid.find(txid) != setHisTxid.end())
        {
            setHisTxid.erase(txid);
        }
    }
    return true;
}

bool CBlockPendingTxFilter::GetFilterTxids(const uint256& hashFork, const bool fAll, std::vector<uint256>& vTxid)
{
    if (hashFork != hashFork)
    {
        return true;
    }
    if (fAll)
    {
        for (auto& txid : setTxid)
        {
            vTxid.push_back(txid);
        }
        for (auto& txid : setHisTxid)
        {
            vTxid.push_back(txid);
        }
    }
    else
    {
        for (auto& txid : setTxid)
        {
            vTxid.push_back(txid);
            setHisTxid.insert(txid);
            if (++nSeqCreate == 0)
            {
                nSeqCreate++;
                mapHisSeq.clear();
                setHisTxid.clear();
                setHisTxid.insert(txid);
            }
            mapHisSeq.insert(std::make_pair(nSeqCreate, txid));
        }
        setTxid.clear();
        nPrevGetChangesTime = GetTime();

        while (setHisTxid.size() > MAX_FILTER_CACHE_COUNT * 2 && mapHisSeq.size() > 0)
        {
            auto it = mapHisSeq.begin();
            setHisTxid.erase(it->second);
            mapHisSeq.erase(it);
        }
    }
    return true;
}

} // namespace hashahead
