// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "filterdata.h"

using namespace std;
using namespace boost::filesystem;
using namespace hnbase;

namespace hashahead
{
namespace storage
{

//////////////////////////////
// CFilterData

CFilterData::CFilterData()
{
}

CFilterData::~CFilterData()
{
}

bool CFilterData::Initialize(const path& pathData)
{
    path pathFilter = pathData / "fdb";

    if (!exists(pathFilter))
    {
        create_directories(pathFilter);
    }

    if (!is_directory(pathFilter))
    {
        return false;
    }

    pathFilterFile = pathFilter / "filter.dat";

    if (exists(pathFilterFile) && !is_regular_file(pathFilterFile))
    {
        return false;
    }

    return true;
}

bool CFilterData::Remove()
{
    if (is_regular_file(pathFilterFile))
    {
        return remove(pathFilterFile);
    }
    return true;
}

bool CFilterData::Save(const std::map<uint256, CBlockLogsFilter>& mapLogFilter, const std::map<uint256, CBlockMakerFilter>& mapBlockFilter, const std::map<uint256, CBlockPendingTxFilter>& mapTxFilter)
{
    FILE* fp = fopen(pathFilterFile.string().c_str(), "w");
    if (fp == nullptr)
    {
        return false;
    }
    fclose(fp);

    if (!is_regular_file(pathFilterFile))
    {
        return false;
    }

    try
    {
        CFileStream fs(pathFilterFile.string().c_str());
        fs << mapLogFilter << mapBlockFilter << mapTxFilter;
    }
    catch (std::exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CFilterData::Load(std::map<uint256, CBlockLogsFilter>& mapLogFilter, std::map<uint256, CBlockMakerFilter>& mapBlockFilter, std::map<uint256, CBlockPendingTxFilter>& mapTxFilter)
{
    mapLogFilter.clear();
    mapBlockFilter.clear();
    mapTxFilter.clear();

    if (!is_regular_file(pathFilterFile))
    {
        return true;
    }

    try
    {
        CFileStream fs(pathFilterFile.string().c_str());
        fs >> mapLogFilter >> mapBlockFilter >> mapTxFilter;
    }
    catch (std::exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }

    remove(pathFilterFile);
    return true;
}

} // namespace storage
} // namespace hashahead
