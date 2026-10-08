// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blockdb.h"

#include "util.h"

using namespace std;
using namespace hnbase;

namespace hashahead
{
namespace storage
{

//////////////////////////////
// CBlockDB

CBlockDB::CBlockDB()
  : fCfgFullDb(false), fCfgTraceDb(false), fCfgCacheTrace(false)
{
}

CBlockDB::~CBlockDB()
{
}

bool CBlockDB::BdInitialize(const boost::filesystem::path& pathData, const uint256& hashGenesisBlockIn, const bool fFullDbIn, const bool fTraceDbIn, const bool fCacheTraceIn, const bool fPruneIn)
{
    fCfgFullDb = fFullDbIn;
    fCfgTraceDb = fTraceDbIn;
    fCfgCacheTrace = fCacheTraceIn;
    fCfgPrune = fPruneIn;
    if (!dbVerify.Initialize(pathData))
    {
        StdLog("CBlockDB", "Initialize: dbVerify initialize fail");
        return false;
    }
    if (!dbFork.Initialize(pathData, hashGenesisBlockIn))
    {
        StdLog("CBlockDB", "Initialize: dbFork initialize fail");
        return false;
    }
    if (!dbBlockIndex.Initialize(pathData))
    {
        StdLog("CBlockDB", "Initialize: dbBlockIndex initialize fail");
        return false;
    }
    if (!dbTxIndex.Initialize(pathData))
    {
        StdLog("CBlockDB", "Initialize: dbTxIndex initialize fail");
        return false;
    }
    if (!dbVote.Initialize(pathData, fPruneIn))
    {
        StdLog("CBlockDB", "Initialize: dbVote initialize fail");
        return false;
    }
    if (!dbHdex.Initialize(pathData, fPruneIn))
    {
        StdLog("CBlockDB", "Initialize: dbHdex initialize fail");
        return false;
    }
    if (!dbState.Initialize(pathData, fPruneIn))
    {
        StdLog("CBlockDB", "Initialize: dbState initialize fail");
        return false;
    }
    if (!dbAddress.Initialize(pathData, hashGenesisBlockIn, fPruneIn))
    {
        StdLog("CBlockDB", "Initialize: dbAddress initialize fail");
        return false;
    }
    if (!dbContract.Initialize(pathData, fPruneIn))
    {
        StdLog("CBlockDB", "Initialize: dbContract initialize fail");
        return false;
    }
    if (!dbAddressBlacklist.Initialize(pathData))
    {
        StdLog("CBlockDB", "Initialize: dbAddressBlacklist initialize fail");
        return false;
    }
    if (!dbMintMinGasPrice.Initialize(pathData))
    {
        StdLog("CBlockDB", "Initialize: dbMintMinGasPrice initialize fail");
        return false;
    }
    if (!dbSnapshot.Initialize(pathData))
    {
        StdLog("CBlockDB", "Initialize: dbSnapshot initialize fail");
        return false;
    }
    if (fCfgFullDb)
    {
        if (!dbAddressTxInfo.Initialize(pathData))
        {
            StdLog("CBlockDB", "Initialize: dbAddressTxInfo initialize fail");
            return false;
        }
    }
    if (fCfgTraceDb)
    {
        if (!dbTrace.Initialize(pathData, fCfgCacheTrace, fPruneIn))
        {
            StdLog("CBlockDB", "Initialize: dbTrace initialize fail");
            return false;
        }
    }
    return LoadAllFork();
}

void CBlockDB::BdDeinitialize()
{
    dbContract.Deinitialize();
    dbAddress.Deinitialize();
    dbState.Deinitialize();
    dbVote.Deinitialize();
    dbHdex.Deinitialize();
    dbTxIndex.Deinitialize();
    dbBlockIndex.Deinitialize();
    dbFork.Deinitialize();
    dbVerify.Deinitialize();
    dbAddressBlacklist.Deinitialize();
    dbMintMinGasPrice.Deinitialize();
    dbSnapshot.Deinitialize();
    if (fCfgFullDb)
    {
        dbAddressTxInfo.Deinitialize();
    }
    if (fCfgTraceDb)
    {
        dbTrace.Deinitialize();
    }
}

void CBlockDB::RemoveAll()
{
    dbContract.Clear();
    dbAddress.Clear();
    dbState.Clear();
    dbVote.Clear();
    dbHdex.Clear();
    dbTxIndex.Clear();
    dbBlockIndex.Clear();
    dbFork.Clear();
    dbVerify.Clear();
    dbAddressBlacklist.Remove();
    dbMintMinGasPrice.Remove();
    dbSnapshot.Remove();
    if (fCfgFullDb)
    {
        dbAddressTxInfo.Clear();
    }
    if (fCfgTraceDb)
    {
        dbTrace.Clear();
    }
}

bool CBlockDB::AddForkContext(const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<uint256, CForkContext>& mapForkCtxt, const std::map<std::string, CCoinContext>& mapSymbolCoin,
                              const std::set<CDestination>& setTimeVaultWhitelist, const std::set<uint256>& setStopFork, const bool fTraceDb, uint256& hashNewRoot)
{
    return dbFork.AddForkContext(hashPrevBlock, hashBlock, mapForkCtxt, mapSymbolCoin, setTimeVaultWhitelist, setStopFork, fTraceDb, hashNewRoot);
}

bool CBlockDB::ListForkContext(std::map<uint256, CForkContext>& mapForkCtxt, const uint256& hashBlock)
{
    return dbFork.ListForkContext(mapForkCtxt, hashBlock);
}

bool CBlockDB::RetrieveForkContext(const uint256& hashFork, CForkContext& ctxt, const uint256& hashMainChainRefBlock)
{
    return dbFork.RetrieveForkContext(hashFork, ctxt, hashMainChainRefBlock);
}

bool CBlockDB::GetForkCtxStatus(const uint256& hashFork, CForkCtxStatus& forkStatus, const uint256& hashMainChainRefBlock)
{
    return dbFork.GetForkCtxStatus(hashFork, forkStatus, hashMainChainRefBlock);
}

bool CBlockDB::GetTraceDbFlag()
{
    return dbFork.GetTraceDbFlag();
}

bool CBlockDB::UpdateForkLast(const uint256& hashFork, const uint256& hashLastBlock)
{
    return dbFork.UpdateForkLast(hashFork, hashLastBlock);
}

bool CBlockDB::RetrieveForkLast(const uint256& hashFork, uint256& hashLastBlock)
{
    return dbFork.RetrieveForkLast(hashFork, hashLastBlock);
}

bool CBlockDB::GetForkCoinCtxByForkSymbol(const std::string& strForkSymbol, CCoinContext& ctxCoin, const uint256& hashMainChainRefBlock)
{
    return dbFork.GetForkCoinCtxByForkSymbol(strForkSymbol, ctxCoin, hashMainChainRefBlock);
}

bool CBlockDB::GetForkHashByForkName(const std::string& strForkName, uint256& hashFork, const uint256& hashMainChainRefBlock)
{
    return dbFork.GetForkHashByForkName(strForkName, hashFork, hashMainChainRefBlock);
}

bool CBlockDB::GetForkHashByChainId(const CChainId nChainId, uint256& hashFork, const uint256& hashMainChainRefBlock)
{
    return dbFork.GetForkHashByChainId(nChainId, hashFork, hashMainChainRefBlock);
}

bool CBlockDB::ListCoinContext(std::map<std::string, CCoinContext>& mapSymbolCoin, const uint256& hashMainChainRefBlock)
{
    return dbFork.ListCoinContext(mapSymbolCoin, hashMainChainRefBlock);
}

bool CBlockDB::GetDexCoinPairBySymbolPair(const std::string& strSymbol1, const std::string& strSymbol2, uint32& nCoinPair, const uint256& hashMainChainRefBlock)
{
    return dbFork.GetDexCoinPairBySymbolPair(strSymbol1, strSymbol2, nCoinPair, hashMainChainRefBlock);
}

bool CBlockDB::GetSymbolPairByDexCoinPair(const uint32 nCoinPair, std::string& strSymbol1, std::string& strSymbol2, const uint256& hashMainChainRefBlock)
{
    return dbFork.GetSymbolPairByDexCoinPair(nCoinPair, strSymbol1, strSymbol2, hashMainChainRefBlock);
}

bool CBlockDB::ListDexCoinPair(const uint32 nCoinPair, const std::string& strCoinSymbol, std::map<uint32, std::pair<std::string, std::string>>& mapDexCoinPair, const uint256& hashMainChainRefBlock)
{
    return dbFork.ListDexCoinPair(nCoinPair, strCoinSymbol, mapDexCoinPair, hashMainChainRefBlock);
}

bool CBlockDB::IsTimeVaultWhitelistAddressExist(const CDestination& address, const uint256& hashMainChainRefBlock)
{
    return dbFork.IsTimeVaultWhitelistAddressExist(address, hashMainChainRefBlock);
}

bool CBlockDB::ListTimeVaultWhitelist(std::set<CDestination>& setTimeVaultWhitelist, const uint256& hashMainChainRefBlock)
{
    return dbFork.ListTimeVaultWhitelist(setTimeVaultWhitelist, hashMainChainRefBlock);
}

bool CBlockDB::SetPruneFlag(const bool fPrune)
{
    return dbFork.SetPruneFlag(fPrune);
}

bool CBlockDB::GetSnapshotForkData(const std::map<uint256, uint256>& mapForkLastBlock, const std::vector<uint256>& vBlockHash, bytes& btSnapData)
{
    return dbFork.GetSnapshotForkData(mapForkLastBlock, vBlockHash, btSnapData);
}

bool CBlockDB::RecoveryForkData(const bytes& btSnapData)
{
    return dbFork.RecoveryForkData(btSnapData);
}

bool CBlockDB::AddNewFork(const uint256& hashFork)
{
    if (!dbFork.UpdateForkLast(hashFork, hashFork))
    {
        return false;
    }
    if (!dbTxIndex.AddNewFork(hashFork))
    {
        RemoveFork(hashFork);
        return false;
    }
    if (!dbState.AddNewFork(hashFork))
    {
        RemoveFork(hashFork);
        return false;
    }
    if (!dbAddress.AddNewFork(hashFork))
    {
        RemoveFork(hashFork);
        return false;
    }
    if (!dbContract.AddNewFork(hashFork))
    {
        RemoveFork(hashFork);
        return false;
    }
    if (fCfgFullDb)
    {
        if (!dbAddressTxInfo.AddNewFork(hashFork))
        {
            RemoveFork(hashFork);
            return false;
        }
    }
    if (fCfgTraceDb)
    {
        if (!dbTrace.AddNewFork(hashFork))
        {
            RemoveFork(hashFork);
            return false;
        }
    }
    return true;
}

bool CBlockDB::LoadFork(const uint256& hashFork)
{
    if (!dbTxIndex.LoadFork(hashFork))
    {
        return false;
    }
    if (!dbState.LoadFork(hashFork))
    {
        return false;
    }
    if (!dbAddress.LoadFork(hashFork))
    {
        return false;
    }
    if (!dbContract.LoadFork(hashFork))
    {
        return false;
    }
    if (fCfgFullDb)
    {
        if (!dbAddressTxInfo.LoadFork(hashFork))
        {
            return false;
        }
    }
    if (fCfgTraceDb)
    {
        if (!dbTrace.LoadFork(hashFork))
        {
            return false;
        }
    }
    return true;
}

bool CBlockDB::RemoveFork(const uint256& hashFork)
{
    dbTxIndex.RemoveFork(hashFork);
    dbState.RemoveFork(hashFork);
    dbAddress.RemoveFork(hashFork);
    dbContract.RemoveFork(hashFork);
    if (fCfgFullDb)
    {
        dbAddressTxInfo.RemoveFork(hashFork);
    }
    if (fCfgTraceDb)
    {
        dbTrace.RemoveFork(hashFork);
    }
    return dbFork.RemoveFork(hashFork);
}

bool CBlockDB::ListFork(vector<pair<uint256, uint256>>& vFork)
{
    std::map<uint256, CForkContext> mapForkCtxt;
    if (!dbFork.ListForkContext(mapForkCtxt))
    {
        return false;
    }
    vFork.clear();
    for (const auto& kv : mapForkCtxt)
    {
        uint256 hashLastBlock;
        if (!dbFork.RetrieveForkLast(kv.first, hashLastBlock))
        {
            hashLastBlock = 0;
        }
        vFork.push_back(make_pair(kv.first, hashLastBlock));
    }
    return true;
}

bool CBlockDB::AddNewBlockIndex(const CBlockIndex& outline)
{
    return dbBlockIndex.AddNewBlockIndex(outline);
}

bool CBlockDB::RemoveBlockIndex(const uint256& hashBlock)
{
    return dbBlockIndex.RemoveBlockIndex(hashBlock);
}

bool CBlockDB::RetrieveBlockIndex(const uint256& hashBlock, CBlockIndex& outline)
{
    return dbBlockIndex.RetrieveBlockIndex(hashBlock, outline);
}

bool CBlockDB::UpdateBlockNumberBlockLongChain(const uint256& hashFork, const std::vector<std::pair<uint64, uint256>>& vRemoveNumberBlock, const std::vector<std::pair<uint64, uint256>>& mapNewNumberBlock)
{
    return dbBlockIndex.UpdateBlockNumberBlockLongChain(hashFork, vRemoveNumberBlock, mapNewNumberBlock);
}

bool CBlockDB::RetrieveBlockHashByNumber(const uint256& hashFork, const uint64 nBlockNumber, uint256& hashBlock)
{
    return dbBlockIndex.RetrieveBlockHashByNumber(hashFork, nBlockNumber, hashBlock);
}

bool CBlockDB::RetrieveBlockHashByHeight(const uint256& hashFork, const uint32 nBlockHeight, std::vector<uint256>& vBlockHash)
{
    return dbBlockIndex.RetrieveBlockHashByHeight(hashFork, nBlockHeight, vBlockHash);
}

bool CBlockDB::GetForkMaxHeight(const uint256& hashFork, uint32& nMaxHeight)
{
    return dbBlockIndex.GetForkMaxHeight(hashFork, nMaxHeight);
}

bool CBlockDB::AddBlockVoteResult(const uint256& hashBlock, const bool fLongChain, const bytes& btBitmap, const bytes& btAggSig, const bool fAtChain, const uint256& hashAtBlock)
{
    return dbBlockIndex.AddBlockVoteResult(hashBlock, fLongChain, btBitmap, btAggSig, fAtChain, hashAtBlock);
}

bool CBlockDB::RemoveBlockVoteResult(const uint256& hashBlock)
{
    return dbBlockIndex.RemoveBlockVoteResult(hashBlock);
}

bool CBlockDB::RetrieveBlockVoteResult(const uint256& hashBlock, bytes& btBitmap, bytes& btAggSig, bool& fAtChain, uint256& hashAtBlock)
{
    return dbBlockIndex.RetrieveBlockVoteResult(hashBlock, btBitmap, btAggSig, fAtChain, hashAtBlock);
}

bool CBlockDB::GetLastBlockVoteResult(const uint256& hashFork, uint256& hashLastBlock, bytes& btBitmap, bytes& btAggSig, bool& fAtChain, uint256& hashAtBlock)
{
    return dbBlockIndex.GetLastBlockVoteResult(hashFork, hashLastBlock, btBitmap, btAggSig, fAtChain, hashAtBlock);
}

bool CBlockDB::GetSnapshotBlockVoteData(const std::vector<uint256>& vBlockHash, bytes& btSnapData)
{
    return dbBlockIndex.GetSnapshotBlockVoteData(vBlockHash, btSnapData);
}

bool CBlockDB::RecoverySnapshotBlockVoteData(bytes& btSnapData)
{
    return dbBlockIndex.RecoverySnapshotBlockVoteData(btSnapData);
}

bool CBlockDB::GetLastConfirmBlock(const uint256& hashFork, uint256& hashLastConfirmBlock)
{
    return dbBlockIndex.GetLastConfirmBlock(hashFork, hashLastConfirmBlock);
}

bool CBlockDB::AddBlockLocalVoteSignFlag(const uint256& hashBlock)
{
    return dbBlockIndex.AddBlockLocalVoteSignFlag(hashBlock);
}

bool CBlockDB::AddBlockVerify(const CBlockIndex& outline, const uint32 nRootCrc)
{
    return dbVerify.AddBlockVerify(outline, nRootCrc);
}

bool CBlockDB::RetrieveBlockVerify(const uint256& hashBlock, CBlockVerify& verifyBlock)
{
    return dbVerify.RetrieveBlockVerify(hashBlock, verifyBlock);
}

std::size_t CBlockDB::GetBlockVerifyCount()
{
    return dbVerify.GetBlockVerifyCount();
}

bool CBlockDB::GetBlockVerify(const std::size_t pos, CBlockVerify& verifyBlock)
{
    return dbVerify.GetBlockVerify(pos, verifyBlock);
}

bool CBlockDB::UpdateDelegateContext(const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDestination, uint256>& mapVote,
                                     const std::map<int, std::map<CDestination, CDiskPos>>& mapEnrollTx, uint256& hashDelegateRoot)
{
    return dbVote.AddNewDelegate(hashPrevBlock, hashBlock, mapVote, mapEnrollTx, hashDelegateRoot);
}

bool CBlockDB::RetrieveDestDelegateVote(const uint256& hashBlock, const CDestination& dest, uint256& nVote)
{
    return dbVote.RetrieveDestDelegateVote(hashBlock, dest, nVote);
}

bool CBlockDB::WalkThroughBlockIndex(CBlockDBWalker& walker)
{
    return dbBlockIndex.WalkThroughBlockIndex(walker);
}

bool CBlockDB::RetrieveTxIndex(const uint256& hashFork, const uint256& txid, uint256& hashTxAtBlock, CTxIndex& txIndex)
{
    return dbTxIndex.RetrieveTxIndex(hashFork, txid, hashTxAtBlock, txIndex);
}

bool CBlockDB::RetrieveTxReceipt(const uint256& hashFork, const uint256& txid, CTransactionReceipt& txReceipt)
{
    return dbTxIndex.RetrieveTxReceipt(hashFork, txid, txReceipt);
}

bool CBlockDB::WalkThroughSnapshotTxIndex(const uint256& hashFork, const uint256& hashLastBlock, WalkerTxIndexKvFunc fnWalker)
{
    return dbTxIndex.WalkThroughSnapshotTxIndex(hashFork, hashLastBlock, fnWalker);
}

bool CBlockDB::WriteTxIndexKvData(const uint256& hashFork, const bytes& btKey, const bytes& btValue)
{
    return dbTxIndex.WriteTxIndexKvData(hashFork, btKey, btValue);
}

bool CBlockDB::RetrieveTxContractReceipt(const uint256& hashFork, const uint256& hashBlock, const uint256& txid, TxContractReceipts& tcrReceipt)
{
    if (fCfgTraceDb)
    {
        return dbTrace.RetrieveTxContractReceipt(hashFork, hashBlock, txid, tcrReceipt);
    }
    return false;
}

bool CBlockDB::ListBlockContractReceipt(const uint256& hashFork, const uint256& hashBlock, BlockContractReceipts& vContractReceipts)
{
    if (fCfgTraceDb)
    {
        return dbTrace.ListBlockContractReceipt(hashFork, hashBlock, vContractReceipts);
    }
    return false;
}

bool CBlockDB::RetrieveTxContractPrevState(const uint256& hashFork, const uint256& hashBlock, const uint256& txid, MapContractPrevState& mapContractPrevState)
{
    if (fCfgTraceDb)
    {
        return dbTrace.RetrieveTxContractPrevState(hashFork, hashBlock, txid, mapContractPrevState);
    }
    return false;
}

bool CBlockDB::ListBlockContractPrevState(const uint256& hashFork, const uint256& hashBlock, BlockContractPrevState& vBlockContractPrevState)
{
    if (fCfgTraceDb)
    {
        return dbTrace.ListBlockContractPrevState(hashFork, hashBlock, vBlockContractPrevState);
    }
    return false;
}

bool CBlockDB::GetContractKvPairList(const uint256& hashFork, const uint256& hashBlock, const CDestination& destContract, const uint256& keyStart, const uint32 nLimit, std::vector<std::pair<uint256, uint256>>& vContractKvPair, uint256& keyNext)
{
    if (fCfgTraceDb)
    {
        return dbTrace.GetContractKvPairList(hashFork, hashBlock, destContract, keyStart, nLimit, vContractKvPair, keyNext);
    }
    return false;
}

bool CBlockDB::ClearTraceDbUnavailableNode(const uint256& hashFork, const uint32 nClearRefHeight)
{
    if (fCfgTraceDb)
    {
        return dbTrace.ClearTraceUnavailableNode(hashFork, nClearRefHeight);
    }
    return false;
}

bool CBlockDB::GetSnapshotTraceData(const uint256& hashFork, const std::vector<uint256>& vBlockHash, bytes& btSnapData)
{
    if (fCfgTraceDb)
    {
        return dbTrace.GetSnapshotTraceData(hashFork, vBlockHash, btSnapData);
    }
    return false;
}

bool CBlockDB::RecoveryTraceData(const bytes& btSnapData)
{
    if (fCfgTraceDb)
    {
        return dbTrace.RecoveryTraceData(btSnapData);
    }
    return true;
}

bool CBlockDB::RetrieveDelegate(const uint256& hash, map<CDestination, uint256>& mapDelegate)
{
    return dbVote.RetrieveDelegatedVote(hash, mapDelegate);
}

bool CBlockDB::RetrieveRangeEnroll(int height, const vector<uint256>& vBlockRange, map<CDestination, CDiskPos>& mapEnrollTxPos)
{
    return dbVote.RetrieveRangeEnroll(height, vBlockRange, mapEnrollTxPos);
}

bool CBlockDB::RetrieveDelegateEnrollStatus(const std::vector<uint256>& vBlockRange, std::map<CDestination, uint32>& mapDelegateEnrollStatus)
{
    return dbVote.RetrieveDelegateEnrollStatus(vBlockRange, mapDelegateEnrollStatus);
}

bool CBlockDB::AddBlockVote(const uint256& hashPrev, const uint256& hashBlock, const std::map<CDestination, CVoteContext>& mapBlockVote,
                            const std::map<CDestination, std::pair<uint32, uint32>>& mapAddPledgeFinalHeight, const std::map<CDestination, uint32>& mapRemovePledgeFinalHeight,
                            const std::map<CDestination, CPledgeVoteContext>& mapPledgeVote, uint256& hashVoteRoot)
{
    return dbVote.AddBlockVote(hashPrev, hashBlock, mapBlockVote, mapAddPledgeFinalHeight, mapRemovePledgeFinalHeight, mapPledgeVote, hashVoteRoot);
}

bool CBlockDB::RetrieveAllDelegateVote(const uint256& hashBlock, std::map<CDestination, std::map<CDestination, CVoteContext>>& mapDelegateVote)
{
    return dbVote.RetrieveAllDelegateVote(hashBlock, mapDelegateVote);
}

bool CBlockDB::RetrieveDestVoteContext(const uint256& hashBlock, const CDestination& destVote, CVoteContext& ctxtVote)
{
    return dbVote.RetrieveDestVoteContext(hashBlock, destVote, ctxtVote);
}

bool CBlockDB::RetrieveDestPledgeVoteContext(const uint256& hashBlock, const CDestination& destVote, CPledgeVoteContext& ctxPledgeVote)
{
    return dbVote.RetrieveDestPledgeVoteContext(hashBlock, destVote, ctxPledgeVote);
}

bool CBlockDB::ListPledgeFinalHeight(const uint256& hashBlock, const uint32 nFinalHeight, std::map<CDestination, std::pair<uint32, uint32>>& mapPledgeFinalHeight)
{
    return dbVote.ListPledgeFinalHeight(hashBlock, nFinalHeight, mapPledgeFinalHeight);
}

bool CBlockDB::WalkThroughDayVote(const uint256& hashBeginBlock, const uint256& hashTailBlock, CDayVoteWalker& walker)
{
    return dbVote.WalkThroughDayVote(hashBeginBlock, hashTailBlock, walker);
}

bool CBlockDB::AddBlockState(const uint256& hashFork, const uint32 nBlockHeight, const uint256& hashPrevRoot, const CBlockRootStatus& statusBlockRoot, const std::map<CDestination, CDestState>& mapBlockState, uint256& hashBlockRoot)
{
    return dbState.AddBlockState(hashFork, nBlockHeight, hashPrevRoot, statusBlockRoot, mapBlockState, hashBlockRoot);
}

bool CBlockDB::CreateCacheStateTrie(const uint256& hashFork, const uint256& hashPrevRoot, const CBlockRootStatus& statusBlockRoot, const std::map<CDestination, CDestState>& mapBlockState, uint256& hashBlockRoot)
{
    return dbState.CreateCacheStateTrie(hashFork, hashPrevRoot, statusBlockRoot, mapBlockState, hashBlockRoot);
}

bool CBlockDB::RetrieveDestState(const uint256& hashFork, const uint256& hashBlockRoot, const CDestination& dest, CDestState& state)
{
    return dbState.RetrieveDestState(hashFork, hashBlockRoot, dest, state);
}

bool CBlockDB::ListDestState(const uint256& hashFork, const uint256& hashBlockRoot, std::map<CDestination, CDestState>& mapBlockState)
{
    return dbState.ListDestState(hashFork, hashBlockRoot, mapBlockState);
}

bool CBlockDB::ClearStateUnavailableNode(const uint256& hashFork, const uint32 nClearRefHeight)
{
    return dbState.ClearStateUnavailableNode(hashFork, nClearRefHeight);
}

bool CBlockDB::ListStateRootKv(const uint256& hashFork, std::vector<std::pair<uint256, bytesmap>>& vRootKv)
{
    return dbState.ListStateRootKv(hashFork, vRootKv);
}

bool CBlockDB::AddStateKvTrie(const uint256& hashFork, const uint32 nBlockHeight, const uint256& hashPrevRoot, const bytesmap& mapKv, uint256& hashNewRoot)
{
    return dbState.AddStateKvTrie(hashFork, nBlockHeight, hashPrevRoot, mapKv, hashNewRoot);
}

bool CBlockDB::AddBlockTxIndexReceipt(const uint256& hashFork, const uint256& hashBlock, const std::map<uint256, CTxIndex>& mapBlockTxIndex, const std::vector<CTransactionReceipt>& vTxReceipts)
{
    return dbTxIndex.AddBlockTxIndexReceipt(hashFork, hashBlock, mapBlockTxIndex, vTxReceipts);
}

bool CBlockDB::UpdateTxIndexBlockLongChain(const uint256& hashFork, const std::vector<uint256>& vRemoveTx, const std::map<uint256, uint256>& mapNewTx)
{
    return dbTxIndex.UpdateTxIndexBlockLongChain(hashFork, vRemoveTx, mapNewTx);
}

bool CBlockDB::AddBlockContractKvValue(const uint256& hashFork, const uint32 nBlockHeight, const uint64 nBlockNumber, const CDestination& destContract, const uint256& hashPrevRoot, const std::map<uint256, bytes>& mapContractState, uint256& hashContractRoot)
{
    return dbContract.AddBlockContractKvValue(hashFork, nBlockHeight, nBlockNumber, destContract, hashPrevRoot, mapContractState, hashContractRoot);
}

bool CBlockDB::CreateCacheContractKvTrie(const uint256& hashFork, const uint256& hashPrevRoot, const std::map<uint256, bytes>& mapContractState, uint256& hashNewRoot)
{
    return dbContract.CreateCacheContractKvTrie(hashFork, hashPrevRoot, mapContractState, hashNewRoot);
}

bool CBlockDB::RetrieveContractKvValue(const uint256& hashFork, const uint256& hashContractRoot, const uint256& key, bytes& value)
{
    return dbContract.RetrieveContractKvValue(hashFork, hashContractRoot, key, value);
}

bool CBlockDB::ClearContractKvRootUnavailableNode(const uint256& hashFork, const uint32 nRemoveLastHeight, bool& fExit)
{
    return dbContract.ClearContractKvRootUnavailableNode(hashFork, nRemoveLastHeight, fExit);
}

bool CBlockDB::GetContractAddressRoot(const uint256& hashFork, const CDestination& destContract, const uint256& hashRoot, uint256& hashPrevRoot, uint32& nBlockHeight, uint64& nBlockNumber)
{
    return dbContract.GetContractAddressRoot(hashFork, destContract, hashRoot, hashPrevRoot, nBlockHeight, nBlockNumber);
}

bool CBlockDB::CreateCacheContractKvRoot(const uint256& hashFork, const uint256& hashPrevRoot, const bytesmap& mapKv, uint256& hashNewRoot)
{
    return dbContract.CreateCacheContractKvRoot(hashFork, hashPrevRoot, mapKv, hashNewRoot);
}

bool CBlockDB::AddContractKvTrie(const uint256& hashFork, const uint32 nBlockHeight, const uint256& hashPrevRoot, const bytesmap& mapKv, uint256& hashNewRoot)
{
    return dbContract.AddContractKvTrie(hashFork, nBlockHeight, hashPrevRoot, mapKv, hashNewRoot);
}

bool CBlockDB::AddAddressContext(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDestination, CAddressContext>& mapAddress, const uint64 nNewAddressCount,
                                 const std::map<CDestination, CTimeVault>& mapTimeVault, const std::map<uint32, CFunctionAddressContext>& mapFunctionAddress,
                                 const std::map<CDestination, uint384>& mapBlsPubkeyContext, uint256& hashNewRoot)
{
    return dbAddress.AddAddressContext(hashFork, hashPrevBlock, hashBlock, mapAddress, nNewAddressCount, mapTimeVault, mapFunctionAddress, mapBlsPubkeyContext, hashNewRoot);
}

bool CBlockDB::AddTokenContractAddressContext(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDestination, CTokenContractAddressContext>& mapTokenContractAddressContext, const bool fAll)
{
    return dbAddress.AddTokenContractAddressContext(hashFork, hashPrevBlock, hashBlock, mapTokenContractAddressContext, fAll);
}

bool CBlockDB::RetrieveAddressContext(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, CAddressContext& ctxAddress)
{
    uint256 hashRefBlock;
    if (hashBlock == 0)
    {
        if (!dbFork.RetrieveForkLast(hashFork, hashRefBlock))
        {
            StdLog("CBlockDB", "Retrieve address context: Retrieve fork last fail, fork: %s", hashFork.ToString().c_str());
            return false;
        }
    }
    else
    {
        hashRefBlock = hashBlock;
    }
    return dbAddress.RetrieveAddressContext(hashFork, hashRefBlock, dest, ctxAddress);
}

bool CBlockDB::RetrieveTokenContractAddressContext(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, CTokenContractAddressContext& ctxAddress)
{
    return dbAddress.RetrieveTokenContractAddressContext(hashFork, hashBlock, dest, ctxAddress);
}

bool CBlockDB::ListAddress(const uint256& hashFork, const uint256& hashBlock, std::map<CDestination, CAddressContext>& mapAddress)
{
    return dbAddress.ListAddress(hashFork, hashBlock, mapAddress);
}

bool CBlockDB::ListContractAddress(const uint256& hashFork, const uint256& hashBlock, std::map<CDestination, CContractAddressContext>& mapContractAddress)
{
    return dbAddress.ListContractAddress(hashFork, hashBlock, mapContractAddress);
}

bool CBlockDB::ListTokenContractAddress(const uint256& hashFork, const uint256& hashBlock, std::map<CDestination, CTokenContractAddressContext>& mapTokenContractAddress)
{
    return dbAddress.ListTokenContractAddress(hashFork, hashBlock, mapTokenContractAddress);
}

bool CBlockDB::GetAddressCount(const uint256& hashFork, const uint256& hashBlock, uint64& nAddressCount, uint64& nNewAddressCount)
{
    return dbAddress.GetAddressCount(hashFork, hashBlock, nAddressCount, nNewAddressCount);
}

bool CBlockDB::ListFunctionAddress(const uint256& hashFork, const uint256& hashBlock, std::map<uint32, CFunctionAddressContext>& mapFunctionAddress)
{
    return dbAddress.ListFunctionAddress(hashFork, hashBlock, mapFunctionAddress);
}

bool CBlockDB::RetrieveFunctionAddress(const uint256& hashFork, const uint256& hashBlock, const uint32 nFuncId, CFunctionAddressContext& ctxFuncAddress)
{
    return dbAddress.RetrieveFunctionAddress(hashFork, hashBlock, nFuncId, ctxFuncAddress);
}

bool CBlockDB::RetrieveBlsPubkeyContext(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, uint384& blsPubkey)
{
    return dbAddress.RetrieveBlsPubkeyContext(hashFork, hashBlock, dest, blsPubkey);
}

bool CBlockDB::GetOwnerLinkTemplateAddress(const uint256& hashFork, const uint256& hashBlock, const CDestination& destOwner, std::map<CDestination, uint8>& mapTemplateAddress)
{
    return dbAddress.GetOwnerLinkTemplateAddress(hashFork, hashBlock, destOwner, mapTemplateAddress);
}

bool CBlockDB::GetDelegateLinkTemplateAddress(const uint256& hashFork, const uint256& hashBlock, const CDestination& destDelegate, const uint32 nTemplateType, const uint64 nBegin, const uint64 nCount, std::vector<std::pair<CDestination, uint8>>& vTemplateAddress)
{
    return dbAddress.GetDelegateLinkTemplateAddress(hashFork, hashBlock, destDelegate, nTemplateType, nBegin, nCount, vTemplateAddress);
}

bool CBlockDB::ClearAddressDbUnavailableNode(const uint256& hashFork, const uint32 nClearRefHeight)
{
    return dbAddress.ClearAddressUnavailableNode(hashFork, nClearRefHeight);
}

bool CBlockDB::GetSnapshotAddressData(const uint256& hashFork, const std::vector<uint256>& vBlockHash, bytes& btSnapData)
{
    return dbAddress.GetSnapshotAddressData(hashFork, vBlockHash, btSnapData);
}

bool CBlockDB::RecoveryAddressData(const bytes& btSnapData)
{
    return dbAddress.RecoveryAddressData(btSnapData);
}

bool CBlockDB::AddCodeContext(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock,
                              const std::map<uint256, CContractSourceCodeContext>& mapSourceCode,
                              const std::map<uint256, CContractCreateCodeContext>& mapContractCreateCode,
                              const std::map<uint256, CContractRunCodeContext>& mapContractRunCode,
                              const std::map<uint256, CTemplateContext>& mapTemplateData,
                              uint256& hashCodeRoot)
{
    return dbAddress.AddCodeContext(hashFork, hashPrevBlock, hashBlock, mapSourceCode, mapContractCreateCode, mapContractRunCode, mapTemplateData, hashCodeRoot);
}

bool CBlockDB::RetrieveSourceCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashSourceCode, CContractSourceCodeContext& ctxtCode)
{
    return dbAddress.RetrieveSourceCodeContext(hashFork, hashBlock, hashSourceCode, ctxtCode);
}

bool CBlockDB::RetrieveContractCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractCreateCode, CContractCreateCodeContext& ctxtCode)
{
    return dbAddress.RetrieveContractCreateCodeContext(hashFork, hashBlock, hashContractCreateCode, ctxtCode);
}

bool CBlockDB::ListContractCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, std::map<uint256, CContractCreateCodeContext>& mapContractCreateCode)
{
    return dbAddress.ListContractCreateCodeContext(hashFork, hashBlock, mapContractCreateCode);
}

bool CBlockDB::AddAddressTxInfo(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, const uint64 nBlockNumber,
                                const std::map<CDestination, std::vector<CDestTxInfo>>& mapAddressTxInfo, const std::map<CDestination, std::vector<CTokenTransRecord>>& mapTokenRecord)
{
    if (fCfgFullDb)
    {
        return dbAddressTxInfo.AddAddressTxInfo(hashFork, hashPrevBlock, hashBlock, nBlockNumber, mapAddressTxInfo, mapTokenRecord);
    }
    return false;
}

bool CBlockDB::UpdateAddressTxInfoBlockLongChain(const uint256& hashFork, const std::vector<uint256>& vRemoveBlock, const std::vector<uint256>& vAddBlock)
{
    if (fCfgFullDb)
    {
        return dbAddressTxInfo.UpdateAddressTxInfoBlockLongChain(hashFork, vRemoveBlock, vAddBlock);
    }
    return false;
}

bool CBlockDB::GetAddressTxCount(const uint256& hashFork, const CDestination& dest, uint64& nTxCount)
{
    if (fCfgFullDb)
    {
        return dbAddressTxInfo.GetAddressTxCount(hashFork, dest, nTxCount);
    }
    return false;
}

bool CBlockDB::RetrieveAddressTxInfo(const uint256& hashFork, const CDestination& dest, const uint64 nTxIndex, CDestTxInfo& ctxtAddressTxInfo)
{
    if (fCfgFullDb)
    {
        return dbAddressTxInfo.RetrieveAddressTxInfo(hashFork, dest, nTxIndex, ctxtAddressTxInfo);
    }
    return false;
}

bool CBlockDB::ListAddressTxInfo(const uint256& hashFork, const CDestination& dest, const uint64 nBeginTxIndex, const uint64 nGetTxCount, const bool fReverse, std::vector<CDestTxInfo>& vAddressTxInfo)
{
    if (fCfgFullDb)
    {
        return dbAddressTxInfo.ListAddressTxInfo(hashFork, dest, nBeginTxIndex, nGetTxCount, fReverse, vAddressTxInfo);
    }
    return false;
}

bool CBlockDB::ListTokenTx(const uint256& hashFork, const CDestination& destContractAddress, const CDestination& destUserAddress, const uint64 nPageNumber, const uint64 nPageSize,
                           const bool fReverse, uint64& nTotalRecordCount, uint64& nPageCount, std::vector<std::pair<uint64, CTokenTransRecord>>& vTokenTxRecord)
{
    if (fCfgFullDb)
    {
        return dbAddressTxInfo.ListTokenTx(hashFork, destContractAddress, destUserAddress, nPageNumber, nPageSize, fReverse, nTotalRecordCount, nPageCount, vTokenTxRecord);
    }
    return false;
}

bool CBlockDB::WalkThroughSnapshotAddressTxKv(const uint256& hashFork, const uint64 nLastBlockNumber, WalkerAddressTxKvFunc fnWalker)
{
    if (fCfgFullDb)
    {
        return dbAddressTxInfo.WalkThroughSnapshotAddressTxKv(hashFork, nLastBlockNumber, fnWalker);
    }
    return false;
}

bool CBlockDB::WalkThroughSnapshotTokenTxKv(const uint256& hashFork, const uint64 nLastBlockNumber, WalkerTokenTxKvFunc fnWalker)
{
    if (fCfgFullDb)
    {
        return dbAddressTxInfo.WalkThroughSnapshotTokenTxKv(hashFork, nLastBlockNumber, fnWalker);
    }
    return false;
}

bool CBlockDB::WriteSnapshotAddressTxKvData(const uint256& hashFork, const bytes& btKey, const bytes& btValue)
{
    if (fCfgFullDb)
    {
        return dbAddressTxInfo.WriteSnapshotAddressTxKvData(hashFork, btKey, btValue);
    }
    return false;
}

bool CBlockDB::WriteSnapshotAddressTxCount(const uint256& hashFork, const uint256& hashLastBlock, const std::map<CDestination, uint64>& mapAddressTxCount)
{
    if (fCfgFullDb)
    {
        return dbAddressTxInfo.WriteSnapshotAddressTxCount(hashFork, hashLastBlock, mapAddressTxCount);
    }
    return false;
}

bool CBlockDB::WriteSnapshotTokenTxCount(const uint256& hashFork, const std::map<CDestination, std::map<CDestination, uint64>>& mapTokenTxCount)
{
    if (fCfgFullDb)
    {
        return dbAddressTxInfo.WriteSnapshotTokenTxCount(hashFork, mapTokenTxCount);
    }
    return false;
}

bool CBlockDB::AddBlockContractTraceData(const uint256& hashFork, const uint256& hashBlock, const BlockContractReceipts& vContractReceipts, const BlockContractPrevState& vContractPrevAddressState)
{
    if (fCfgTraceDb)
    {
        return dbTrace.AddBlockContractTraceData(hashFork, hashBlock, vContractReceipts, vContractPrevAddressState);
    }
    return false;
}

bool CBlockDB::AddBlockContractKvData(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDestination, std::map<uint256, bytes>>& mapTraceContractKvData)
{
    if (fCfgTraceDb)
    {
        return dbTrace.AddBlockContractKvData(hashFork, hashPrevBlock, hashBlock, mapTraceContractKvData);
    }
    return false;
}

bool CBlockDB::AddVoteReward(const uint256& hashFork, const uint32 nChainId, const uint256& hashPrevBlock, const uint256& hashBlock, const uint32 nBlockHeight,
                             const std::map<CDestination, uint256>& mapVoteReward, const CDestination& destMint, const uint8 nMintTemplateType, const uint256& nMintReward, uint256& hashNewRoot)
{
    return dbVote.AddVoteReward(hashFork, nChainId, hashPrevBlock, hashBlock, nBlockHeight, mapVoteReward, hashNewRoot);
}

bool CBlockDB::ListVoteReward(const uint32 nChainId, const uint256& hashBlock, const CDestination& dest, const uint32 nGetCount, std::vector<std::pair<uint32, uint256>>& vVoteReward)
{
    return dbVote.ListVoteReward(nChainId, hashBlock, dest, nGetCount, vVoteReward);
}

bool CBlockDB::AddBlacklistAddress(const CDestination& dest)
{
    return dbAddressBlacklist.AddAddress(dest);
}

void CBlockDB::RemoveBlacklistAddress(const CDestination& dest)
{
    dbAddressBlacklist.RemoveAddress(dest);
}

bool CBlockDB::IsExistBlacklistAddress(const CDestination& dest)
{
    return dbAddressBlacklist.IsExist(dest);
}

void CBlockDB::ListBlacklistAddress(set<CDestination>& setAddressOut)
{
    dbAddressBlacklist.ListAddress(setAddressOut);
}

bool CBlockDB::UpdateForkMintMinGasPrice(const uint256& hashFork, const uint256& nMinGasPrice)
{
    return dbMintMinGasPrice.UpdateForkMintMinGasPrice(hashFork, nMinGasPrice);
}

bool CBlockDB::GetForkMintMinGasPrice(const uint256& hashFork, uint256& nMinGasPrice)
{
    return dbMintMinGasPrice.GetForkMintMinGasPrice(hashFork, nMinGasPrice);
}

bool CBlockDB::IsSnapshotBlock(const uint256& hashBlock)
{
    return dbSnapshot.IsSnapshotBlock(hashBlock);
}

bool CBlockDB::StartSnapshot(const uint256& hashLastBlock, const uint32 nMaxSnapshots)
{
    return dbSnapshot.StartSnapshot(hashLastBlock, nMaxSnapshots);
}

bool CBlockDB::SaveBlockFile(const uint256& hashLastBlock, const uint32 nFile, const uint32 nOffset)
{
    return dbSnapshot.SaveBlockFile(hashLastBlock, nFile, nOffset);
}

bool CBlockDB::SaveSnapshotData(const uint256& hashLastBlock, const uint8 nDataType, const char* pSnapData, const uint32 nSnapDataSize)
{
    return dbSnapshot.SaveSnapshotData(hashLastBlock, nDataType, pSnapData, nSnapDataSize);
}

bool CBlockDB::GetSnapshotFileList(const uint256& hashSnapBlock, std::vector<CSnapshotFileInfo>& vSnapFilelist)
{
    return dbSnapshot.GetSnapshotFileList(hashSnapBlock, vSnapFilelist);
}

bool CBlockDB::ReadSnapshotFileData(const uint256& hashSnapBlock, const std::string& strFileName, const uint64 nOffset, const uint64 nReadSize, bytes& btReadData)
{
    return dbSnapshot.ReadSnapshotFileData(hashSnapBlock, strFileName, nOffset, nReadSize, btReadData);
}

bool CBlockDB::GetSnapshotDownFileList(const uint256& hashSnapBlock, std::vector<CSnapshotFileInfo>& vSnapFilelist)
{
    return dbSnapshot.GetSnapshotDownFileList(hashSnapBlock, vSnapFilelist);
}

bool CBlockDB::RemoveSnapshotDownBlock(const uint256& hashSnapBlock)
{
    return dbSnapshot.RemoveSnapshotDownBlock(hashSnapBlock);
}

uint64 CBlockDB::GetSnapshotDownFileSize(const uint256& hashSnapBlock, const std::string& strFileName)
{
    return dbSnapshot.GetSnapshotDownFileSize(hashSnapBlock, strFileName);
}

bool CBlockDB::WriteSnapshotDownFileData(const uint256& hashSnapBlock, const std::string& strFileName, const uint64 nOffset, const bytes& btWriteData)
{
    return dbSnapshot.WriteSnapshotDownFileData(hashSnapBlock, strFileName, nOffset, btWriteData);
}

bool CBlockDB::VerifyBlockRoot(const bool fPrimary, const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock,
                               const uint256& hashLocalStateRoot, CBlockRoot& localBlockRoot, const bool fVerifyAllNode)
{
    if (fPrimary)
    {
        if (!dbFork.VerifyForkContext(hashPrevBlock, hashBlock, localBlockRoot.hashForkContextRoot, fVerifyAllNode))
        {
            StdError("CBlockDB", "Verify block root: Verify fork context fail, block: %s", hashBlock.GetHex().c_str());
            return false;
        }
        if (!dbVote.VerifyDelegateVote(hashPrevBlock, hashBlock, localBlockRoot.hashDelegateRoot, fVerifyAllNode))
        {
            StdError("CBlockDB", "Verify block root: Verify delegate fail, block: %s", hashBlock.GetHex().c_str());
            return false;
        }
        if (!dbVote.VerifyVote(hashPrevBlock, hashBlock, localBlockRoot.hashVoteRoot, fVerifyAllNode))
        {
            StdError("CBlockDB", "Verify block root: Verify vote fail, block: %s", hashBlock.GetHex().c_str());
            return false;
        }
    }
    if (!dbState.VerifyState(hashFork, hashLocalStateRoot, fVerifyAllNode))
    {
        StdError("CBlockDB", "Verify block root: Verify state fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    localBlockRoot.hashStateRoot = hashLocalStateRoot;
    if (!dbAddress.VerifyAddressContext(hashFork, hashPrevBlock, hashBlock, localBlockRoot.hashAddressRoot, fVerifyAllNode))
    {
        StdError("CBlockDB", "Verify block root: Verify address context fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    if (!dbContract.VerifyCodeContext(hashFork, hashPrevBlock, hashBlock, localBlockRoot.hashCodeRoot, fVerifyAllNode))
    {
        StdError("CBlockDB", "Verify block root: Verify code context fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    if (!dbBlockIndex.VerifyBlockNumberContext(hashFork, hashPrevBlock, hashBlock, localBlockRoot.hashBlockNumberRoot, fVerifyAllNode))
    {
        StdError("CBlockDB", "Verify block root: Verify blocknumber fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    if (!dbTxIndex.VerifyTxIndex(hashFork, hashPrevBlock, hashBlock, localBlockRoot.hashTxIndexRoot, fVerifyAllNode))
    {
        StdError("CBlockDB", "Verify block root: Verify txindex fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    if (!dbVote.VerifyVoteReward(hashFork, hashPrevBlock, hashBlock, localBlockRoot.hashVoteRewardRoot, fVerifyAllNode))
    {
        StdError("CBlockDB", "Verify block root: Verify reward lock fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    if (fCfgFullDb)
    {
        uint256 hashAddressTxInfoRoot;
        if (!dbAddressTxInfo.VerifyAddressTxInfo(hashFork, hashPrevBlock, hashBlock, hashAddressTxInfoRoot, fVerifyAllNode))
        {
            StdError("CBlockDB", "Verify block root: Verify address tx info fail, block: %s", hashBlock.GetHex().c_str());
            return false;
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////
bool CBlockDB::LoadAllFork()
{
    map<uint256, CForkContext> mapForkCtxt;
    if (!dbFork.ListForkContext(mapForkCtxt))
    {
        StdLog("CBlockDB", "Load all fork: ListForkContext fail");
        return false;
    }

    for (const auto& kv : mapForkCtxt)
    {
        if (!dbTxIndex.LoadFork(kv.first))
        {
            StdLog("CBlockDB", "Load all fork: dbTxIndex LoadFork fail");
            return false;
        }
        if (!dbState.LoadFork(kv.first))
        {
            StdLog("CBlockDB", "Load all fork: dbState LoadFork fail");
            return false;
        }
        if (!dbAddress.LoadFork(kv.first))
        {
            StdLog("CBlockDB", "Load all fork: dbAddress LoadFork fail");
            return false;
        }
        if (!dbContract.LoadFork(kv.first))
        {
            StdLog("CBlockDB", "Load all fork: dbContract LoadFork fail");
            return false;
        }
        if (fCfgFullDb)
        {
            if (!dbAddressTxInfo.LoadFork(kv.first))
            {
                StdLog("CBlockDB", "Load all fork: dbAddressTxInfo LoadFork fail");
                return false;
            }
        }
    }
    return true;
}

} // namespace storage
} // namespace hashahead
