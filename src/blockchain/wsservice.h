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
