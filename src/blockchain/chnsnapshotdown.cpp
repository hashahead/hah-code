// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chnsnapshotdown.h"

using namespace std;
using namespace hnbase;
using boost::asio::ip::tcp;

namespace hashahead
{

////////////////////////////////////////////////////
// CSnapshotDownChannel

CSnapshotDownChannel::CSnapshotDownChannel()
{
    pPeerNet = nullptr;
    pCoreProtocol = nullptr;
    pBlockChain = nullptr;
}

CSnapshotDownChannel::~CSnapshotDownChannel()
{
}

bool CSnapshotDownChannel::HandleInitialize()
{
    if (!GetObject("peernet", pPeerNet))
    {
        Error("Failed to request peer net");
        return false;
    }

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

void CSnapshotDownChannel::HandleDeinitialize()
{
    pPeerNet = nullptr;
    pCoreProtocol = nullptr;
    pBlockChain = nullptr;
}

bool CSnapshotDownChannel::HandleInvoke()
{
    return network::ISnapshotDownChannel::HandleInvoke();
}

void CSnapshotDownChannel::HandleHalt()
{
    network::ISnapshotDownChannel::HandleHalt();
}

}; // namespace hashahead
