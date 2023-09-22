// Copyright (c) 2021-2023 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HASHAHEAD_MODE_MODE_TYPE_H
#define HASHAHEAD_MODE_MODE_TYPE_H

namespace hashahead
{
// mode type
enum class EModeType
{
    MODE_ERROR = 0, // ERROR type
    MODE_SERVER,    // server
    MODE_CONSOLE,   // console
    MODE_MINER,     // miner
};

} // namespace hashahead

#endif // HASHAHEAD_MODE_MODE_TYPE_H
