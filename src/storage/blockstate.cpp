// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blockstate.h"

#include <boost/timer/timer.hpp>
#include <cstdio>

#include "blockbase.h"
#include "bloomfilter/bloomfilter.h"
#include "delegatecomm.h"
#include "hevm/evmexec.h"
#include "structure/merkletree.h"
#include "template/delegate.h"
#include "template/fork.h"
#include "template/pledge.h"
#include "template/template.h"
#include "template/vote.h"
#include "util.h"

using namespace std;
using namespace boost::filesystem;
using namespace hnbase;
using namespace hashahead::hvm;

namespace hashahead
{
namespace storage
{

//////////////////////////////
// CFuncOutputParam

std::string CFuncOutputParam::GetStatusInfo(const int nStatus)
{
    switch (nStatus)
    {
    case BS_EVMC_SUCCESS:
        return "Success";
    case BS_EVMC_FAILURE:
        return "Generic execution failure";
    case BS_EVMC_REVERT:
        return "Execution terminated with REVERT opcode";
    case BS_EVMC_OUT_OF_GAS:
        return "The execution has run out of gas";
    case BS_EVMC_INVALID_INSTRUCTION:
        return "The designated INVALID instruction has been hit during execution";
    case BS_EVMC_UNDEFINED_INSTRUCTION:
        return "An undefined instruction has been encountered";
    case BS_EVMC_STACK_OVERFLOW:
        return "The execution has attempted to put more items on the EVM stack than the specified limit";
    case BS_EVMC_STACK_UNDERFLOW:
        return "Execution of an opcode has required more items on the EVM stack";
    case BS_EVMC_BAD_JUMP_DESTINATION:
        return "Execution has violated the jump destination restrictions";
    case BS_EVMC_INVALID_MEMORY_ACCESS:
        return "Tried to read outside memory bounds";
    case BS_EVMC_CALL_DEPTH_EXCEEDED:
        return "Call depth has exceeded the limit (if any)";
    case BS_EVMC_STATIC_MODE_VIOLATION:
        return "Tried to execute an operation which is restricted in static mode";
    case BS_EVMC_PRECOMPILE_FAILURE:
        return "A call to a precompiled or system contract has ended with a failure";
    case BS_EVMC_CONTRACT_VALIDATION_FAILURE:
        return "Contract validation has failed (e.g. due to EVM 1.5 jump validity";
    case BS_EVMC_ARGUMENT_OUT_OF_RANGE:
        return "An argument to a state accessing method has a value outside of the accepted range of values";
    case BS_EVMC_WASM_UNREACHABLE_INSTRUCTION:
        return "A WebAssembly unreachable instruction has been hit during execution";
    case BS_EVMC_WASM_TRAP:
        return "A WebAssembly trap has been hit during execution. This can be for many reasons, including division by zero, validation errors";
    case BS_EVMC_INTERNAL_ERROR:
        return "EVM implementation generic internal error";
    case BS_EVMC_REJECTED:
        return "The execution of the given code and/or message has been rejected by the EVM implementation";
    case BS_EVMC_OUT_OF_MEMORY:
        return "The VM failed to allocate the amount of memory needed for execution";
    default:
        return "Other error";
    }
    return "";
}

//////////////////////////////
// CBlockState

CBlockState::CBlockState(CBlockBase& dbBlockBaseIn, const uint256& hashForkIn, const CForkContext& ctxForkIn, const CBlock& block, const uint256& hashPrevStateRootIn, const uint32 nPrevBlockTimeIn, const std::map<CDestination, CAddressContext>& mapAddressContext, const bool fBtTraceDbIn)
  : dbBlockBase(dbBlockBaseIn), nBlockType(block.nType), nLocalChainId(CBlock::GetBlockChainIdByHash(hashForkIn)), hashFork(hashForkIn), ctxFork(ctxForkIn), hashPrevBlock(block.hashPrev),
    hashPrevStateRoot(hashPrevStateRootIn), nPrevBlockTime(nPrevBlockTimeIn), nOriBlockGasLimit(block.nGasLimit.Get64()), nSurplusBlockGasLimit(block.nGasLimit.Get64()),
    nBlockTimestamp(block.GetBlockTime()), nBlockHeight(block.GetBlockHeight()), nBlockNumber(block.GetBlockNumber()), destMint(block.txMint.GetToAddress()),
    mintTx(block.txMint), txidMint(block.txMint.GetHash()), fPrimaryBlock(block.IsPrimary()), mapBlockAddressContext(mapAddressContext), mapBlockProve(block.mapProve), fBtTraceDb(fBtTraceDbIn)
{
    hashRefBlock = 0;
    if (fPrimaryBlock)
    {
        if (block.IsProofOfPoa())
        {
            CProofOfPoa proof;
            if (block.GetPoaProof(proof))
            {
                nAgreement = proof.nAgreement;
            }
        }
        else
        {
            CProofOfDelegate proof;
            if (block.GetDelegateProof(proof))
            {
                nAgreement = proof.nAgreement;
            }
        }
    }
    else
    {
        CProofOfPiggyback proof;
        if (block.GetPiggybackProof(proof))
        {
            hashRefBlock = proof.hashRefBlock;
            nAgreement = proof.nAgreement;
        }
    }
    for (auto& tx : block.vtx)
    {
        setBlockBloomData.insert(tx.GetHash().GetBytes());
    }
    uint256 nMintCoint;
    if (!block.GetMintCoinProof(nMintCoint))
    {
        nMintCoint = 0;
    }
    uint256 nTotalTxFee;
    for (auto& tx : block.vtx)
    {
        nTotalTxFee += tx.GetTxFee();
    }
    nOriginalBlockMintReward = nMintCoint + nTotalTxFee;
}

CBlockState::CBlockState(CBlockBase& dbBlockBaseIn, const uint256& hashForkIn, const CForkContext& ctxForkIn, const uint256& hashPrevBlockIn, const uint256& hashPrevStateRootIn, const uint32 nPrevBlockTimeIn,
                         const uint64 nOriBlockGasLimitIn, const uint64 nBlockTimestampIn, const int nBlockHeightIn, const uint64 nBlockNumberIn, const bool fPrimaryBlockIn, const bool fBtTraceDbIn)
  : dbBlockBase(dbBlockBaseIn), nBlockType(0), nLocalChainId(CBlock::GetBlockChainIdByHash(hashForkIn)), hashFork(hashForkIn), ctxFork(ctxForkIn), hashPrevBlock(hashPrevBlockIn), hashPrevStateRoot(hashPrevStateRootIn), nPrevBlockTime(nPrevBlockTimeIn),
    nOriBlockGasLimit(nOriBlockGasLimitIn), nSurplusBlockGasLimit(nOriBlockGasLimitIn), nBlockTimestamp(nBlockTimestampIn), nBlockHeight(nBlockHeightIn), nBlockNumber(nBlockNumberIn), fPrimaryBlock(fPrimaryBlockIn), fBtTraceDb(fBtTraceDbIn)
{
}

bool CBlockState::GetDestState(const CDestination& dest, CDestState& stateDest)
{
    auto mt = mapCacheContractData.find(dest);
    if (mt != mapCacheContractData.end() && !mt->second.cacheDestState.IsNull())
    {
        stateDest = mt->second.cacheDestState;
        return true;
    }
    auto it = mapBlockState.find(dest);
    if (it != mapBlockState.end())
    {
        stateDest = it->second;
        return true;
    }
    return dbBlockBase.RetrieveDestState(hashFork, hashPrevStateRoot, dest, stateDest);
}

void CBlockState::SetDestState(const CDestination& dest, const CDestState& stateDest)
{
    mapBlockState[dest] = stateDest;
}

void CBlockState::SetCacheDestState(const CDestination& dest, const CDestState& stateDest)
{
    mapCacheContractData[dest].cacheDestState = stateDest;
}

bool CBlockState::GetDestKvData(const CDestination& dest, const uint256& key, bytes& value)
{
    auto nt = mapCacheContractData.find(dest);
    if (nt != mapCacheContractData.end())
    {
        auto mt = nt->second.cacheContractKv.find(key);
        if (mt != nt->second.cacheContractKv.end())
        {
            value = mt->second;
            return true;
        }
    }
    auto it = mapContractKvState.find(dest);
    if (it != mapContractKvState.end())
    {
        auto mt = it->second.find(key);
        if (mt != it->second.end())
        {
            value = mt->second;
            return true;
        }
    }
    CDestState stateDest;
    if (!GetDestState(dest, stateDest))
    {
        return false;
    }
    return dbBlockBase.RetrieveContractKvValue(hashFork, stateDest.GetStorageRoot(), key, value);
}

bool CBlockState::GetAddressContext(const CDestination& dest, CAddressContext& ctxAddress)
{
    auto it = mapCacheAddressContext.find(dest);
    if (it != mapCacheAddressContext.end())
    {
        ctxAddress = it->second;
        return true;
    }
    auto nt = mapBlockAddressContext.find(dest);
    if (nt != mapBlockAddressContext.end())
    {
        ctxAddress = nt->second;
        return true;
    }
    return dbBlockBase.RetrieveAddressContext(hashFork, hashPrevBlock, dest, ctxAddress);
}

bool CBlockState::IsContractAddress(const CDestination& addr)
{
    CAddressContext ctxAddress;
    if (!GetAddressContext(addr, ctxAddress))
    {
        return false;
    }
    return ctxAddress.IsContract();
}

bool CBlockState::GetContractRunCode(const CDestination& destContractIn, uint256& hashContractCreateCode, CDestination& destCodeOwner, uint256& hashContractRunCode, bytes& btContractRunCode, bool& fDestroy)
{
    CDestState stateContract;
    if (!GetDestState(destContractIn, stateContract))
    {
        StdLog("CBlockState", "Get contract run code: Get contract state failed, contract address: %s", destContractIn.ToString().c_str());
        return false;
    }
    if (stateContract.IsDestroy())
    {
        StdLog("CBlockState", "Get contract run code: Contract has been destroyed, contract address: %s", destContractIn.ToString().c_str());
        fDestroy = true;
        return true;
    }
    fDestroy = false;

    bytes btDestCodeData;
    if (!GetDestKvData(destContractIn, destContractIn.ToHash(), btDestCodeData))
    {
        StdLog("CBlockState", "Get contract run code: Get contract code fail, contract address: %s", destContractIn.ToString().c_str());
        return false;
    }

    CContractDestCodeContext ctxDestCode;
    try
    {
        CBufStream ss(btDestCodeData);
        ss >> ctxDestCode;
    }
    catch (std::exception& e)
    {
        StdLog("CBlockState", "Get contract run code: Parse contract code fail, contract address: %s", destContractIn.ToString().c_str());
        return false;
    }

    CContractRunCodeContext ctxRunCode;
    bool fCacheContext = false;
    if (VERIFY_FHX_HEIGHT_BRANCH_001(CBlock::GetBlockHeightByHash(hashPrevBlock)))
    {
        auto it = mapCacheContractRunCodeContext.find(ctxDestCode.hashContractRunCode);
        if (it != mapCacheContractRunCodeContext.end())
        {
            ctxRunCode = it->second;
            fCacheContext = true;
        }
    }

    hashContractCreateCode = ctxDestCode.hashContractCreateCode;
    destCodeOwner = ctxDestCode.destCodeOwner;
    hashContractRunCode = ctxDestCode.hashContractRunCode;
    btContractRunCode = ctxRunCode.btContractRunCode;
    return true;
}

bool CBlockState::GetContractCreateCode(const CDestination& destContractIn, CTxContractData& txcd)
{
    bytes btDestCodeData;
    if (!GetDestKvData(destContractIn, destContractIn.ToHash(), btDestCodeData))
    {
        StdLog("CBlockState", "Get contract create code: Get contract code fail, contract address: %s", destContractIn.ToString().c_str());
        return false;
    }

    CContractDestCodeContext ctxDestCode;
    try
    {
        CBufStream ss(btDestCodeData);
        ss >> ctxDestCode;
    }
    catch (std::exception& e)
    {
        StdLog("CBlockState", "Get contract create code: Parse contract code fail, contract address: %s", destContractIn.ToString().c_str());
        return false;
    }

    if (VERIFY_FHX_HEIGHT_BRANCH_001(CBlock::GetBlockHeightByHash(hashPrevBlock)))
    {
        CContractCreateCodeContext ctxtCode;
        auto it = mapCacheContractCreateCodeContext.find(ctxDestCode.hashContractCreateCode);
        if (it != mapCacheContractCreateCodeContext.end())
        {
            txcd.nMuxType = 1;
            txcd.strType = ctxtCode.strType;
            txcd.strName = ctxtCode.strName;
            txcd.destCodeOwner = ctxtCode.destCodeOwner;
            txcd.nCompressDescribe = 0;
            txcd.btDescribe.assign(ctxtCode.strDescribe.begin(), ctxtCode.strDescribe.end());
            txcd.nCompressCode = 0;
            txcd.btCode = ctxtCode.btCreateCode; // contract create code
            txcd.nCompressSource = 0;
            txcd.btSource.clear(); // source code
            return true;
        }
    }

    if (!dbBlockBase.GetBlockContractCreateCodeData(hashFork, hashPrevBlock, ctxDestCode.hashContractCreateCode, txcd))
    {
        StdLog("CBlockState", "Get contract create code: Get contract create code fail, hashContractCreateCode: %s, contract address: %s, prev block: %s",
               ctxDestCode.hashContractCreateCode.GetHex().c_str(), destContractIn.ToString().c_str(), hashPrevBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CBlockState::GetBlockHashByNumber(const uint64 nBlockNumberIn, uint256& hashBlockOut)
{
    uint256 hashLastBlock;
    if (VERIFY_FHX_HEIGHT_BRANCH_003(CBlock::GetBlockHeightByHash(hashPrevBlock)))
    {
        hashLastBlock = hashPrevBlock;
    }
    return dbBlockBase.GetBlockIndexHashByNumberNoLock(hashFork, hashLastBlock, nBlockNumberIn, hashBlockOut);
}

void CBlockState::GetBlockBloomData(bytes& btBloomDataOut)
{
    if (!setBlockBloomData.empty())
    {
        CNhBloomFilter bf(setBlockBloomData.size() * 4);
        for (auto& bt : setBlockBloomData)
        {
            bf.Add(bt);
        }
        bf.GetData(btBloomDataOut);
    }
}

bool CBlockState::GetDestBalance(const CDestination& dest, uint256& nBalance)
{
    CDestState stateDest;
    if (!GetDestState(dest, stateDest))
    {
        StdLog("CBlockState", "Get dest balance: Get dest state failed, dest: %s", dest.ToString().c_str());
        return false;
    }
    uint256 nLockedAmount;
    if (!GetDestLockedAmount(dest, nLockedAmount))
    {
        StdLog("CBlockState", "Get dest balance: Get locked amount failed, dest: %s", dest.ToString().c_str());
        return false;
    }
    if (stateDest.GetBalance() > nLockedAmount)
    {
        nBalance = stateDest.GetBalance() - nLockedAmount;
    }
    else
    {
        nBalance = 0;
    }
    return true;
}

uint64 CBlockState::GetAddressLastTxNonce(const CDestination& addr)
{
    CDestState state;
    if (!GetDestState(addr, state))
    {
        return 0;
    }
    return state.GetTxNonce();
}

bool CBlockState::SetAddressLastTxNonce(const CDestination& addr, const uint64 nNonce)
{
    CDestState state;
    if (!GetDestState(addr, state))
    {
        return false;
    }
    state.SetTxNonce(nNonce);
    SetCacheDestState(addr, state);
    return true;
}

bool CBlockState::GetTransientValue(const CDestination& dest, const uint256& key, bytes& value) const
{
    auto it = mapCacheTransientStorage.find(dest);
    if (it != mapCacheTransientStorage.end())
    {
        auto mt = it->second.find(key);
        if (mt != it->second.end())
        {
            value = mt->second;
            return true;
        }
    }
    return false;
}

void CBlockState::SetTransientValue(const CDestination& dest, const uint256& key, const bytes& value)
{
    mapCacheTransientStorage[dest][key] = value;
}

bool CBlockState::IsTimeVaultWhitelistAddressExist(const CDestination& address)
{
    if (setBlockTimeVaultWhitelist.count(address) > 0)
    {
        return true;
    }
    if (setCacheTimeVaultWhitelist.count(address) > 0)
    {
        return true;
    }

    uint256 hashPrimaryRefBlock;
    if (fPrimaryBlock)
    {
        hashPrimaryRefBlock = hashPrevBlock;
    }
    else
    {
        hashPrimaryRefBlock = hashRefBlock;
    }
    if (dbBlockBase.IsTimeVaultWhitelistAddressExist(address, hashPrimaryRefBlock))
    {
        return true;
    }
    return false;
}

void CBlockState::AddCacheContractPrevState(const CDestination& address, const std::map<uint256, bytes>& mapContractKv)
{
    auto it = mapCacheContractPrevAddressState.find(address);
    if (it == mapCacheContractPrevAddressState.end())
    {
        CDestState stateDest;
        bytes btContractRunCode;
        if (GetDestState(address, stateDest) && stateDest.IsContract())
        {
            uint256 hashContractCreateCode;
            CDestination destCodeOwner;
            uint256 hashContractRunCode;
            bool fDestroy;
            if (!GetContractRunCode(address, hashContractCreateCode, destCodeOwner, hashContractRunCode, btContractRunCode, fDestroy))
            {
                btContractRunCode.clear();
            }
        }
        it = mapCacheContractPrevAddressState.insert(std::make_pair(address, CContractPrevState(stateDest.GetBalance(), stateDest.GetTxNonce() + 1, btContractRunCode))).first;
    }
    CContractPrevState& prevState = it->second;
    for (const auto& kv : mapContractKv)
    {
        if (prevState.mapStorage.count(kv.first) == 0)
        {
            prevState.mapStorage[kv.first] = kv.second;
        }
    }
}

const VmOperationTraceLogs& CBlockState::GetCacheVmOpTraceLogs() const
{
    return vCacheVmOpTraceLogs;
}

const TxContractReceipts& CBlockState::GetCacheContractReceipts() const
{
    return vCacheContractReceipt;
}

const MapContractPrevState& CBlockState::GetCacheContractPrevAddressState() const
{
    return mapCacheContractPrevAddressState;
}

bool CBlockState::SaveContractRunCode(const CDestination& destContractIn, const bytes& btContractRunCode, const CTxContractData& txcd, const uint256& txidCreate)
{
    uint256 hashContractCreateCode = txcd.GetContractCreateCodeHash();
    uint256 hashContractRunCode = hashahead::crypto::CryptoKeccakHash(btContractRunCode.data(), btContractRunCode.size());

    CContractDestCodeContext ctxDestCode(hashContractCreateCode, hashContractRunCode, txcd.GetCodeOwner());
    bytes btDestCodeData;
    CBufStream ss;
    ss << ctxDestCode;
    ss.GetData(btDestCodeData);

    CDestState stateContractDest;
    if (GetDestState(destContractIn, stateContractDest) && stateContractDest.GetCodeHash() != 0)
    {
        StdLog("CBlockState", "Save contract run code: contract address already exists, contract address: %s", destContractIn.ToString().c_str());
        return false;
    }
    stateContractDest.SetType(CDestination::PREFIX_CONTRACT);
    stateContractDest.SetCodeHash(hashContractRunCode);

    auto& cachContract = mapCacheContractData[destContractIn];
    cachContract.cacheContractKv[destContractIn.ToHash()] = btDestCodeData;
    cachContract.cacheDestState = stateContractDest;

    mapCacheContractCreateCodeContext[hashContractCreateCode] = CContractCreateCodeContext(txcd.GetType(), txcd.GetName(), txcd.GetDescribe(), txcd.GetCodeOwner(), txcd.GetCode(),
                                                                                           txidCreate, txcd.GetSourceCodeHash(), hashContractRunCode);
    mapCacheContractRunCodeContext[hashContractRunCode] = CContractRunCodeContext(hashContractCreateCode, btContractRunCode);
    mapCacheAddressContext[destContractIn] = CAddressContext(CContractAddressContext(txcd.GetType(), txcd.GetCodeOwner(), txcd.GetName(), txcd.GetDescribe(), txidCreate, txcd.GetSourceCodeHash(), txcd.GetContractCreateCodeHash(), hashContractRunCode));
    return true;
}

bool CBlockState::ExecFunctionContract(const CDestination& destFromIn, const CDestination& destToIn, const uint256& nAmount, const bytes& btData, const uint64 nGasLimit, uint64& nGasLeft, int& nStatus, bytes& btResult)
{
    if (btData.size() < 4)
    {
        StdLog("CBlockState", "Exec function contract: data size error, data size: %lu", btData.size());
        return false;
    }
    bytes btTxParam;
    if (btData.size() > 4)
    {
        btTxParam.assign(btData.begin() + 4, btData.end());
    }
    bytes btFuncSign(btData.begin(), btData.begin() + 4);
    CTransactionLogs logs;
    nGasLeft = nGasLimit;
    return true;
}

bool CBlockState::Selfdestruct(const CDestination& destContractIn, const CDestination& destBeneficiaryIn)
{
    CDestState stateContract;
    CDestState stateBeneficiary;
    if (!GetDestState(destContractIn, stateContract))
    {
        StdLog("CBlockState", "Selfdestruct: Get contract state fail, contract address: %s", destContractIn.ToString().c_str());
        return false;
    }
    if (stateContract.IsDestroy())
    {
        return true;
    }
    if (!GetDestState(destBeneficiaryIn, stateBeneficiary))
    {
        stateBeneficiary.SetNull();
        stateBeneficiary.SetType(CDestination::PREFIX_PUBKEY); // WAIT_CHECK
    }

    stateBeneficiary.IncBalance(stateContract.GetBalance());
    stateContract.SetBalance(0);
    stateContract.SetDestroy(true);

    SetCacheDestState(destContractIn, stateContract);
    SetCacheDestState(destBeneficiaryIn, stateBeneficiary);
    return true;
}

void CBlockState::SaveCodeOwnerGasUsed(const CDestination& destParentCodeContractIn, const CDestination& destCodeContractIn, const CDestination& destCodeOwner, const uint64 nGasUsed)
{
    if (nGasUsed > 0)
    {
        if (VERIFY_FHX_HEIGHT_BRANCH_002(CBlock::GetBlockHeightByHash(hashPrevBlock)))
        {
            if (!destParentCodeContractIn.IsNull())
            {
                mapCacheContractDestCodeGasUsed[destParentCodeContractIn].nSubGasUsed += nGasUsed;
            }

            auto& gasCode = mapCacheContractDestCodeGasUsed[destCodeContractIn];
            if (gasCode.destCodeOwner.IsNull())
            {
                gasCode.destCodeOwner = destCodeOwner;
            }
            gasCode.nGasUsed += nGasUsed;
        }
        else
        {
            mapCacheOwnerDestCodeGasUsed[destCodeOwner] += nGasUsed;
        }
    }
}

void CBlockState::SaveRunResult(const CDestination& destContractIn, const std::vector<CTransactionLogs>& vLogsIn, const std::map<uint256, bytes>& mapCacheKv,
                                const std::map<uint256, bytes>& mapTraceKv, const std::map<CDestination, std::map<uint256, bytes>>& mapOldAddressKeyValue)
{
    auto& cacheContract = mapCacheContractData[destContractIn];

    // CDestState stateContractDest;
    // if (GetDestState(destContractIn, stateContractDest))
    // {
    //     cacheContract.cacheDestState = stateContractDest;
    // }

    for (auto& kv : mapCacheKv)
    {
        cacheContract.cacheContractKv[kv.first] = kv.second;
    }
    for (auto& logs : vLogsIn)
    {
        cacheContract.cacheContractLogs.push_back(logs);
    }

    if (fBtTraceDb)
    {
        for (auto& kv : mapTraceKv)
        {
            cacheContract.traceContractKv[kv.first] = kv.second;
        }
        for (const auto& kv : mapOldAddressKeyValue)
        {
            AddCacheContractPrevState(kv.first, kv.second);
        }
    }

    if (mapCacheAddressContext.find(destContractIn) == mapCacheAddressContext.end())
    {
        CAddressContext ctxAddress;
        if (!GetAddressContext(destContractIn, ctxAddress))
        {
            StdError("CBlockState", "Save run result: Get address context fail, destContractIn: %s", destContractIn.ToString().c_str());
        }
        else
        {
            mapCacheAddressContext[destContractIn] = ctxAddress;
        }
    }
}

bool CBlockState::ContractTransfer(const CDestination& from, const CDestination& to, const uint256& amount, const uint64 nGasLimit, uint64& nGasLeft, const CAddressContext& ctxToAddress, const uint8 nTransferType)
{
    if (ctxToAddress.IsTemplate())
    {
        if (ctxToAddress.GetTemplateType() == TEMPLATE_VOTE && !fEnableStakeVote)
        {
            return false;
        }
        if (ctxToAddress.GetTemplateType() == TEMPLATE_PLEDGE && !fEnableStakePledge)
        {
            return false;
        }
    }
    if (nGasLeft < FUNCTION_TX_GAS_TRANS)
    {
        StdLog("CBlockState", "Contract transfer: Gas not enough, gas left: %lu, from: %s", nGasLeft, from.ToString().c_str());
        nGasLeft = 0;
        return false;
    }
    nGasLeft -= FUNCTION_TX_GAS_TRANS;

    CDestState stateFrom;
    if (!GetDestState(from, stateFrom))
    {
        StdLog("CBlockState", "Contract transfer: Get dest state fail, from: %s", from.ToString().c_str());
        return false;
    }
    if (stateFrom.GetBalance() < amount)
    {
        StdLog("CBlockState", "Contract transfer: Balance not enough, balance: %s, amount: %s, from: %s",
               CoinToTokenBigFloat(stateFrom.GetBalance()).c_str(), CoinToTokenBigFloat(amount).c_str(), from.ToString().c_str());
        return false;
    }

    CDestState stateTo;
    if (!GetDestState(to, stateTo))
    {
        stateTo.SetNull();

        CAddressContext ctxTempAddress;
        if (!GetAddressContext(to, ctxTempAddress))
        {
            mapCacheAddressContext[to] = ctxToAddress;
            stateTo.SetType(ctxToAddress.GetDestType(), ctxToAddress.GetTemplateType()); // WAIT_CHECK
        }
        else
        {
            mapCacheAddressContext[to] = ctxTempAddress;
            stateTo.SetType(ctxTempAddress.GetDestType(), ctxTempAddress.GetTemplateType());
        }
    }
    else
    {
        CAddressContext ctxTempAddress;
        if (!GetAddressContext(to, ctxTempAddress))
        {
            if (ctxToAddress.GetDestType() == stateTo.GetDestType()
                && ctxToAddress.GetTemplateType() == stateTo.GetTemplateType())
            {
                mapCacheAddressContext[to] = ctxToAddress;
            }
            else
            {
                StdLog("CBlockState", "Contract transfer: State address context error, State address type: %d-%d, in address type: %d-%d, from: %s, to: %s",
                       stateTo.GetDestType(), stateTo.GetTemplateType(),
                       ctxToAddress.GetDestType(), ctxToAddress.GetTemplateType(),
                       from.ToString().c_str(), to.ToString().c_str());
                return false;
            }
        }
        else
        {
            mapCacheAddressContext[to] = ctxTempAddress;
        }
    }

    CAddressContext ctxTempFromAddress;
    if (!GetAddressContext(from, ctxTempFromAddress))
    {
        StdLog("CBlockState", "Contract transfer: Get from address context failed, from: %s", from.ToString().c_str());
        return false;
    }
    if (mapCacheAddressContext.count(from) == 0)
    {
        mapCacheAddressContext[from] = ctxTempFromAddress;
    }

    // verify reward lock
    uint256 nLockedAmount;
    if (!GetDestLockedAmount(from, nLockedAmount))
    {
        StdLog("CBlockState", "Contract transfer: Get reward locked amount failed, from: %s", from.ToString().c_str());
        return false;
    }
    if (nLockedAmount > 0 && stateFrom.GetBalance() < nLockedAmount + amount)
    {
        StdLog("CBlockState", "Contract transfer: Balance locked, balance: %s, lock amount: %s, amount: %s, from: %s",
               CoinToTokenBigFloat(stateFrom.GetBalance()).c_str(), CoinToTokenBigFloat(nLockedAmount).c_str(),
               CoinToTokenBigFloat(amount).c_str(), from.ToString().c_str());
        return false;
    }

    if (VERIFY_FHX_HEIGHT_BRANCH_002(nBlockHeight))
    {
        if (ctxToAddress.GetTemplateType() == TEMPLATE_PLEDGE
            && !dbBlockBase.VerifyAddressPledgeVote(to, hashPrevBlock))
        {
            StdLog("CBlockState", "Contract transfer: Voting has been banned, from: %s, to: %s",
                   from.ToString().c_str(), to.ToString().c_str());
            return false;
        }
    }

    stateFrom.DecBalance(amount);
    stateTo.IncBalance(amount);

    SetCacheDestState(from, stateFrom);
    SetCacheDestState(to, stateTo);

    vCacheContractTransfer.push_back(CContractTransfer(nTransferType, from, to, amount));
    return true;
}

bool CBlockState::IsContractDestroy(const CDestination& destContractIn)
{
    CDestState stateContract;
    if (!GetDestState(destContractIn, stateContract))
    {
        StdLog("CBlockState", "Is contract destroy: Get contract state failed, contract address: %s", destContractIn.ToString().c_str());
        return true;
    }
    if (stateContract.IsDestroy())
    {
        StdLog("CBlockState", "Is contract destroy: Contract has been destroyed, contract address: %s", destContractIn.ToString().c_str());
        return true;
    }
    return false;
}

bool CBlockState::AddContractRunReceipt(const CTxContractReceipt& tcReceipt, const bool fFirstReceipt)
{
    if (fBtTraceDb)
    {
        if (fFirstReceipt)
        {
            vCacheContractReceipt.insert(vCacheContractReceipt.begin(), tcReceipt);
        }
        else
        {
            vCacheContractReceipt.push_back(tcReceipt);
        }
    }
    return true;
}

bool CBlockState::AddVmOperationTraceLog(const CVmOperationTraceLog& vmOpTraceLog)
{
    vCacheVmOpTraceLogs.push_back(vmOpTraceLog);
    return true;
}

//////////////////////////////////////////
bool CBlockState::GetDestLockedAmount(const CDestination& dest, uint256& nLockedAmount)
{
    CAddressContext ctxAddress;
    if (!GetAddressContext(dest, ctxAddress))
    {
        StdLog("CBlockState", "Get loacked amount: Get address context fail, dest: %s", dest.ToString().c_str());
        return false;
    }
    CDestState stateDest;
    if (!GetDestState(dest, stateDest))
    {
        StdLog("CBlockState", "Get loacked amount: Get address state fail, dest: %s", dest.ToString().c_str());
        return false;
    }
    if (!dbBlockBase.GetAddressLockedAmount(hashFork, hashPrevBlock, dest, ctxAddress, stateDest.GetBalance(), nLockedAmount))
    {
        StdLog("CBlockState", "Get loacked amount: Get address locked amount fail, dest: %s", dest.ToString().c_str());
        return false;
    }

    auto nt = mapBlockRewardLocked.find(dest);
    if (nt != mapBlockRewardLocked.end())
    {
        nLockedAmount += nt->second;
    }
    return true;
}

bool CBlockState::VerifyFunctionAddressRepeat(const CDestination& destNewFunction)
{
    for (auto& kv : mapCacheFunctionAddress)
    {
        if (kv.second.GetFunctionAddress() == destNewFunction)
        {
            return false;
        }
    }
    for (auto& kv : mapBlockFunctionAddress)
    {
        if (kv.second.GetFunctionAddress() == destNewFunction)
        {
            return false;
        }
    }
    return true;
}

bool CBlockState::VerifyFunctionAddressDisable(const uint32 nFuncId)
{
    auto it = mapCacheFunctionAddress.find(nFuncId);
    if (it != mapCacheFunctionAddress.end())
    {
        if (it->second.IsDisableModify())
        {
            return false;
        }
    }
    else
    {
        auto mt = mapBlockFunctionAddress.find(nFuncId);
        if (mt != mapBlockFunctionAddress.end())
        {
            if (mt->second.IsDisableModify())
            {
                return false;
            }
        }
    }
    return true;
}

bool CBlockState::GetContractStringParam(const uint8* pParamBeginPos, const std::size_t nParamSize, const uint8* pCurrParamPos, const std::size_t nSurplusParamLen, std::string& strParamOut)
{
    if (nSurplusParamLen < 32)
    {
        StdLog("CBlockState", "Get contract string param: Param var pos error, surplus param len: %lu, param size: %lu", nSurplusParamLen, nParamSize);
        return false;
    }
    uint256 tempData;
    tempData.FromBigEndian(pCurrParamPos, 32);
    std::size_t nVarPos = tempData.Get64();

    if (nParamSize < nVarPos)
    {
        StdLog("CBlockState", "Get contract string param: Param var pos error, var pos: %lu, param size: %lu", nVarPos, nParamSize);
        return false;
    }
    const uint8* p = pParamBeginPos + nVarPos;
    std::size_t nSurplusSize = nParamSize - nVarPos;

    if (nSurplusSize < 32)
    {
        StdLog("CBlockState", "Get contract string param: Surplus size not enough1, surplus size: %lu", nSurplusSize);
        return false;
    }
    tempData.FromBigEndian(p, 32);
    uint64 nStringSize = tempData.Get64();
    p += 32;
    nSurplusSize -= 32;

    if (nStringSize == 0)
    {
        StdLog("CBlockState", "Get contract string param: String size is 0, surplus size: %lu", nSurplusSize);
        return false;
    }
    const std::size_t nSectByteCount = (nStringSize / 32 + ((nStringSize % 32) == 0 ? 0 : 1)) * 32;
    if (nSurplusSize < nSectByteCount)
    {
        StdLog("CBlockState", "Get contract string param: Surplus size not enough2, surplus size: %lu, sect size: %lu, string size: %lu", nSurplusSize, nSectByteCount, nStringSize);
        return false;
    }
    strParamOut.assign(p, p + nStringSize);
    return true;
}

uint64 CBlockState::GetMaxDexCoinOrderNumber(const CDestination& destFrom, const std::string& strCoinSymbolOwner, const std::string& strCoinSymbolPeer)
{
    uint256 hashCoinPair = CDexOrderHeader::GetCoinPairHashStatic(strCoinSymbolOwner, strCoinSymbolPeer);
    uint8 nOwnerCoinFlag = CDexOrderHeader::GetOwnerCoinFlagStatic(strCoinSymbolOwner, strCoinSymbolPeer);
    uint64 nMaxNumber = 0;
    for (auto& kv : mapCacheDexOrder)
    {
        if (kv.first.GetOrderAddress() == destFrom && kv.first.GetCoinPairHash() == hashCoinPair && kv.first.GetOwnerCoinFlag() == nOwnerCoinFlag)
        {
            if (kv.first.GetOrderNumber() > nMaxNumber)
            {
                nMaxNumber = kv.first.GetOrderNumber();
            }
        }
    }
    if (nMaxNumber == 0)
    {
        for (auto& kv : mapBlockDexOrder)
        {
            prevState.mapStorage[kv.first] = kv.second;
        }
    }
}

} // namespace storage
} // namespace hashahead
