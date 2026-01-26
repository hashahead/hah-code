// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HASHAHEAD_CHAINSNAPSHOT_H
#define HASHAHEAD_CHAINSNAPSHOT_H

#include "base.h"

namespace hashahead
{

class CChainSnapshot : public IChainSnapshot
{
public:
    CChainSnapshot();
    ~CChainSnapshot();

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    bool HandleInvoke() override;
    void HandleHalt() override;
    bool StartHeightSnapshot(const uint32 nSnapshotHeight, uint256& hashSnapshotBlock) override;
    uint32 GetSnapshotStatus(uint256& hashSnapshotBlock) override;

    void SnapshotThreadFunc();
    bool WaitExitEvent(const int64 nSeconds);

    void SnapshotWork();
    bool CheckSnapshotBlock(uint256& hashSnapshotBlock, std::vector<uint256>& vForkHash);

protected:
    ICoreProtocol* pCoreProtocol;
    IBlockChain* pBlockChain;

    bool fCfgChainSnapshot;
    uint32 nCfgSnapshotCycleDays;
    uint32 nCfgMaxSnapshots;
    uint32 nCfgSnapshotCycleHeight;
    bool fCfgSnapshotRecovery;
    uint32 nRpcCreateSnapshotHeight;
    std::string strCfgSnapshotRecoveryDir;

    hnbase::CThread thrSnapshot;
    bool fThreadRun;
    boost::condition_variable condExit;
    boost::mutex mutex;

    uint32 nSnapshotStatus; // 0: no snapshot, 1: snapshoting, 2: snapshot end
    uint256 hashSnapshotingBlock;
    boost::mutex mutexStatus;
};

} // namespace hashahead

#endif // HASHAHEAD_CHAINSNAPSHOT_H
