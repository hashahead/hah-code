// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_FILTERDATA_H
#define STORAGE_FILTERDATA_H

#include <boost/filesystem.hpp>

#include "cmstruct.h"
#include "hnbase.h"

namespace hashahead
{
namespace storage
{

class CFilterData
{
public:
    CFilterData();
    ~CFilterData();

    bool Initialize(const boost::filesystem::path& pathData);
    bool Remove();
    bool Save(const std::map<uint256, CBlockLogsFilter>& mapLogFilter, const std::map<uint256, CBlockMakerFilter>& mapBlockFilter, const std::map<uint256, CBlockPendingTxFilter>& mapTxFilter);
    bool Load(std::map<uint256, CBlockLogsFilter>& mapLogFilter, std::map<uint256, CBlockMakerFilter>& mapBlockFilter, std::map<uint256, CBlockPendingTxFilter>& mapTxFilter);

protected:
    boost::filesystem::path pathFilterFile;
};

} // namespace storage
} // namespace hashahead

#endif // STORAGE_FILTERDATA_H
