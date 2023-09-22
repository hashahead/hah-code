// Copyright (c) 2021-2023 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_PURGER_H
#define STORAGE_PURGER_H

#include <boost/filesystem.hpp>

#include "uint256.h"

namespace hashahead
{
namespace storage
{

class CPurger
{
public:
    bool ResetDB(const boost::filesystem::path& pathDataLocation, const uint256& hashGenesisBlockIn, const bool fFullDbIn) const;
    bool RemoveBlockFile(const boost::filesystem::path& pathDataLocation) const;
    bool operator()(const boost::filesystem::path& pathDataLocation, const uint256& hashGenesisBlockIn, const bool fFullDbIn) const
    {
        return (ResetDB(pathDataLocation, hashGenesisBlockIn, fFullDbIn) && RemoveBlockFile(pathDataLocation));
    }
};

} // namespace storage
} // namespace hashahead

#endif //STORAGE_PURGER_H
