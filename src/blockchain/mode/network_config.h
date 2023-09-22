// Copyright (c) 2021-2023 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HASHAHEAD_MODE_NETWORK_CONFIG_H
#define HASHAHEAD_MODE_NETWORK_CONFIG_H

#include <string>
#include <vector>

#include "mode/basic_config.h"

namespace hashahead
{
class CNetworkConfig : virtual public CBasicConfig, virtual public CNetworkConfigOption
{
public:
    CNetworkConfig();
    virtual ~CNetworkConfig();
    virtual bool PostLoad();
    virtual std::string ListConfig() const;
    virtual std::string Help() const;

public:
    unsigned short nPort;
    unsigned int nMaxInBounds;
    unsigned int nMaxOutBounds;
};

} // namespace hashahead

#endif // HASHAHEAD_MODE_NETWORK_CONFIG_H
