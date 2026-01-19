// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "memvmhost.h"

namespace hashahead
{
namespace hvm
{

bool CMemVmHost::Get(const uint256& key, bytes& value) const
{
    auto it = mapKv.find(key);
    if (it == mapKv.end())
    {
        return false;
    }
    value = it->second;
    return true;
}

bool CMemVmHost::GetTransientValue(const CDestination& dest, const uint256& key, bytes& value) const
{
    return true;
}

void CMemVmHost::SetTransientValue(const CDestination& dest, const uint256& key, const bytes& value)
{
}

uint256 CMemVmHost::GetTxid() const
{
    return uint256(1);
}

uint256 CMemVmHost::GetTxNonce() const
{
    return uint256(1);
}

uint64 CMemVmHost::GetAddressLastTxNonce(const CDestination& addr)
{
    return 0;
}

bool CMemVmHost::SetAddressLastTxNonce(const CDestination& addr, const uint64 nNonce)
{
    return true;
}

CDestination CMemVmHost::GetStorageContractAddress() const
{
    return destStorageContract;
}

CDestination CMemVmHost::GetCodeParentAddress() const
{
    return destCodeParent;
}

CDestination CMemVmHost::GetCodeLocalAddress() const
{
    return destCodeLocal;
}

CDestination CMemVmHost::GetCodeOwnerAddress() const
{
    return destCodeOwner;
}

bool CMemVmHost::GetBalance(const CDestination& addr, uint256& balance) const
{
    auto it = mapState.find(addr);
    if (it != mapState.end())
    {
        balance = it->second.nBalance;
        return true;
    }
    return false;
}

bool CMemVmHost::ContractTransfer(const CDestination& from, const CDestination& to, const uint256& amount, const uint64 nGasLimit, uint64& nGasLeft)
{
    return false;
}

uint256 CMemVmHost::GetBlockHash(const uint64 nBlockNumber)
{
    return uint256();
}

bool CMemVmHost::IsContractAddress(const CDestination& addr)
{
    return false;
}

bool CMemVmHost::GetContractRunCode(const CDestination& destContractIn, uint256& hashContractCreateCode, CDestination& destCodeOwner, uint256& hashContractRunCode, bytes& btContractRunCode, bool& fDestroy)
{
    return false;
}

bool CMemVmHost::GetContractCreateCode(const CDestination& destContractIn, CTxContractData& txcd)
{
    return false;
}

CVmHostFaceDBPtr CMemVmHost::CloneHostDB(const CDestination& destStorageContractIn, const CDestination& destCodeParentIn, const CDestination& destCodeLocalIn, const CDestination& destCodeOwnerIn)
{
    return CVmHostFaceDBPtr(new CMemVmHost(destStorageContractIn, destCodeParentIn, destCodeLocalIn, destCodeOwnerIn));
}

void CMemVmHost::ModifyHostAddress(const CDestination& destCodeParentIn, const CDestination& destCodeLocalIn, const CDestination& destCodeOwnerIn)
{
    destCodeParent = destCodeParentIn;
    destCodeLocal = destCodeLocalIn;
    destCodeOwner = destCodeOwnerIn;
}

void CMemVmHost::SaveCodeOwnerGasUsed(const CDestination& destParentCodeContractIn, const CDestination& destCodeContractIn, const CDestination& destCodeOwnerIn, const uint64 nGasUsed)
{
}

void CMemVmHost::SaveRunResult(const std::vector<CTransactionLogs>& vLogsIn, const std::map<uint256, bytes>& mapCacheKv,
                               const std::map<uint256, bytes>& mapTraceKv, const std::map<CDestination, std::map<uint256, bytes>>& mapOldAddressKeyValue)
{
}

bool CMemVmHost::AddContractRunReceipt(const CTxContractReceipt& tcReceipt, const bool fFirstReceipt)
{
    return true;
}

bool CMemVmHost::AddVmOperationTraceLog(const CVmOperationTraceLog& vmOpTraceLog)
{
    return true;
}

bool CMemVmHost::SaveContractRunCode(const bytes& btContractRunCode, const CTxContractData& txcd)
{
    return true;
}

bool CMemVmHost::ExecFunctionContract(const CDestination& destFromIn, const CDestination& destToIn, const uint256& nAmount, const bytes& btData, const uint64 nGasLimit, uint64& nGasLeft, int& nStatus, bytes& btResult)
{
    return true;
}

bool CMemVmHost::Selfdestruct(const CDestination& destBeneficiaryIn)
{
    return true;
}

} // namespace hvm
} // namespace hashahead
