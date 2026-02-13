// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_HDEXDB_H
#define STORAGE_HDEXDB_H

#include <map>

#include "block.h"
#include "dbstruct.h"
#include "destination.h"
#include "hnbase.h"
#include "matchdex.h"
#include "timeseries.h"
#include "transaction.h"
#include "triedb.h"
#include "uint256.h"

namespace hashahead
{
namespace storage
{

///////////////////////////////////
// CCacheBlockDexOrder

class CCacheBlockDexOrder
{
public:
    CCacheBlockDexOrder()
    {
        ptrMatchDex = MAKE_SHARED_MATCH_DEX();
    }

    void Clear()
    {
        hashLastBlock = 0;
    }

    void SetLastBlock(const uint256& hashBlock)
    {
        hashLastBlock = hashBlock;
    }
    const uint256& GetLastBlock() const
    {
        return hashLastBlock;
    }
    const SHP_MATCH_DEX GetMatchDex() const
    {
        return ptrMatchDex;
    }

    bool AddDexOrderCache(const uint256& hashDexOrder, const CDestination& destOrder, const uint64 nOrderNumber, const CDexOrderBody& dexOrder,
                          const CChainId nOrderAtChainId, const uint256& hashOrderAtBlock, const uint256& nPrevCompletePrice);
    bool UpdateCompleteOrderCache(const uint256& hashCoinPair, const uint256& hashDexOrder, const uint256& nCompleteAmount, const uint64 nCompleteCount);
    void UpdatePeerProveLastBlock(const CChainId nPeerChainId, const uint256& hashLastProveBlock);
    void UpdateCompletePrice(const uint256& hashCoinPair, const uint256& nCompletePrice);

    bool GetMatchDexResult(std::map<uint256, CMatchOrderResult>& mapMatchResult);
    bool ListMatchDexOrder(const std::string& strCoinSymbolSell, const std::string& strCoinSymbolBuy, const uint64 nGetCount, CRealtimeDexOrder& realDexOrder);

    friend bool operator==(const CCacheBlockDexOrder& a, const CCacheBlockDexOrder& b)
    {
        if (a.hashLastBlock != b.hashLastBlock)
        {
            return false;
        }
        if (*a.ptrMatchDex != *b.ptrMatchDex)
        {
            return false;
        }
        return true;
    }
    friend inline bool operator!=(const CCacheBlockDexOrder& a, const CCacheBlockDexOrder& b)
    {
        return (!(a == b));
    }

protected:
    uint256 hashLastBlock;

    SHP_MATCH_DEX ptrMatchDex;
};
typedef std::shared_ptr<CCacheBlockDexOrder> SHP_CACHE_BLOCK_DEX_ORDER;
#define MAKE_SHARED_CACHE_BLOCK_DEX_ORDER std::make_shared<CCacheBlockDexOrder>

///////////////////////////////////
// CHdexDB

class CHdexDB
{
public:
    CHdexDB()
      : fPrune(false) {}

    bool Initialize(const boost::filesystem::path& pathData, const bool fPruneIn = false);
    void Deinitialize();
    void Clear();

    bool AddDexOrder(const uint256& hashFork, const uint256& hashRefBlock, const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDexOrderHeader, CDexOrderBody>& mapDexOrder, const std::map<CChainId, std::vector<CBlockCoinTransferProve>>& mapCrossTransferProve,
                     const std::map<uint256, uint256>& mapCoinPairCompletePrice, const std::set<CChainId>& setPeerCrossChainId, const std::map<CDexOrderHeader, std::vector<CCompDexOrderRecord>>& mapCompDexOrderRecord, const std::map<CChainId, CBlockProve>& mapBlockProve, uint256& hashNewRoot);
protected:
    enum
    {
        MAX_CACHE_BLOCK_DEX_ORDER_COUNT = 32
    };

    hnbase::CRWAccess rwAccess;
    CTrieDB dbTrie;
    bool fPrune;
};

} // namespace storage
} // namespace hashahead

#endif // STORAGE_HDEXDB_H
