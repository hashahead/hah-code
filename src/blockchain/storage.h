// Copyright (c) 2021-2023 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HASHAHEAD_STORAGE_H
#define HASHAHEAD_STORAGE_H

#include <boost/filesystem.hpp>
#include <hnbase.h>

#include "config.h"

namespace hashahead
{

class CEntry : public hnbase::CEntry
{
public:
    CEntry();
    ~CEntry();
    bool Initialize(int argc, char* argv[]);
    bool Run();
    void Exit();

protected:
    bool InitializeService();
    bool InitializeClient();
    //    hnbase::CHttpHostConfig GetRPCHostConfig();
    //    hnbase::CHttpHostConfig GetWebUIHostConfig();

    boost::filesystem::path GetDefaultDataDir();

    bool SetupEnvironment();
    bool RunInBackground(const boost::filesystem::path& pathData);
    void ExitBackground(const boost::filesystem::path& pathData);

protected:
    CConfig config;
    hnbase::CLog log;
    hnbase::CDocker docker;
};

} // namespace hashahead

#endif //HASHAHEAD_STORAGE_H
