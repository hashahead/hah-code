// Copyright (c) 2021-2023 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HVM_EVMEXEC_H
#define HVM_EVMEXEC_H

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

//////////////////////////////
CDestination CreateContractAddressByNonce(const CDestination& _sender, const uint256& _nonce);

//////////////////////////////
// CEvmExec

class CEvmExec
{
public:
    CEvmExec(CVmHostFaceDB& dbHostIn, const uint256& hashForkIn, const CChainId& chainIdIn, const uint256& nAgreementIn)
      : dbHost(dbHostIn), hashFork(hashForkIn), chainId(chainIdIn), nAgreement(nAgreementIn) {}

    bool evmExec(const CDestination& from, const CDestination& to, const CDestination& destContractIn, const CDestination& destCodeOwner, const uint64 nTxGasLimit,
                 const uint256& nGasPrice, const uint256& nTxAmount, const CDestination& destBlockMint, const uint64 nBlockTimestamp,
                 const int nBlockHeight, const uint64 nBlockGasLimit, const bytes& btContractCode, const bytes& btRunParam, const CTxContractData& txcd);

protected:
    CVmHostFaceDB& dbHost;
    uint256 hashFork;
    CChainId chainId;
    uint256 nAgreement;

public:
    uint64 nGasLeft;
    int nStatusCode;
    bytes vResult;
    std::map<uint256, bytes> mapCacheKv;
};

} // namespace hvm
} // namespace hashahead

#endif // HVM_EVMEXEC_H
