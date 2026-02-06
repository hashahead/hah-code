// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NETWORK_PEERNET_H
#define NETWORK_PEERNET_H

#include "hnbase.h"
#include "peerevent.h"
#include "proto.h"

namespace hashahead
{
namespace network
{

class INetChannel : public hnbase::IIOModule, virtual public CBbPeerEventListener
{
public:
    INetChannel()
      : IIOModule("netchannel") {}
    virtual int GetPrimaryChainHeight() = 0;
    virtual bool IsForkSynchronized(const uint256& hashFork) const = 0;
    virtual void BroadcastBlockInv(const uint256& hashFork, const uint256& hashBlock) = 0;
    virtual void BroadcastTxInv(const uint256& hashFork) = 0;
    virtual void SubscribeFork(const uint256& hashFork, const uint64& nNonce) = 0;
    virtual void UnsubscribeFork(const uint256& hashFork) = 0;
    virtual bool SubmitCachePoaBlock(const CConsensusParam& consParam) = 0;
    virtual bool IsLocalCachePoaBlock(int nHeight, bool& fIsPos) = 0;
    virtual bool AddCacheLocalPoaBlock(const CBlock& block) = 0;
};

class IBlockChannel : public hnbase::IIOModule, virtual public CBbPeerEventListener
{
public:
    IBlockChannel()
      : IIOModule("blockchannel") {}
    virtual void SubscribeFork(const uint256& hashFork, const uint64 nNonce) = 0;
    virtual void BroadcastBlock(const uint64 nRecvNetNonce, const uint256& hashFork, const uint256& hashBlock, const uint256& hashPrev, const CBlock& block) = 0;
};

class ICertTxChannel : public hnbase::IIOModule, virtual public CBbPeerEventListener
{
public:
    ICertTxChannel()
      : IIOModule("certtxchannel") {}
    virtual void BroadcastCertTx(const uint64 nRecvNetNonce, const std::vector<CTransaction>& vtx) = 0;
};

class IUserTxChannel : public hnbase::IIOModule, virtual public CBbPeerEventListener
{
public:
    IUserTxChannel(const uint32 nThreadCount = 1)
      : IIOModule("usertxchannel", nThreadCount) {}
    virtual void SubscribeFork(const uint256& hashFork, const uint64 nNonce) = 0;
    virtual void BroadcastUserTx(const uint64 nRecvNetNonce, const uint256& hashFork, const std::vector<CTransaction>& vtx) = 0;
};

class IBlockVoteChannel : public hnbase::IIOModule, virtual public CBbPeerEventListener
{
public:
    IBlockVoteChannel(const uint32 nThreadCount = 1)
      : IIOModule("blockvotechannel", nThreadCount) {}
    virtual void SubscribeFork(const uint256& hashFork, const uint64 nNonce) = 0;
    virtual void UpdateNewBlock(const uint256& hashFork, const uint256& hashBlock, const uint256& hashRefBlock, const int64 nBlockTime, const uint64 nNonce) = 0;
};

class IBlockCrossProveChannel : public hnbase::IIOModule, virtual public CBbPeerEventListener
{
public:
    IBlockCrossProveChannel(const uint32 nThreadCount = 1)
      : IIOModule("blockcrossprovechannel", nThreadCount) {}
    virtual void SubscribeFork(const uint256& hashFork, const uint64 nNonce) = 0;
    virtual void BroadcastBlockProve(const uint256& hashFork, const uint256& hashBlock, const uint64 nNonce, const std::map<CChainId, CBlockProve>& mapBlockProve) = 0;
};

class ISnapshotDownChannel : public hnbase::IIOModule, virtual public CBbPeerEventListener
{
public:
    ISnapshotDownChannel()
      : IIOModule("snapshotdownchannel") {}
};

class IDelegatedChannel : public hnbase::IIOModule, virtual public CBbPeerEventListener
{
public:
    IDelegatedChannel()
      : IIOModule("delegatedchannel") {}
    virtual void PrimaryUpdate(int nStartHeight,
                               const std::vector<std::pair<uint256, std::map<CDestination, size_t>>>& vEnrolledWeight,
                               const std::vector<std::pair<uint256, std::map<CDestination, std::vector<unsigned char>>>>& vDistributeData,
                               const std::map<CDestination, std::vector<unsigned char>>& mapPublishData,
                               const uint256& hashDistributeOfPublish)
        = 0;
};

class CBbPeerNet : public hnbase::CPeerNet, virtual public CBbPeerEventListener
{
public:
    CBbPeerNet();
    ~CBbPeerNet();
    virtual void BuildHello(hnbase::CPeer* pPeer, hnbase::CBufStream& ssPayload);
    virtual uint32 BuildPing(hnbase::CPeer* pPeer, hnbase::CBufStream& ssPayload);
    void HandlePeerWriten(hnbase::CPeer* pPeer) override;
    virtual bool HandlePeerHandshaked(hnbase::CPeer* pPeer, uint32 nTimerId);
    virtual bool HandlePeerRecvMessage(hnbase::CPeer* pPeer, int nChannel, int nCommand,
                                       hnbase::CBufStream& ssPayload);
    uint32 SetPingTimer(uint32 nOldTimerId, uint64 nNonce, int64 nElapse);

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    bool HandleEvent(CEventPeerSubscribe& eventSubscribe) override;
    bool HandleEvent(CEventPeerUnsubscribe& eventUnsubscribe) override;
    bool HandleEvent(CEventPeerInv& eventInv) override;
    bool HandleEvent(CEventPeerGetData& eventGetData) override;
    bool HandleEvent(CEventPeerGetBlocks& eventGetBlocks) override;
    bool HandleEvent(CEventPeerTx& eventTx) override;
    bool HandleEvent(CEventPeerBlock& eventBlock) override;
    bool HandleEvent(CEventPeerGetFail& eventGetFail) override;
    bool HandleEvent(CEventPeerMsgRsp& eventMsgRsp) override;

    bool HandleEvent(CEventPeerBlockSubscribe& eventSubscribe) override;
    bool HandleEvent(CEventPeerBlockUnsubscribe& eventUnsubscribe) override;
    bool HandleEvent(CEventPeerBlockBks& eventBks) override;

    bool HandleEvent(CEventPeerCerttxSubscribe& eventSubscribe) override;
    bool HandleEvent(CEventPeerCerttxUnsubscribe& eventUnsubscribe) override;
    bool HandleEvent(CEventPeerCerttxTxs& eventTxs) override;

    bool HandleEvent(CEventPeerUsertxSubscribe& eventSubscribe) override;
    bool HandleEvent(CEventPeerUsertxUnsubscribe& eventUnsubscribe) override;
    bool HandleEvent(CEventPeerUsertxTxs& eventTxs) override;

    bool HandleEvent(CEventPeerBulletin& eventBulletin) override;
    bool HandleEvent(CEventPeerGetDelegated& eventGetDelegated) override;
    bool HandleEvent(CEventPeerDistribute& eventDistribute) override;
    bool HandleEvent(CEventPeerPublish& eventPublish) override;

    hnbase::CPeer* CreatePeer(hnbase::CIOClient* pClient, uint64 nNonce, bool fInBound) override;
    void DestroyPeer(hnbase::CPeer* pPeer) override;
    hnbase::CPeerInfo* GetPeerInfo(hnbase::CPeer* pPeer, hnbase::CPeerInfo* pInfo) override;
    CAddress GetGateWayAddress(const CNetHost& gateWayAddr);
    bool SendChannelMessage(int nChannel, uint64 nNonce, int nCommand, CBufStream& ssPayload);
    bool SendDataMessage(uint64 nNonce, int nCommand, hnbase::CBufStream& ssPayload);
    bool SendDelegatedMessage(uint64 nNonce, int nCommand, hnbase::CBufStream& ssPayload);
    bool SetInvTimer(uint64 nNonce, std::vector<CInv>& vInv);
    virtual void ProcessAskFor(hnbase::CPeer* pPeer);
    void Configure(uint32 nMagicNumIn, uint32 nVersionIn, uint64 nServiceIn,
                   const std::string& subVersionIn, bool fEnclosedIn, const uint256& hashGenesisIn)
    {
        nMagicNum = nMagicNumIn;
        nVersion = nVersionIn;
        nService = nServiceIn;
        subVersion = subVersionIn;
        fEnclosed = fEnclosedIn;
        hashGenesis = hashGenesisIn;
    }
    virtual bool CheckPeerVersion(uint32 nVersionIn, uint64 nServiceIn, const std::string& subVersionIn) = 0;
    uint32 CreateSeq(uint64 nNonce);

protected:
    INetChannel* pNetChannel;
    IBlockChannel* pBlockChannel;
    ICertTxChannel* pCertTxChannel;
    IUserTxChannel* pUserTxChannel;
    IDelegatedChannel* pDelegatedChannel;
    uint32 nMagicNum;
    uint32 nVersion;
    uint64 nService;
    bool fEnclosed;
    std::string subVersion;
    uint256 hashGenesis;
    std::set<boost::asio::ip::tcp::endpoint> setDNSeed;
    uint64 nSeqCreate;
};

} // namespace network
} // namespace hashahead

#endif //NETWORK_PEERNET_H
