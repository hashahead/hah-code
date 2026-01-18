// Copyright (c) 2021-2023 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HASHAHEAD_RECOVERY_H
#define HASHAHEAD_RECOVERY_H

#include "base.h"

namespace hashahead
{

class CRecovery : public IRecovery
{
public:
    CRecovery();
    ~CRecovery();

    bool IsExit() const;
    bool AddRecoveryBlock(const CBlockEx& block);

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    bool HandleInvoke() override;
    void HandleHalt() override;
    bool IsRecoverying() override;

    bool GetForkLastHeight(std::map<uint32, uint32>& mapForkLastHeight);

    void RecoveryThreadFunc();
    void RecoveryWork();

protected:
    IDispatcher* pDispatcher;
    IBlockChain* pBlockChain;
    ICoreProtocol* pCoreProtocol;

    bool fRecoveryWork;
    std::string strRecoveryDir;

    hnbase::CThread thrRecovery;
    std::atomic<bool> fExit;

    uint32 nPrimaryChainId;
    uint32 nPrimaryMinHeight;

    std::map<uint32, std::queue<CBlockEx>> mapForkBlockQueue; // key: fork chain id
};

} // namespace hashahead

#endif // HASHAHEAD_RECOVERY_H
