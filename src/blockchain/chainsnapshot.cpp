// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainsnapshot.h"

#include "block.h"

using namespace hnbase;

namespace hashahead
{

/////////////////////////////////////////////////
// CChainSnapshot

CChainSnapshot::CChainSnapshot()
  : thrSnapshot("snapshot", boost::bind(&CChainSnapshot::SnapshotThreadFunc, this))
{
    fThreadRun = false;

    pCoreProtocol = nullptr;
    pBlockChain = nullptr;
}

CChainSnapshot::~CChainSnapshot()
{
}

bool CChainSnapshot::HandleInitialize()
{
    if (!GetObject("coreprotocol", pCoreProtocol))
    {
        Error("Failed to request coreprotocol");
        return false;
    }
    if (!GetObject("blockchain", pBlockChain))
    {
        Error("Failed to request blockchain");
        return false;
    }
    return true;
}

void CChainSnapshot::HandleDeinitialize()
{
    pCoreProtocol = nullptr;
    pBlockChain = nullptr;

    fThreadRun = false;

    fCfgChainSnapshot = false;
    nCfgSnapshotCycleDays = 0;
    nCfgMaxSnapshots = 0;
    nCfgSnapshotCycleHeight = 0;
    fCfgSnapshotRecovery = false;
    strCfgSnapshotRecoveryDir = "";
    nSnapshotStatus = 0;
}

bool CChainSnapshot::HandleInvoke()
{
    if (fCfgSnapshotRecovery)
    {
        if (!pBlockChain->SnapshotRecovery(strCfgSnapshotRecoveryDir))
        {
            StdLog("CChainSnapshot", "Handle invoke: Snapshot recovery failed");
            return false;
        }
    }
    if (fCfgChainSnapshot)
    {
        fThreadRun = true;
        if (!ThreadDelayStart(thrSnapshot))
        {
            StdLog("CChainSnapshot", "Handle invoke: Start thread failed");
            return false;
        }
    }
    return true;
}

void CChainSnapshot::HandleHalt()
{
    if (fThreadRun)
    {
        fThreadRun = false;
        condExit.notify_all();

        thrSnapshot.Interrupt();
        ThreadExit(thrSnapshot);
    }
}

bool CChainSnapshot::StartHeightSnapshot(const uint32 nSnapshotHeight, uint256& hashSnapshotBlock)
{
    uint32 nSnapHeight = ((nSnapshotHeight / DAY_HEIGHT) * DAY_HEIGHT) + MAX_SNAPSHOT_STATE_HEIGHT;
    if (nSnapHeight <= DAY_HEIGHT)
    {
        StdLog("CChainSnapshot", "Start height snapshot: Snapshot height not enough, snapshot height: %d", nSnapshotHeight);
        return false;
    }
    if (nSnapHeight > nSnapshotHeight)
    {
        nSnapHeight -= DAY_HEIGHT;
    }

    std::vector<uint256> vForkHash;
    if (!GetHeightSnapshotBlock(nSnapHeight, hashSnapshotBlock, vForkHash))
    {
        StdLog("CChainSnapshot", "Start height snapshot: Get height snapshot block failed, shapshot height: %d", nSnapHeight);
        return false;
    }

    {
        boost::unique_lock<boost::mutex> lock(mutexStatus);
        if (nRpcCreateSnapshotHeight != 0)
        {
            StdLog("CChainSnapshot", "Start height snapshot: There is already an rpc snapshot in progress, shapshoting height: %d", nRpcCreateSnapshotHeight);
            return false;
        }
        nRpcCreateSnapshotHeight = nSnapHeight;
        if (!fThreadRun)
        {
            fThreadRun = true;
            if (!ThreadDelayStart(thrSnapshot))
            {
                StdLog("CChainSnapshot", "Start height snapshot: Start thread failed");
                return false;
            }
        }
        else
        {
            condExit.notify_all();
        }
    }
    return true;
}

uint32 CChainSnapshot::GetSnapshotStatus(uint256& hashSnapshotBlock)
{
    boost::unique_lock<boost::mutex> lock(mutexStatus);
    hashSnapshotBlock = hashSnapshotingBlock;
    return nSnapshotStatus;
}

void CChainSnapshot::SnapshotThreadFunc()
{
}

} // namespace hashahead
