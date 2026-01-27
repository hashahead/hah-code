// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HASHAHEAD_CHNBLOCKVOTE_H
#define HASHAHEAD_CHNBLOCKVOTE_H

#include "base.h"
#include "consblockvote.h"
#include "peernet.h"

using namespace consensus;
using namespace consensus::consblockvote;

namespace hashahead
{

class CBlockVoteChnPeer
{
public:
    CBlockVoteChnPeer()
      : nService(0) {}
    CBlockVoteChnPeer(uint64 nServiceIn, const network::CAddress& addr)
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

class CBlockVoteChannel : public network::IBlockVoteChannel
{
public:
    CBlockVoteChannel();
    ~CBlockVoteChannel();

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    bool HandleInvoke() override;
    void HandleHalt() override;

protected:
    network::CBbPeerNet* pPeerNet;
    ICoreProtocol* pCoreProtocol;
    IBlockChain* pBlockChain;
};

} // namespace hashahead

#endif // HASHAHEAD_CHNBLOCKVOTE_H
