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

bool CBlockVoteChannel::HandleEvent(network::CEventPeerActive& eventActive)
{
    const uint64 nNonce = eventActive.nNonce;
    mapChnPeer[nNonce] = CBlockVoteChnPeer(eventActive.data.nService, eventActive.data);

    StdLog("CBlockVoteChannel", "CEvent Peer Active: peer: [0x%lx] %s", nNonce, GetPeerAddressInfo(nNonce).c_str());

    for (auto& kv : mapChnFork)
    {
        kv.second.AddPeerNode(nNonce);
    }
    return true;
}

bool CBlockVoteChannel::HandleEvent(network::CEventPeerDeactive& eventDeactive)
{
    const uint64 nNonce = eventDeactive.nNonce;
    StdLog("CBlockVoteChannel", "CEvent Peer Deactive: peer: [0x%lx] %s", nNonce, GetPeerAddressInfo(nNonce).c_str());

    mapChnPeer.erase(nNonce);
    for (auto& kv : mapChnFork)
    {
        kv.second.RemovePeerNode(nNonce);
    }
    return true;
}

bool CBlockVoteChannel::HandleEvent(network::CEventPeerBlockVoteProtoData& eventBvp)
{
    auto it = mapChnFork.find(eventBvp.hashFork);
    if (it != mapChnFork.end())
    {
        it->second.OnNetData(eventBvp.nNonce, eventBvp.data);
    }
    return true;
}

//------------------------------------------
bool CBlockVoteChannel::HandleEvent(network::CEventLocalBlockvoteTimer& eventTimer)
{
    for (auto& kv : mapChnFork)
    {
        kv.second.OnTimer();
    }
    OnWaitNewBlock();
    return true;
}

bool CBlockVoteChannel::HandleEvent(network::CEventLocalBlockvoteSubscribeFork& eventSubsFork)
{
    for (auto& hashFork : eventSubsFork.data)
    {
        CBlockStatus status;
        if (!pBlockChain->GetLastBlockStatus(hashFork, status))
        {
            StdLog("CBlockVoteChannel", "CEvent Local Blockvote Subscribe Fork: Fork is not enabled, fork: %s", hashFork.GetHex().c_str());
            continue;
        }
        StdLog("CBlockVoteChannel", "CEvent Local Blockvote Subscribe Fork: Subscribe fork, last block: %s, ref block: %s, fork: %s",
               status.hashBlock.ToString().c_str(), status.hashRefBlock.ToString().c_str(), hashFork.GetHex().c_str());

        if (mapChnFork.count(hashFork) == 0)
        {
            int64 nEpochDuration = 0;
            if (hashFork == pCoreProtocol->GetGenesisBlockHash())
            {
                nEpochDuration = BLOCK_TARGET_SPACING * 1000;
            }
            else
            {
                nEpochDuration = EXTENDED_BLOCK_SPACING * 1000;
            }

            auto it = mapChnFork.insert(std::make_pair(hashFork, CBlockVoteChnFork(hashFork, nEpochDuration,
                                                                                   boost::bind(&CBlockVoteChannel::SendNetData, this, _1, _2, _3, hashFork),
                                                                                   boost::bind(&CBlockVoteChannel::GetVoteBlockCandidatePubkey, this, _1, _2, _3, _4, _5, _6, hashFork),
                                                                                   boost::bind(&CBlockVoteChannel::AddBlockLocalVoteSignFlag, this, _1, hashFork),
                                                                                   boost::bind(&CBlockVoteChannel::CommitBlockVoteResult, this, _1, _2, _3, hashFork))))
                          .first;
            if (it != mapChnFork.end())
            {
                for (auto& kv : MintConfig()->mapMint)
                {
                    const uint256& keyMint = kv.first;
                    CCryptoBlsKey blskey;
                    if (!crypto::CryptoBlsMakeNewKey(blskey, keyMint))
                    {
                        StdLog("CBlockVoteChannel", "CEvent Local Blockvote Subscribe Fork: Make new key fail, fork: %s", hashFork.GetHex().c_str());
                        continue;
                    }
                    if (!it->second.AddLocalConsKey(blskey.secret, blskey.pubkey))
                    {
                        StdLog("CBlockVoteChannel", "CEvent Local Blockvote Subscribe Fork: Add local cons key fail, fork: %s", hashFork.GetHex().c_str());
                        continue;
                    }
                }
                for (auto& kv : mapChnPeer)
                {
                    if (it->second.AddPeerNode(kv.first))
                    {
                        StdLog("CBlockVoteChannel", "CEvent Local Blockvote Subscribe Fork: Add peer node success, peer: [0x%lx] %s", kv.first, GetPeerAddressInfo(kv.first).c_str());
                    }
                    else
                    {
                        StdLog("CBlockVoteChannel", "CEvent Local Blockvote Subscribe Fork: Add peer node fail, peer: [0x%lx] %s", kv.first, GetPeerAddressInfo(kv.first).c_str());
                    }
                }

                if (!AddBlockVoteCandidatePubkey(status.hashBlock, status.nBlockHeight, status.nBlockTime, it->second))
                {
                    StdLog("CBlockVoteChannel", "CEvent Local Blockvote Subscribe Fork: Update block vote fail, last block: %s, fork: %s", status.hashBlock.ToString().c_str(), hashFork.GetHex().c_str());
                    continue;
                }
                StdDebug("CBlockVoteChannel", "CEvent Local Blockvote Subscribe Fork: Update block vote success, last block: %s, fork: %s", status.hashBlock.ToString().c_str(), hashFork.GetHex().c_str());
            }
            else
            {
                StdLog("CBlockVoteChannel", "CEvent Local Blockvote Subscribe Fork: Add fork fail, last block: %s, fork: %s", status.hashBlock.ToString().c_str(), hashFork.GetHex().c_str());
            }
        }
        else
        {
            StdLog("CBlockVoteChannel", "CEvent Local Blockvote Subscribe Fork: Fork existed, last block: %s, fork: %s", status.hashBlock.ToString().c_str(), hashFork.GetHex().c_str());
        }
    }
    return true;
}

} // namespace hashahead
