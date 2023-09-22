// Copyright (c) 2021-2023 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HASHAHEAD_ENTRY_H
#define HASHAHEAD_ENTRY_H

#include <boost/filesystem.hpp>
#include <hnbase.h>

#include "config.h"

namespace hashahead
{

class CBbEntry : public hnbase::CEntry
{
public:
    static CBbEntry& GetInstance();

public:
    ~CBbEntry();
    bool Initialize(int argc, char* argv[]);
    bool Run() override;
    void Exit();

protected:
    bool InitializeModules(const EModeType& mode);
    bool AttachModule(hnbase::IBase* pBase);

    bool GetRPCHostConfig(std::vector<hnbase::CHttpHostConfig>& vHostCfg);

    void PurgeStorage();

    boost::filesystem::path GetDefaultDataDir();

    bool SetupEnvironment();
    bool RunInBackground(const boost::filesystem::path& pathData);
    void ExitBackground(const boost::filesystem::path& pathData);

protected:
    CConfig config;
    hnbase::CLog log;
    hnbase::CDocker docker;

private:
    CBbEntry();
};

} // namespace hashahead

#endif //HASHAHEAD_ENTRY_H
