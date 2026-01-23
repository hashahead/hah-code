// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HASHAHEAD_NATOPEN_H
#define HASHAHEAD_NATOPEN_H

#include "base.h"
#include "network.h"

namespace hashahead
{

class CNatPortMapping : public INatPortMapping
{
public:
    CNatPortMapping();
    ~CNatPortMapping();

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    bool HandleInvoke() override;
    void HandleHalt() override;

protected:
    ICoreProtocol* pCoreProtocol;
    IBlockChain* pBlockChain;
    CNetwork* pNetwork;

    std::string strCfgNat;
    std::string strCfgNatLocalIp;
    std::string strCfgNatExtIp;
    uint16 nCfgNatExtPort;
    uint16 nCfgListenPort;
    bool fCfgNatModifyGateway;
    std::string strLocalIpaddress;

    hnbase::CThread thrNatWork;
    bool fThreadRun;
    boost::condition_variable condExit;
    boost::mutex mutex;

    int64 nPrevUpnpMapTime;
};

} // namespace hashahead

#endif // HASHAHEAD_NATOPEN_H
