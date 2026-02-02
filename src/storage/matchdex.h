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
// CDexOrderKey

class CDexOrderKey
{
public:
    CDexOrderKey()
      : nHeight(0), nSlot(0) {}
    CDexOrderKey(const uint256 nPriceIn, const uint32 nHeightIn, const uint16 nSlotIn, const uint256& hashOrderRandomIn)
      : nPrice(nPriceIn), nHeight(nHeightIn), nSlot(nSlotIn), hashOrderRandom(hashOrderRandomIn) {}

    uint64 GetHeightSlotValue() const
    {
        return GetHeightSlotStatic(nHeight, nSlot);
    }

    static inline uint64 GetHeightSlotStatic(const uint32 nHeight, const uint16 nSlot)
    {
        return (((uint64)nHeight << 32) | nSlot);
    }
    static inline uint32 GetHeightByHsStatic(const uint64 nHeightSlot)
    {
        return (uint32)(nHeightSlot >> 32);
    }
    static inline uint16 GetSlotByHsStatic(const uint64 nHeightSlot)
    {
        return (uint16)(nHeightSlot & 0xFFFF);
    }

    friend bool operator==(const CDexOrderKey& a, const CDexOrderKey& b);
    friend inline bool operator!=(const CDexOrderKey& a, const CDexOrderKey& b)
    {
        return (!(a == b));
    }

public:
    uint256 nPrice;
    uint32 nHeight;
    uint16 nSlot;
    uint256 hashOrderRandom;
};

///////////////////////////////////
// CDexOrderValue

class CDexOrderValue
{
public:
    CDexOrderValue()
      : nOrderNumber(0), nCompleteCount(0) {}
    CDexOrderValue(const CDestination& destOrderIn, const uint64 nOrderNumberIn, const uint256& nOrderAmountIn, const uint256& nCompleteAmountIn,
                   const uint64 nCompleteCountIn, const CChainId nOrderAtChainIdIn, const uint256& hashOrderAtBlockIn)
      : destOrder(destOrderIn), nOrderNumber(nOrderNumberIn), nOrderAmount(nOrderAmountIn), nCompleteAmount(nCompleteAmountIn),
        nCompleteCount(nCompleteCountIn), nOrderAtChainId(nOrderAtChainIdIn), hashOrderAtBlock(hashOrderAtBlockIn) {}

    uint256 GetSurplusAmount() const
    {
        if (nOrderAmount > nCompleteAmount)
        {
            return nOrderAmount - nCompleteAmount;
        }
        return 0;
    }

    friend bool operator==(const CDexOrderValue& a, const CDexOrderValue& b);
    friend inline bool operator!=(const CDexOrderValue& a, const CDexOrderValue& b)
    {
        return (!(a == b));
    }

public:
    CDestination destOrder;
    uint64 nOrderNumber;
    uint256 nOrderAmount;
    uint256 nCompleteAmount;
    uint64 nCompleteCount;
    CChainId nOrderAtChainId;
    uint256 hashOrderAtBlock;
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
