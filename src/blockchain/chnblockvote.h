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

protected:
    network::CBbPeerNet* pPeerNet;
    ICoreProtocol* pCoreProtocol;
    IBlockChain* pBlockChain;
};

} // namespace hashahead

#endif // HASHAHEAD_CHNBLOCKVOTE_H
