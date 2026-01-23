// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HASHAHEAD_CHNBLOCKCROSSPROVE_H
#define HASHAHEAD_CHNBLOCKCROSSPROVE_H

#include "base.h"
#include "consblockvote.h"
#include "peernet.h"

using namespace consensus;

namespace hashahead
{

class CBlockCrossProveChnPeer
{
public:
    CBlockCrossProveChnPeer()
      : nService(0) {}
    CBlockCrossProveChnPeer(uint64 nServiceIn, const network::CAddress& addr)
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
    void Subscribe(const uint256& hashFork)
    {
        setSubscribeFork.insert(hashFork);
    }
    void Unsubscribe(const uint256& hashFork)
    {
        setSubscribeFork.erase(hashFork);
    }
    bool IsSubscribe(const uint256& hashFork) const
    {
        //return (setSubscribeFork.count(hashFork) > 0);
        return true;
    }

public:
    uint64 nService;
    network::CAddress addressRemote;
    std::set<uint256> setSubscribeFork;
};

class CBlockCrossProveChnFork
{
public:
    CBlockCrossProveChnFork(const uint256& hashForkIn)
      : hashFork(hashForkIn) {}

protected:
    const uint256 hashFork;
};

class CBlockCrossProveChannel : public network::IBlockCrossProveChannel
{
public:
    CBlockCrossProveChannel();
    ~CBlockCrossProveChannel();

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    bool HandleInvoke() override;
    void HandleHalt() override;

    bool HandleEvent(network::CEventPeerActive& eventActive) override;
    bool HandleEvent(network::CEventPeerDeactive& eventDeactive) override;

    bool HandleEvent(network::CEventPeerBlockCrossProveData& eventBcp) override;

protected:
    network::CBbPeerNet* pPeerNet;
    ICoreProtocol* pCoreProtocol;
    IBlockChain* pBlockChain;

    std::map<uint64, CBlockCrossProveChnPeer> mapChnPeer;
    std::map<uint256, CBlockCrossProveChnFork> mapChnFork;
    uint32 nBlockCrossProveTimerId;

    std::map<uint256, std::map<uint256, bytes, CustomBlockHashCompare>> mapBroadcastProve; // key1: recv fork hash, key2: src block hash, value: prove data
};

} // namespace hashahead

#endif // HASHAHEAD_CHNBLOCKCROSSPROVE_H
