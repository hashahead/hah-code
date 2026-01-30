// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_TRACEDB_H
#define STORAGE_TRACEDB_H

#include <boost/thread/thread.hpp>

#include "dbstruct.h"
#include "hnbase.h"
#include "transaction.h"
#include "triedb.h"

#define MAX_CACHE_BLOCK_COUNT 1024

namespace hashahead
{
namespace storage
{

class CTraceDB
{
public:
    CTraceDB()
      : fUseCacheData(false), fPrune(false) {}
    bool Initialize(const boost::filesystem::path& pathData, const bool fUseCacheDataIn, const bool fPruneIn = false);
    void Deinitialize();

protected:
    boost::filesystem::path pathTrace;
    hnbase::CRWAccess rwAccess;
    bool fUseCacheData;
    bool fPrune;
};

} // namespace storage
} // namespace hashahead

#endif // STORAGE_TRACEDB_H
