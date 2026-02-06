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

bool CSnapshotDownChannel::HandleEvent(network::CEventPeerActive& eventActive)
{
    const uint64 nNonce = eventActive.nNonce;
    mapChnPeer[nNonce] = CSnapDownChnPeer(eventActive.data.nService, eventActive.data);

    StdLog("CSnapshotDownChannel", "CEvent Peer Active: peer: %s", GetPeerAddressInfo(nNonce).c_str());

    if (!strCfgSnapshotDownAddress.empty() && hashCfgSnapshotDownBlock != 0 && GetPeerAddressInfo(nNonce) == strCfgSnapshotDownAddress)
    {
        nSnapshotDownNetId = nNonce;
        if (!RequstFileList(hashCfgSnapshotDownBlock))
        {
            StdLog("CSnapshotDownChannel", "CEvent Peer Active: Request file list failed, peer: %s", GetPeerAddressInfo(nNonce).c_str());
        }
    }
    return true;
}

bool CSnapshotDownChannel::HandleEvent(network::CEventPeerDeactive& eventDeactive)
{
    const uint64 nNonce = eventDeactive.nNonce;
    StdLog("CSnapshotDownChannel", "CEvent Peer Deactive: peer: %s", GetPeerAddressInfo(nNonce).c_str());

    mapChnPeer.erase(nNonce);

    if (nSnapshotDownNetId == nNonce)
    {
        nSnapshotDownNetId = 0;
    }
    return true;
}

bool CSnapshotDownChannel::HandleEvent(network::CEventPeerSnapshotDownData& event)
{
    const uint64 nNetId = event.nNonce;
    const uint256& hashFork = event.hashFork;

    try
    {
        uint8 nMsgId = 0;

        CBufStream ss(event.data);
        ss >> nMsgId;

        switch (nMsgId)
        {
        case SNAP_DOWN_MSGID_FILELIST_REQ:
        {
            uint256 hashSnapBlock;
            ss >> hashSnapBlock;
            if (!HandleMsgFilelistReq(nNetId, hashFork, hashSnapBlock))
            {
                return false;
            }
            break;
        }
        case SNAP_DOWN_MSGID_FILELIST_RSP:
        {
            CSnapDownMsgFilelistRsp body;
            ss >> body;
            if (!HandleMsgFilelistRsp(nNetId, hashFork, body))
            {
                return false;
            }
            break;
        }
        case SNAP_DOWN_MSGID_DOWNDATA_REQ:
        {
            CSnapDownMsgDownDataReq body;
            ss >> body;
            if (!HandleMsgDownDataReq(nNetId, hashFork, body))
            {
                return false;
            }
            break;
        }
        case SNAP_DOWN_MSGID_DOWNDATA_RSP:
        {
            CSnapDownMsgDownDataRsp body;
            ss >> body;
            if (!HandleMsgDownDataRsp(nNetId, hashFork, body))
            {
                return false;
            }
            break;
        }
        }
    }
    catch (std::exception& e)
    {
        hnbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------
bool CSnapshotDownChannel::HandleMsgFilelistReq(const uint64 nNetId, const uint256& hashFork, const uint256& hashSnapBlock)
{
    CSnapDownMsgFilelistRsp bodyRsp(hashSnapBlock);

    if (!pBlockChain->GetSnapshotFileList(hashSnapBlock, bodyRsp.vFilelist))
    {
        bodyRsp.vFilelist.clear();
        StdLog("CSnapshotDownChannel", "Handle msg filelist req: Get snapshot file list failed, snap block: %s", hashSnapBlock.ToString().c_str());
    }

    CBufStream ss;
    ss << SNAP_DOWN_MSGID_FILELIST_RSP << bodyRsp;

    network::CEventPeerSnapshotDownData event(nNetId, hashFork);
    ss.GetData(event.data);
    if (!pPeerNet->DispatchEvent(&event))
    {
        StdLog("CSnapshotDownChannel", "Handle msg filelist req: Dispatch event failed, snap block: %s", hashSnapBlock.ToString().c_str());
        return false;
    }
    return true;
}

bool CSnapshotDownChannel::HandleMsgFilelistRsp(const uint64 nNetId, const uint256& hashFork, const CSnapDownMsgFilelistRsp& body)
{
    if (nNetId != nSnapshotDownNetId)
    {
        return true;
    }

    if (body.vFilelist.empty())
    {
        StdLog("CSnapshotDownChannel", "Handle msg file list rsp: File list is empty, snap block: %s", body.hashSnapBlock.ToString().c_str());
    }
    else if (body.hashSnapBlock != hashCfgSnapshotDownBlock)
    {
        StdLog("CSnapshotDownChannel", "Handle msg file list rsp: Snapshot block error, config block: %s, snap block: %s", hashCfgSnapshotDownBlock.ToString().c_str(), body.hashSnapBlock.ToString().c_str());
    }
    else
    {
        std::vector<CSnapshotFileInfo> vLocalFilelist;
        if (!pBlockChain->GetSnapshotDownFileList(body.hashSnapBlock, vLocalFilelist))
        {
            vLocalFilelist.clear();
        }
        for (auto& vd : body.vFilelist)
        {
            auto it = std::find_if(vLocalFilelist.begin(), vLocalFilelist.end(), [&](const CSnapshotFileInfo& fileInfo) -> bool { return (fileInfo.strFileName == vd.strFileName); });
            if (it == vLocalFilelist.end() || it->nFileSize != vd.nFileSize)
            {
                vRemoteSnapFilelist.push_back(vd);
                StdLog("CSnapshotDownChannel", "Handle msg file list rsp: need download file name: %s, file size: %lu, snap block: %s", vd.strFileName.c_str(), vd.nFileSize, body.hashSnapBlock.ToString().c_str());
            }
        }
        if (!vRemoteSnapFilelist.empty())
        {
            hashSnapshotBlock = body.hashSnapBlock;

            if (!RequstNextFile(nNetId, hashFork, body.hashSnapBlock))
            {
                StdLog("CSnapshotDownChannel", "Handle msg file list rsp: Requst next file failed, snap block: %s", body.hashSnapBlock.ToString().c_str());
                return false;
            }
        }
    }
    return true;
}

}; // namespace hashahead
