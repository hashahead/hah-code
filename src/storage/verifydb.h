// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_VERIFYDB_H
#define STORAGE_VERIFYDB_H

#include "block.h"
#include "hnbase.h"
#include "timeseries.h"

namespace hashahead
{
namespace storage
{

class CVerifyDB
{
public:
    CVerifyDB() {}
    bool Initialize(const boost::filesystem::path& pathData);
    void Deinitialize();
    void Clear();

    bool AddBlockVerify(const CBlockIndex& outline, const uint32 nRootCrc);
    bool RetrieveBlockVerify(const uint256& hashBlock, CBlockVerify& verifyBlock);
    std::size_t GetBlockVerifyCount();
    bool GetBlockVerify(const std::size_t pos, CBlockVerify& verifyBlock);

protected:
    bool LoadVerifyDB();

protected:
    CTimeSeriesCached tsVerify;

    mutable boost::shared_mutex rwAccess;
    std::map<uint256, std::size_t> mapVerifyIndex;
    std::vector<CBlockVerify> vVerifyIndex;
};

} // namespace storage
} // namespace hashahead

#endif // STORAGE_VERIFYDB_H
