// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HNBASE_NETIO_NETIO_H
#define HNBASE_NETIO_NETIO_H

#include "event/eventproc.h"
namespace hnbase
{

class IIOProc : public IBase
{
public:
    IIOProc(const std::string& ownKeyIn)
      : IBase(ownKeyIn) {}
    virtual bool DispatchEvent(CEvent* pEvent) = 0;
};

class IIOModule : public CEventProc
{
public:
    IIOModule(const std::string& ownKeyIn, const uint32 nThreadCount = 1)
      : CEventProc(ownKeyIn, nThreadCount) {}
};

} // namespace hnbase

#endif //HNBASE_NETIO_NETIO_H
