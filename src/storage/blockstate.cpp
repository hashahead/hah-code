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

} // namespace storage
} // namespace hashahead
