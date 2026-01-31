// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_BLOCKBASE_H
#define STORAGE_BLOCKBASE_H

#include <boost/range/adaptor/reversed.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <list>
#include <map>
#include <numeric>

#include "../hvm/vface/vmhostface.h"
#include "block.h"
#include "blockdb.h"
#include "blockstate.h"
#include "cmstruct.h"
#include "dbstruct.h"
#include "forkcontext.h"
#include "hnbase.h"
#include "param.h"
#include "profile.h"
#include "timeseries.h"

using namespace hashahead::hvm;

namespace fs = boost::filesystem;

namespace hashahead
{
namespace storage
{

class CBlockHeightIndex
{
public:
    CBlockHeightIndex()
      : nTimeStamp(0) {}
    CBlockHeightIndex(const uint64 nTimeStampIn, const CDestination& destMintIn, const uint256& hashRefBlockIn)
      : nTimeStamp(nTimeStampIn), destMint(destMintIn), hashRefBlock(hashRefBlockIn) {}

public:
    uint64 nTimeStamp;
    CDestination destMint;
    uint256 hashRefBlock;
};

class CContractHostDB : public CVmHostFaceDB
{
public:
    CContractHostDB(CBlockState& blockStateIn, const CDestination& destStorageContractIn, const CDestination& destCodeParentIn, const CDestination& destCodeLocalIn,
                    const CDestination& destCodeOwnerIn, const uint256& txidIn, const uint64 nTxNonceIn)
      : blockState(blockStateIn), destStorageContract(destStorageContractIn), destCodeParent(destCodeParentIn), destCodeLocal(destCodeLocalIn),
        destCodeOwner(destCodeOwnerIn), txid(txidIn), nTxNonce(nTxNonceIn) {}

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
    CBlockState& blockState;
    CDestination destStorageContract; // storage contract address
    CDestination destCodeParent;      // code parent contract address
    CDestination destCodeLocal;       // code local contract address
    CDestination destCodeOwner;       // code owner address
    uint256 txid;
    uint64 nTxNonce;
};

//////////////////////////////////////////
// CCacheBlockReceipt

class CCacheBlockReceipt
{
public:
    CCacheBlockReceipt() {}

    void AddBlockReceiptCache(const uint256& hashBlock, const std::vector<CTransactionReceipt>& vBlockReceipt);
    bool GetBlockReceiptCache(const uint256& hashBlock, std::vector<CTransactionReceipt>& vBlockReceipt);

protected:
    enum
    {
        MAX_CACHE_BLOCK_RECEIPT_COUNT = 1000 * 1000
    };

    mutable hnbase::CRWAccess rwBrcAccess;
    std::map<CChainId, std::map<uint256, std::vector<CTransactionReceipt>, CustomBlockHashCompare>> mapBlockReceiptCache; // key: chainid, value: key: block hash, value: receipt
};

//////////////////////////////////////////
// CBlockBase

class CBlockBase
{
public:
    CBlockBase();
    ~CBlockBase();
    bool BsInitialize(const fs::path& pathDataLocation, const uint256& hashGenesisBlockIn, const bool fFullDbIn, const bool fTraceDbIn,
                      const bool fCacheTrace, const bool fRewardCheckIn, const bool fRenewDB, const bool fPruneDb);
    void BsDeinitialize();
    void Clear();
    bool IsEmpty();
    const uint256& GetGenesisBlockHash() const;
    bool Exists(const uint256& hash);
    bool ExistsTx(const uint256& hashFork, const uint256& txid);
    bool Initiate(const uint256& hashGenesis, const CBlock& blockGenesis, const uint256& nChainTrust);
    bool CheckForkLongChain(const uint256& hashFork, const uint256& hashBlock, const CBlockEx& block, const BlockIndexPtr pIndexNew);
    bool Retrieve(const BlockIndexPtr pIndex, CBlock& block);
    bool Retrieve(const uint256& hash, CBlockEx& block);
    bool Retrieve(const BlockIndexPtr pIndex, CBlockEx& block);
    BlockIndexPtr RetrieveIndex(const uint256& hashBlock);
    BlockIndexPtr RetrieveFork(const uint256& hashFork);
    BlockIndexPtr RetrieveFork(const std::string& strName);
    bool RetrieveProfile(const uint256& hashFork, CProfile& profile, const uint256& hashMainChainRefBlock);
    bool RetrieveAncestry(const uint256& hashFork, std::vector<std::pair<uint256, uint256>> vAncestry);
    bool RetrieveOrigin(const uint256& hashFork, CBlock& block);
    bool RetrieveTxAndIndex(const uint256& hashFork, const uint256& txid, CTransaction& tx, uint256& hashAtFork, CTxIndex& txIndex);
    bool RetrieveAvailDelegate(const uint256& hash, int height, const std::vector<uint256>& vBlockRange,
                               const uint256& nMinEnrollAmount,
                               std::map<CDestination, std::size_t>& mapWeight,
                               std::map<CDestination, std::vector<unsigned char>>& mapEnrollData,
                               std::vector<std::pair<CDestination, uint256>>& vecAmount);
    bool RetrieveDestDelegateVote(const uint256& hashBlock, const CDestination& destDelegate, uint256& nVoteAmount);
    void ListForkIndex(std::map<uint256, CBlockIndex*>& mapForkIndex);
    void ListForkIndex(std::multimap<int, CBlockIndex*>& mapForkIndex);
    bool GetBlockBranchList(const uint256& hashBlock, std::vector<CBlockEx>& vRemoveBlockInvertedOrder, std::vector<CBlockEx>& vAddBlockPositiveOrder);
    bool GetBlockBranchListNolock(const uint256& hashBlock, const uint256& hashForkLastBlock, const CBlockEx* pBlockex, std::vector<CBlockEx>& vRemoveBlockInvertedOrder, std::vector<CBlockEx>& vAddBlockPositiveOrder);
    bool LoadIndex(CBlockOutline& diskIndex);
    bool LoadTx(const uint32 nTxFile, const uint32 nTxOffset, CTransaction& tx);
    bool AddBlockForkContext(const uint256& hashBlock, const CBlockEx& blockex, const std::map<CDestination, CAddressContext>& mapAddressContext, uint256& hashNewRoot);
    bool VerifyBlockForkTx(const uint256& hashPrev, const CTransaction& tx, std::vector<std::pair<CDestination, CForkContext>>& vForkCtxt);
    bool VerifyOriginBlock(const CBlock& block, const CProfile& parentProfile);
    bool ListForkContext(std::map<uint256, CForkContext>& mapForkCtxt, const uint256& hashBlock = uint256());
    bool RetrieveForkContext(const uint256& hashFork, CForkContext& ctxt, const uint256& hashMainChainRefBlock = uint256());
    bool RetrieveForkLast(const uint256& hashFork, uint256& hashLastBlock);
    bool GetForkHashByForkName(const std::string& strForkName, uint256& hashFork, const uint256& hashMainChainRefBlock = uint256());
    bool GetForkHashByChainId(const CChainId nChainId, uint256& hashFork, const uint256& hashMainChainRefBlock = uint256());
    int GetForkCreatedHeight(const uint256& hashFork, const uint256& hashRefBlock);
    bool GetForkStorageMaxHeight(const uint256& hashFork, uint32& nMaxHeight);
    bool GetBlockHashByNumber(const uint256& hashFork, const uint64 nBlockNumber, uint256& hashBlock);

    bool GetForkBlockLocator(const uint256& hashFork, CBlockLocator& locator, uint256& hashDepth, int nIncStep);
    bool GetForkBlockInv(const uint256& hashFork, const CBlockLocator& locator, std::vector<uint256>& vBlockHash, size_t nMaxCount);

    bool GetDelegateVotes(const uint256& hashGenesis, const uint256& hashRefBlock, const CDestination& destDelegate, uint256& nVotes);
    bool RetrieveDestVoteContext(const uint256& hashBlock, const CDestination& destVote, CVoteContext& ctxtVote);
    bool ListPledgeFinalHeight(const uint256& hashBlock, const uint32 nFinalHeight, std::map<CDestination, std::pair<uint32, uint32>>& mapPledgeFinalHeight);
    bool WalkThroughDayVote(const uint256& hashBeginBlock, const uint256& hashTailBlock, CDayVoteWalker& walker);
    bool GetDelegateList(const uint256& hashRefBlock, const uint32 nStartIndex, const uint32 nCount, std::multimap<uint256, CDestination>& mapVotes);
    bool RetrieveAllDelegateVote(const uint256& hashBlock, std::map<CDestination, std::map<CDestination, CVoteContext>>& mapDelegateVote);
    bool GetDelegateMintRewardRatio(const uint256& hashBlock, const CDestination& destDelegate, uint32& nRewardRation);
    bool VerifyRepeatBlock(const uint256& hashFork, const uint256& hashBlock, const uint32 height, const CDestination& destMint, const uint16 nBlockType,
                           const uint64 nBlockTimeStamp, const uint64 nRefBlockTimeStamp, const uint32 nExtendedBlockSpacing);
    bool GetBlockDelegateVote(const uint256& hashBlock, std::map<CDestination, uint256>& mapVote);
    bool GetRangeDelegateEnroll(int height, const std::vector<uint256>& vBlockRange, std::map<CDestination, CDiskPos>& mapEnrollTxPos);
    bool VerifyRefBlock(const uint256& hashGenesis, const uint256& hashRefBlock);
    CBlockIndex* GetForkValidLast(const uint256& hashGenesis, const uint256& hashFork);
    bool VerifySameChain(const uint256& hashPrevBlock, const uint256& hashAfterBlock);
    bool GetLastRefBlockHash(const uint256& hashFork, const uint256& hashBlock, uint256& hashRefBlock, bool& fOrigin);
    bool GetPrimaryHeightBlockTime(const uint256& hashLastBlock, int nHeight, uint256& hashBlock, int64& nTime);
    bool VerifyPrimaryHeightRefBlockTime(const int nHeight, const int64 nTime);
    bool UpdateForkNext(const uint256& hashFork, CBlockIndex* pIndexLast, const std::vector<CBlockEx>& vBlockRemove, const std::vector<CBlockEx>& vBlockAddNew);
    bool RetrieveDestState(const uint256& hashFork, const uint256& hashBlockRoot, const CDestination& dest, CDestState& state);
    SHP_BLOCK_STATE CreateBlockStateRoot(const uint256& hashFork, const CBlock& block, const uint256& hashPrevStateRoot, const uint32 nPrevBlockTime, uint256& hashStateRoot, uint256& hashReceiptRoot,
                                         uint256& nBlockGasUsed, bytes& btBloomDataOut, uint256& nTotalMintRewardOut, const std::map<CDestination, CAddressContext>& mapAddressContext);
    bool UpdateBlockState(const uint256& hashFork, const uint256& hashBlock, const CBlockEx& block, const std::map<CDestination, CAddressContext>& mapAddressContext, uint256& hashNewStateRoot, SHP_BLOCK_STATE& ptrBlockStateOut);
    bool UpdateBlockTxIndex(const uint256& hashFork, const CBlockEx& block, const uint32 nFile, const uint32 nOffset, const std::map<uint256, CTransactionReceipt>& mapBlockTxReceipts, uint256& hashNewRoot);
    bool UpdateBlockAddressTxInfo(const uint256& hashFork, const uint256& hashBlock, const CBlock& block,
                                  const std::map<uint256, std::vector<CContractTransfer>>& mapContractTransferIn,
                                  const std::map<uint256, uint256>& mapBlockTxFeeUsedIn, const std::map<uint256, std::map<CDestination, uint256>>& mapBlockCodeDestFeeUsedIn,
                                  uint256& hashNewRoot);
    bool UpdateBlockAddress(const uint256& hashFork, const uint256& hashBlock, const CBlock& block, const std::map<CDestination, CAddressContext>& mapAddressContextIn, const std::map<CDestination, CDestState>& mapBlockStateIn,
                            const std::map<CDestination, uint256>& mapBlockPayTvFeeIn, const std::map<uint32, CFunctionAddressContext>& mapBlockFunctionAddressIn, uint256& hashNewRoot);
    bool UpdateBlockCode(const uint256& hashFork, const uint256& hashBlock, const CBlock& block, const uint32 nFile, const uint32 nBlockOffset, const std::map<uint256, CContractCreateCodeContext>& mapContractCreateCodeContextIn,
                         const std::map<uint256, CContractRunCodeContext>& mapContractRunCodeContextIn, const std::map<CDestination, CAddressContext>& mapAddressContext, uint256& hashCodeRoot);
    bool UpdateBlockVoteReward(const uint256& hashFork, const uint32 nChainId, const uint256& hashBlock, const CBlockEx& block, uint256& hashNewRoot);
    bool RetrieveContractKvValue(const uint256& hashFork, const uint256& hashContractRoot, const uint256& key, bytes& value);
    bool AddBlockContractKvValue(const uint256& hashFork, const uint256& hashPrevRoot, uint256& hashContractRoot, const std::map<uint256, bytes>& mapContractState);
    bool RetrieveAddressContext(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, CAddressContext& ctxAddress);
    bool ListContractAddress(const uint256& hashFork, const uint256& hashBlock, std::map<CDestination, CContractAddressContext>& mapContractAddress);
    bool RetrieveTimeVault(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, CTimeVault& tv);
    bool GetAddressCount(const uint256& hashFork, const uint256& hashBlock, uint64& nAddressCount, uint64& nNewAddressCount);
    bool ListFunctionAddress(const uint256& hashBlock, std::map<uint32, CFunctionAddressContext>& mapFunctionAddress);
    bool RetrieveFunctionAddress(const uint256& hashBlock, const uint32 nFuncId, CFunctionAddressContext& ctxFuncAddress);
    bool GetDefaultFunctionAddress(const uint32 nFuncId, CDestination& destDefFunction);
    bool VerifyNewFunctionAddress(const uint256& hashBlock, const CDestination& destFrom, const uint32 nFuncId, const CDestination& destNewFunction, std::string& strErr);
    bool CallContractCode(const bool fEthCall, const uint256& hashFork, const CChainId& chainId, const uint256& nAgreement, const uint32 nHeight, const CDestination& destMint, const uint256& nBlockGasLimit,
                          const CDestination& from, const CDestination& to, const uint256& nGasPrice, const uint256& nGasLimit, const uint256& nAmount,
                          const bytes& data, const uint64 nTimeStamp, const uint256& hashPrevBlock, const uint256& hashPrevStateRoot, const uint64 nPrevBlockTime, uint64& nGasLeft, int& nStatus, bytes& btResult);
    bool GetTxContractData(const uint32 nTxFile, const uint32 nTxOffset, CTxContractData& txcdCode, uint256& txidCreate);
    bool GetBlockSourceCodeData(const uint256& hashFork, const uint256& hashBlock, const uint256& hashSourceCode, CTxContractData& txcdCode);
    bool GetBlockContractCreateCodeData(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractCreateCode, CTxContractData& txcdCode);
    bool RetrieveForkContractCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractCreateCode, CContractCreateCodeContext& ctxtCode);
    bool RetrieveLinkGenesisContractCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractCreateCode, CContractCreateCodeContext& ctxtCode, bool& fLinkGenesisFork);
    bool GetForkContractCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractCreateCode, CContractCodeContext& ctxtContractCode);
    bool ListContractCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& txid, std::map<uint256, CContractCodeContext>& mapCreateCode);
    bool VerifyContractAddress(const uint256& hashFork, const uint256& hashBlock, const CDestination& destContract);
    bool VerifyCreateCodeTx(const uint256& hashFork, const uint256& hashBlock, const CTransaction& tx);
    bool GetAddressTxCount(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, uint64& nTxCount);
    bool RetrieveAddressTxInfo(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, const uint64 nTxIndex, CDestTxInfo& ctxtAddressTxInfo);
    bool ListAddressTxInfo(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, const uint64 nBeginTxIndex, const uint64 nGetTxCount, const bool fReverse, std::vector<CDestTxInfo>& vAddressTxInfo);
    bool GetVoteRewardLockedAmount(const uint256& hashFork, const uint256& hashPrevBlock, const CDestination& dest, uint256& nLockedAmount);
    bool GetBlockAddress(const uint256& hashFork, const uint256& hashBlock, const CBlock& block, std::map<CDestination, CAddressContext>& mapBlockAddress);
    bool GetTransactionReceipt(const uint256& hashFork, const uint256& txid, CTransactionReceiptEx& txReceiptex);
    uint256 AddLogsFilter(const uint256& hashClient, const uint256& hashFork, const CLogsFilter& logsFilter);
    void RemoveFilter(const uint256& nFilterId);
    bool GetTxReceiptLogsByFilterId(const uint256& nFilterId, const bool fAll, ReceiptLogsVec& receiptLogs);
    bool GetTxReceiptsByLogsFilter(const uint256& hashFork, const CLogsFilter& logsFilter, ReceiptLogsVec& vReceiptLogs);
    uint256 AddBlockFilter(const uint256& hashClient, const uint256& hashFork);
    bool GetFilterBlockHashs(const uint256& hashFork, const uint256& nFilterId, const bool fAll, std::vector<uint256>& vBlockHash);
    uint256 AddPendingTxFilter(const uint256& hashClient, const uint256& hashFork);
    void AddPendingTx(const uint256& hashFork, const uint256& txid);
    bool GetFilterTxids(const uint256& hashFork, const uint256& nFilterId, const bool fAll, std::vector<uint256>& vTxid);
    bool GetCreateForkLockedAmount(const CDestination& dest, const uint256& hashPrevBlock, const bytes& btAddressData, uint256& nLockedAmount);
    bool VerifyAddressVoteRedeem(const CDestination& dest, const uint256& hashPrevBlock);
    bool GetAddressLockedAmount(const uint256& hashFork, const uint256& hashPrevBlock, const CDestination& dest, const CAddressContext& ctxAddress, const uint256& nBalance, uint256& nLockedAmount);

    bool AddBlacklistAddress(const CDestination& dest);
    void RemoveBlacklistAddress(const CDestination& dest);
    bool IsExistBlacklistAddress(const CDestination& dest);
    void ListBlacklistAddress(set<CDestination>& setAddressOut);

    bool UpdateForkMintMinGasPrice(const uint256& hashFork, const uint256& nMinGasPrice);
    uint256 GetForkMintMinGasPrice(const uint256& hashFork);

protected:
    CBlockIndex* GetIndex(const uint256& hash) const;
    CBlockIndex* GetForkLastIndex(const uint256& hashFork);
    CBlockIndex* GetOrCreateIndex(const uint256& hash);
    CBlockIndex* GetBranch(CBlockIndex* pIndexRef, CBlockIndex* pIndex, std::vector<CBlockIndex*>& vPath);
    CBlockIndex* GetOriginIndex(const uint256& txidMint);
    void UpdateBlockHeightIndex(const uint256& hashFork, const uint256& hashBlock, const uint64 nBlockTimeStamp, const CDestination& destMint, const uint256& hashRefBlock);
    void RemoveBlockIndex(const uint256& hashFork, const uint256& hashBlock);
    void UpdateBlockRef(const uint256& hashFork, const uint256& hashBlock, const uint256& hashRefBlock);
    bool UpdateBlockLongChain(const uint256& hashFork, const std::vector<CBlockEx>& vBlockRemove, const std::vector<CBlockEx>& vBlockAddNew);
    void UpdateBlockNext(CBlockIndex* pIndexLast);
    CBlockIndex* AddNewIndex(const uint256& hash, const CBlock& block, const uint32 nFile, const uint32 nOffset, const uint32 nCrc, const uint256& nChainTrust, const uint256& hashNewStateRoot);
    bool LoadForkProfile(const CBlockIndex* pIndexOrigin, CProfile& profile);
    bool UpdateDelegate(const uint256& hashFork, const uint256& hashBlock, const CBlockEx& block, const uint32 nFile, const uint32 nOffset,
                        const uint256& nMinEnrollAmount, const std::map<CDestination, CAddressContext>& mapAddressContext,
                        const std::map<CDestination, CDestState>& mapAccStateIn, uint256& hashDelegateRoot);
    bool UpdateVote(const uint256& hashFork, const uint256& hashBlock, const CBlockEx& block,
                    const std::map<CDestination, CAddressContext>& mapAddressContext, const std::map<CDestination, CDestState>& mapAccStateIn,
                    const std::map<CDestination, std::pair<uint32, uint32>>& mapBlockModifyPledgeFinalHeightIn, uint256& hashVoteRoot);
    bool IsValidBlock(CBlockIndex* pForkLast, const uint256& hashBlock);
    bool VerifyValidBlock(CBlockIndex* pIndexGenesisLast, const CBlockIndex* pIndex);
    CBlockIndex* GetLongChainLastBlock(const uint256& hashFork, int nStartHeight, CBlockIndex* pIndexGenesisLast, const std::set<uint256>& setInvalidHash);
    bool GetTxIndex(const uint256& hashFork, const uint256& txid, uint256& hashAtFork, CTxIndex& txIndex);
    void ClearCache();
    bool LoadDB();
    bool VerifyDB();
    bool VerifyBlockDB(const CBlockVerify& verifyBlock, CBlockOutline& outline, CBlockRoot& blockRoot, const bool fVerify);
    bool RepairBlockDB(const CBlockVerify& verifyBlock, CBlockRoot& blockRoot, CBlockEx& block, CBlockIndex** ppIndexNew);
    bool LoadBlockIndex(CBlockOutline& outline, CBlockIndex** ppIndexNew);

protected:
    enum
    {
        MAX_CACHE_BLOCK_STATE = 64
    };

    mutable hnbase::CRWAccess rwAccess;
    bool fCfgFullDb;
    bool fCfgRewardCheck;
    uint256 hashGenesisBlock;
    CBlockDB dbBlock;
    CTimeSeriesCached tsBlock;
    std::map<uint256, CBlockIndex*> mapIndex;
    std::map<uint256, CForkHeightIndex> mapForkHeightIndex;
    CBlockFilter blockFilter;
};

} // namespace storage
} // namespace hashahead

#endif //STORAGE_BLOCKBASE_H
