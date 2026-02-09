// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HASHAHEAD_WSSERVICE_H
#define HASHAHEAD_WSSERVICE_H

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include "base.h"
#include "event.h"
#include "hnbase.h"

namespace hashahead
{

/////////////////////////////

typedef websocketpp::server<websocketpp::config::asio> WSSERVER;

using websocketpp::connection_hdl;
using websocketpp::frame::opcode::value;
using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

// pull out the type of messages sent by our config
typedef WSSERVER::message_ptr message_ptr;
typedef WSSERVER::connection_ptr connection_ptr;

/////////////////////////////
// CClientSubscribe

class CClientSubscribe
{
public:
    CClientSubscribe()
      : nClientConnId(0), nSubsType(0) {}
    CClientSubscribe(const uint64 nClientConnIdIn, const uint8 nSubsTypeIn, const std::set<CDestination>& setSubsAddressIn, const std::set<uint256>& setSubsTopicsIn)
      : nClientConnId(nClientConnIdIn), nSubsType(nSubsTypeIn), setSubsAddress(setSubsAddressIn), setSubsTopics(setSubsTopicsIn) {}

    void matchesLogs(CTransactionReceipt const& _m, MatchLogsVec& vLogs) const;

public:
    uint64 nClientConnId;
    uint8 nSubsType;
    std::set<CDestination> setSubsAddress;
    std::set<uint256> setSubsTopics;
};

// CWsService

class CWsService : public IWsService, virtual public CWsServiceEventListener
{
public:
    CWsService();
    ~CWsService();

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    bool HandleInvoke() override;
    void HandleHalt() override;

protected:
    hnbase::IIOModule* pRpcModIOModule;
};

} // namespace hashahead

#endif // HASHAHEAD_WSSERVICE_H
