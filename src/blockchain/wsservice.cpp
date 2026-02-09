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

/////////////////////////////
// CClientSubscribe

void CClientSubscribe::matchesLogs(CTransactionReceipt const& _m, MatchLogsVec& vLogs) const
{
    for (unsigned i = 0; i < _m.vLogs.size(); ++i)
    {
        auto& logs = _m.vLogs[i];
        if (!setSubsAddress.empty() && setSubsAddress.count(logs.address) == 0)
        {
            continue;
        }
        if (!setSubsTopics.empty())
        {
            bool m = false;
            for (auto& h : logs.topics)
            {
                if (setSubsTopics.count(h) > 0)
                {
                    m = true;
                    break;
                }
            }
            if (!m)
            {
                continue;
            }
        }
        vLogs.push_back(CMatchLogs(i, logs));
    }
}

/////////////////////////////
// CWsSubscribeFork

uint128 CWsSubscribeFork::AddSubscribe(const uint64 nClientConnId, const uint8 nSubsType, const std::set<CDestination>& setSubsAddress, const std::set<uint256>& setSubsTopics)
{
    auto& mapTypeSubs = mapClientSubscribe[nSubsType];

    auto it = mapClientConnect.find(nClientConnId);
    if (it != mapClientConnect.end())
    {
        auto mt = it->second.find(nSubsType);
        if (mt != it->second.end())
        {
            const uint128& nSubsId = mt->second;
            mapTypeSubs[nSubsId] = CClientSubscribe(nClientConnId, nSubsType, setSubsAddress, setSubsTopics);
            return nSubsId;
        }
    }

    uint128 nSubsId;
    do
    {
        nSubsId = CreateSubsId(nSubsType, nClientConnId);
    } while (mapTypeSubs.find(nSubsId) != mapTypeSubs.end());

    mapTypeSubs.insert(std::make_pair(nSubsId, CClientSubscribe(nClientConnId, nSubsType, setSubsAddress, setSubsTopics)));
    mapClientConnect[nClientConnId][nSubsType] = nSubsId;
    return nSubsId;
}

void CWsSubscribeFork::RemoveClientAllSubscribe(const uint64 nClientConnId)
{
    auto it = mapClientConnect.find(nClientConnId);
    if (it != mapClientConnect.end())
    {
        for (const auto& kv : it->second)
        {
            const uint8 nSubsType = kv.first;
            const uint128& nSubsId = kv.second;

            auto mt = mapClientSubscribe.find(nSubsType);
            if (mt != mapClientSubscribe.end())
            {
                mt->second.erase(nSubsId);
            }
        }
        mapClientConnect.erase(it);
    }
}

void CWsSubscribeFork::RemoveSubscribe(const uint128& nSubsId)
{
    uint8 nSubsType = GetSubsIdType(nSubsId);
    auto it = mapClientSubscribe.find(nSubsType);
    if (it != mapClientSubscribe.end())
    {
        auto nt = it->second.find(nSubsId);
        if (nt != it->second.end())
        {
            auto mt = mapClientConnect.find(nt->second.nClientConnId);
            if (mt != mapClientConnect.end())
            {
                mt->second.erase(nSubsType);
                if (mt->second.empty())
                {
                    mapClientConnect.erase(mt);
                }
            }
            it->second.erase(nt);
        }
    }
}

const std::map<uint128, CClientSubscribe>& CWsSubscribeFork::GetSubsListByType(const uint8 nSubsType)
{
    return mapClientSubscribe[nSubsType];
}

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
