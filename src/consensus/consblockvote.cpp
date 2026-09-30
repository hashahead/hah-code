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

bool CConsBlock::AddCommitVoteSign(const uint384& pubkeyNode, const bytes& btSig)
{
    auto it = mapCommitVoteSig.find(pubkeyNode);
    if (it == mapCommitVoteSig.end())
    {
        auto mt = mapCandidateNodeIndex.find(pubkeyNode);
        if (mt != mapCandidateNodeIndex.end() && mt->second < vCommitVoteCandidateNodePubkey.size())
        {
            mapCommitVoteSig.insert(make_pair(pubkeyNode, btSig));
            bmBlockCommitVoteBitmap.SetBit(mt->second);

            vCommitVoteCandidateNodePubkey[mt->second].SetStatus(CNodePubkey::ES_COMPLETED);

#ifdef CBV_SHOW_DEBUG
            StdDebug("CConsBlock", "Add commit vote sig: Add commit vote sig success, pubkey: %s, block: %s",
                     pubkeyNode.GetHex().c_str(), hashBlock.GetBhString().c_str());
#endif
        }
        else
        {
            if (mt == mapCandidateNodeIndex.end())
            {
                StdLog("CConsBlock", "Add commit vote sig: Pubkey not exist, pubkey: %s", pubkeyNode.GetHex().c_str());
            }
            else
            {
                StdLog("CConsBlock", "Add commit vote sig: Pubkey index error, index: %d, candidate pubkey count: %lu, pubkey: %s",
                       mt->second, vPreVoteCandidateNodePubkey.size(), pubkeyNode.GetHex().c_str());
            }
        }
    }
    return true;
}

bool CConsBlock::GetPreVoteBitmap(bytes& btBitmap)
{
    if (bmBlockPreVoteBitmap.HasValidBit())
    {
        bmBlockPreVoteBitmap.GetBytes(btBitmap);
        return true;
    }
    return false;
}

bool CConsBlock::GetCommitVoteBitmap(bytes& btBitmap)
{
    if (bmBlockCommitVoteBitmap.HasValidBit())
    {
        bmBlockCommitVoteBitmap.GetBytes(btBitmap);
        return true;
    }
    return false;
}

void CConsBlock::GetPreVoteSigByBitmap(const bytes& btGetBitmap, map<uint384, bytes>& mapSigOut)
{
    CBitmap btm;
    if (!btm.ImportBytes(btGetBitmap))
    {
        StdLog("CConsBlock", "Get pre vote sig by bitmap: Bitmap error");
        return;
    }

    vector<uint32> vIndexList;
    btm.GetIndexList(vIndexList);
    if (vIndexList.empty())
    {
        StdLog("CConsBlock", "Get pre vote sig by bitmap: Index list empty, max bit: %d, valid bit: %d", btm.GetMaxBits(), btm.HasValidBit());
        return;
    }

    for (auto& index : vIndexList)
    {
        if (index < (uint32)(vPreVoteCandidateNodePubkey.size()))
        {
            const uint384& pubkeyNode = vPreVoteCandidateNodePubkey[index].pubkey;
            auto it = mapPreVoteSig.find(pubkeyNode);
            if (it != mapPreVoteSig.end())
            {
                mapSigOut.insert(make_pair(pubkeyNode, it->second));
            }
            else
            {
                StdLog("CConsBlock", "Get pre vote sig by bitmap: Find pre vote fail, index: %d, pre vote size: %lu, pubkey: %s, block: %s",
                       index, mapPreVoteSig.size(), pubkeyNode.GetHex().c_str(), hashBlock.GetBhString().c_str());
            }
        }
    }
}

void CConsBlock::GetCommitVoteSigByBitmap(const CBitmap& bmGetBitmap, map<uint384, bytes>& mapSigOut)
{
    vector<uint32> vIndexList;
    bmGetBitmap.GetIndexList(vIndexList);
    if (vIndexList.empty())
    {
        StdLog("CConsBlock", "Get commit vote sig by bitmap: Index list empty, has bit: %d", bmGetBitmap.GetValidBits());
        return;
    }

    for (auto& index : vIndexList)
    {
        if (index < (uint32)(vCommitVoteCandidateNodePubkey.size()))
        {
            const uint384& pubkeyNode = vCommitVoteCandidateNodePubkey[index].pubkey;
            auto it = mapCommitVoteSig.find(pubkeyNode);
            if (it != mapCommitVoteSig.end())
            {
                mapSigOut.insert(make_pair(pubkeyNode, it->second));
            }
            else
            {
                StdLog("CConsBlock", "Get commit vote sig by bitmap: Find commit vote fail, index: %d, commit vote size: %lu, pubkey: %s, block: %s",
                       index, mapPreVoteSig.size(), pubkeyNode.GetHex().c_str(), hashBlock.GetBhString().c_str());
            }
        }
    }
}

bool CConsBlock::GetPreVoteAwaitBitmap(const bytes& btBitmapPeer, bytes& btBitmapOut)
{
    CBitmap btmPeer;
    if (!btmPeer.ImportBytes(btBitmapPeer))
    {
        return false;
    }
    if (btmPeer.GetMaxBits() != bmBlockPreVoteBitmap.GetMaxBits())
    {
        return false;
    }
    if (btmPeer == bmBlockPreVoteBitmap)
    {
        return false;
    }

    //(local | peer) ^ local
    btmPeer |= bmBlockPreVoteBitmap;
    btmPeer ^= bmBlockPreVoteBitmap;

    vector<uint32> vIndexList;
    btmPeer.GetIndexList(vIndexList);

    CBitmap bmOut;
    bmOut.Initialize(bmBlockPreVoteBitmap.GetMaxBits());

    uint32 nAddCount = 0;
    for (auto& index : vIndexList)
    {
        if (index < vPreVoteCandidateNodePubkey.size())
        {
            auto& node = vPreVoteCandidateNodePubkey[index];
            node.CheckStatus();
            if (node.GetStatus() == CNodePubkey::ES_INIT)
            {
                bmOut.SetBit(index);
                node.SetStatus(CNodePubkey::ES_WAITING);
                if (++nAddCount >= MAX_AWAIT_BIT_COUNT)
                {
                    break;
                }
            }
        }
    }
    if (nAddCount == 0)
    {
        return false;
    }
    bmOut.GetBytes(btBitmapOut);
    return true;
}

bool CConsBlock::GetCommitVoteAwaitBitmap(const bytes& btBitmapPeer, bytes& btBitmapOut)
{
    CBitmap btmPeer;
    if (!btmPeer.ImportBytes(btBitmapPeer))
    {
        return false;
    }
    if (btmPeer.GetMaxBits() != bmBlockCommitVoteBitmap.GetMaxBits())
    {
        return false;
    }
    if (btmPeer == bmBlockCommitVoteBitmap)
    {
        return false;
    }

    //(local | peer) ^ local
    btmPeer |= bmBlockCommitVoteBitmap;
    btmPeer ^= bmBlockCommitVoteBitmap;

    vector<uint32> vIndexList;
    btmPeer.GetIndexList(vIndexList);

    CBitmap bmOut;
    bmOut.Initialize(bmBlockCommitVoteBitmap.GetMaxBits());

    uint32 nAddCount = 0;
    for (auto& index : vIndexList)
    {
        if (index < vCommitVoteCandidateNodePubkey.size())
        {
            auto& node = vCommitVoteCandidateNodePubkey[index];
            node.CheckStatus();
            if (node.GetStatus() == CNodePubkey::ES_INIT)
            {
                bmOut.SetBit(index);
                node.SetStatus(CNodePubkey::ES_WAITING);
                if (++nAddCount >= MAX_AWAIT_BIT_COUNT)
                {
                    break;
                }
            }
        }
    }
    if (nAddCount == 0)
    {
        return false;
    }
    bmOut.GetBytes(btBitmapOut);
    return true;
}

bool CConsBlock::GetPubkeysByBitmap(const CBitmap& bmBitmap, vector<uint384>& vPubkeys)
{
    vector<uint32> vIndexList;
    bmBitmap.GetIndexList(vIndexList);
    for (auto& index : vIndexList)
    {
        if (index >= vPreVoteCandidateNodePubkey.size())
        {
            StdLog("CConsBlock", "Get pre vote pubkeys by bitmap: Index error, index: %d, block: %s", index, hashBlock.GetBhString().c_str());
            return false;
        }
        vPubkeys.push_back(vPreVoteCandidateNodePubkey[index].pubkey);
    }
    if (vPubkeys.size() < (vPreVoteCandidateNodePubkey.size() * 2 / 3))
    {
        StdLog("CConsBlock", "Get pre vote pubkeys by bitmap: Sign not enough, sign count: %lu, candidate count: %lu, index count: %lu, block: %s",
               vPubkeys.size(), vPreVoteCandidateNodePubkey.size(), vIndexList.size(), hashBlock.GetBhString().c_str());
        return false;
    }
    return true;
}

bool CConsBlock::GetLocalPreVoteSign(const int64 nEpochDurationIn, CBitmap& bmPreVoteBitmap, vector<uint384>& vPubkeys, vector<bytes>& vSigs)
{
    // int64 nWaitTime = nEpochDurationIn / 10;
    // if (nWaitTime < 500)
    // {
    //     nWaitTime = 500;
    // }
    // else if (nWaitTime > 3000)
    // {
    //     nWaitTime = 3000;
    // }
    // if (GetTimeMillis() - nVoteBeginTime < nWaitTime)
    // {
    //     return false;
    // }

#ifdef CBV_SHOW_DEBUG
    StdDebug("CConsBlock", "Get local pre vote sig: pre vote sig count: %lu, candidate node count: %lu, block: %s",
             mapPreVoteSig.size(), mapCandidateNodeIndex.size(), hashBlock.GetBhString().c_str());
#endif

    if (mapPreVoteSig.size() >= (mapCandidateNodeIndex.size() * 2 / 3))
    {
        vector<uint32> vIndexList;
        bmBlockPreVoteBitmap.GetIndexList(vIndexList);
        if (vIndexList.empty() || vIndexList.size() != mapPreVoteSig.size())
        {
            StdLog("CConsBlock", "Get local pre vote sig: Pre vote bitmap bits error, bits: %lu, sigs: %lu, block: %s",
                   vIndexList.size(), mapPreVoteSig.size(), hashBlock.GetBhString().c_str());
            return false;
        }
        vPubkeys.reserve(vIndexList.size());
        vSigs.reserve(vIndexList.size());
        for (auto& index : vIndexList)
        {
            if (index >= vPreVoteCandidateNodePubkey.size())
            {
                StdLog("CConsBlock", "Get local pre vote sig: Index error, index: %d, block: %s", index, hashBlock.GetBhString().c_str());
                return false;
            }
            auto& pubkey = vPreVoteCandidateNodePubkey[index].pubkey;
            vPubkeys.push_back(pubkey);

            auto it = mapPreVoteSig.find(pubkey);
            if (it == mapPreVoteSig.end())
            {
                StdLog("CConsBlock", "Get local pre vote sig: Find sig fail, index: %d, pubkey: %s, block: %s",
                       index, pubkey.GetHex().c_str(), hashBlock.GetBhString().c_str());
                return false;
            }
            vSigs.push_back(it->second);
        }
        bmPreVoteBitmap = bmBlockPreVoteBitmap;
        return true;
    }
    return false;
}

bool CConsBlock::GetLocalCommitVoteSign(const int64 nEpochDurationIn, CBitmap& bmCommitVoteBitmap, vector<uint384>& vPubkeys, vector<bytes>& vSigs)
{
    // int64 nWaitTime = nEpochDurationIn / 10;
    // if (nWaitTime < 500)
    // {
    //     nWaitTime = 500;
    // }
    // else if (nWaitTime > 3000)
    // {
    //     nWaitTime = 3000;
    // }
    // if (GetTimeMillis() - nVoteBeginTime < nWaitTime)
    // {
    //     return false;
    // }

#ifdef CBV_SHOW_DEBUG
    StdDebug("CConsBlock", "Get local commit vote sig: commit vote sig count: %lu, candidate node count: %lu, block: %s",
             mapCommitVoteSig.size(), mapCandidateNodeIndex.size(), hashBlock.GetBhString().c_str());
#endif

    if (mapCommitVoteSig.size() >= (mapCandidateNodeIndex.size() * 2 / 3))
    {
        vector<uint32> vIndexList;
        bmBlockCommitVoteBitmap.GetIndexList(vIndexList);
        if (vIndexList.empty() || vIndexList.size() != mapCommitVoteSig.size())
        {
            StdLog("CConsBlock", "Get local commit vote sig: Commit vote bitmap bits error, bits: %lu, sigs: %lu, block: %s",
                   vIndexList.size(), mapCommitVoteSig.size(), hashBlock.GetBhString().c_str());
            return false;
        }
        vPubkeys.reserve(vIndexList.size());
        vSigs.reserve(vIndexList.size());
        for (auto& index : vIndexList)
        {
            if (index >= vCommitVoteCandidateNodePubkey.size())
            {
                StdLog("CConsBlock", "Get local commit vote sig: Index error, index: %d, block: %s", index, hashBlock.GetBhString().c_str());
                return false;
            }
            auto& pubkey = vCommitVoteCandidateNodePubkey[index].pubkey;
            vPubkeys.push_back(pubkey);

            auto it = mapCommitVoteSig.find(pubkey);
            if (it == mapCommitVoteSig.end())
            {
                StdLog("CConsBlock", "Get local commit vote sig: Find sig fail, index: %d, pubkey: %s, block: %s",
                       index, pubkey.GetHex().c_str(), hashBlock.GetBhString().c_str());
                return false;
            }
            vSigs.push_back(it->second);
        }
        bmCommitVoteBitmap = bmBlockCommitVoteBitmap;
        return true;
    }
    return false;
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
