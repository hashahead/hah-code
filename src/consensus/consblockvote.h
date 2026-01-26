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
