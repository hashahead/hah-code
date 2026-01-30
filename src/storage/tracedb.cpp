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
