// Copyright (c) 2021-2023 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HNBASE_PEERNET_PEERINFO_H
#define HNBASE_PEERNET_PEERINFO_H

#include <string>

namespace hnbase
{

class CPeerInfo
{
public:
    virtual ~CPeerInfo() {}

public:
    std::string strAddress;
    bool fInBound;
    int64 nActive;
    int64 nLastRecv;
    int64 nLastSend;
    int nScore;
};

} // namespace hnbase

#endif //HNBASE_PEERNET_PEERINFO_H
