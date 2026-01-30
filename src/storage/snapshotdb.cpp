// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "snapshotdb.h"

#include "dbstruct.h"
#include "timeseries.h"

using namespace std;
using namespace hnbase;

namespace fs = boost::filesystem;

namespace hashahead
{
namespace storage
{

//////////////////////////////
// CSnapshotDB

CSnapshotDB::CSnapshotDB()
{
}

CSnapshotDB::~CSnapshotDB()
{
}

bool CSnapshotDB::Initialize(const fs::path& pathData)
{
    pathDataLocation = pathData;
    pathSnapshot = pathDataLocation / "snapshot";
    if (!fs::exists(pathSnapshot))
    {
        fs::create_directories(pathSnapshot);
    }
    if (!fs::is_directory(pathSnapshot))
    {
        return false;
    }
    return true;
}

void CSnapshotDB::Deinitialize()
{
}

bool CSnapshotDB::Remove()
{
    return true;
}

} // namespace storage
} // namespace hashahead
