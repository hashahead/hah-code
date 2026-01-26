// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "wsservice.h"

#include "version.h"

using namespace std;
using namespace hnbase;
using namespace hashahead::crypto;

namespace hashahead
{

//////////////////////////////
// CWsService

CWsService::CWsService()
{
    pRpcModIOModule = nullptr;
}

CWsService::~CWsService()
{
}

//----------------------------------------------------------------------------
bool CWsService::HandleInitialize()
{
    if (!GetObject("rpcmod", pRpcModIOModule))
    {
        Error("Failed to request rpcmod");
        return false;
    }
    return true;
}

void CWsService::HandleDeinitialize()
{
    pRpcModIOModule = nullptr;
}

bool CWsService::HandleInvoke()
{
    if (!IWsService::HandleInvoke())
    {
        return false;
    }
    return true;
}

void CWsService::HandleHalt()
{
    IWsService::HandleHalt();
}

} // namespace hashahead
