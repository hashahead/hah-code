// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_TXPOOLDATA_H
#define STORAGE_TXPOOLDATA_H

#include <boost/filesystem.hpp>

#include "hnbase.h"
#include "transaction.h"

namespace hashahead
{
namespace storage
{

class CTxPoolData
{
public:
    CTxPoolData();
    ~CTxPoolData();
    bool Initialize(const boost::filesystem::path& pathData);
    bool Remove();
    bool Save(const std::map<uint256, std::vector<std::pair<uint256, CTransaction>>>& mapTx);
    bool Load(std::map<uint256, std::vector<std::pair<uint256, CTransaction>>>& mapTx);
    bool LoadCheck(std::map<uint256, std::vector<std::pair<uint256, CTransaction>>>& mapTx);

protected:
    boost::filesystem::path pathTxPoolFile;
};

} // namespace storage
} // namespace hashahead

#endif //STORAGE_TXPOOLDATA_H
