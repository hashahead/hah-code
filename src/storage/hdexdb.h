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
