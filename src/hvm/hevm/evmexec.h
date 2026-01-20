// Copyright (c) 2021-2025 The HashAhead developers
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
    CEvmExec(CVmHostFaceDB& dbHostIn, const uint256& hashForkIn, const CChainId& chainIdIn, const uint256& txidIn, const uint256& nAgreementIn)
      : dbHost(dbHostIn), hashFork(hashForkIn), chainId(chainIdIn), txid(txidIn), nAgreement(nAgreementIn), nGasLeft(0), nUsedGas(0), nStatusCode(0) {}

protected:
    CVmHostFaceDB& dbHost;
    uint256 hashFork;
    CChainId chainId;
    uint256 txid;
    uint256 nAgreement;

public:
    uint64 nGasLeft;
    uint64 nUsedGas;
    int nStatusCode;
    bytes vResult;
};

} // namespace hvm
} // namespace hashahead

#endif // HVM_EVMEXEC_H
