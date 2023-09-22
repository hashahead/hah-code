// Copyright (c) 2021-2023 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HNBASE_DOCKER_THREAD_H
#define HNBASE_DOCKER_THREAD_H

#include <boost/function.hpp>
#include <boost/thread/thread.hpp>
#include <string>

namespace hnbase
{

class CDock;

class CThread
{
    friend class CDocker;

public:
    typedef boost::function<void()> ThreadFunc;
    CThread(const std::string& strNameIn, ThreadFunc fnCallbackIn)
      : strThreadName(strNameIn), pThread(nullptr), fnCallback(fnCallbackIn), fRunning(false)
    {
    }

    virtual ~CThread()
    {
        delete pThread;
    }

    bool IsRunning()
    {
        return fRunning;
    }

    void Interrupt()
    {
        if (pThread)
        {
            pThread->interrupt();
        }
    }

    void Exit()
    {
        if (pThread)
        {
            pThread->join();
            delete pThread;
            pThread = nullptr;
        }
    }

protected:
    const std::string strThreadName;
    boost::thread* pThread;
    ThreadFunc fnCallback;
    bool fRunning;
};

} // namespace hnbase

#endif //HNBASE_DOCKER_THREAD_H
