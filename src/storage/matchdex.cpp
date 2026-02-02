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

} // namespace storage
} // namespace hashahead
