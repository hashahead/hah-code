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
}

bool CPruneDb::HandleInvoke()
{
    return true;
}

void CPruneDb::HandleHalt()
{
}

} // namespace hashahead
