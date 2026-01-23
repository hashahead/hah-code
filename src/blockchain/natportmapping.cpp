// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "natportmapping.h"

#include <ifaddrs.h>
#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/upnpcommands.h>
#include <miniupnpc/upnperrors.h>

#include "natpmp.h"

#define LEASE_DURATION 3600
#define UPNP_ORDER_WAIT_DURATION 2000

using namespace hnbase;

namespace hashahead
{

/////////////////////////////////////////////////
// CNatPortMapping

CNatPortMapping::CNatPortMapping()
  : thrNatWork("natportmapping", boost::bind(&CNatPortMapping::NatThreadFunc, this))
{
    fThreadRun = false;

    pCoreProtocol = nullptr;
    pBlockChain = nullptr;
    pNetwork = nullptr;

    nPrevUpnpMapTime = GetTime() - LEASE_DURATION + 10;
    nCfgNatExtPort = 0;
    nCfgListenPort = 0;
    fCfgNatModifyGateway = false;
}

CNatPortMapping::~CNatPortMapping()
{
}

bool CNatPortMapping::HandleInitialize()
{
    if (!GetObject("coreprotocol", pCoreProtocol))
    {
        Error("Failed to request coreprotocol");
        return false;
    }
    if (!GetObject("blockchain", pBlockChain))
    {
        Error("Failed to request blockchain");
        return false;
    }
    if (!GetObject("peernet", pNetwork))
    {
        Error("Failed to request network");
        return false;
    }

    strCfgNat = NetworkConfig()->strNat;
    if (!strCfgNat.empty())
    {
        std::transform(strCfgNat.begin(), strCfgNat.end(), strCfgNat.begin(),
                       [](unsigned char c) { return std::tolower(c); });
    }
    strCfgNatLocalIp = NetworkConfig()->strNatLocalIp;
    strCfgNatExtIp = NetworkConfig()->strNatExtIp;
    nCfgNatExtPort = NetworkConfig()->nNatExtPort;
    nCfgListenPort = NetworkConfig()->nPort;
    fCfgNatModifyGateway = NetworkConfig()->fNatModifyGateway;
    if ((strCfgNat == "any" || strCfgNat == "upnp" || strCfgNat == "pmp") && strCfgNatLocalIp.empty())
    {
        if (!GetLocalIp(strLocalIpaddress))
        {
            StdLog("CNatPortMapping", "Get local ip address failed");
            strLocalIpaddress = "";
            strCfgNat = "";
        }
    }
    StdLog("CNatPortMapping", "Handle initialize: nat: %s, nat local ip: %s, nat ext ip: %s, nat ext port: %d, listen port: %d, local ip: %s",
           strCfgNat.c_str(), strCfgNatLocalIp.c_str(), strCfgNatExtIp.c_str(), nCfgNatExtPort, nCfgListenPort, strLocalIpaddress.c_str());
    return true;
}

void CNatPortMapping::HandleDeinitialize()
{
    pCoreProtocol = nullptr;
    pBlockChain = nullptr;
    pNetwork = nullptr;

    fThreadRun = false;
}

bool CNatPortMapping::HandleInvoke()
{
    if (strCfgNat == "any" || strCfgNat == "upnp" || strCfgNat == "pmp")
    {
        fThreadRun = true;
        if (!ThreadDelayStart(thrNatWork))
        {
            StdLog("CNatPortMapping", "Handle invoke: Start thread failed");
            return false;
        }
    }
    return true;
}

void CNatPortMapping::HandleHalt()
{
    if (fThreadRun)
    {
        fThreadRun = false;
        condExit.notify_all();

        thrNatWork.Interrupt();
        ThreadExit(thrNatWork);
    }
}

void CNatPortMapping::NatThreadFunc()
{
    while (WaitExitEvent(1))
    {
        NatPortMapWork();
    }
}

bool CNatPortMapping::WaitExitEvent(const int64 nSeconds)
{
    if (nSeconds > 0 && fThreadRun)
    {
        try
        {
            boost::system_time const timeout = boost::get_system_time() + boost::posix_time::seconds(nSeconds);
            boost::unique_lock<boost::mutex> lock(mutex);
            condExit.timed_wait(lock, timeout);
        }
        catch (const boost::thread_interrupted&)
        {
            StdLog("CNatPortMapping", "Wait exit event thread_interrupted error");
            return fThreadRun;
        }
        catch (const boost::thread_resource_error&)
        {
            StdLog("CNatPortMapping", "Wait exit event thread_resource_error error");
            return fThreadRun;
        }
        catch (...)
        {
            StdLog("CNatPortMapping", "Wait exit event other error");
            return fThreadRun;
        }
    }
    return fThreadRun;
}

void CNatPortMapping::NatPortMapWork()
{
    int64 nCurrTime = GetTime();
    if (nCurrTime - nPrevUpnpMapTime >= LEASE_DURATION)
    {
        nPrevUpnpMapTime = nCurrTime;

        uint16 nExtPort = 0;
        if (nCfgNatExtPort != 0)
        {
            nExtPort = nCfgNatExtPort;
        }
        else
        {
            nExtPort = nCfgListenPort;
        }

        std::string strLocalIp;
        if (!strCfgNatLocalIp.empty())
        {
            strLocalIp = strCfgNatLocalIp;
        }
        else
        {
            if (!GetNetLocalIp(strLocalIp))
            {
                strLocalIp = strLocalIpaddress;
            }
        }

        bool fUpnp = true;
        if (strCfgNat == "any" || strCfgNat == "upnp")
        {
            if (!UpnpPortMapping(strLocalIp, nExtPort, nCfgListenPort))
            {
                StdLog("CNatPortMapping", "Nat port map work: Upnp port mapping failed, local ip: %s, ext port: %d, listen port: %d", strLocalIp.c_str(), nExtPort, nCfgListenPort);
                if (strCfgNat == "any")
                {
                    if (!PmpPortMapping(strLocalIp, nExtPort, nCfgListenPort, &fThreadRun))
                    {
                        StdLog("CNatPortMapping", "Nat port map work: Pmp port mapping failed, local ip: %s, ext port: %d, listen port: %d", strLocalIp.c_str(), nExtPort, nCfgListenPort);
                        return;
                    }
                    fUpnp = false;
                }
                else
                {
                    return;
                }
            }
            StdLog("CNatPortMapping", "Nat port map work: Upnp port mapping success, local ip: %s, ext port: %d, listen port: %d", strLocalIp.c_str(), nExtPort, nCfgListenPort);
        }
        else if (strCfgNat == "pmp")
        {
            if (!PmpPortMapping(strLocalIp, nExtPort, nCfgListenPort, &fThreadRun))
            {
                StdLog("CNatPortMapping", "Nat port map work: Pmp port mapping failed, local ip: %s, ext port: %d, listen port: %d", strLocalIp.c_str(), nExtPort, nCfgListenPort);
                return;
            }
            fUpnp = false;
        }

        if (fCfgNatModifyGateway)
        {
            if (!fThreadRun)
            {
                return;
            }

            std::string strExtIp;
            if (!strCfgNatExtIp.empty())
            {
                strExtIp = strCfgNatExtIp;
            }
            else
            {
                if (fUpnp)
                {
                    if (!GetUpnpExtIpaddr(strExtIp))
                    {
                        StdLog("CNatPortMapping", "Nat port map work: Get upnp ext ip address failed");
                        return;
                    }
                }
                else
                {
                    if (!GetPmpPublicAddress(strExtIp, &fThreadRun))
                    {
                        StdLog("CNatPortMapping", "Nat port map work: Get pmp ext ip address failed");
                        return;
                    }
                }
            }
            if (!SetGatewayAddress(strExtIp, nExtPort))
            {
                StdLog("CNatPortMapping", "Nat port map work: Set gateway address failed, ext ip: %s, ext port: %d", strExtIp.c_str(), nExtPort);
            }
            else
            {
                StdLog("CNatPortMapping", "Nat port map work: Set gateway address success, ext ip: %s, ext port: %d", strExtIp.c_str(), nExtPort);
            }
        }
    }
}
} // namespace hashahead
