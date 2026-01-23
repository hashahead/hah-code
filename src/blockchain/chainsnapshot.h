// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HASHAHEAD_CHAINSNAPSHOT_H
#define HASHAHEAD_CHAINSNAPSHOT_H

#include "base.h"

namespace hashahead
{

class CChainSnapshot : public IChainSnapshot
{
public:
    CChainSnapshot();
    ~CChainSnapshot();

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    bool HandleInvoke() override;
    void HandleHalt() override;

    void SnapshotThreadFunc();

protected:
    ICoreProtocol* pCoreProtocol;
    IBlockChain* pBlockChain;
};

} // namespace hashahead

#endif // HASHAHEAD_CHAINSNAPSHOT_H
