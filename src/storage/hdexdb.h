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
