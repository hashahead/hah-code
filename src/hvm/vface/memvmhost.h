// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HVM_MEMVMHOST_H
#define HVM_MEMVMHOST_H

#include "vmhostface.h"

namespace hashahead
{
namespace hvm
{

class CMemAddress
{
public:
    CMemAddress() {}

public:
    uint256 nBalance;
    std::map<uint256, bytes> mapKv;
};

class CMemVmHost : public CVmHostFaceDB
{
public:
    CMemVmHost(const CDestination& destStorageContractIn = CDestination(), const CDestination& destCodeParentIn = CDestination(),
               const CDestination& destCodeLocalIn = CDestination(), const CDestination& destCodeOwnerIn = CDestination())
      : destStorageContract(destStorageContractIn), destCodeParent(destCodeParentIn), destCodeLocal(destCodeLocalIn), destCodeOwner(destCodeOwnerIn) {}

    virtual bool Get(const uint256& key, bytes& value) const;
    virtual bool GetTransientValue(const CDestination& dest, const uint256& key, bytes& value) const;
    virtual void SetTransientValue(const CDestination& dest, const uint256& key, const bytes& value);
    virtual uint256 GetTxid() const;
    virtual uint256 GetTxNonce() const;
    virtual uint64 GetAddressLastTxNonce(const CDestination& addr);
    virtual bool SetAddressLastTxNonce(const CDestination& addr, const uint64 nNonce);
    virtual CDestination GetStorageContractAddress() const;
    virtual CDestination GetCodeParentAddress() const;
    virtual CDestination GetCodeLocalAddress() const;
    virtual CDestination GetCodeOwnerAddress() const;
    virtual bool GetBalance(const CDestination& addr, uint256& balance) const;
    virtual bool ContractTransfer(const CDestination& from, const CDestination& to, const uint256& amount, const uint64 nGasLimit, uint64& nGasLeft);
    virtual uint256 GetBlockHash(const uint64 nBlockNumber);
    virtual bool IsContractAddress(const CDestination& addr);

    virtual bool GetContractRunCode(const CDestination& destContractIn, uint256& hashContractCreateCode, CDestination& destCodeOwner, uint256& hashContractRunCode, bytes& btContractRunCode, bool& fDestroy);
    virtual bool GetContractCreateCode(const CDestination& destContractIn, CTxContractData& txcd);
    virtual CVmHostFaceDBPtr CloneHostDB(const CDestination& destStorageContractIn, const CDestination& destCodeParentIn, const CDestination& destCodeLocalIn, const CDestination& destCodeOwnerIn);
    virtual void ModifyHostAddress(const CDestination& destCodeParentIn, const CDestination& destCodeLocalIn, const CDestination& destCodeOwnerIn);
    virtual void SaveCodeOwnerGasUsed(const CDestination& destParentCodeContractIn, const CDestination& destCodeContractIn, const CDestination& destCodeOwnerIn, const uint64 nGasUsed);
    virtual void SaveRunResult(const std::vector<CTransactionLogs>& vLogsIn, const std::map<uint256, bytes>& mapCacheKv,
                               const std::map<uint256, bytes>& mapTraceKv, const std::map<CDestination, std::map<uint256, bytes>>& mapOldAddressKeyValue);
    virtual bool SaveContractRunCode(const bytes& btContractRunCode, const CTxContractData& txcd);
    virtual bool ExecFunctionContract(const CDestination& destFromIn, const CDestination& destToIn, const uint256& nAmount, const bytes& btData, const uint64 nGasLimit, uint64& nGasLeft, int& nStatus, bytes& btResult);
    virtual bool Selfdestruct(const CDestination& destBeneficiaryIn);
    virtual bool AddContractRunReceipt(const CTxContractReceipt& tcReceipt, const bool fFirstReceipt = false);
    virtual bool AddVmOperationTraceLog(const CVmOperationTraceLog& vmOpTraceLog);

protected:
    CDestination destStorageContract; // storage contract address
    CDestination destCodeParent;      // code parent contract address
    CDestination destCodeLocal;       // code local contract address
    CDestination destCodeOwner;       // code owner address
    std::map<uint256, bytes> mapKv;
    std::map<CDestination, CMemAddress> mapState;
};

} // namespace hvm
} // namespace hashahead

#endif // HVM_VMHOSEFACE_H
