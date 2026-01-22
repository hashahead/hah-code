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

} // namespace hashahead
