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
