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
}

bool CChainSnapshot::HandleInvoke()
{
    return true;
}

void CChainSnapshot::HandleHalt()
{
}

void CChainSnapshot::SnapshotThreadFunc()
{
}

} // namespace hashahead
