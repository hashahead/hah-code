// Copyright (c) 2021-2023 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HASHAHEAD_MODE_FORK_CONFIG_H
#define HASHAHEAD_MODE_FORK_CONFIG_H

#include <string>
#include <vector>

#include "mode/basic_config.h"

namespace hashahead
{
class CForkConfig : virtual public CBasicConfig, virtual public CForkConfigOption
{
public:
    CForkConfig();
    virtual ~CForkConfig();
    virtual bool PostLoad();
    virtual std::string ListConfig() const;
    virtual std::string Help() const;

public:
    bool fAllowAnyFork;
};

} // namespace hashahead

#endif // HASHAHEAD_MODE_FORK_CONFIG_H
