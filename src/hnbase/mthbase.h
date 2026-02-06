// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef __MTHBASE_H
#define __MTHBASE_H

#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <queue>

#include "type.h"
#include "util.h"

namespace hnbase
{

inline unsigned long GetCurThreadId()
{
    std::string threadId = boost::lexical_cast<std::string>(boost::this_thread::get_id());
    unsigned long threadNumber = 0;
    threadNumber = std::stoul(threadId, nullptr, 16);
    return threadNumber;
}

class CBaseUniqueId : public boost::noncopyable
{
public:
    CBaseUniqueId(const uint64 ui64Id)
      : ui64UniqueId(ui64Id) {}
    CBaseUniqueId(CBaseUniqueId& uid)
      : ui64UniqueId(uid.ui64UniqueId) {}

    uint8 GetPortType() const
    {
        return ((PBASE_UNIQUE_ID)&ui64UniqueId)->ucPortType;
    }
    uint8 GetDirection() const
    {
        return ((PBASE_UNIQUE_ID)&ui64UniqueId)->ucDirection;
    }
    uint8 GetType() const
    {
        return ((PBASE_UNIQUE_ID)&ui64UniqueId)->ucType;
    }
    uint8 GetSubType() const
    {
        return ((PBASE_UNIQUE_ID)&ui64UniqueId)->ucSubType;
    }
    uint32 GetId() const
    {
        return ((PBASE_UNIQUE_ID)&ui64UniqueId)->uiId;
    }
    uint64 GetUniqueId() const
    {
        return ui64UniqueId;
    }

    CBaseUniqueId& operator=(const CBaseUniqueId& uid)
    {
        ui64UniqueId = uid.ui64UniqueId;
        return *this;
    }

    static uint64 CreateUniqueId(const uint8 ucPortType, const uint8 ucDirection, const uint8 ucType, const uint8 ucSubType = 0);

private:
    static uint32 uiIdCreate;
    static boost::mutex lockCreate;

#pragma pack(1)
    typedef struct _BASE_UNIQUE_ID
    {
        uint32 uiId;
        uint8 ucPortType : 1;  //0: tcp, 1: udp
        uint8 ucDirection : 1; //0: in, 1: out
        uint8 ucReserve : 6;
        uint8 ucType;
        uint8 ucSubType;
        uint8 ucRand;
    } BASE_UNIQUE_ID, *PBASE_UNIQUE_ID;
#pragma pack()

    uint64 ui64UniqueId;
};

class CMthWait;

class CMthEvent
{
public:
    CMthEvent(const bool bManualResetIn = false, const bool bInitSigStateIn = false)
      : fManualReset(bManualResetIn), fSingleFlag(bInitSigStateIn)
    {
        ui64EventId = CBaseUniqueId::CreateUniqueId(0, 0, 0xEF);
    }
    ~CMthEvent()
    {
        mapWait.clear();
    }

    uint64 GetEventId() const
    {
        return ui64EventId;
    }

    void SetEvent();
    void ResetEvent();

    bool AddWait(CMthWait* pWait);
    void DelWait(CMthWait* pWait);

    bool Wait(const uint32 ui32Timeout); /* ui32Timeout is milliseconds */

    bool QuerySetWaitObj(void* pWaitObj);
    void ClearWaitObj(void* pWaitObj);
    void ClearAllWaitObj();

private:
    boost::mutex lockEvent;
    boost::condition_variable_any condEvent;

    bool fSingleFlag;
    bool fManualReset;

    uint64 ui64EventId;

    std::map<uint64, CMthWait*> mapWait;
};

} // namespace hnbase

#endif // __HSM_MTHBASE_H
