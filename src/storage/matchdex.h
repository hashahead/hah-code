// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_MATCHDEX_H
#define STORAGE_MATCHDEX_H

#include <map>

#include "block.h"
#include "destination.h"
#include "hnbase.h"
#include "transaction.h"
#include "uint256.h"

namespace hashahead
{
namespace storage
{

///////////////////////////////////
// CMatchDex

class CMatchDex
{
public:
    CMatchDex() {}

public:
    std::map<CChainId, std::set<uint256>> mapChainIdLinkCoinDexPair; // key: at chain id, value: coin dex pair hash
};
typedef std::shared_ptr<CMatchDex> SHP_MATCH_DEX;
#define MAKE_SHARED_MATCH_DEX std::make_shared<CMatchDex>

} // namespace storage
} // namespace hashahead

#endif // STORAGE_MATCHDEX_H
