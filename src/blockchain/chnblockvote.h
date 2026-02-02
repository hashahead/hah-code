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

class CBlockVoteResult
{
public:
    CBlockVoteResult()
      : nVoteBeginTimeMillis(GetTimeMillis()), nVoteCommitTimeMillis(0) {}

    void SetVoteResult(const bytes& btBitmapIn, const bytes& btAggSigIn)
    {
        btBitmap = btBitmapIn;
        btAggSig = btAggSigIn;
        nVoteCommitTimeMillis = GetTimeMillis();
    }
    void GetVoteResult(bytes& btBitmapOut, bytes& btAggSigOut) const
    {
        btBitmapOut = btBitmap;
        btAggSigOut = btAggSig;
    }
    int64 GetVoteBeginTime() const
    {
        return nVoteBeginTimeMillis;
    }
    int64 GetVoteEndTime() const
    {
        return nVoteCommitTimeMillis;
    }

protected:
    bytes btBitmap;
    bytes btAggSig;

    int64 nVoteBeginTimeMillis;
    int64 nVoteCommitTimeMillis;
};

class CUpdateBlockData
{
public:
    CUpdateBlockData()
      : nBlockTime(0), nNonce(0) {}
    CUpdateBlockData(const uint256& hashForkIn, const uint256& hashBlockIn, const uint256& hashRefBlockIn, const int64 nBlockTimeIn, const uint64 nNonceIn)
      : hashFork(hashForkIn), hashBlock(hashBlockIn), hashRefBlock(hashRefBlockIn), nBlockTime(nBlockTimeIn), nNonce(nNonceIn) {}

public:
    uint256 hashFork;
    uint256 hashBlock;
    uint256 hashRefBlock;
    int64 nBlockTime;
    uint64 nNonce;
};

class CBlockVoteChnFork
{
public:
    CBlockVoteChnFork(const uint256& hashForkIn, const int64 nEpochDurationIn, funcSendNetData sendNetDataIn, funcGetVoteBlockCandidatePubkey getVoteBlockCandidatePubkeyIn, funcAddBlockLocalSignFlag addBlockLocalSignFlagIn, funcCommitVoteResult commitVoteResultIn)
      : hashFork(hashForkIn), consBlockVote(network::PROTO_CHN_BLOCK_VOTE, nEpochDurationIn, sendNetDataIn, getVoteBlockCandidatePubkeyIn, addBlockLocalSignFlagIn, commitVoteResultIn) {}

    bool AddLocalConsKey(const uint256& prikey, const uint384& pubkey);
    bool AddBlockCandidatePubkey(const uint256& hashBlock, const uint32 nBlockHeight, const int64 nBlockTime, const vector<uint384>& vPubkey);

protected:
    const uint256 hashFork;
    CConsBlockVote consBlockVote;
    std::map<uint256, CBlockVoteResult, CustomBlockHashCompare> mapVoteResult;
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

    bool HandleEvent(network::CEventPeerActive& eventActive) override;
protected:
    network::CBbPeerNet* pPeerNet;
    ICoreProtocol* pCoreProtocol;
    IBlockChain* pBlockChain;
};

} // namespace hashahead

#endif // HASHAHEAD_CHNBLOCKVOTE_H
