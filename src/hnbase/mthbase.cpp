// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "mthbase.h"

#include "util.h"

using namespace std;

namespace hnbase
{

//----------------------------------------------------------------------------------------
uint32 CBaseUniqueId::uiIdCreate = 0;
boost::mutex CBaseUniqueId::lockCreate;

uint64 CBaseUniqueId::CreateUniqueId(const uint8 ucPortType, const uint8 ucDirection, const uint8 ucType, const uint8 ucSubType)
{
    boost::unique_lock<boost::mutex> writeLock(lockCreate);

    if (uiIdCreate == 0)
    {
        srand(time(NULL));
        uiIdCreate = rand();
        if (uiIdCreate == 0)
        {
            uiIdCreate = 1;
        }
    }

    uint64 ui64UniqueId = 0;
    ((PBASE_UNIQUE_ID)&ui64UniqueId)->uiId = uiIdCreate;
    ((PBASE_UNIQUE_ID)&ui64UniqueId)->ucPortType = ucPortType;
    ((PBASE_UNIQUE_ID)&ui64UniqueId)->ucDirection = ucDirection;
    ((PBASE_UNIQUE_ID)&ui64UniqueId)->ucType = ucType;
    ((PBASE_UNIQUE_ID)&ui64UniqueId)->ucSubType = ucSubType;
    ((PBASE_UNIQUE_ID)&ui64UniqueId)->ucRand = GetTimeMillis() & 0xFF;

    if (++uiIdCreate == 0)
    {
        uiIdCreate = 1;
    }

    return ui64UniqueId;
}

//----------------------------------------------------------------------------------------
void CMthEvent::SetEvent()
{
    boost::unique_lock<boost::mutex> lock(lockEvent);

    if (!fSingleFlag)
    {
        fSingleFlag = true;

        if (fManualReset)
        {
            condEvent.notify_all();
        }
        else
        {
            condEvent.notify_one();
        }

        CMthWait* pWait;
        std::map<uint64, CMthWait*>::iterator it;
        for (it = mapWait.begin(); it != mapWait.end(); it++)
        {
            pWait = it->second;
            if (pWait)
            {
                pWait->SetSignal(this);
            }
        }
    }
}

void CMthEvent::ResetEvent()
{
    boost::unique_lock<boost::mutex> lock(lockEvent);
    fSingleFlag = false;
}

bool CMthEvent::AddWait(CMthWait* pWait)
{
    boost::unique_lock<boost::mutex> lock(lockEvent);
    if (pWait == NULL)
    {
        return false;
    }

    if (mapWait.count(pWait->GetWaitId()) == 0)
    {
        if (!mapWait.insert(make_pair(pWait->GetWaitId(), pWait)).second)
        {
            return false;
        }
    }

    return true;
}

void CMthEvent::DelWait(CMthWait* pWait)
{
    boost::unique_lock<boost::mutex> lock(lockEvent);
    if (pWait == NULL)
    {
        return;
    }

    if (mapWait.count(pWait->GetWaitId()))
    {
        mapWait.erase(pWait->GetWaitId());
    }
}
} // namespace hnbase
