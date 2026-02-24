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
    void PruneForkKvData(const uint256& hashFork, const uint32 nRefLastHeight);

protected:
    ICoreProtocol* pCoreProtocol;
    IBlockChain* pBlockChain;

    bool fCfgPruneStateData;
    bool fCfgTraceDb;
    uint32 nCfgPruneRetentionDays;
    uint32 nCfgPruneReserveHeight;

    hnbase::CThread thrPruneDb;
    bool fExit;
    boost::condition_variable condExit;
    boost::mutex mutex;

    std::map<uint256, uint32> mapPrevPruneHeight;
    std::map<uint256, uint32> mapPrevKvPruneHeight;
};

} // namespace hashahead

#endif // HASHAHEAD_PRUNEDB_H
