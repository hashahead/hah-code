// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chnsnapshotdown.h"

using namespace std;
using namespace hnbase;
using boost::asio::ip::tcp;

#define READ_SNAPSHOT_FILE_SIZE (1024 * 1024 * 2)

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

    if (!NetworkConfig()->strSnapDownAddress.empty() && !NetworkConfig()->strSnapDownBlock.empty())
    {
        const uint16 nPortDefault = (NetworkConfig()->fTestNet ? DEFAULT_TESTNET_P2PPORT : DEFAULT_P2PPORT);

        CNetHost hostSnapshotDown;
        if (!hostSnapshotDown.SetHostPort(NetworkConfig()->strSnapDownAddress, nPortDefault))
        {
            Error("Set host port failed");
            return false;
        }
        strCfgSnapshotDownAddress = hostSnapshotDown.ToString();
        hashCfgSnapshotDownBlock.SetHex(NetworkConfig()->strSnapDownBlock.c_str());
        if (hashCfgSnapshotDownBlock == 0)
        {
            strCfgSnapshotDownAddress.clear();
            Error("Snapshot block error, config snapshot block: %s", NetworkConfig()->strSnapDownBlock.c_str());
            return false;
        }
    }
    StdLog("CSnapshotDownChannel", "Handle initialize: Snapshot down address: %s, snapshot block: %s", strCfgSnapshotDownAddress.c_str(), hashCfgSnapshotDownBlock.ToString().c_str());
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
