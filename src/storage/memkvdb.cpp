// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "memkvdb.h"

using namespace hnbase;

namespace hashahead
{
namespace storage
{

CMemDBEngine::CMemDBEngine()
  : fBatchhCommit(false) {}

CMemDBEngine::~CMemDBEngine() {}

bool CMemDBEngine::Open()
{
    mapKv.clear();
    fBatchhCommit = false;
    mapBatchCache.clear();
    return true;
}

void CMemDBEngine::Close()
{
    mapKv.clear();
    fBatchhCommit = false;
    mapBatchCache.clear();
}

bool CMemDBEngine::TxnBegin()
{
    fBatchhCommit = true;
    mapBatchCache.clear();
    return true;
}

bool CMemDBEngine::TxnCommit()
{
    if (fBatchhCommit)
    {
        for (auto& kv : mapBatchCache)
        {
            mapKv[kv.first] = kv.second;
        }
        fBatchhCommit = false;
        mapBatchCache.clear();
    }
    return true;
}

void CMemDBEngine::TxnAbort()
{
    fBatchhCommit = false;
    mapBatchCache.clear();
}

bool CMemDBEngine::Get(CBufStream& ssKey, CBufStream& ssValue)
{
    bytes btKey;
    ssKey.GetData(btKey);

    if (fBatchhCommit)
    {
        auto it = mapBatchCache.find(btKey);
        if (it != mapBatchCache.end())
        {
            ssValue.Write((char*)it->second.data(), it->second.size());
            return true;
        }
    }

    auto it = mapKv.find(btKey);
    if (it != mapKv.end())
    {
        ssValue.Write((char*)it->second.data(), it->second.size());
        return true;
    }
    return false;
}

bool CMemDBEngine::Put(CBufStream& ssKey, CBufStream& ssValue, bool fOverwrite)
{
    bytes btKey, btValue;
    ssKey.GetData(btKey);
    ssValue.GetData(btValue);
    if (fBatchhCommit)
    {
        mapBatchCache[btKey] = btValue;
    }
    else
    {
        mapKv[btKey] = btValue;
    }
    return true;
}

bool CMemDBEngine::Remove(CBufStream& ssKey)
{
    bytes btKey;
    ssKey.GetData(btKey);
    if (fBatchhCommit)
    {
        mapBatchCache.erase(btKey);
    }
    mapKv.erase(btKey);
    return true;
}

bool CMemDBEngine::RemoveAll()
{
    mapKv.clear();
    mapBatchCache.clear();
    return true;
}

bool CMemDBEngine::MoveFirst()
{
    if (mapKv.empty())
    {
        return false;
    }
    itCurr = mapKv.begin();
    return true;
}

bool CMemDBEngine::MoveTo(CBufStream& ssKey)
{
    if (mapKv.empty())
    {
        return false;
    }
    bytes btKey;
    ssKey.GetData(btKey);
    itCurr = mapKv.find(btKey);
    if (itCurr == mapKv.end())
    {
        return false;
    }
    return true;
}

bool CMemDBEngine::MoveNext(CBufStream& ssKey, CBufStream& ssValue)
{
    if (itCurr == mapKv.end())
    {
        return false;
    }
    ssKey.Write((char*)itCurr->first.data(), itCurr->first.size());
    ssValue.Write((char*)itCurr->second.data(), itCurr->second.size());
    ++itCurr;
    return true;
}

} // namespace storage
} // namespace hashahead
