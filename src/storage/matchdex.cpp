// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "matchdex.h"

#include "block.h"
#include "param.h"

using namespace std;
using namespace hnbase;

namespace hashahead
{
namespace storage
{

///////////////////////////////////
// CDexOrderKey

bool operator==(const CDexOrderKey& a, const CDexOrderKey& b)
{
    if (a.nPrice != b.nPrice)
    {
        StdLog("CDexOrderKey", "Compare: nPrice error, a.nPrice: %s, b.nPrice: %s", CoinToTokenBigFloat(a.nPrice).c_str(), CoinToTokenBigFloat(b.nPrice).c_str());
        return false;
    }
    if (a.nHeight != b.nHeight)
    {
        StdLog("CDexOrderKey", "Compare: nHeight error, a.nHeight: %d, b.nHeight: %d", a.nHeight, b.nHeight);
        return false;
    }
    if (a.nSlot != b.nSlot)
    {
        StdLog("CDexOrderKey", "Compare: nSlot error, a.nSlot: %d, b.nSlot: %d", a.nSlot, b.nSlot);
        return false;
    }
    if (a.hashOrderRandom != b.hashOrderRandom)
    {
        StdLog("CDexOrderKey", "Compare: hashOrderRandom error, a.hashOrderRandom: %s, b.hashOrderRandom: %s", a.hashOrderRandom.ToString().c_str(), b.hashOrderRandom.ToString().c_str());
        return false;
    }
    return true;
}

///////////////////////////////////
// CDexOrderValue

bool operator==(const CDexOrderValue& a, const CDexOrderValue& b)
{
    if (a.destOrder != b.destOrder)
    {
        StdLog("CDexOrderValue", "Compare: destOrder error, a.destOrder: %s, b.destOrder: %s", a.destOrder.ToString().c_str(), b.destOrder.ToString().c_str());
        return false;
    }
    if (a.nOrderNumber != b.nOrderNumber)
    {
        StdLog("CDexOrderValue", "Compare: nOrderNumber error, a.nOrderNumber: %lu, b.nOrderNumber: %lu", a.nOrderNumber, b.nOrderNumber);
        return false;
    }
    if (a.nOrderAmount != b.nOrderAmount)
    {
        StdLog("CDexOrderKey", "Compare: nOrderAmount error, a.nOrderAmount: %s, b.nOrderAmount: %s", CoinToTokenBigFloat(a.nOrderAmount).c_str(), CoinToTokenBigFloat(b.nOrderAmount).c_str());
        return false;
    }
    if (a.nCompleteAmount != b.nCompleteAmount)
    {
        StdLog("CDexOrderKey", "Compare: nOrderAmount error, a.nOrderAmount: %s, b.nOrderAmount: %s", CoinToTokenBigFloat(a.nOrderAmount).c_str(), CoinToTokenBigFloat(b.nOrderAmount).c_str());
        return false;
    }
    if (a.nCompleteCount != b.nCompleteCount)
    {
        StdLog("CDexOrderValue", "Compare: nCompleteCount error, a.nCompleteCount: %lu, b.nCompleteCount: %lu", a.nCompleteCount, b.nCompleteCount);
        return false;
    }
    if (a.nOrderAtChainId != b.nOrderAtChainId)
    {
        StdLog("CDexOrderValue", "Compare: nOrderAtChainId error, a.nOrderAtChainId: %u, b.nOrderAtChainId: %u", a.nOrderAtChainId, b.nOrderAtChainId);
        return false;
    }
    if (a.hashOrderAtBlock != b.hashOrderAtBlock)
    {
        StdLog("CDexOrderValue", "Compare: hashOrderAtBlock error, a.hashOrderAtBlock: %s, b.hashOrderAtBlock: %s", a.hashOrderAtBlock.GetBhString().c_str(), b.hashOrderAtBlock.GetBhString().c_str());
        return false;
    }
    return true;
}

///////////////////////////////////
// CCoinDexPair

bool CCoinDexPair::AddOrder(const uint256& hashDexOrder, const std::string& strCoinSymbolOwner, const CDestination& destOrder, const uint64 nOrderNumber, const uint256& nOrderAmount, const uint256& nOrderPrice, const uint256& nCompleteAmount,
                            const uint64 nCompleteCount, const CChainId nOrderAtChainId, const uint256& hashOrderAtBlock, const uint32 nHeight, const uint16 nSlot, const uint256& hashOrderRandom)
{
    CDexOrderKey keyOrder(nOrderPrice, nHeight, nSlot, hashOrderRandom);
    CDexOrderValue* pOrderValue = nullptr;
    uint8 nOrderType = 0;
    if (strCoinSymbolOwner == strCoinSymbolSell)
    {
        pOrderValue = &mapSellOrder[keyOrder];
        nOrderType = CDP_ORDER_TYPE_SELL;
    }
    else if (strCoinSymbolOwner == strCoinSymbolBuy)
    {
        pOrderValue = &mapBuyOrder[keyOrder];
        nOrderType = CDP_ORDER_TYPE_BUY;
    }
    else
    {
        StdLog("CCoinDexPair", "Add Order: Coin symbol owner error, Coin symbol owner: %s, symbol sell: %s, symbol buy: %s",
               strCoinSymbolOwner.c_str(), strCoinSymbolSell.c_str(), strCoinSymbolBuy.c_str());
        return false;
    }

    pOrderValue->destOrder = destOrder;
    pOrderValue->nOrderNumber = nOrderNumber;
    pOrderValue->nOrderAmount = nOrderAmount;
    pOrderValue->nCompleteAmount = nCompleteAmount;
    pOrderValue->nCompleteCount = nCompleteCount;
    pOrderValue->nOrderAtChainId = nOrderAtChainId;
    pOrderValue->hashOrderAtBlock = hashOrderAtBlock;

    mapDexOrderIndex[hashDexOrder] = std::make_pair(keyOrder, nOrderType); // key: dex order hash, value: 1: order key, 2: order type

    if (nSellChainId != 0 && nSellChainId == nBuyChainId && hashOrderAtBlock != 0)
    {
        uint64 nHeightSlot = CDexOrderKey::GetHeightSlotStatic(CBlock::GetBlockHeightByHash(hashOrderAtBlock), CBlock::GetBlockSlotByHash(hashOrderAtBlock));
        if (nHeightSlot > nMatchHeightSlot)
        {
            nMatchHeightSlot = nHeightSlot;
        }
    }
    return true;
}

void CCoinDexPair::SetPrevCompletePrice(const uint256& nPrevPrice)
{
    nPrevCompletePrice = nPrevPrice;
}
} // namespace storage
} // namespace hashahead
