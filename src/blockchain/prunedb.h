// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HASHAHEAD_PRUNEDB_H
#define HASHAHEAD_PRUNEDB_H

#include "base.h"

namespace hashahead
{

class CPruneDb : public IPruneDb
{
public:
    CPruneDb();
    ~CPruneDb();

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    bool HandleInvoke() override;
    void HandleHalt() override;

    void PruneDbThreadFunc();
    bool WaitExitEvent(const int64 nSeconds);

    void PruneStateWork();
    void PruneForkData(const uint256& hashFork, const uint32 nRefLastHeight);
protected:
    ICoreProtocol* pCoreProtocol;
    IBlockChain* pBlockChain;
};

} // namespace hashahead

#endif // HASHAHEAD_PRUNEDB_H
