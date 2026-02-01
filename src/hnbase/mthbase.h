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

private:
    static uint32 uiIdCreate;
    static boost::mutex lockCreate;

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

private:
    boost::mutex lockEvent;
    boost::condition_variable_any condEvent;
};

} // namespace hnbase

#endif // __HSM_MTHBASE_H
