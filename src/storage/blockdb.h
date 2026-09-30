// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_BLOCKDB_H
#define STORAGE_BLOCKDB_H

#include "addressblacklistdb.h"
#include "addressdb.h"
#include "addresstxinfodb.h"
#include "block.h"
#include "blockindexdb.h"
#include "cfgmintmingasprice.h"
#include "contractdb.h"
#include "dbstruct.h"
#include "forkcontext.h"
#include "forkdb.h"
#include "hdexdb.h"
#include "snapshotdb.h"
#include "statedb.h"
#include "tracedb.h"
#include "transaction.h"
#include "txindexdb.h"
#include "verifydb.h"
#include "votedb.h"

namespace hashahead
{
namespace storage
{

class CBlockDB
{
public:
    CBlockDB();
    ~CBlockDB();
    bool BdInitialize(const boost::filesystem::path& pathData, const uint256& hashGenesisBlockIn, const bool fFullDbIn, const bool fTraceDbIn, const bool fCacheTraceIn, const bool fPruneIn);
    void BdDeinitialize();
    void RemoveAll();
    bool AddForkContext(const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<uint256, CForkContext>& mapForkCtxt, const std::map<std::string, CCoinContext>& mapSymbolCoin,
                        const std::set<CDestination>& setTimeVaultWhitelist, const std::set<uint256>& setStopFork, const bool fTraceDb, uint256& hashNewRoot);
    bool ListForkContext(std::map<uint256, CForkContext>& mapForkCtxt, const uint256& hashBlock = uint256());
    bool RetrieveForkContext(const uint256& hashFork, CForkContext& ctxt, const uint256& hashMainChainRefBlock = uint256());
    bool GetForkCtxStatus(const uint256& hashFork, CForkCtxStatus& forkStatus, const uint256& hashMainChainRefBlock = uint256());
    bool GetTraceDbFlag();
    bool UpdateForkLast(const uint256& hashFork, const uint256& hashLastBlock);
    bool RetrieveForkLast(const uint256& hashFork, uint256& hashLastBlock);
    bool GetForkCoinCtxByForkSymbol(const std::string& strForkSymbol, CCoinContext& ctxCoin, const uint256& hashMainChainRefBlock = uint256());
    bool GetForkHashByForkName(const std::string& strForkName, uint256& hashFork, const uint256& hashMainChainRefBlock = uint256());
    bool GetForkHashByChainId(const CChainId nChainId, uint256& hashFork, const uint256& hashMainChainRefBlock = uint256());
    bool ListCoinContext(std::map<std::string, CCoinContext>& mapSymbolCoin, const uint256& hashMainChainRefBlock = uint256());
    bool GetDexCoinPairBySymbolPair(const std::string& strSymbol1, const std::string& strSymbol2, uint32& nCoinPair, const uint256& hashMainChainRefBlock = uint256());
    bool GetSymbolPairByDexCoinPair(const uint32 nCoinPair, std::string& strSymbol1, std::string& strSymbol2, const uint256& hashMainChainRefBlock = uint256());
    bool ListDexCoinPair(const uint32 nCoinPair, const std::string& strCoinSymbol, std::map<uint32, std::pair<std::string, std::string>>& mapDexCoinPair, const uint256& hashMainChainRefBlock = uint256());
    bool IsTimeVaultWhitelistAddressExist(const CDestination& address, const uint256& hashMainChainRefBlock = uint256());
    bool ListTimeVaultWhitelist(std::set<CDestination>& setTimeVaultWhitelist, const uint256& hashMainChainRefBlock = uint256());
    bool SetPruneFlag(const bool fPrune);
    bool GetSnapshotForkData(const std::map<uint256, uint256>& mapForkLastBlock, const std::vector<uint256>& vBlockHash, bytes& btSnapData);
    bool RecoveryForkData(const bytes& btSnapData);

    bool AddNewFork(const uint256& hashFork);
    bool LoadFork(const uint256& hashFork);
    bool RemoveFork(const uint256& hashFork);
    bool ListFork(std::vector<std::pair<uint256, uint256>>& vFork);
    bool AddNewBlockIndex(const CBlockIndex& outline);
    bool RemoveBlockIndex(const uint256& hashBlock);
    bool RetrieveBlockIndex(const uint256& hashBlock, CBlockIndex& outline);
    bool UpdateBlockNumberBlockLongChain(const uint256& hashFork, const std::vector<std::pair<uint64, uint256>>& vRemoveNumberBlock, const std::vector<std::pair<uint64, uint256>>& mapNewNumberBlock);
    bool RetrieveBlockHashByNumber(const uint256& hashFork, const uint64 nBlockNumber, uint256& hashBlock);
    bool RetrieveBlockHashByHeight(const uint256& hashFork, const uint32 nBlockHeight, std::vector<uint256>& vBlockHash);
    bool GetForkMaxHeight(const uint256& hashFork, uint32& nMaxHeight);
    bool AddBlockVoteResult(const uint256& hashBlock, const bool fLongChain, const bytes& btBitmap, const bytes& btAggSig, const bool fAtChain, const uint256& hashAtBlock);
    bool RemoveBlockVoteResult(const uint256& hashBlock);
    bool RetrieveBlockVoteResult(const uint256& hashBlock, bytes& btBitmap, bytes& btAggSig, bool& fAtChain, uint256& hashAtBlock);
    bool GetLastBlockVoteResult(const uint256& hashFork, uint256& hashLastBlock, bytes& btBitmap, bytes& btAggSig, bool& fAtChain, uint256& hashAtBlock);
    bool GetSnapshotBlockVoteData(const std::vector<uint256>& vBlockHash, bytes& btSnapData);
    bool RecoverySnapshotBlockVoteData(bytes& btSnapData);
    bool GetLastConfirmBlock(const uint256& hashFork, uint256& hashLastConfirmBlock);
    bool AddBlockLocalVoteSignFlag(const uint256& hashBlock);
    bool AddBlockVerify(const CBlockIndex& outline, const uint32 nRootCrc);
    bool RetrieveBlockVerify(const uint256& hashBlock, CBlockVerify& verifyBlock);
    std::size_t GetBlockVerifyCount();
    bool GetBlockVerify(const std::size_t pos, CBlockVerify& verifyBlock);
    bool UpdateDelegateContext(const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDestination, uint256>& mapVote,
                               const std::map<int, std::map<CDestination, CDiskPos>>& mapEnrollTx, uint256& hashDelegateRoot);
    bool RetrieveDestDelegateVote(const uint256& hashBlock, const CDestination& dest, uint256& nVote);
    bool WalkThroughBlockIndex(CBlockDBWalker& walker);
    bool RetrieveTxIndex(const uint256& hashFork, const uint256& txid, uint256& hashTxAtBlock, CTxIndex& txIndex);
    bool RetrieveTxReceipt(const uint256& hashFork, const uint256& txid, CTransactionReceipt& txReceipt);
    bool WalkThroughSnapshotTxIndex(const uint256& hashFork, const uint256& hashLastBlock, WalkerTxIndexKvFunc fnWalker);
    bool WriteTxIndexKvData(const uint256& hashFork, const bytes& btKey, const bytes& btValue);
    bool RetrieveTxContractReceipt(const uint256& hashFork, const uint256& hashBlock, const uint256& txid, TxContractReceipts& tcrReceipt);
    bool ListBlockContractReceipt(const uint256& hashFork, const uint256& hashBlock, BlockContractReceipts& vContractReceipts);
    bool RetrieveTxContractPrevState(const uint256& hashFork, const uint256& hashBlock, const uint256& txid, MapContractPrevState& mapContractPrevState);
    bool ListBlockContractPrevState(const uint256& hashFork, const uint256& hashBlock, BlockContractPrevState& vBlockContractPrevState);
    bool GetContractKvPairList(const uint256& hashFork, const uint256& hashBlock, const CDestination& destContract, const uint256& keyStart, const uint32 nLimit, std::vector<std::pair<uint256, uint256>>& vContractKvPair, uint256& keyNext);
    bool ClearTraceDbUnavailableNode(const uint256& hashFork, const uint32 nClearRefHeight);
    bool GetSnapshotTraceData(const uint256& hashFork, const std::vector<uint256>& vBlockHash, bytes& btSnapData);
    bool RecoveryTraceData(const bytes& btSnapData);
    bool RetrieveDelegate(const uint256& hash, std::map<CDestination, uint256>& mapDelegate);
    bool RetrieveRangeEnroll(int height, const std::vector<uint256>& vBlockRange, std::map<CDestination, CDiskPos>& mapEnrollTxPos);
    bool RetrieveDelegateEnrollStatus(const std::vector<uint256>& vBlockRange, std::map<CDestination, uint32>& mapDelegateEnrollStatus);
    bool AddBlockVote(const uint256& hashPrev, const uint256& hashBlock, const std::map<CDestination, CVoteContext>& mapBlockVote,
                      const std::map<CDestination, std::pair<uint32, uint32>>& mapAddPledgeFinalHeight, const std::map<CDestination, uint32>& mapRemovePledgeFinalHeight,
                      const std::map<CDestination, CPledgeVoteContext>& mapPledgeVote, uint256& hashVoteRoot);
    bool RetrieveAllDelegateVote(const uint256& hashBlock, std::map<CDestination, std::map<CDestination, CVoteContext>>& mapDelegateVote);
    bool RetrieveDestVoteContext(const uint256& hashBlock, const CDestination& destVote, CVoteContext& ctxtVote);
    bool RetrieveDestPledgeVoteContext(const uint256& hashBlock, const CDestination& destVote, CPledgeVoteContext& ctxPledgeVote);
    bool ListPledgeFinalHeight(const uint256& hashBlock, const uint32 nFinalHeight, std::map<CDestination, std::pair<uint32, uint32>>& mapPledgeFinalHeight);
    bool WalkThroughDayVote(const uint256& hashBeginBlock, const uint256& hashTailBlock, CDayVoteWalker& walker);
    bool AddBlockState(const uint256& hashFork, const uint32 nBlockHeight, const uint256& hashPrevRoot, const CBlockRootStatus& statusBlockRoot, const std::map<CDestination, CDestState>& mapBlockState, uint256& hashBlockRoot);
    bool CreateCacheStateTrie(const uint256& hashFork, const uint256& hashPrevRoot, const CBlockRootStatus& statusBlockRoot, const std::map<CDestination, CDestState>& mapBlockState, uint256& hashBlockRoot);
    bool RetrieveDestState(const uint256& hashFork, const uint256& hashBlockRoot, const CDestination& dest, CDestState& state);
    bool ListDestState(const uint256& hashFork, const uint256& hashBlockRoot, std::map<CDestination, CDestState>& mapBlockState);
    bool ClearStateUnavailableNode(const uint256& hashFork, const uint32 nClearRefHeight);
    bool ListStateRootKv(const uint256& hashFork, std::vector<std::pair<uint256, bytesmap>>& vRootKv);
    bool AddStateKvTrie(const uint256& hashFork, const uint32 nBlockHeight, const uint256& hashPrevRoot, const bytesmap& mapKv, uint256& hashNewRoot);
    bool AddBlockTxIndexReceipt(const uint256& hashFork, const uint256& hashBlock, const std::map<uint256, CTxIndex>& mapBlockTxIndex, const std::vector<CTransactionReceipt>& vTxReceipts);
    bool UpdateTxIndexBlockLongChain(const uint256& hashFork, const std::vector<uint256>& vRemoveTx, const std::map<uint256, uint256>& mapNewTx);
    bool AddBlockContractKvValue(const uint256& hashFork, const uint32 nBlockHeight, const uint64 nBlockNumber, const CDestination& destContract, const uint256& hashPrevRoot, const std::map<uint256, bytes>& mapContractState, uint256& hashContractRoot);
    bool CreateCacheContractKvTrie(const uint256& hashFork, const uint256& hashPrevRoot, const std::map<uint256, bytes>& mapContractState, uint256& hashNewRoot);
    bool RetrieveContractKvValue(const uint256& hashFork, const uint256& hashContractRoot, const uint256& key, bytes& value);
    bool ClearContractKvRootUnavailableNode(const uint256& hashFork, const uint32 nRemoveLastHeight, bool& fExit);
    bool GetContractAddressRoot(const uint256& hashFork, const CDestination& destContract, const uint256& hashRoot, uint256& hashPrevRoot, uint32& nBlockHeight, uint64& nBlockNumber);
    bool CreateCacheContractKvRoot(const uint256& hashFork, const uint256& hashPrevRoot, const bytesmap& mapKv, uint256& hashNewRoot);
    bool AddContractKvTrie(const uint256& hashFork, const uint32 nBlockHeight, const uint256& hashPrevRoot, const bytesmap& mapKv, uint256& hashNewRoot);
    bool AddAddressContext(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDestination, CAddressContext>& mapAddress, const uint64 nNewAddressCount,
                           const std::map<CDestination, CTimeVault>& mapTimeVault, const std::map<uint32, CFunctionAddressContext>& mapFunctionAddress,
                           const std::map<CDestination, uint384>& mapBlsPubkeyContext, uint256& hashNewRoot);
    bool AddTokenContractAddressContext(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDestination, CTokenContractAddressContext>& mapTokenContractAddressContext, const bool fAll);
    bool RetrieveAddressContext(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, CAddressContext& ctxAddress);
    bool RetrieveTokenContractAddressContext(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, CTokenContractAddressContext& ctxAddress);
    bool ListAddress(const uint256& hashFork, const uint256& hashBlock, std::map<CDestination, CAddressContext>& mapAddress);
    bool ListContractAddress(const uint256& hashFork, const uint256& hashBlock, std::map<CDestination, CContractAddressContext>& mapContractAddress);
    bool ListTokenContractAddress(const uint256& hashFork, const uint256& hashBlock, std::map<CDestination, CTokenContractAddressContext>& mapTokenContractAddress);
    bool GetAddressCount(const uint256& hashFork, const uint256& hashBlock, uint64& nAddressCount, uint64& nNewAddressCount);
    bool ListFunctionAddress(const uint256& hashFork, const uint256& hashBlock, std::map<uint32, CFunctionAddressContext>& mapFunctionAddress);
    bool RetrieveFunctionAddress(const uint256& hashFork, const uint256& hashBlock, const uint32 nFuncId, CFunctionAddressContext& ctxFuncAddress);
    bool AddCodeContext(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock,
                        const std::map<uint256, CContractSourceCodeContext>& mapSourceCode,
                        const std::map<uint256, CContractCreateCodeContext>& mapContractCreateCode,
                        const std::map<uint256, CContractRunCodeContext>& mapContractRunCode,
                        const std::map<uint256, CTemplateContext>& mapTemplateData,
                        uint256& hashCodeRoot);
    bool RetrieveSourceCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashSourceCode, CContractSourceCodeContext& ctxtCode);
    bool RetrieveContractCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractCreateCode, CContractCreateCodeContext& ctxtCode);
    bool RetrieveContractRunCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractRunCode, CContractRunCodeContext& ctxtCode);
    bool ListContractCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, std::map<uint256, CContractCreateCodeContext>& mapContractCreateCode);

    bool AddAddressTxInfo(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, const uint64 nBlockNumber, const std::map<CDestination, std::vector<CDestTxInfo>>& mapAddressTxInfo, uint256& hashNewRoot);
    bool GetAddressTxCount(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, uint64& nTxCount);
    bool RetrieveAddressTxInfo(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, const uint64 nTxIndex, CDestTxInfo& ctxtAddressTxInfo);
    bool ListAddressTxInfo(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, const uint64 nBeginTxIndex, const uint64 nGetTxCount, const bool fReverse, std::vector<CDestTxInfo>& vAddressTxInfo);

    bool AddVoteReward(const uint256& hashFork, const uint32 nChainId, const uint256& hashPrevBlock, const uint256& hashBlock, const uint32 nBlockHeight, const std::map<CDestination, uint256>& mapVoteReward, uint256& hashNewRoot);
    bool ListVoteReward(const uint32 nChainId, const uint256& hashBlock, const CDestination& dest, const uint32 nGetCount, std::vector<std::pair<uint32, uint256>>& vVoteReward);

    bool AddBlacklistAddress(const CDestination& dest);
    void RemoveBlacklistAddress(const CDestination& dest);
    bool IsExistBlacklistAddress(const CDestination& dest);
    void ListBlacklistAddress(set<CDestination>& setAddressOut);

    bool UpdateForkMintMinGasPrice(const uint256& hashFork, const uint256& nMinGasPrice);
    bool GetForkMintMinGasPrice(const uint256& hashFork, uint256& nMinGasPrice);

    bool VerifyBlockRoot(const bool fPrimary, const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock,
                         const uint256& hashLocalStateRoot, CBlockRoot& localBlockRoot, const bool fVerifyAllNode = true);

protected:
    bool LoadAllFork();

protected:
    bool fCfgFullDb;

    CForkDB dbFork;
    CBlockIndexDB dbBlockIndex;
    CTxIndexDB dbTxIndex;
    CVoteDB dbVote;
    CStateDB dbState;
    CAddressDB dbAddress;
    CVerifyDB dbVerify;
    CContractDB dbContract;
    CAddressBlacklistDB dbAddressBlacklist;
    CCfgMintMinGasPriceDB dbMintMinGasPrice;

    CAddressTxInfoDB dbAddressTxInfo;
};

} // namespace storage
} // namespace hashahead

#endif //STORAGE_BLOCKDB_H
