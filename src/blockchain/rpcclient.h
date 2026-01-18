// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HASHAHEAD_RPCCLIENT_H
#define HASHAHEAD_RPCCLIENT_H

#include "json/json_spirit_value.h"
#include <boost/asio.hpp>
#include <string>
#include <thread>
#include <vector>

#include "base.h"
#include "hnbase.h"
#include "rpc/rpc.h"

namespace hashahead
{

class CRPCClient : public hnbase::IIOModule, virtual public hnbase::CHttpEventListener
{
public:
    CRPCClient(bool fConsole = true);
    ~CRPCClient();
    void ConsoleHandleLine(const std::string& strLine);

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    bool HandleInvoke() override;
    void HandleHalt() override;
    const CRPCClientConfig* Config();

    bool HandleEvent(hnbase::CEventHttpGetRsp& event) override;
    bool GetResponse(uint64 nNonce, const std::string& content);
    bool CallRPC(rpc::CRPCParamPtr spParam, int nReqId);
    bool CallConsoleCommand(const std::vector<std::string>& vCommand);
    void LaunchConsole();
    void LaunchCommand();
    void CancelCommand();

    void EnterLoop();
    void LeaveLoop();

protected:
    hnbase::IIOProc* pHttpGet;
    hnbase::CThread thrDispatch;
    std::vector<std::string> vArgs;
    uint64 nLastNonce;
    hnbase::CIOCompletion ioComplt;
    std::atomic<bool> isConsoleRunning;
};

} // namespace hashahead
#endif //HASHAHEAD_RPCCLIENT_H
