// Copyright (c) 2021-2023 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_ADDRESSBLACKLISTDB_H
#define STORAGE_ADDRESSBLACKLISTDB_H

#include <boost/filesystem.hpp>

#include "destination.h"
#include "hnbase.h"

namespace hashahead
{
namespace storage
{

class CAddressBlacklistDB
{
public:
    CAddressBlacklistDB();
    ~CAddressBlacklistDB();

    bool Initialize(const boost::filesystem::path& pathData);
    void Deinitialize();
    bool Remove();
    bool AddAddress(const CDestination& dest);
    void RemoveAddress(const CDestination& dest);
    bool IsExist(const CDestination& dest);
    void ListAddress(std::set<CDestination>& setAddressOut);

protected:
    bool Load();
    bool Save();

protected:
    hnbase::CRWAccess rwAccess;
    boost::filesystem::path pathFile;
    std::set<CDestination> setBlacklistAddress;
};

} // namespace storage
} // namespace hashahead

#endif // STORAGE_ADDRESSBLACKLISTDB_H
