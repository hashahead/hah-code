// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_SNAPSHOTDB_H
#define STORAGE_SNAPSHOTDB_H

#include <boost/filesystem.hpp>

#include "block.h"
#include "cmstruct.h"
#include "destination.h"
#include "hnbase.h"

using namespace std;

namespace fs = boost::filesystem;

namespace hashahead
{
namespace storage
{

class CSnapshotDB
{
public:
    CSnapshotDB();
    ~CSnapshotDB();

    bool Initialize(const fs::path& pathData);
    void Deinitialize();
    bool Remove();

    bool IsSnapshotBlock(const uint256& hashBlock);
protected:
    hnbase::CRWAccess rwAccess;

    fs::path pathDataLocation;
    fs::path pathSnapshot;
};

} // namespace storage
} // namespace hashahead

#endif // STORAGE_SNAPSHOTDB_H
