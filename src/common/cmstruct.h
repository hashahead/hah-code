// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COMMON_CMSTRUCT_H
#define COMMON_CMSTRUCT_H

#include <set>
#include <stream/stream.h>
#include <vector>

#include "block.h"
#include "crypto.h"
#include "destination.h"
#include "param.h"
#include "transaction.h"
#include "uint256.h"

namespace hashahead
{

//////////////////////////////
// CVmCallTx
class CVmCallTx
{
public:
    CVmCallTx()
      : fEthCall(false), nTxNonce(0), fNeedVerifyToAddress(true) {}

public:
    bool fEthCall;
    CDestination destFrom;
    CDestination destTo;
    uint64 nTxNonce;
    uint256 nGasPrice;
    uint256 nGasLimit;
    uint256 nAmount;
    bytes btData;
    bool fNeedVerifyToAddress;
};

// CBlockLogsFilter

class CBlockLogsFilter
{
public:
    CBlockLogsFilter() {}

    bool isTimeout();
};

} // namespace hashahead

#endif // COMMON_CMSTRUCT_H
