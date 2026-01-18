// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HASHAHEAD_MODE_MINT_CONFIG_H
#define HASHAHEAD_MODE_MINT_CONFIG_H

#include <string>

#include "destination.h"
#include "mode/basic_config.h"
#include "uint256.h"

namespace hashahead
{

enum
{
    NODE_TYPE_COMMON,
    NODE_TYPE_SUPER,
    NODE_TYPE_FORK
};

class CMintConfig : virtual public CBasicConfig, virtual public CMintConfigOption
{
public:
    CMintConfig();
    virtual ~CMintConfig();
    virtual bool PostLoad();
    virtual std::string ListConfig() const;
    virtual std::string Help() const;

public:
    int nPeerType;
    std::map<uint256, std::pair<CDestination, uint32>> mapMint;
};

} // namespace hashahead

#endif // HASHAHEAD_MODE_MINT_CONFIG_H
