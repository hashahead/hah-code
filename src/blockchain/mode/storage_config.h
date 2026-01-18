// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HASHAHEAD_MODE_STORAGE_CONFIG_H
#define HASHAHEAD_MODE_STORAGE_CONFIG_H

#include <string>

#include "mode/basic_config.h"

namespace hashahead
{
class CStorageConfig : virtual public CBasicConfig, virtual public CStorageConfigOption
{
public:
    CStorageConfig();
    virtual ~CStorageConfig();
    virtual bool PostLoad();
    virtual std::string ListConfig() const;
    virtual std::string Help() const;
};

} // namespace hashahead

#endif // HASHAHEAD_MODE_STORAGE_CONFIG_H
