// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "prunedb.h"

#include "block.h"

using namespace hnbase;

namespace hashahead
{

/////////////////////////////////////////////////
// CPruneDb

CPruneDb::CPruneDb()
  : thrPruneDb("prunedb", boost::bind(&CPruneDb::PruneDbThreadFunc, this))
{
    fExit = false;
    fCfgPruneStateData = false;
    fCfgTraceDb = false;
    nCfgPruneRetentionDays = 0;
    nCfgPruneReserveHeight = 0;

    pCoreProtocol = nullptr;
    pBlockChain = nullptr;
}

CPruneDb::~CPruneDb()
{
}

bool CPruneDb::HandleInitialize()
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

    fCfgPruneStateData = StorageConfig()->fPrune;
    nCfgPruneRetentionDays = StorageConfig()->nPruneRetentionDays;
    fCfgTraceDb = Config()->fTraceDb;
    if (fCfgPruneStateData)
    {
        if (nCfgPruneRetentionDays < 2)
        {
            nCfgPruneRetentionDays = 2;
        }
        else
        {
            const uint32 nMaxDays = 180;
            if (nCfgPruneRetentionDays > nMaxDays)
            {
                nCfgPruneRetentionDays = nMaxDays;
            }
        }
        nCfgPruneReserveHeight = nCfgPruneRetentionDays * DAY_HEIGHT;
    }
    else
    {
        nCfgPruneRetentionDays = 0;
        nCfgPruneReserveHeight = 0;
    }
    StdLog("CPruneDb", "Handle initialize: prune: %s, prune retention days: %d, prune reserve height: %d", (fCfgPruneStateData ? "true" : "false"), nCfgPruneRetentionDays, nCfgPruneReserveHeight);
    return true;
}

void CPruneDb::HandleDeinitialize()
{
    pCoreProtocol = nullptr;
    pBlockChain = nullptr;

    fExit = false;
    fCfgPruneStateData = false;
    nCfgPruneRetentionDays = 0;
    nCfgPruneReserveHeight = 0;
}

bool CPruneDb::HandleInvoke()
{
    if (fCfgPruneStateData)
    {
        fExit = false;
        if (!ThreadDelayStart(thrPruneDb))
        {
            StdLog("CPruneDb", "Handle invoke: Start thread failed");
            return false;
        }
    }
    return true;
}

void CPruneDb::HandleHalt()
{
    if (fCfgPruneStateData)
    {
        fExit = true;
        condExit.notify_all();

        thrPruneDb.Interrupt();
        ThreadExit(thrPruneDb);
    }
}

void CPruneDb::PruneDbThreadFunc()
{
    while (WaitExitEvent(10))
    {
        PruneStateWork();
    }
}

bool CPruneDb::WaitExitEvent(const int64 nSeconds)
{
    if (nSeconds > 0 && !fExit)
    {
        try
        {
            boost::system_time const timeout = boost::get_system_time() + boost::posix_time::seconds(nSeconds);
            boost::unique_lock<boost::mutex> lock(mutex);
            condExit.timed_wait(lock, timeout);
        }
        catch (const boost::thread_interrupted&)
        {
            return !fExit;
        }
        catch (const boost::thread_resource_error&)
        {
            return !fExit;
        }
        catch (...)
        {
            return !fExit;
        }
    }
    return !fExit;
}

void CPruneDb::PruneStateWork()
{
    std::vector<uint256> vForkHash;
    const uint32 nMinLastHeight = pBlockChain->GetAllForkMinLastBlockHeight(&vForkHash);
    if (nMinLastHeight == 0xFFFFFFFF)
    {
        StdLog("CPruneDb", "Prune state work: Min last height error");
        return;
    }
    for (auto& hashFork : vForkHash)
    {
        PruneForkData(hashFork, nMinLastHeight);
        PruneForkKvData(hashFork, nMinLastHeight);
    }
}

} // namespace hashahead
