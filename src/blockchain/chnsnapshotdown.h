// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HASHAHEAD_CHNSNAPSHOTDOWN_H
#define HASHAHEAD_CHNSNAPSHOTDOWN_H

#include "base.h"
#include "cmstruct.h"
#include "hnbase.h"
#include "peernet.h"

using namespace hnbase;

namespace hashahead
{

////////////////////////////////////////////////////
// down msg id

const uint8 SNAP_DOWN_MSGID_FILELIST_REQ = 0x01;
const uint8 SNAP_DOWN_MSGID_FILELIST_RSP = 0x02;
const uint8 SNAP_DOWN_MSGID_DOWNDATA_REQ = 0x03;
const uint8 SNAP_DOWN_MSGID_DOWNDATA_RSP = 0x04;

////////////////////////////////////////////////////
// CSnapDownMsgFilelistRsp

class CSnapDownMsgFilelistRsp
{
    friend class hnbase::CStream;

public:
    CSnapDownMsgFilelistRsp() {}
    CSnapDownMsgFilelistRsp(const uint256& hashSnapBlockIn)
      : hashSnapBlock(hashSnapBlockIn) {}

public:
    uint256 hashSnapBlock;
    std::vector<CSnapshotFileInfo> vFilelist;

protected:
    template <typename O>
    void Serialize(hnbase::CStream& s, O& opt)
    {
        s.Serialize(hashSnapBlock, opt);
        s.Serialize(vFilelist, opt);
    }
};

////////////////////////////////////////////////////
// CSnapDownMsgDownDataReq

class CSnapDownMsgDownDataReq
{
    friend class hnbase::CStream;

public:
    CSnapDownMsgDownDataReq()
      : nOffset(0) {}
    CSnapDownMsgDownDataReq(const uint256& hashSnapBlockIn, const std::string& strFileNameIn, const uint64 nOffsetIn)
      : hashSnapBlock(hashSnapBlockIn), strFileName(strFileNameIn), nOffset(nOffsetIn) {}

public:
    uint256 hashSnapBlock;
    std::string strFileName;
    uint64 nOffset;

protected:
    template <typename O>
    void Serialize(hnbase::CStream& s, O& opt)
    {
        s.Serialize(hashSnapBlock, opt);
        s.Serialize(strFileName, opt);
        s.Serialize(nOffset, opt);
    }
};

////////////////////////////////////////////////////
// CSnapDownMsgDownDataRsp

class CSnapDownMsgDownDataRsp
{
    friend class hnbase::CStream;

public:
    CSnapDownMsgDownDataRsp()
      : nOffset(0) {}
    CSnapDownMsgDownDataRsp(const uint256& hashSnapBlockIn, const std::string& strFileNameIn, const uint64 nOffsetIn, const bytes& btDataIn)
      : hashSnapBlock(hashSnapBlockIn), strFileName(strFileNameIn), nOffset(nOffsetIn), btData(btDataIn) {}

public:
    uint256 hashSnapBlock;
    std::string strFileName;
    uint64 nOffset;
    bytes btData;

protected:
    template <typename O>
    void Serialize(hnbase::CStream& s, O& opt)
    {
        s.Serialize(hashSnapBlock, opt);
        s.Serialize(strFileName, opt);
        s.Serialize(nOffset, opt);
        s.Serialize(btData, opt);
    }
};

////////////////////////////////////////////////////
// CSnapDownChnPeer

class CSnapDownChnPeer
{
public:
    CSnapDownChnPeer()
      : nService(0) {}
    CSnapDownChnPeer(uint64 nServiceIn, const network::CAddress& addr)
      : nService(nServiceIn), addressRemote(addr) {}

    const std::string GetRemoteAddress()
    {
        std::string strRemoteAddress;
        boost::asio::ip::tcp::endpoint ep;
        boost::system::error_code ec;
        addressRemote.ssEndpoint.GetEndpoint(ep);
        if (ep.address().is_v6())
        {
            strRemoteAddress = string("[") + ep.address().to_string(ec) + "]:" + std::to_string(ep.port());
        }
        else
        {
            strRemoteAddress = ep.address().to_string(ec) + ":" + std::to_string(ep.port());
        }
        return strRemoteAddress;
    }

public:
    uint64 nService;
    network::CAddress addressRemote;
};

////////////////////////////////////////////////////
// CSnapshotDownChannel

class CSnapshotDownChannel : public network::ISnapshotDownChannel
{
public:
    CSnapshotDownChannel();
    ~CSnapshotDownChannel();

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    bool HandleInvoke() override;
    void HandleHalt() override;

    bool HandleEvent(network::CEventPeerActive& eventActive) override;
    bool HandleEvent(network::CEventPeerDeactive& eventDeactive) override;
    bool HandleEvent(network::CEventPeerSnapshotDownData& event) override;

    bool HandleMsgFilelistReq(const uint64 nNetId, const uint256& hashFork, const uint256& hashSnapBlock);
    bool HandleMsgFilelistRsp(const uint64 nNetId, const uint256& hashFork, const CSnapDownMsgFilelistRsp& body);

protected:
    network::CBbPeerNet* pPeerNet;
    ICoreProtocol* pCoreProtocol;
    IBlockChain* pBlockChain;
};

} // namespace hashahead

#endif // HASHAHEAD_CHNSNAPSHOTDOWN_H
