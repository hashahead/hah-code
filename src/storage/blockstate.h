// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_BLOCKSTATE_H
#define STORAGE_BLOCKSTATE_H

#include <boost/range/adaptor/reversed.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <list>
#include <map>
#include <numeric>

#include "block.h"
#include "cmstruct.h"
#include "forkcontext.h"
#include "hnbase.h"
#include "param.h"

namespace hashahead
{
namespace storage
{

//////////////////////////////
// CFuncInputParam & CFuncOutputParam

class CBlockBase;

class CFuncInputParam
{
public:
    CFuncInputParam()
      : nGasLimit(0) {}

public:
    CDestination destFrom;
    CDestination destTo;
    uint256 nAmount;
    bytes btTxParam;
    uint64 nGasLimit;
};

class CFuncOutputParam
{
public:
    CFuncOutputParam()
      : nGasLeft(0), nStatus(0) {}

    static std::string GetStatusInfo(const int nStatus);

public:
    uint64 nGasLeft;
    CTransactionLogs logs;
    bytes btResult;

    int nStatus;
    std::string strError;
    std::string strRevertReason;
};

enum bs_evmc_status_code
{
    BS_EVMC_SUCCESS = 0,
    BS_EVMC_FAILURE = 1,
    BS_EVMC_REVERT = 2,
    BS_EVMC_OUT_OF_GAS = 3,
    BS_EVMC_INVALID_INSTRUCTION = 4,
    BS_EVMC_UNDEFINED_INSTRUCTION = 5,
    BS_EVMC_STACK_OVERFLOW = 6,
    BS_EVMC_STACK_UNDERFLOW = 7,
    BS_EVMC_BAD_JUMP_DESTINATION = 8,
    BS_EVMC_INVALID_MEMORY_ACCESS = 9,
    BS_EVMC_CALL_DEPTH_EXCEEDED = 10,
    BS_EVMC_STATIC_MODE_VIOLATION = 11,
    BS_EVMC_PRECOMPILE_FAILURE = 12,
    BS_EVMC_CONTRACT_VALIDATION_FAILURE = 13,
    BS_EVMC_ARGUMENT_OUT_OF_RANGE = 14,
    BS_EVMC_WASM_UNREACHABLE_INSTRUCTION = 15,
    BS_EVMC_WASM_TRAP = 16,
    BS_EVMC_INTERNAL_ERROR = -1,
    BS_EVMC_REJECTED = -2,
    BS_EVMC_OUT_OF_MEMORY = -3
};

//////////////////////////////
// CBlockState

class CBlockState
{
    typedef bool (CBlockState::*FuncContractCall)(const CFuncInputParam& funcInputParam, CFuncOutputParam& funcOutParam);

public:
    CBlockState(CBlockBase& dbBlockBaseIn, const uint256& hashForkIn, const CForkContext& ctxForkIn, const CBlock& block, const uint256& hashPrevStateRootIn, const uint32 nPrevBlockTimeIn,
                const std::map<CDestination, CAddressContext>& mapAddressContext, const bool fBtTraceDbIn);

    CBlockState(CBlockBase& dbBlockBaseIn, const uint256& hashForkIn, const CForkContext& ctxForkIn, const uint256& hashPrevBlockIn, const uint256& hashPrevStateRootIn, const uint32 nPrevBlockTimeIn,
                const uint64 nOriBlockGasLimitIn, const uint64 nBlockTimestampIn, const int nBlockHeightIn, const uint64 nBlockNumberIn, const bool fPrimaryBlockIn, const bool fBtTraceDbIn);

    bool GetDestState(const CDestination& dest, CDestState& stateDest);
    void SetDestState(const CDestination& dest, const CDestState& stateDest);
    void SetCacheDestState(const CDestination& dest, const CDestState& stateDest);
    bool GetDestKvData(const CDestination& dest, const uint256& key, bytes& value);
    bool GetAddressContext(const CDestination& dest, CAddressContext& ctxAddress);
    bool IsContractAddress(const CDestination& addr);
    bool GetContractRunCode(const CDestination& destContractIn, uint256& hashContractCreateCode, CDestination& destCodeOwner, uint256& hashContractRunCode, bytes& btContractRunCode, bool& fDestroy);
    bool GetContractCreateCode(const CDestination& destContractIn, CTxContractData& txcd);
    bool GetBlockHashByNumber(const uint64 nBlockNumberIn, uint256& hashBlockOut);
    void GetBlockBloomData(bytes& btBloomDataOut);
    bool GetDestBalance(const CDestination& dest, uint256& nBalance);
    uint64 GetAddressLastTxNonce(const CDestination& addr);
    bool SetAddressLastTxNonce(const CDestination& addr, const uint64 nNonce);
    bool GetTransientValue(const CDestination& dest, const uint256& key, bytes& value) const;
    void SetTransientValue(const CDestination& dest, const uint256& key, const bytes& value);
    bool IsTimeVaultWhitelistAddressExist(const CDestination& address);
    void AddCacheContractPrevState(const CDestination& address, const std::map<uint256, bytes>& mapContractKv);
    const VmOperationTraceLogs& GetCacheVmOpTraceLogs() const;
    const TxContractReceipts& GetCacheContractReceipts() const;
    const MapContractPrevState& GetCacheContractPrevAddressState() const;

    bool SaveContractRunCode(const CDestination& destContractIn, const bytes& btContractRunCode, const CTxContractData& txcd, const uint256& txidCreate);
    bool ExecFunctionContract(const CDestination& destFromIn, const CDestination& destToIn, const uint256& nAmount, const bytes& btData, const uint64 nGasLimit, uint64& nGasLeft, int& nStatus, bytes& btResult);
    bool Selfdestruct(const CDestination& destContractIn, const CDestination& destBeneficiaryIn);
    void SaveCodeOwnerGasUsed(const CDestination& destParentCodeContractIn, const CDestination& destCodeContractIn, const CDestination& destCodeOwner, const uint64 nGasUsed);
    void SaveRunResult(const CDestination& destContractIn, const std::vector<CTransactionLogs>& vLogsIn, const std::map<uint256, bytes>& mapCacheKv,
                       const std::map<uint256, bytes>& mapTraceKv, const std::map<CDestination, std::map<uint256, bytes>>& mapOldAddressKeyValue);
    bool ContractTransfer(const CDestination& from, const CDestination& to, const uint256& amount, const uint64 nGasLimit, uint64& nGasLeft, const CAddressContext& ctxToAddress, const uint8 nTransferType);
    bool IsContractDestroy(const CDestination& destContractIn);
    bool AddContractRunReceipt(const CTxContractReceipt& tcReceipt, const bool fFirstReceipt = false);
    bool AddVmOperationTraceLog(const CVmOperationTraceLog& vmOpTraceLog);

protected:
    bool GetDestLockedAmount(const CDestination& dest, uint256& nLockedAmount);
    CBlockBase& dbBlockBase;
    const uint16 nBlockType;
    const CChainId nLocalChainId;
    const uint256 hashFork;
    const CForkContext ctxFork;
    const uint256 hashPrevBlock;
    const uint256 hashPrevStateRoot;
    const uint32 nPrevBlockTime;
    const uint64 nOriBlockGasLimit;
    const uint64 nBlockTimestamp;
    const int nBlockHeight;
    const uint64 nBlockNumber;
    const CDestination destMint;
    const CTransaction mintTx;
    const uint256 txidMint;
    const bool fPrimaryBlock;
    const bool fBtTraceDb;
    const std::map<CChainId, CBlockProve> mapBlockProve; // key: peer chainid

    uint256 nOriginalBlockMintReward;
    uint256 hashRefBlock;
    uint256 nAgreement;

    class CCacheContractData
    {
    public:
        CCacheContractData() {}

    public:
        CDestState cacheDestState;
        std::vector<CTransactionLogs> cacheContractLogs;
        std::map<uint256, bytes> cacheContractKv;
        std::map<uint256, bytes> traceContractKv;
    };
    class CCodeGasData
    {
    public:
        CCodeGasData()
          : nGasUsed(0), nSubGasUsed(0) {}
        CCodeGasData(const CDestination& destOwner, const uint64 nGas, const uint64 nSubGas)
          : destCodeOwner(destOwner), nGasUsed(nGas), nSubGasUsed(nSubGas) {}

    public:
        CDestination destCodeOwner;
        uint64 nGasUsed;
        uint64 nSubGasUsed;
    };
    class CCacheTxData
    {
    public:
        CCacheTxData() {}
        void Clear()
        {
            destCacheTxFromAddress.SetNull();
            nCacheTxFromTvGasFee = 0;
            nCacheTxFromTvGas = 0;
        }

    public:
        CDestination destCacheTxFromAddress;
        uint256 nCacheTxFromTvGasFee;
        uint256 nCacheTxFromTvGas;
    };
    std::map<CDestination, CCacheContractData> mapCacheContractData;
    std::map<CDestination, CAddressContext> mapCacheAddressContext;
    std::map<uint256, CContractCreateCodeContext> mapCacheContractCreateCodeContext;
    std::map<uint256, CContractRunCodeContext> mapCacheContractRunCodeContext;
    std::vector<CContractTransfer> vCacheContractTransfer;
    std::map<CDestination, uint64> mapCacheOwnerDestCodeGasUsed;
    std::map<CDestination, CCodeGasData> mapCacheContractDestCodeGasUsed;
    std::map<CDestination, std::pair<uint32, uint32>> mapCacheModifyPledgeFinalHeight;
    std::map<uint32, CFunctionAddressContext> mapCacheFunctionAddress;
    std::map<std::string, CCoinContext> mapCacheSymbolCoin;
    std::map<CDexOrderHeader, CDexOrderBody> mapCacheDexOrder;
    std::map<CChainId, CBlockCrosschainProve> mapCacheCrossProve; // key is peer chainid
    std::map<CDestination, std::map<uint256, bytes>> mapCacheTransientStorage;
    std::set<CDestination> setCacheTimeVaultWhitelist;
    std::set<uint256> setCacheStopFork;
    TxContractReceipts vCacheContractReceipt;
    MapContractPrevState mapCacheContractPrevAddressState;
    VmOperationTraceLogs vCacheVmOpTraceLogs;
    CCacheTxData cacheTxData;

    uint64 nSurplusBlockGasLimit;

    std::map<CDestination, uint256> mapBlockRewardLocked;

public:
    std::map<CDestination, CDestState> mapBlockState;
    std::map<CDestination, uint256> mapContractKvPrevRoot;
    std::map<CDestination, std::map<uint256, bytes>> mapContractKvState;
    std::map<CDestination, std::map<uint256, bytes>> mapBlockTraceContractKvData;
    std::map<CDestination, CAddressContext> mapBlockAddressContext;
    std::map<CDestination, uint256> mapBlockPayTvFee;
    std::map<uint256, CContractCreateCodeContext> mapBlockContractCreateCodeContext;
    std::map<uint256, CContractRunCodeContext> mapBlockContractRunCodeContext;
    std::map<uint256, std::vector<CContractTransfer>> mapBlockContractTransfer; // key is txid
    std::map<uint256, uint256> mapBlockTxFeeUsed;
    std::map<uint256, std::map<CDestination, uint256>> mapBlockCodeDestFeeUsed; // key is txid
    std::map<CDestination, std::pair<uint32, uint32>> mapBlockModifyPledgeFinalHeight;
    std::vector<uint256> vReceiptHash;
    std::vector<CTransactionReceipt> vBlockTxReceipts;
    std::map<uint256, std::size_t> mapBlockTxReceipts; // key: txid, value: vBlockTxReceipts index
    CTransactionReceipt receiptMintTx;
    std::map<uint32, CFunctionAddressContext> mapBlockFunctionAddress;
    std::map<std::string, CCoinContext> mapBlockSymbolCoin;
    std::map<CDexOrderHeader, CDexOrderBody> mapBlockDexOrder;
    std::set<CDestination> setBlockTimeVaultWhitelist;
    std::set<uint256> setBlockStopFork;
    CBlockStorageProve proveBlockCrosschain;
    std::map<uint256, uint256> mapCoinPairCompletePrice; // key: coin pair hash, value: complete price
    std::map<CDexOrderHeader, std::vector<CCompDexOrderRecord>> mapCompDexOrderRecord;
    BlockContractReceipts vBlockContractReceipt;                                     // first is txid
    BlockContractPrevState vBlockContractPrevAddressState;                           // first is txid
    std::vector<std::pair<uint256, VmOperationTraceLogs>> vBlockVmOpTraceLogs;       // first is txid
    std::map<CDestination, std::vector<CTokenTransRecord>> mapBlockTokenTransRecord; // key: contract address
    uint256 nBlockFeeLeft;
    std::set<bytes> setBlockBloomData;
};

typedef std::shared_ptr<CBlockState> SHP_BLOCK_STATE;
#define MAKE_SHARED_BLOCK_STATE std::make_shared<CBlockState>

} // namespace storage
} // namespace hashahead

#endif //STORAGE_BLOCKSTATE_H
