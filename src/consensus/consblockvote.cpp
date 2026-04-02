// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "consblockvote.h"

#include "msgblockvote.pb.h"

namespace consensus
{

namespace consblockvote
{

using namespace std;
using namespace hashahead::crypto;

// #define CBV_SHOW_DEBUG

/////////////////////////////////
#define PSD_SET_MSG(MSGID, PBMSG, OUTMSG)                              \
    OUTMSG.resize(PBMSG.ByteSizeLong() + 1);                           \
    if (!PBMSG.SerializeToArray(OUTMSG.data() + 1, OUTMSG.size() - 1)) \
    {                                                                  \
        StdError(__PRETTY_FUNCTION__, "SerializeToArray fail");        \
        return false;                                                  \
    }                                                                  \
    OUTMSG[0] = MSGID;

/////////////////////////////////
// CConsKey

bool CConsKey::Sign(const uint256& hash, bytes& btSig)
{
    return CryptoBlsSign(prikey, hash.GetBytes(), btSig);
}

bool CConsKey::Verify(const uint256& hash, const bytes& btSig)
{
    return CryptoBlsVerify(pubkey, hash.GetBytes(), btSig);
}

/////////////////////////////////
// CConsBlock

bool CConsBlock::SetCandidateNodeList(const vector<uint384>& vCandidateNodePubkeyIn)
{
    if (vCandidateNodePubkeyIn.empty())
    {
        StdLog("CConsBlock", "Set candidate node list: Candidate node is empty");
        return false;
    }

    mapCandidateNodeIndex.clear();
    vPreVoteCandidateNodePubkey.clear();
    vCommitVoteCandidateNodePubkey.clear();
    bmBlockPreVoteBitmap.Initialize(vCandidateNodePubkeyIn.size());
    bmBlockCommitVoteBitmap.Initialize(vCandidateNodePubkeyIn.size());

    for (uint32 i = 0; i < (uint32)(vCandidateNodePubkeyIn.size()); i++)
    {
        auto& pubkey = vCandidateNodePubkeyIn[i];
        auto it = mapCandidateNodeIndex.find(pubkey);
        if (it != mapCandidateNodeIndex.end())
        {
            StdLog("CConsBlock", "Set candidate node list: Candidate node index existed");
            mapCandidateNodeIndex.clear();
            vPreVoteCandidateNodePubkey.clear();
            vCommitVoteCandidateNodePubkey.clear();
            bmBlockPreVoteBitmap.Clear();
            bmBlockCommitVoteBitmap.Clear();
            return false;
        }
        mapCandidateNodeIndex.insert(make_pair(pubkey, i));
        vPreVoteCandidateNodePubkey.push_back(CNodePubkey(i, pubkey));
        vCommitVoteCandidateNodePubkey.push_back(CNodePubkey(i, pubkey));
    }
    return true;
}

bool CConsBlock::IsExistCandidateNodePubkey(const uint384& pubkeyNode) const
{
    return (mapCandidateNodeIndex.find(pubkeyNode) != mapCandidateNodeIndex.end());
}

bool CConsBlock::ExistPreVoteSign(const uint384& pubkeyNode)
{
    return (mapPreVoteSig.find(pubkeyNode) != mapPreVoteSig.end());
}

bool CConsBlock::ExistCommitVoteSign(const uint384& pubkeyNode)
{
    return (mapCommitVoteSig.find(pubkeyNode) != mapCommitVoteSig.end());
}

bool CConsBlock::AddPreVoteSign(const uint384& pubkeyNode, const bytes& btSig)
{
    auto it = mapPreVoteSig.find(pubkeyNode);
    if (it == mapPreVoteSig.end())
    {
        auto mt = mapCandidateNodeIndex.find(pubkeyNode);
        if (mt != mapCandidateNodeIndex.end() && mt->second < vPreVoteCandidateNodePubkey.size())
        {
            mapPreVoteSig.insert(make_pair(pubkeyNode, btSig));
            bmBlockPreVoteBitmap.SetBit(mt->second);

            vPreVoteCandidateNodePubkey[mt->second].SetStatus(CNodePubkey::ES_COMPLETED);
#ifdef CBV_SHOW_DEBUG
            StdDebug("CConsBlock", "Add pre vote sig: Add pre vote sig success, pubkey: %s, block: %s",
                     pubkeyNode.GetHex().c_str(), hashBlock.GetBhString().c_str());
#endif
        }
        else
        {
            if (mt == mapCandidateNodeIndex.end())
            {
                StdLog("CConsBlock", "Add pre vote sig: Pubkey not exist, pubkey: %s", pubkeyNode.GetHex().c_str());
            }
            else
            {
                StdLog("CConsBlock", "Add pre vote sig: Pubkey index error, index: %d, candidate pubkey count: %lu, pubkey: %s",
                       mt->second, vPreVoteCandidateNodePubkey.size(), pubkeyNode.GetHex().c_str());
            }
        }
    }
    return true;
}

/////////////////////////////////
// CConsBlockVote

bool CConsBlockVote::AddConsKey(const uint256& prikey, const uint384& pubkey)
{
    if (mapConsKey.find(prikey) == mapConsKey.end())
    {
        mapConsKey.insert(make_pair(prikey, CConsKey(prikey, pubkey)));
    }
    return true;
}
} // namespace consblockvote
} // namespace consensus
