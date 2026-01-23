// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chnblockcrossprove.h"

#include <boost/bind.hpp>

using namespace std;
using namespace hnbase;
using boost::asio::ip::tcp;

#define BLOCK_CROSS_PROVE_CHANNEL_THREAD_COUNT 1
#define BLOCK_CROSS_PROVE_TIMER_TIME 1000
#define BLOCK_CROSS_PROVE_CACHE_BROADCAST_PROVE_COUNT 10000

namespace hashahead
{

////////////////////////////////////////////////////
// CBlockCrossProveChnFork

////////////////////////////////////////////////////
// CBlockCrossProveChannel

CBlockCrossProveChannel::CBlockCrossProveChannel()
  : network::IBlockCrossProveChannel(BLOCK_CROSS_PROVE_CHANNEL_THREAD_COUNT), nBlockCrossProveTimerId(0)
{
    pPeerNet = nullptr;
    pCoreProtocol = nullptr;
    pBlockChain = nullptr;
}

CBlockCrossProveChannel::~CBlockCrossProveChannel()
{
}

bool CBlockCrossProveChannel::HandleInitialize()
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

    StdLog("CBlockCrossProveChannel", "HandleInitialize");
    return true;
}

void CBlockCrossProveChannel::HandleDeinitialize()
{
    pPeerNet = nullptr;
    pCoreProtocol = nullptr;
    pBlockChain = nullptr;
}

bool CBlockCrossProveChannel::HandleInvoke()
{
    nBlockCrossProveTimerId = SetTimer(BLOCK_CROSS_PROVE_TIMER_TIME, boost::bind(&CBlockCrossProveChannel::BlockCrossProveTimerFunc, this, _1));
    if (nBlockCrossProveTimerId == 0)
    {
        StdLog("CBlockCrossProveChannel", "Handle Invoke: Set timer fail");
        return false;
    }
    return network::IBlockCrossProveChannel::HandleInvoke();
}

void CBlockCrossProveChannel::HandleHalt()
{
    if (nBlockCrossProveTimerId != 0)
    {
        CancelTimer(nBlockCrossProveTimerId);
        nBlockCrossProveTimerId = 0;
    }
    network::IBlockCrossProveChannel::HandleHalt();
}

bool CBlockCrossProveChannel::HandleEvent(network::CEventPeerActive& eventActive)
{
    const uint64 nNonce = eventActive.nNonce;
    mapChnPeer[nNonce] = CBlockCrossProveChnPeer(eventActive.data.nService, eventActive.data);

    StdLog("CBlockCrossProveChannel", "CEvent Peer Active: peer: %s", GetPeerAddressInfo(nNonce).c_str());
    return true;
}

bool CBlockCrossProveChannel::HandleEvent(network::CEventPeerDeactive& eventDeactive)
{
    const uint64 nNonce = eventDeactive.nNonce;
    StdLog("CBlockCrossProveChannel", "CEvent Peer Deactive: peer: %s", GetPeerAddressInfo(nNonce).c_str());

    mapChnPeer.erase(nNonce);
    return true;
}

} // namespace hashahead
