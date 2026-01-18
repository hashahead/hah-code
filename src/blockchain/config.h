// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HASHAHEAD_CONFIG_H
#define HASHAHEAD_CONFIG_H

#include <string>
#include <type_traits>
#include <vector>

#include "hnbase.h"
#include "mode/mode.h"

namespace hashahead
{
class CConfig
{
public:
    CConfig();
    ~CConfig();
    bool Load(int argc, char* argv[], const boost::filesystem::path& pathDefault,
              const std::string& strConfile);
    bool PostLoad();
    void ListConfig() const;
    std::string Help() const;

    inline CBasicConfig* GetConfig()
    {
        return pImpl;
    }
    inline EModeType GetModeType()
    {
        return emMode;
    }

protected:
    EModeType emMode;
    std::string subCmd;
    CBasicConfig* pImpl;
};

} // namespace hashahead

#endif // HASHAHEAD_CONFIG_H
