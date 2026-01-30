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
