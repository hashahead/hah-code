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

} // namespace hashahead
