// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HVM_VMHOSTFACE_H
#define HVM_VMHOSTFACE_H

#include "destination.h"
#include "hnbase.h"
#include "transaction.h"
#include "uint256.h"

namespace hashahead
{
namespace hvm
{

class CVmHostFaceDB;
typedef boost::shared_ptr<CVmHostFaceDB> CVmHostFaceDBPtr;

class CVmHostFaceDB
{
public:
    virtual ~CVmHostFaceDB(){};

    virtual bool Get(const uint256& key, bytes& value) const = 0;
    virtual bool GetTransientValue(const CDestination& dest, const uint256& key, bytes& value) const = 0;
    virtual void SetTransientValue(const CDestination& dest, const uint256& key, const bytes& value) = 0;
    virtual uint256 GetTxid() const = 0;
    virtual uint256 GetTxNonce() const = 0;
    virtual uint64 GetAddressLastTxNonce(const CDestination& addr) = 0;
    virtual bool SetAddressLastTxNonce(const CDestination& addr, const uint64 nNonce) = 0;
    virtual CDestination GetStorageContractAddress() const = 0;
    virtual CDestination GetCodeParentAddress() const = 0;
    virtual CDestination GetCodeLocalAddress() const = 0;
    virtual CDestination GetCodeOwnerAddress() const = 0;
    virtual bool GetBalance(const CDestination& addr, uint256& balance) const = 0;
    virtual bool ContractTransfer(const CDestination& from, const CDestination& to, const uint256& amount, const uint64 nGasLimit, uint64& nGasLeft) = 0;
    virtual uint256 GetBlockHash(const uint64 nBlockNumber) = 0;
    virtual bool IsContractAddress(const CDestination& addr) = 0;

    virtual bool GetContractRunCode(const CDestination& destContractIn, uint256& hashContractCreateCode, CDestination& destCodeOwner, uint256& hashContractRunCode, bytes& btContractRunCode, bool& fDestroy) = 0;
    virtual bool GetContractCreateCode(const CDestination& destContractIn, CTxContractData& txcd) = 0;
    virtual CVmHostFaceDBPtr CloneHostDB(const CDestination& destStorageContractIn, const CDestination& destCodeParentIn, const CDestination& destCodeLocalIn, const CDestination& destCodeOwnerIn) = 0;
    virtual void ModifyHostAddress(const CDestination& destCodeParentIn, const CDestination& destCodeLocalIn, const CDestination& destCodeOwnerIn) = 0;
    virtual void SaveCodeOwnerGasUsed(const CDestination& destParentCodeContractIn, const CDestination& destCodeContractIn, const CDestination& destCodeOwner, const uint64 nGasUsed) = 0;
    virtual void SaveRunResult(const std::vector<CTransactionLogs>& vLogsIn, const std::map<uint256, bytes>& mapCacheKv,
                               const std::map<uint256, bytes>& mapTraceKv, const std::map<CDestination, std::map<uint256, bytes>>& mapOldAddressKeyValue)
        = 0;
    virtual bool SaveContractRunCode(const bytes& btContractRunCode, const CTxContractData& txcd) = 0;
    virtual bool ExecFunctionContract(const CDestination& destFromIn, const CDestination& destToIn, const uint256& nAmount, const bytes& btData, const uint64 nGasLimit, uint64& nGasLeft, int& nStatus, bytes& btResult) = 0;
    virtual bool Selfdestruct(const CDestination& destBeneficiaryIn) = 0;
    virtual bool AddContractRunReceipt(const CTxContractReceipt& tcReceipt, const bool fFirstReceipt = false) = 0;
    virtual bool AddVmOperationTraceLog(const CVmOperationTraceLog& vmOpTraceLog) = 0;

protected:
    CVmHostFaceDB() = default;
};

} // namespace hvm
} // namespace hashahead

#endif // HVM_VMHOSTFACE_H
