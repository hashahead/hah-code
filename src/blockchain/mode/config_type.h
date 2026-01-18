// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HASHAHEAD_MODE_TEMPLATE_CONFIG_TYPE_H
#define HASHAHEAD_MODE_TEMPLATE_CONFIG_TYPE_H

#include <tuple>

namespace hashahead
{
class CBasicConfig;
class CForkConfig;
class CMintConfig;
class CNetworkConfig;
class CRPCServerConfig;
class CRPCClientConfig;
class CStorageConfig;

///
// When add new config, add type to EConfigType add class pointer to ___ConfigTypeTemplate
//

// config type
enum class EConfigType
{
    BASIC,     // CBasicConfig
    FORK,      // CForkConfig
    MINT,      // CMintConfig
    NETWORK,   // CNetworkConfig
    RPCSERVER, // CRPCServerConfig
    RPCCLIENT, // CRPCClientConfig
    STORAGE    // CStorageConfig
};

namespace config_type
{
// config type class template
static const auto ___ConfigTypeTemplate = std::make_tuple(
    (CBasicConfig*)nullptr,
    (CForkConfig*)nullptr,
    (CMintConfig*)nullptr,
    (CNetworkConfig*)nullptr,
    (CRPCServerConfig*)nullptr,
    (CRPCClientConfig*)nullptr,
    (CStorageConfig*)nullptr);
} // namespace config_type

} // namespace hashahead

#endif // HASHAHEAD_MODE_TEMPLATE_CONFIG_TYPE_H
