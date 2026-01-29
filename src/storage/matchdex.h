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
// CMatchTools

class CMatchTools
{
public:
    CMatchTools() {}

public:
    static inline uint256 CalcPeerPrice(const uint256& nSelfPrice, const uint256& nPriceAnchor)
    {
        if (nSelfPrice == 0)
        {
            return 0;
        }
        return nPriceAnchor * nPriceAnchor / nSelfPrice;
    }
    static inline uint256 CalcCompletePrice(const uint256& nBuyPrice, const uint256& nSellPrice, const uint256& nPrevCompletePrice)
    {
        if (nSellPrice >= nPrevCompletePrice)
        {
            return nSellPrice;
        }
        if (nBuyPrice >= nPrevCompletePrice)
        {
            return nPrevCompletePrice;
        }
        return nBuyPrice;
    }
    static inline uint256 CalcBuyAmount(const uint256& nSellAmount, const uint256& nCompletePrice, const uint256& nSellPriceAnchor)
    {
        if (nSellPriceAnchor == 0)
        {
            return 0;
        }
        return nSellAmount * nCompletePrice / nSellPriceAnchor;
    }
    static inline uint256 CalcSellAmount(const uint256& nBuyAmount, const uint256& nCompletePrice, const uint256& nSellPriceAnchor)
    {
        if (nCompletePrice == 0)
        {
            return 0;
        }
        return nBuyAmount * nSellPriceAnchor / nCompletePrice;
    }
};

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
