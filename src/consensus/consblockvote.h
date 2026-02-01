// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef __CONSENSUS_CONSBLOCKVOTE_H
#define __CONSENSUS_CONSBLOCKVOTE_H

#include "conscommon.h"

namespace consensus
{

namespace consblockvote
{

using namespace std;
using namespace hnbase;
using namespace hashahead::crypto;

/////////////////////////////////
// Protocol version

#define BLOCK_VOTE_PRO_VER_1 1

/////////////////////////////////
// const define

#define MAX_AWAIT_BIT_COUNT 20
#define MAX_CONS_HEIGHT_COUNT 2048

/////////////////////////////////
// Message id

#define PS_MSGID_BLOCKVOTE_SUBSCRIBE_REQ 1
#define PS_MSGID_BLOCKVOTE_SUBSCRIBE_RSP 2

/////////////////////////////////
// CNetNode

class CNetNode
{
public:
    CNetNode()
      : fSubscribe(false), nPeerVersion(0), nPrevBitmapReqTime(0) {}

    void Subscribe()
    {
        fSubscribe = true;
    }
    bool IsSubscribe() const
    {
        return fSubscribe;
    }
    void SetPeerVersion(const uint32 nPeerVersionIn)
    {
        nPeerVersion = nPeerVersionIn;
    }
    uint32 GetPeerVersion() const
    {
        return nPeerVersion;
    }

    void SetPrevBitmapReqTime()
    {
        nPrevBitmapReqTime = GetTimeMillis();
    }
    bool CheckPrevBitmapReqTime(const int64 nTimeLen)
    {
        if (fSubscribe && GetTimeMillis() - nPrevBitmapReqTime >= nTimeLen)
        {
            nPrevBitmapReqTime = GetTimeMillis();
            return true;
        }
        return false;
    }

private:
    bool fSubscribe;
    uint32 nPeerVersion;
    int64 nPrevBitmapReqTime;
};

/////////////////////////////////
// CConsKey

class CConsKey
{
public:
    CConsKey(const uint256& prikeyIn, const uint384& pubkeyIn)
      : prikey(prikeyIn), pubkey(pubkeyIn) {}

    bool Sign(const uint256& hash, bytes& btSig);
    bool Verify(const uint256& hash, const bytes& btSig);

public:
    const uint256 prikey;
    const uint384 pubkey;
};

class CNodePubkey
{
public:
    CNodePubkey(const uint32 nIndexIn, const uint384& pubkeyIn)
      : nIndex(nIndexIn), pubkey(pubkeyIn), eStatus(ES_INIT), tmStatusTime(time(NULL)) {}

    enum E_STATUS
    {
        ES_INIT,
        ES_WAITING,
        ES_COMPLETED,
    };

    void InitStatus()
    {
        eStatus = ES_INIT;
        tmStatusTime = time(NULL);
    }
    void SetStatus(const E_STATUS eStatusIn)
    {
        eStatus = eStatusIn;
        tmStatusTime = time(NULL);
    }
    E_STATUS GetStatus() const
    {
        return eStatus;
    }
    void CheckStatus()
    {
        if (eStatus == ES_WAITING && time(NULL) - tmStatusTime >= 100)
        {
            eStatus = ES_INIT;
            tmStatusTime = time(NULL);
        }
    }

public:
    const uint32 nIndex;
    const uint384 pubkey;

    E_STATUS eStatus;
    time_t tmStatusTime;
};

/////////////////////////////////
// CConsBlock

class CConsBlock
{
public:
    CConsBlock(const uint256& hashBlockIn, const uint32 nBlockEpochIn, const int64 nVoteBeginTimeIn, const uint64 nAddTimeIn)
      : hashBlock(hashBlockIn), nBlockEpoch(nBlockEpochIn), nVoteBeginTime(nVoteBeginTimeIn), nBeginTimeMillis(GetTimeMillis()), nAddTime(nAddTimeIn) {}

public:
    const uint256 hashBlock;
    const uint32 nBlockEpoch;
    const int64 nVoteBeginTime;
    map<uint384, uint32> mapCandidateNodeIndex; // key: node pubkey, value: index
};

/////////////////////////////////
// CConsBlockVote

class CConsBlockVote
{
public:
    CConsBlockVote(const uint8 nTunnelIdIn, const int64 nEpochDurationIn, funcSendNetData sendNetDataIn, funcGetVoteBlockCandidatePubkey getVoteBlockCandidatePubkeyIn, funcAddBlockLocalSignFlag addBlockLocalSignFlagIn, funcCommitVoteResult commitVoteResultIn)
      : nTunnelId(nTunnelIdIn), nEpochDuration(nEpochDurationIn), sendNetData(sendNetDataIn), getVoteBlockCandidatePubkey(getVoteBlockCandidatePubkeyIn), addBlockLocalSignFlag(addBlockLocalSignFlagIn), commitVoteResult(commitVoteResultIn), nPrevCheckPreVoteBitmapTime(0) {}

    bool AddConsKey(const uint256& prikey, const uint384& pubkey);

private:
    const uint8 nTunnelId;
    const int64 nEpochDuration; //Unit: milliseconds
    funcSendNetData sendNetData;
    funcGetVoteBlockCandidatePubkey getVoteBlockCandidatePubkey;
    funcAddBlockLocalSignFlag addBlockLocalSignFlag;
    funcCommitVoteResult commitVoteResult;

    map<uint256, CConsKey> mapConsKey;     // key: prikey
    map<uint64, CNetNode> mapNetNode;      // key: node netid
    map<uint256, CConsBlock> mapConsBlock; // key: block hash
    map<uint64, set<uint256>> mapAddTime;

    int64 nPrevCheckPreVoteBitmapTime;
};

} // namespace consblockvote
} // namespace consensus

#endif
