// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HASHAHEAD_CHNBLOCKCROSSPROVE_H
#define HASHAHEAD_CHNBLOCKCROSSPROVE_H

#include "base.h"
#include "consblockvote.h"
#include "peernet.h"

using namespace consensus;

namespace hashahead
{

class CBlockCrossProveChannel : public network::IBlockCrossProveChannel
{
public:
    CBlockCrossProveChannel();
    ~CBlockCrossProveChannel();

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    bool HandleInvoke() override;
    void HandleHalt() override;

    bool HandleEvent(network::CEventPeerActive& eventActive) override;
    bool HandleEvent(network::CEventPeerDeactive& eventDeactive) override;

protected:
    network::CBbPeerNet* pPeerNet;
    ICoreProtocol* pCoreProtocol;
    IBlockChain* pBlockChain;
};

} // namespace hashahead

#endif // HASHAHEAD_CHNBLOCKCROSSPROVE_H
