// Copyright (c) 2021-2023 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HASHAHEAD_MODE_RPC_CONFIG_H
#define HASHAHEAD_MODE_RPC_CONFIG_H

#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <string>
#include <vector>

#include "mode/basic_config.h"

namespace hashahead
{

class CRPCBasicConfig : virtual public CBasicConfig, virtual public CRPCBasicConfigOption
{
public:
    CRPCBasicConfig();
    virtual ~CRPCBasicConfig();
    virtual bool PostLoad();
    virtual std::string ListConfig() const;
    virtual std::string Help() const;

public:
    unsigned int nRPCPort;
};

class CRPCServerConfig : virtual public CRPCBasicConfig, virtual public CRPCServerConfigOption
{
public:
    CRPCServerConfig();
    virtual ~CRPCServerConfig();
    virtual bool PostLoad();
    virtual std::string ListConfig() const;
    virtual std::string Help() const;

public:
    boost::asio::ip::tcp::endpoint epRPC;
    std::vector<std::pair<uint32, uint16>> vecChainIdRpcPort;
};

class CRPCClientConfig : virtual public CRPCBasicConfig, virtual public CRPCClientConfigOption
{
public:
    CRPCClientConfig();
    virtual ~CRPCClientConfig();
    virtual bool PostLoad();
    virtual std::string ListConfig() const;
    virtual std::string Help() const;
};

} // namespace hashahead

#endif // HASHAHEAD_MODE_RPC_CONFIG_H
