// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_MEMKVDB_H
#define STORAGE_MEMKVDB_H

#include "hnbase.h"

using namespace hnbase;

namespace hashahead
{
namespace storage
{

class CMemDBEngine : public CKVDBEngine
{
public:
    CMemDBEngine();
    ~CMemDBEngine();

    bool Open() override;
    void Close() override;
    bool TxnBegin() override;
    bool TxnCommit() override;
    void TxnAbort() override;
    bool Get(CBufStream& ssKey, CBufStream& ssValue) override;
    bool Put(CBufStream& ssKey, CBufStream& ssValue, bool fOverwrite) override;
    bool Remove(CBufStream& ssKey) override;
    bool RemoveAll() override;
    bool MoveFirst() override;
    bool MoveTo(CBufStream& ssKey) override;
    bool MoveNext(CBufStream& ssKey, CBufStream& ssValue) override;

protected:
    bytesmap mapKv;
    bool fBatchhCommit;
    bytesmap mapBatchCache;
    bytesmap::iterator itCurr;
};

} // namespace storage
} // namespace hashahead

#endif // STORAGE_MEMKVDB_H
