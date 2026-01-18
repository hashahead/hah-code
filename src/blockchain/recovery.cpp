// Copyright (c) 2021-2023 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "recovery.h"

#include <boost/filesystem.hpp>

#include "block.h"
#include "core.h"
#include "purger.h"
#include "timeseries.h"

using namespace boost::filesystem;
using namespace hnbase;

namespace hashahead
{

class CRecovery;

/////////////////////////////////////////////////
// CRecoveryWalker

class CRecoveryWalker : public storage::CTSWalker<CBlockEx>
{
public:
    CRecoveryWalker(CRecovery* pRecoveryIn, IDispatcher* pDispatcherIn, const size_t nSizeIn, const map<uint32, uint32>& mapForkLastHeightIn, const bool fSyncForkHeightIn)
      : pRecovery(pRecoveryIn), pDispatcher(pDispatcherIn), nSize(nSizeIn), mapForkLastHeight(mapForkLastHeightIn), fSyncForkHeight(fSyncForkHeightIn)
    {
        nIncSize = nSizeIn / 100;
        if (nIncSize == 0)
        {
            nIncSize = 1;
        }
        nNextSize = nIncSize;
        nWalkedFileSize = 0;
        nCurrentFile = 0;
        nCurrentOffset = 0;
        nRecoveryBlockCount = 0;
    }

    bool Walk(const CBlockEx& t, uint32 nFile, uint32 nOffset) override
    {
        if (pRecovery->IsExit())
        {
            StdLog("CRecovery", "Recovery Interrupted, block: %lu: %s", t.GetBlockNumber(), t.GetHash().GetBhString().c_str());
            return false;
        }

        if (!t.IsGenesis())
        {
            auto it = mapForkLastHeight.find(t.GetChainId());
            if (it == mapForkLastHeight.end() || t.GetBlockHeight() >= it->second)
            {
                if (!fSyncForkHeight)
                {
                    Errno err = pDispatcher->AddNewBlock(t);
                    if (err == OK)
                    {
                        hnbase::StdTrace("Recovery", "Recovery block: %lu: %s", t.GetBlockNumber(), t.GetHash().GetBhString().c_str());
                    }
                    else if (err != ERR_ALREADY_HAVE)
                    {
                        hnbase::StdError("Recovery", "Recovery block: %s, file: %u, offset: %u, error: %s",
                                         t.GetHash().GetBhString().c_str(), nFile, nOffset, ErrorString(err));
                        return false;
                    }
                }
                else
                {
                    if (!pRecovery->AddRecoveryBlock(t))
                    {
                        return false;
                    }
                }
            }
        }

        if (nCurrentFile != nFile)
        {
            nWalkedFileSize += nCurrentOffset;
            nCurrentFile = nFile;
        }
        nCurrentOffset = nOffset;
        nRecoveryBlockCount++;

        if (nWalkedFileSize + nOffset > nNextSize)
        {
            StdLog("CRecovery", "Recovered: %lu%%, block count: %lu", nNextSize / nIncSize, nRecoveryBlockCount);
            nNextSize += nIncSize;
        }
        return true;
    }

protected:
    CRecovery* pRecovery;
    IDispatcher* pDispatcher;
    const size_t nSize;
    const map<uint32, uint32> mapForkLastHeight;
    size_t nNextSize;
    size_t nIncSize;
    size_t nWalkedFileSize;
    uint32 nCurrentFile;
    uint32 nCurrentOffset;
    size_t nRecoveryBlockCount;
    bool fSyncForkHeight;
};

/////////////////////////////////////////////////
// CRecovery

CRecovery::CRecovery()
  : thrRecovery("recovery", boost::bind(&CRecovery::RecoveryThreadFunc, this))
{
    fRecoveryWork = false;
    fExit = false;

    pDispatcher = nullptr;
    pBlockChain = nullptr;
    pCoreProtocol = nullptr;

    nPrimaryMinHeight = 0;
    nPrimaryChainId = 0;
}

CRecovery::~CRecovery()
{
}

bool CRecovery::IsExit() const
{
    return fExit;
}

bool CRecovery::HandleInitialize()
{
    if (!GetObject("dispatcher", pDispatcher))
    {
        Error("Failed to request dispatcher");
        return false;
    }
    if (!GetObject("blockchain", pBlockChain))
    {
        Error("Failed to request blockchain");
        return false;
    }
    if (!GetObject("coreprotocol", pCoreProtocol))
    {
        Error("Failed to request coreprotocol");
        return false;
    }

    if (StorageConfig()->fIsRecoveryBlock && !StorageConfig()->strRecoveryDir.empty())
    {
        fRecoveryWork = true;
        strRecoveryDir = StorageConfig()->strRecoveryDir;
    }
    else
    {
        fRecoveryWork = false;
    }
    nPrimaryChainId = CBlock::GetBlockChainIdByHash(pCoreProtocol->GetGenesisBlockHash());
    return true;
}

void CRecovery::HandleDeinitialize()
{
    pDispatcher = nullptr;
    pBlockChain = nullptr;
    pCoreProtocol = nullptr;

    fExit = false;
    fRecoveryWork = false;
}

bool CRecovery::HandleInvoke()
{
    if (fRecoveryWork)
    {
        fExit = false;
        if (!ThreadDelayStart(thrRecovery))
        {
            fRecoveryWork = false;
            return false;
        }
    }
    return true;
}

void CRecovery::HandleHalt()
{
    if (fRecoveryWork)
    {
        fExit = true;

        thrRecovery.Interrupt();
        ThreadExit(thrRecovery);

        fRecoveryWork = false;
    }
}

bool CRecovery::IsRecoverying()
{
    return fRecoveryWork;
}

bool CRecovery::GetForkLastHeight(map<uint32, uint32>& mapForkLastHeight)
{
    std::map<uint256, CForkContext> mapCtxFork;
    if (!pBlockChain->ListForkContext(mapCtxFork, 0))
    {
        return false;
    }
    for (auto& kv : mapCtxFork)
    {
        CBlockStatus status;
        if (pBlockChain->GetLastBlockStatus(kv.first, status))
        {
            uint32 nLastHeight = CBlock::GetBlockHeightByHash(status.hashBlock);
            if (nLastHeight <= 10)
            {
                nLastHeight = 0;
            }
            else
            {
                nLastHeight -= 10;
            }
            mapForkLastHeight.insert(std::make_pair(CBlock::GetBlockChainIdByHash(kv.first), nLastHeight));
            StdLog("CRecovery", "Get fork last height: recovery last height: %d, fork last block: %s", nLastHeight, status.hashBlock.GetBhString().c_str());
        }
    }
    return true;
}

void CRecovery::RecoveryThreadFunc()
{
    if (fRecoveryWork)
    {
        RecoveryWork();
        fRecoveryWork = false;
    }
}

void CRecovery::RecoveryWork()
{
    StdLog("CRecovery", "Recovery begin, dir: %s", strRecoveryDir.c_str());

    path blockDir(strRecoveryDir);
    if (!exists(blockDir))
    {
        StdError("CRecovery", "Recovery dir not exist, dir: %s", strRecoveryDir.c_str());
        return;
    }

    storage::CTimeSeriesCached tsBlock;
    if (!tsBlock.Initialize(blockDir, "block"))
    {
        StdError("CRecovery", "Recovery initialze fail");
        return;
    }

    map<uint32, uint32> mapForkLastHeight;
    if (!GetForkLastHeight(mapForkLastHeight))
    {
        StdError("CRecovery", "Get fork last height fail");
        return;
    }

    size_t nSize = tsBlock.GetSize();
    CRecoveryWalker walker(this, pDispatcher, nSize, mapForkLastHeight, NetworkConfig()->fSyncForkHeight);
    uint32 nLastFile;
    uint32 nLastPos;
    if (!tsBlock.WalkThrough(walker, nLastFile, nLastPos, false))
    {
        StdError("CRecovery", "Recovery walkthrough fail");
        return;
    }
    StdLog("CRecovery", "Recovered success! dir: %s", strRecoveryDir.c_str());
}

//------------------------------------------------------------------------------------
bool CRecovery::AddRecoveryBlock(const CBlockEx& block)
{
    mapForkBlockQueue[block.GetChainId()].push(block);

    const uint32 nAddHeight = 1;
    uint32 nGetCount;
    do
    {
        nGetCount = 0;

        uint32 nMinHeight = pBlockChain->GetAllForkMinLastBlockHeight();
        if (nPrimaryMinHeight == 0 || nMinHeight > nPrimaryMinHeight)
        {
            nPrimaryMinHeight = nMinHeight;
        }
        for (auto& kv : mapForkBlockQueue)
        {
            const uint32 nChainId = kv.first;
            auto& qBlockQueue = kv.second;
            if (qBlockQueue.empty())
            {
                continue;
            }
            const CBlockEx& block = qBlockQueue.front();

            if (nChainId == nPrimaryChainId)
            {
                if (block.GetBlockHeight() > nPrimaryMinHeight + nAddHeight)
                {
                    continue;
                }
            }
            else if (block.IsOrigin())
            {
                if (block.GetBlockHeight() > nPrimaryMinHeight + nAddHeight)
                {
                    continue;
                }
                if (!pBlockChain->Exists(block.hashPrev))
                {
                    continue;
                }
            }
            else
            {
                if (block.GetBlockHeight() > nMinHeight + nAddHeight)
                {
                    continue;
                }
                if (block.IsSubsidiary() || block.IsExtended() || block.IsVacant())
                {
                    CProofOfPiggyback proof;
                    if (!block.GetPiggybackProof(proof) || proof.hashRefBlock == 0)
                    {
                        StdLog("CRecovery", "Add recovery block: Get refblock fail, block: %s", block.GetHash().GetBhString().c_str());
                    }
                    else
                    {
                        if (!pBlockChain->Exists(proof.hashRefBlock))
                        {
                            const uint32 nRefHeight = CBlock::GetBlockHeightByHash(proof.hashRefBlock);
                            if (nRefHeight > nPrimaryMinHeight)
                            {
                                nPrimaryMinHeight = nRefHeight;
                            }
                            continue;
                        }
                    }
                }
                else
                {
                    StdError("CRecovery", "Add recovery block: Ref block is 0, block: %s", block.GetHash().GetBhString().c_str());
                }
            }

            Errno err = pDispatcher->AddNewBlock(block);
            if (err != OK)
            {
                StdLog("CRecovery", "Add recovery block: Add new block failed, error: %s, block: %s", ErrorString(err), block.GetHash().GetBhString().c_str());
                if (err == ERR_ALREADY_HAVE)
                {
                    qBlockQueue.pop();
                    nGetCount++;
                    continue;
                }
                return false;
            }
            StdTrace("Recovery", "Add recovery block: Recovery block: %lu: %s", block.GetBlockNumber(), block.GetHash().GetBhString().c_str());

            qBlockQueue.pop();
            nGetCount++;
        }
    } while (nGetCount > 0);

    return true;
}

} // namespace hashahead
