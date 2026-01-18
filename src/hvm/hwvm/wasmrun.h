// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HVM_WASMRUN_H
#define HVM_WASMRUN_H

#include <map>

#include "../vface/vmhostface.h"
#include "block.h"
#include "destination.h"
#include "hnbase.h"
#include "transaction.h"
#include "uint256.h"

namespace hashahead
{
namespace hvm
{

class CContractRun
{
public:
    CContractRun(CVmHostFaceDB& dbHostIn, const uint256& hashForkIn)
      : dbHost(dbHostIn), hashFork(hashForkIn) {}

    bool RunContract(const CDestination& from, const CDestination& to, const CDestination& destContractIn, const CDestination& destCodeOwner, const uint64 nTxGasLimit,
                     const uint256& nGasPrice, const uint256& nTxAmount, const CDestination& destBlockMint, const uint64 nBlockTimestamp,
                     const int nBlockHeight, const uint64 nBlockGasLimit, const bytes& btContractCode, const bytes& btRunParam, const CTxContractData& txcd);

protected:
    CVmHostFaceDB& dbHost;
    uint256 hashFork;

public:
    uint64 nGasLeft;
    int nStatusCode;
    bytes vResult;
    std::map<uint256, bytes> mapCacheKv;
};

} // namespace hvm
} // namespace hashahead

#endif // HVM_WASMRUN_H
