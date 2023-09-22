// Copyright (c) 2021-2023 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HASHAHEAD_NETWORK_H
#define HASHAHEAD_NETWORK_H

#include "base.h"
#include "config.h"
#include "peernet.h"

namespace hashahead
{

class CNetwork : public network::CBbPeerNet
{
public:
    CNetwork();
    ~CNetwork();
    bool CheckPeerVersion(uint32 nVersionIn, uint64 nServiceIn, const std::string& subVersionIn) override;

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    const CNetworkConfig* NetworkConfig()
    {
        return dynamic_cast<const CNetworkConfig*>(hnbase::IBase::Config());
    }
    const CStorageConfig* StorageConfig()
    {
        return dynamic_cast<const CStorageConfig*>(hnbase::IBase::Config());
    }

protected:
    ICoreProtocol* pCoreProtocol;
};

} // namespace hashahead

#endif //HASHAHEAD_NETWORK_H
