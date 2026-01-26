// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chnblockvote.h"

#include <boost/bind.hpp>

using namespace std;
using namespace hnbase;
using namespace consensus::consblockvote;
using boost::asio::ip::tcp;

namespace hashahead
{

////////////////////////////////////////////////////
// CBlockVoteChannel

CBlockVoteChannel::CBlockVoteChannel()
{
    pPeerNet = nullptr;
    pCoreProtocol = nullptr;
    pBlockChain = nullptr;
}

CBlockVoteChannel::~CBlockVoteChannel()
{
}

bool CBlockVoteChannel::HandleInitialize()
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

    StdLog("CBlockVoteChannel", "HandleInitialize");
    return true;
}

void CBlockVoteChannel::HandleDeinitialize()
{
    pPeerNet = nullptr;
    pCoreProtocol = nullptr;
    pBlockChain = nullptr;
}

bool CBlockVoteChannel::HandleInvoke()
{
    return network::IBlockVoteChannel::HandleInvoke();
}

void CBlockVoteChannel::HandleHalt()
{
    network::IBlockVoteChannel::HandleHalt();
}

} // namespace hashahead
