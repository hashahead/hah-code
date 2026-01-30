// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chnblockvote.h"

#include <boost/bind.hpp>

using namespace std;
using namespace hnbase;
using namespace consensus::consblockvote;
using boost::asio::ip::tcp;

#define BLOCK_VOTE_CHANNEL_THREAD_COUNT 1
#define BLOCK_VOTE_TIMER_TIME 1000
#define MAX_BLOCK_VOTE_RESULT_COUNT 1000

namespace hashahead
{

////////////////////////////////////////////////////
// CBlockVoteChnFork

bool CBlockVoteChnFork::AddLocalConsKey(const uint256& prikey, const uint384& pubkey)
{
    return consBlockVote.AddConsKey(prikey, pubkey);
}

bool CBlockVoteChnFork::AddBlockCandidatePubkey(const uint256& hashBlock, const uint32 nBlockHeight, const int64 nBlockTime, const vector<uint384>& vPubkey)
{
    while (mapVoteResult.size() >= MAX_BLOCK_VOTE_RESULT_COUNT)
    {
        mapVoteResult.erase(mapVoteResult.begin());
    }
    mapVoteResult[hashBlock] = CBlockVoteResult();
    return consBlockVote.AddCandidatePubkey(hashBlock, nBlockHeight, nBlockTime, vPubkey);
}

void CBlockVoteChnFork::CheckBlockVoteState(const uint256& hashBlock)
{
    consBlockVote.CheckBlockVoteState(hashBlock);
}

bool CBlockVoteChnFork::GetBlockVoteResult(const uint256& hashBlock, bytes& btBitmap, bytes& btAggSig)
{
    return consBlockVote.GetBlockVoteResult(hashBlock, btBitmap, btAggSig);
}

bool CBlockVoteChnFork::AddPeerNode(const uint64 nPeerNonce)
{
    return consBlockVote.AddNetNode(nPeerNonce);
}

void CBlockVoteChnFork::RemovePeerNode(const uint64 nPeerNonce)
{
    consBlockVote.RemoveNetNode(nPeerNonce);
}

void CBlockVoteChnFork::OnNetData(const uint64 nPeerNonce, const bytes& btNetData)
{
    consBlockVote.OnEventNetData(nPeerNonce, btNetData);
}

void CBlockVoteChnFork::OnTimer()
{
    consBlockVote.OnTimer();
}

void CBlockVoteChnFork::SetVoteResult(const uint256& hashBlock, const bytes& btBitmapIn, const bytes& btAggSigIn)
{
    auto it = mapVoteResult.find(hashBlock);
    if (it != mapVoteResult.end())
    {
        it->second.SetVoteResult(btBitmapIn, btAggSigIn);

        CBitmap bmVoteBitmap;
        bmVoteBitmap.ImportBytes(btBitmapIn);
        // StdDebug("TEST", "Commit Block Vote Result: vote bitmap: %s, agg sig: %s, vote duration: %lu ms, block: [%d] %s, fork: %s",
        //          bmVoteBitmap.GetBitmapString().c_str(), ToHexString(btAggSigIn).c_str(), GetTimeMillis() - it->second.GetVoteBeginTime(),
        //          CBlock::GetBlockHeightByHash(hashBlock), hashBlock.GetHex().c_str(), hashFork.GetHex().c_str());
    }
}

////////////////////////////////////////////////////
// CBlockVoteChannel

CBlockVoteChannel::CBlockVoteChannel()
  : network::IBlockVoteChannel(BLOCK_VOTE_CHANNEL_THREAD_COUNT), nBlockVoteTimerId(0)
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
    nBlockVoteTimerId = SetTimer(BLOCK_VOTE_TIMER_TIME, boost::bind(&CBlockVoteChannel::BlockVoteTimerFunc, this, _1));
    if (nBlockVoteTimerId == 0)
    {
        StdLog("CBlockVoteChannel", "Handle Invoke: Set timer fail");
        return false;
    }
    return network::IBlockVoteChannel::HandleInvoke();
}

void CBlockVoteChannel::HandleHalt()
{
    if (nBlockVoteTimerId != 0)
    {
        CancelTimer(nBlockVoteTimerId);
        nBlockVoteTimerId = 0;
    }
    network::IBlockVoteChannel::HandleHalt();
}

} // namespace hashahead
