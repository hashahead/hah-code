// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blockbase.h"

#include <boost/timer/timer.hpp>
#include <cstdio>

#include "bloomfilter/bloomfilter.h"
#include "dbstruct.h"
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
using namespace hnbase;
using namespace hashahead::hvm;

namespace fs = boost::filesystem;

namespace hashahead
{
namespace storage
{

//////////////////////////////
// CBlockBaseDBWalker

class CBlockWalker : public CBlockDBWalker
{
public:
    CBlockWalker(CBlockBase* pBaseIn)
      : pBase(pBaseIn) {}
    bool Walk(CBlockIndex& outline) override
    {
        return !!(pBase->LoadBlockIndex(outline));
    }

public:
    CBlockBase* pBase;
};

//////////////////////////////
// CContractHostDB

bool CContractHostDB::Get(const uint256& key, bytes& value) const
{
    return blockState.GetDestKvData(destStorageContract, key, value);
}

bool CContractHostDB::GetTransientValue(const CDestination& dest, const uint256& key, bytes& value) const
{
    return blockState.GetTransientValue(dest, key, value);
}

void CContractHostDB::SetTransientValue(const CDestination& dest, const uint256& key, const bytes& value)
{
    blockState.SetTransientValue(dest, key, value);
}

uint256 CContractHostDB::GetTxid() const
{
    return txid;
}

uint256 CContractHostDB::GetTxNonce() const
{
    return nTxNonce;
}

uint64 CContractHostDB::GetAddressLastTxNonce(const CDestination& addr)
{
    return blockState.GetAddressLastTxNonce(addr);
}

bool CContractHostDB::SetAddressLastTxNonce(const CDestination& addr, const uint64 nNonce)
{
    return blockState.SetAddressLastTxNonce(addr, nNonce);
}

CDestination CContractHostDB::GetStorageContractAddress() const
{
    return destStorageContract;
}

CDestination CContractHostDB::GetCodeParentAddress() const
{
    return destCodeParent;
}

CDestination CContractHostDB::GetCodeLocalAddress() const
{
    return destCodeLocal;
}

CDestination CContractHostDB::GetCodeOwnerAddress() const
{
    return destCodeOwner;
}

bool CContractHostDB::GetBalance(const CDestination& addr, uint256& balance) const
{
    if (!blockState.GetDestBalance(addr, balance))
    {
        StdLog("CContractHostDB", "Get Balance: Get dest state fail, dest: %s", addr.ToString().c_str());
        return false;
    }
    return true;
}

bool CContractHostDB::ContractTransfer(const CDestination& from, const CDestination& to, const uint256& amount, const uint64 nGasLimit, uint64& nGasLeft)
{
    CAddressContext ctxToAddress;
    if (!blockState.GetAddressContext(to, ctxToAddress))
    {
        StdLog("CContractHostDB", "Contract transfer: Get to address context fail, to: %s", to.ToString().c_str());
        ctxToAddress = CAddressContext(CPubkeyAddressContext());
    }
    if (!blockState.ContractTransfer(from, to, amount, nGasLimit, nGasLeft, ctxToAddress, CContractTransfer::CT_CONTRACT))
    {
        StdLog("CContractHostDB", "Contract transfer: Contract transfer fail, from: %s, to: %s", from.ToString().c_str(), to.ToString().c_str());
        return false;
    }
    return true;
}

uint256 CContractHostDB::GetBlockHash(const uint64 nBlockNumber)
{
    uint256 hashBlock;
    if (!blockState.GetBlockHashByNumber(nBlockNumber, hashBlock))
    {
        StdLog("CContractHostDB", "GetBlockHash: Get block hash fail, number: %lu", nBlockNumber);
        return uint256();
    }
    return hashBlock;
}

bool CContractHostDB::IsContractAddress(const CDestination& addr)
{
    return blockState.IsContractAddress(addr);
}

bool CContractHostDB::GetContractRunCode(const CDestination& destContractIn, uint256& hashContractCreateCode, CDestination& destCodeOwner, uint256& hashContractRunCode, bytes& btContractRunCode, bool& fDestroy)
{
    return blockState.GetContractRunCode(destContractIn, hashContractCreateCode, destCodeOwner, hashContractRunCode, btContractRunCode, fDestroy);
}

bool CContractHostDB::GetContractCreateCode(const CDestination& destContractIn, CTxContractData& txcd)
{
    return blockState.GetContractCreateCode(destContractIn, txcd);
}

bool CContractHostDB::SaveContractRunCode(const bytes& btContractRunCode, const CTxContractData& txcd)
{
    return blockState.SaveContractRunCode(destStorageContract, btContractRunCode, txcd, txid);
}

bool CContractHostDB::ExecFunctionContract(const CDestination& destFromIn, const CDestination& destToIn, const uint256& nAmount, const bytes& btData, const uint64 nGasLimit, uint64& nGasLeft, int& nStatus, bytes& btResult)
{
    return blockState.ExecFunctionContract(destFromIn, destToIn, nAmount, btData, nGasLimit, nGasLeft, nStatus, btResult);
}

bool CContractHostDB::Selfdestruct(const CDestination& destBeneficiaryIn)
{
    return blockState.Selfdestruct(destStorageContract, destBeneficiaryIn);
}

CVmHostFaceDBPtr CContractHostDB::CloneHostDB(const CDestination& destStorageContractIn, const CDestination& destCodeParentIn, const CDestination& destCodeLocalIn, const CDestination& destCodeOwnerIn)
{
    return CVmHostFaceDBPtr(new CContractHostDB(blockState, destStorageContractIn, destCodeParentIn, destCodeLocalIn, destCodeOwnerIn, txid, nTxNonce));
}

void CContractHostDB::ModifyHostAddress(const CDestination& destCodeParentIn, const CDestination& destCodeLocalIn, const CDestination& destCodeOwnerIn)
{
    destCodeParent = destCodeParentIn;
    destCodeLocal = destCodeLocalIn;
    destCodeOwner = destCodeOwnerIn;
}

void CContractHostDB::SaveCodeOwnerGasUsed(const CDestination& destParentCodeContractIn, const CDestination& destCodeContractIn, const CDestination& destCodeOwnerIn, const uint64 nGasUsed)
{
    blockState.SaveCodeOwnerGasUsed(destParentCodeContractIn, destCodeContractIn, destCodeOwnerIn, nGasUsed);
}

void CContractHostDB::SaveRunResult(const std::vector<CTransactionLogs>& vLogsIn, const std::map<uint256, bytes>& mapCacheKv,
                                    const std::map<uint256, bytes>& mapTraceKv, const std::map<CDestination, std::map<uint256, bytes>>& mapOldAddressKeyValue)
{
    blockState.SaveRunResult(destStorageContract, vLogsIn, mapCacheKv, mapTraceKv, mapOldAddressKeyValue);
}

bool CContractHostDB::AddContractRunReceipt(const CTxContractReceipt& tcReceipt, const bool fFirstReceipt)
{
    return blockState.AddContractRunReceipt(tcReceipt, fFirstReceipt);
}

bool CContractHostDB::AddVmOperationTraceLog(const CVmOperationTraceLog& vmOpTraceLog)
{
    return blockState.AddVmOperationTraceLog(vmOpTraceLog);
}

//////////////////////////////
// CCacheBlockReceipt

void CCacheBlockReceipt::AddBlockReceiptCache(const uint256& hashBlock, const std::vector<CTransactionReceipt>& vBlockReceipt)
{
    CWriteLock wlock(rwBrcAccess);

    const CChainId nChainId = CBlock::GetBlockChainIdByHash(hashBlock);
    auto it = mapBlockReceiptCache.find(nChainId);
    if (it == mapBlockReceiptCache.end())
    {
        it = mapBlockReceiptCache.insert(std::make_pair(nChainId, std::map<uint256, std::vector<CTransactionReceipt>, CustomBlockHashCompare>())).first;
    }
    auto& mapReceipts = it->second;
    while (mapReceipts.size() >= MAX_CACHE_BLOCK_RECEIPT_COUNT)
    {
        mapReceipts.erase(mapReceipts.begin());
    }
    mapReceipts[hashBlock] = vBlockReceipt;
}

bool CCacheBlockReceipt::GetBlockReceiptCache(const uint256& hashBlock, std::vector<CTransactionReceipt>& vBlockReceipt)
{
    CReadLock rlock(rwBrcAccess);

    auto it = mapBlockReceiptCache.find(CBlock::GetBlockChainIdByHash(hashBlock));
    if (it == mapBlockReceiptCache.end())
    {
        return false;
    }
    auto mt = it->second.find(hashBlock);
    if (mt == it->second.end())
    {
        return false;
    }
    vBlockReceipt = mt->second;
    return true;
}

//////////////////////////////
// CBlockBase

CBlockBase::CBlockBase()
  : fCfgFullDb(false), fCfgTraceDb(false), fCfgCacheTrace(false), fCfgRewardCheck(false), fCfgPrune(false)
{
}

CBlockBase::~CBlockBase()
{
    dbBlock.BdDeinitialize();
    tsBlock.Deinitialize();
}

bool CBlockBase::BsInitialize(const fs::path& pathDataLocation, const uint256& hashGenesisBlockIn, const bool fFullDbIn, const bool fTraceDbIn,
                              const bool fCacheTrace, const bool fRewardCheckIn, const bool fRenewDB, const bool fPruneDb)
{
    hashGenesisBlock = hashGenesisBlockIn;
    fCfgFullDb = fFullDbIn;
    fCfgTraceDb = fTraceDbIn;
    fCfgCacheTrace = fCacheTrace;
    fCfgRewardCheck = fRewardCheckIn;
    fCfgPrune = fPruneDb;

    StdLog("BlockBase", "Initializing... (Path : %s)", pathDataLocation.string().c_str());

    if (!dbBlock.BdInitialize(pathDataLocation, hashGenesisBlockIn, fFullDbIn, fTraceDbIn, fCacheTrace, fPruneDb))
    {
        StdError("BlockBase", "Failed to initialize block db");
        return false;
    }

    if (!dbBlock.SetPruneFlag(fPruneDb))
    {
        if (fPruneDb)
        {
            StdError("BlockBase", "Database has not been set to prune status. Please set prune to FALSE");
        }
        else
        {
            StdError("BlockBase", "Database is already in prune state. Please set prune to TRUE");
        }
        return false;
    }

    if (!tsBlock.Initialize(pathDataLocation / "block", BLOCKFILE_PREFIX))
    {
        dbBlock.BdDeinitialize();
        StdError("BlockBase", "Failed to initialize block tsfile");
        return false;
    }

    if (fRenewDB)
    {
        Clear();
    }
    else if (!LoadDB())
    {
        dbBlock.BdDeinitialize();
        tsBlock.Deinitialize();
        {
            CWriteLock wlock(rwAccess);

            ClearCache();
        }
        StdError("BlockBase", "Failed to load block db");
        return false;
    }

    setRunSysFlag(hashGenesisBlock);

    StdLog("BlockBase", "Initialized");
    return true;
}

void CBlockBase::BsDeinitialize()
{
    dbBlock.BdDeinitialize();
    tsBlock.Deinitialize();
    {
        CWriteLock wlock(rwAccess);

        ClearCache();
    }
    StdLog("BlockBase", "Deinitialized");
}

const uint256& CBlockBase::GetGenesisBlockHash() const
{
    return hashGenesisBlock;
}

bool CBlockBase::Exists(const uint256& hash)
{
    CReadLock rlock(rwAccess);

    return (!!GetIndex(hash));
}

bool CBlockBase::ExistsTx(const uint256& hashFork, const uint256& txid)
{
    uint256 hashAtFork;
    uint256 hashTxAtBlock;
    CTxIndex txIndex;
    return GetTxIndex(hashFork, txid, hashAtFork, hashTxAtBlock, txIndex);
}

bool CBlockBase::IsEmpty()
{
    CReadLock rlock(rwAccess);

    return !GetIndex(hashGenesisBlock);
}

void CBlockBase::Clear()
{
    CWriteLock wlock(rwAccess);

    dbBlock.RemoveAll();
    ClearCache();
}

bool CBlockBase::Initiate(const uint256& hashGenesis, const CBlock& blockGenesis, const uint256& nChainTrust)
{
    if (!IsEmpty())
    {
        StdError("BlockBase", "Add genesis block: Is not empty");
        return false;
    }
    return true;
}

bool CBlockBase::CheckForkLongChain(const uint256& hashFork, const uint256& hashBlock, const CBlockEx& block, const BlockIndexPtr pIndexNew)
{
    if (!pIndexNew->IsPrimary() && !pIndexNew->IsOrigin())
    {
        CProofOfPiggyback proof;
        if (!block.GetPiggybackProof(proof) || proof.hashRefBlock == 0)
        {
            StdLog("BlockBase", "Add long chain last: Load proof fail, block: %s", hashBlock.GetHex().c_str());
            return false;
        }

        BlockIndexPtr pIndexRef = GetIndex(proof.hashRefBlock);
        if (!pIndexRef || !pIndexRef->IsPrimary())
        {
            StdLog("BlockBase", "Add long chain last: Retrieve ref index fail, ref block: %s, block: %s",
                   proof.hashRefBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
            return false;
        }

        if (!VerifyRefBlock(GetGenesisBlockHash(), proof.hashRefBlock))
        {
            StdLog("BlockBase", "Add long chain last: Ref block short chain, refblock: %s, new block: %s, fork: %s",
                   proof.hashRefBlock.GetHex().c_str(), hashBlock.GetHex().c_str(), hashFork.GetHex().c_str());
            return false;
        }
    }
    if (!VerifyBlockConfirmChain(pIndexNew))
    {
        StdLog("BlockBase", "Add long chain last: Verify block confirm chain fail, new block: %s, fork: %s",
               hashBlock.GetHex().c_str(), hashFork.GetHex().c_str());
        return false;
    }

    BlockIndexPtr pIndexFork = GetForkLastIndex(hashFork);
    if (pIndexFork
        && (pIndexFork->nChainTrust > pIndexNew->nChainTrust
            || (pIndexFork->nChainTrust == pIndexNew->nChainTrust && !IsBlockIndexEquivalent(pIndexNew, pIndexFork))))
    {
        StdLog("BlockBase", "Add long chain last: Short chain, new block height: %d, block type: %s, block: %s, fork chain trust: %s, new block trust: %s, fork last block: %s, fork: %s",
               pIndexNew->GetBlockHeight(), GetBlockTypeStr(block.nType, block.txMint.GetTxType()).c_str(), hashBlock.GetHex().c_str(),
               pIndexFork->nChainTrust.GetValueHex().c_str(), pIndexNew->nChainTrust.GetValueHex().c_str(), pIndexFork->GetBlockHash().GetHex().c_str(), pIndexFork->GetOriginHash().GetHex().c_str());
        return false;
    }
    return true;
}

bool CBlockBase::Retrieve(const BlockIndexPtr pIndex, CBlock& block)
{
    block.SetNull();

    CBlockEx blockex;
    if (!tsBlock.Read(blockex, pIndex->nFile, pIndex->nOffset, true, true))
    {
        StdTrace("BlockBase", "RetrieveFromIndex::Read %s block failed, File: %d, Offset: %d",
                 pIndex->GetBlockHash().ToString().c_str(), pIndex->nFile, pIndex->nOffset);
        return false;
    }
    block = static_cast<CBlock>(blockex);
    return true;
}

bool CBlockBase::Retrieve(const uint256& hash, CBlockEx& block)
{
    block.SetNull();

    BlockIndexPtr pIndex;
    {
        CReadLock rlock(rwAccess);

        if (!(pIndex = GetIndex(hash)))
        {
            StdTrace("BlockBase", "Retrieve from block hash: Get index failed, block: %s", hash.ToString().c_str());
            return false;
        }
    }
    if (!tsBlock.Read(block, pIndex->nFile, pIndex->nOffset, true, true))
    {
        StdTrace("BlockBase", "Retrieve from block hash: Read block failed, block: %s", hash.ToString().c_str());

        return false;
    }
    return true;
}

bool CBlockBase::Retrieve(const BlockIndexPtr pIndex, CBlockEx& block)
{
    block.SetNull();
    if (!tsBlock.Read(block, pIndex->nFile, pIndex->nOffset, true, true))
    {
        StdTrace("BlockBase", "Retrieve from index: Get index failed, block: %s", pIndex->GetBlockHash().ToString().c_str());
        return false;
    }
    return true;
}

BlockIndexPtr CBlockBase::RetrieveIndex(const uint256& hashBlock)
{
    CReadLock rlock(rwAccess);
    return GetIndex(hashBlock);
}

BlockIndexPtr CBlockBase::RetrieveFork(const uint256& hashFork)
{
    CReadLock rlock(rwAccess);
    return GetForkLastIndex(hashFork);
}

BlockIndexPtr CBlockBase::RetrieveFork(const string& strName)
{
    std::map<uint256, CForkContext> mapForkCtxt;
    if (!dbBlock.ListForkContext(mapForkCtxt))
    {
        return nullptr;
    }
    for (const auto& kv : mapForkCtxt)
    {
        if (kv.second.strName == strName)
        {
            return RetrieveFork(kv.first);
        }
    }
    return nullptr;
}

bool CBlockBase::RetrieveProfile(const uint256& hashFork, CProfile& profile, const uint256& hashMainChainRefBlock)
{
    CForkContext ctxt;
    if (!dbBlock.RetrieveForkContext(hashFork, ctxt, hashMainChainRefBlock))
    {
        StdTrace("BlockBase", "Retrieve Profile: Retrieve fork context fail, fork: %s", hashFork.ToString().c_str());
        return false;
    }
    profile = ctxt.GetProfile();
    return true;
}

bool CBlockBase::RetrieveAncestry(const uint256& hashFork, vector<pair<uint256, uint256>> vAncestry)
{
    CForkContext ctxt;
    if (!dbBlock.RetrieveForkContext(hashFork, ctxt))
    {
        StdTrace("BlockBase", "Ancestry Retrieve hashFork %s failed", hashFork.ToString().c_str());
        return false;
    }

    while (ctxt.hashParent != 0)
    {
        vAncestry.push_back(make_pair(ctxt.hashParent, ctxt.hashJoint));
        if (!dbBlock.RetrieveForkContext(ctxt.hashParent, ctxt))
        {
            return false;
        }
    }

    std::reverse(vAncestry.begin(), vAncestry.end());
    return true;
}

bool CBlockBase::RetrieveOrigin(const uint256& hashFork, CBlock& block)
{
    block.SetNull();

    CForkContext ctxt;
    if (!dbBlock.RetrieveForkContext(hashFork, ctxt))
    {
        StdTrace("BlockBase", "Retrieve Origin: Retrieve Fork Context %s block failed", hashFork.ToString().c_str());
        return false;
    }

    uint256 hashAtFork;
    uint256 hashTxAtBlock;
    CTxIndex txIndex;
    CTransaction tx;
    if (!RetrieveTxAndIndex(GetGenesisBlockHash(), ctxt.txidEmbedded, tx, hashAtFork, hashTxAtBlock, txIndex))
    {
        StdTrace("BlockBase", "Retrieve Origin: Retrieve Tx %s tx failed", ctxt.txidEmbedded.ToString().c_str());
        return false;
    }

    bytes btTempData;
    if (!tx.GetTxData(CTransaction::DF_FORKDATA, btTempData))
    {
        StdTrace("BlockBase", "Retrieve Origin: fork data error, txid: %s", ctxt.txidEmbedded.ToString().c_str());
        return false;
    }

    try
    {
        CBufStream ss(btTempData);
        ss >> block;
    }
    catch (exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CBlockBase::RetrieveTxAndIndex(const uint256& hashFork, const uint256& txid, CTransaction& tx, uint256& hashAtFork, uint256& hashTxAtBlock, CTxIndex& txIndex)
{
    if (!GetTxIndex(hashFork, txid, hashAtFork, hashTxAtBlock, txIndex))
    {
        StdLog("BlockBase", "Retrieve Tx: Get tx index fail, txid: %s, fork: %s", txid.ToString().c_str(), hashFork.ToString().c_str());
        return false;
    }
    tx.SetNull();
    if (!tsBlock.Read(tx, txIndex.nFile, txIndex.nOffset, false, true))
    {
        StdLog("BlockBase", "Retrieve Tx: Read fail, txid: %s, fork: %s", txid.ToString().c_str(), hashFork.ToString().c_str());
        return false;
    }
    return true;
}

bool CBlockBase::RetrieveAvailDelegate(const uint256& hash, int height, const vector<uint256>& vBlockRange,
                                       const uint256& nMinEnrollAmount,
                                       map<CDestination, size_t>& mapWeight,
                                       map<CDestination, vector<unsigned char>>& mapEnrollData,
                                       vector<pair<CDestination, uint256>>& vecAmount)
{
    map<CDestination, uint256> mapVote;
    if (!dbBlock.RetrieveDelegate(hash, mapVote))
    {
        StdTrace("BlockBase", "Retrieve Avail Delegate: Retrieve Delegate %s block failed",
                 hash.ToString().c_str());
        return false;
    }

    map<CDestination, CDiskPos> mapEnrollTxPos;
    if (!dbBlock.RetrieveRangeEnroll(height, vBlockRange, mapEnrollTxPos))
    {
        StdTrace("BlockBase", "Retrieve Avail Retrieve Enroll block %s height %d failed",
                 hash.ToString().c_str(), height);
        return false;
    }

    map<pair<uint256, CDiskPos>, pair<CDestination, vector<uint8>>> mapSortEnroll;
    for (auto it = mapVote.begin(); it != mapVote.end(); ++it)
    {
        if ((*it).second >= nMinEnrollAmount)
        {
            const CDestination& dest = (*it).first;
            map<CDestination, CDiskPos>::iterator mi = mapEnrollTxPos.find(dest);
            if (mi != mapEnrollTxPos.end())
            {
                CTransaction tx;
                if (!tsBlock.Read(tx, (*mi).second, false, true))
                {
                    StdLog("BlockBase", "Retrieve Avail Delegate::Read %s tx failed", tx.GetHash().ToString().c_str());
                    return false;
                }

                bytes btCertData;
                if (!tx.GetTxData(CTransaction::DF_CERTTXDATA, btCertData))
                {
                    StdLog("BlockBase", "Retrieve Avail Delegate: tx data error1, txid: %s",
                           tx.GetHash().ToString().c_str());
                    return false;
                }

                mapSortEnroll.insert(make_pair(make_pair(it->second, mi->second), make_pair(dest, btCertData)));
            }
        }
    }
    // first 25 destination sorted by amount and sequence
    for (auto it = mapSortEnroll.rbegin(); it != mapSortEnroll.rend() && mapWeight.size() < MAX_DELEGATE_THRESH; it++)
    {
        mapWeight.insert(make_pair(it->second.first, 1));
        mapEnrollData.insert(make_pair(it->second.first, it->second.second));
        vecAmount.push_back(make_pair(it->second.first, it->first.first));
    }
    for (const auto& d : vecAmount)
    {
        StdTrace("BlockBase", "Retrieve Avail Delegate: dest: %s, amount: %s",
                 d.first.ToString().c_str(), CoinToTokenBigFloat(d.second).c_str());
    }
    return true;
}

bool CBlockBase::RetrieveDestDelegateVote(const uint256& hashBlock, const CDestination& destDelegate, uint256& nVoteAmount)
{
    return dbBlock.RetrieveDestDelegateVote(hashBlock, destDelegate, nVoteAmount);
}

void CBlockBase::ListForkIndex(std::map<uint256, BlockIndexPtr>& mapForkIndex)
{
    CReadLock rlock(rwAccess);

    std::map<uint256, CForkContext> mapForkCtxt;
    if (!dbBlock.ListForkContext(mapForkCtxt))
    {
        return;
    }
    for (const auto& kv : mapForkCtxt)
    {
        BlockIndexPtr pIndex = GetForkLastIndex(kv.first);
        if (pIndex)
        {
            mapForkIndex.insert(make_pair(kv.first, pIndex));
        }
    }
}

void CBlockBase::ListForkIndex(multimap<int, BlockIndexPtr>& mapForkIndex)
{
    CReadLock rlock(rwAccess);

    std::map<uint256, CForkContext> mapForkCtxt;
    if (!dbBlock.ListForkContext(mapForkCtxt))
    {
        return;
    }
    for (const auto& kv : mapForkCtxt)
    {
        BlockIndexPtr pIndex = GetForkLastIndex(kv.first);
        if (pIndex)
        {
            BlockIndexPtr pOriginIndex = GetOriginBlockIndex(pIndex);
            if (pOriginIndex)
            {
                mapForkIndex.insert(make_pair(pOriginIndex->GetBlockHeight() - 1, pIndex));
            }
        }
    }
}

bool CBlockBase::GetBlockBranchList(const uint256& hashBlock, std::vector<CBlockEx>& vRemoveBlockInvertedOrder, std::vector<CBlockEx>& vAddBlockPositiveOrder)
{
    CReadLock rlock(rwAccess);

    return GetBlockBranchListNolock(GetIndex(hashBlock), 0, nullptr, vRemoveBlockInvertedOrder, vAddBlockPositiveOrder);
}

bool CBlockBase::GetPrevBlockHashList(const uint256& hashBlock, const uint32 nGetCount, std::vector<uint256>& vPrevBlockhash)
{
    CReadLock rlock(rwAccess);

    BlockIndexPtr pIndex = GetIndex(hashBlock);
    if (!pIndex)
    {
        return false;
    }
    pIndex = GetPrevBlockIndex(pIndex);
    while (pIndex && !pIndex->IsOrigin() && vPrevBlockhash.size() < (std::size_t)nGetCount)
    {
        vPrevBlockhash.push_back(pIndex->GetBlockHash());
        pIndex = GetPrevBlockIndex(pIndex);
    }
    return true;
}

BlockIndexPtr CBlockBase::LoadBlockIndex(const CBlockIndex& outline)
{
    BlockIndexPtr pIndex = MAKE_SHARED_BLOCK_INDEX(outline);
    if (pIndex)
    {
        if (!GetCacheBlockIndex(outline.GetBlockHash()))
        {
            AddCacheBlockIndex(pIndex);
        }
        return pIndex;
    }
    return nullptr;
}

bool CBlockBase::LoadTx(const uint32 nTxFile, const uint32 nTxOffset, CTransaction& tx)
{
    tx.SetNull();
    if (!tsBlock.Read(tx, nTxFile, nTxOffset, false, true))
    {
        StdTrace("BlockBase", "LoadTx::Read %s block failed", tx.GetHash().ToString().c_str());
        return false;
    }
    return true;
}

bool CBlockBase::AddBlockForkContext(const uint256& hashBlock, const CBlockEx& blockex, const std::map<CDestination, CAddressContext>& mapAddressContext, const std::map<std::string, CCoinContext>& mapNewSymbolCoin,
                                     const std::set<CDestination>& setTimeVaultWhitelist, const std::set<uint256>& setStopFork, uint256& hashNewRoot)
{
    map<uint256, CForkContext> mapNewForkCtxt;
    if (hashBlock == GetGenesisBlockHash())
    {
        CProfile profile;
        if (!blockex.GetForkProfile(profile))
        {
            StdError("BlockBase", "Add block fork context: Load genesis block proof failed, block: %s", hashBlock.ToString().c_str());
            return false;
        }

        CForkContext ctxt(hashBlock, uint256(), blockex.txMint.GetHash(), profile);
        ctxt.nJointHeight = 0;
        mapNewForkCtxt.insert(make_pair(hashBlock, ctxt));
    }
    else
    {
        vector<pair<CDestination, CForkContext>> vForkCtxt;
        for (size_t i = 0; i < blockex.vtx.size(); i++)
        {
            const CTransaction& tx = blockex.vtx[i];
            if (!tx.GetToAddress().IsNull())
            {
                auto it = mapAddressContext.find(tx.GetToAddress());
                if (it != mapAddressContext.end())
                {
                    if (it->second.IsTemplate() && it->second.GetTemplateType() == TEMPLATE_FORK)
                    {
                        if (!VerifyBlockForkTx(blockex.hashPrev, tx, vForkCtxt))
                        {
                            StdLog("BlockBase", "Add block fork context: Verify block fork tx fail, txid: %s, block: %s", tx.GetHash().ToString().c_str(), hashBlock.ToString().c_str());
                        }
                    }
                }
            }
            if (!tx.GetFromAddress().IsNull())
            {
                auto mt = mapAddressContext.find(tx.GetFromAddress());
                if (mt != mapAddressContext.end())
                {
                    if (mt->second.IsTemplate() && mt->second.GetTemplateType() == TEMPLATE_FORK)
                    {
                        auto it = vForkCtxt.begin();
                        while (it != vForkCtxt.end())
                        {
                            if (it->first == tx.GetFromAddress())
                            {
                                StdLog("BlockBase", "Add block fork context: cancel fork, block: %s, fork: %s, dest: %s",
                                       hashBlock.ToString().c_str(), it->second.hashFork.ToString().c_str(),
                                       tx.GetFromAddress().ToString().c_str());
                                it = vForkCtxt.erase(it);
                            }
                            else
                            {
                                ++it;
                            }
                        }
                    }
                }
            }
        }
        for (const auto& vd : vForkCtxt)
        {
            mapNewForkCtxt.insert(make_pair(vd.second.hashFork, vd.second));
        }
    }

    if (!dbBlock.AddForkContext(blockex.hashPrev, hashBlock, mapNewForkCtxt, mapNewSymbolCoin, setTimeVaultWhitelist, setStopFork, fCfgTraceDb, hashNewRoot))
    {
        StdLog("BlockBase", "Add block fork context: Add fork context to db fail, block: %s", hashBlock.ToString().c_str());
        return false;
    }
    return true;
}

bool CBlockBase::VerifyBlockForkTx(const uint256& hashPrev, const CTransaction& tx, vector<pair<CDestination, CForkContext>>& vForkCtxt)
{
    bytes btTempData;
    if (!tx.GetTxData(CTransaction::DF_FORKDATA, btTempData))
    {
        StdLog("BlockBase", "Verify block fork tx: tx data error, tx: %s", tx.GetHash().ToString().c_str());
        return false;
    }

    CBlock block;
    CProfile profile;
    try
    {
        CBufStream ss(btTempData);
        ss >> block;
    }
    catch (std::exception& e)
    {
        StdLog("BlockBase", "Verify block fork tx: invalid fork data, tx: %s", tx.GetHash().ToString().c_str());
        return false;
    }
    if (!block.IsOrigin() || block.IsPrimary())
    {
        StdLog("BlockBase", "Verify block fork tx: invalid block, tx: %s", tx.GetHash().ToString().c_str());
        return false;
    }
    if (!block.GetForkProfile(profile))
    {
        StdLog("BlockBase", "Verify block fork tx: invalid profile, tx: %s", tx.GetHash().ToString().c_str());
        return false;
    }
    uint256 hashNewFork = block.GetHash();

    do
    {
        CForkContext ctxtParent;
        if (!dbBlock.RetrieveForkContext(profile.hashParent, ctxtParent, hashPrev))
        {
            bool fFindParent = false;
            for (const auto& vd : vForkCtxt)
            {
                if (vd.second.hashFork == profile.hashParent)
                {
                    ctxtParent = vd.second;
                    fFindParent = true;
                    break;
                }
            }
            if (!fFindParent)
            {
                StdLog("BlockBase", "Verify block fork tx: Retrieve parent context, tx: %s", tx.GetHash().ToString().c_str());
                break;
            }
        }

        if (!VerifyOriginBlock(block, ctxtParent.GetProfile()))
        {
            StdLog("BlockBase", "Verify block fork tx: Verify origin fail, tx: %s", tx.GetHash().ToString().c_str());
            break;
        }

        if (!VerifyForkFlag(hashNewFork, profile.nChainId, profile.strSymbol, profile.strName, hashPrev))
        {
            StdLog("BlockBase", "Verify block fork tx: Verify fork flag fail, fork: %s, txid: %s, prev block: %s", hashNewFork.ToString().c_str(), tx.GetHash().ToString().c_str(), hashPrev.ToString().c_str());
            break;
        }

        bool fCheckRet = true;
        for (const auto& vd : vForkCtxt)
        {
            if (vd.second.hashFork == hashNewFork
                || vd.second.strSymbol == profile.strSymbol
                || vd.second.strName == profile.strName
                || vd.second.nChainId == profile.nChainId)
            {
                StdLog("BlockBase", "Verify block fork tx: fork existed or name repeated, tx: %s, new fork: %s, symbol: %s, name: %s, chainid: %d",
                       tx.GetHash().ToString().c_str(), hashNewFork.GetHex().c_str(), vd.second.strSymbol.c_str(), vd.second.strName.c_str(), profile.nChainId);
                fCheckRet = false;
                break;
            }
        }
        if (!fCheckRet)
        {
            break;
        }

        vForkCtxt.push_back(make_pair(tx.GetToAddress(), CForkContext(block.GetHash(), block.hashPrev, tx.GetHash(), profile)));
        StdLog("BlockBase", "Verify block fork tx success: valid fork: %s, tx: %s", hashNewFork.GetHex().c_str(), tx.GetHash().ToString().c_str());
    } while (0);

    return true;
}

bool CBlockBase::VerifyForkFlag(const uint256& hashNewFork, const CChainId nChainId, const std::string& strForkSymbol, const std::string& strForkName, const uint256& hashPrevBlock)
{
    if (hashNewFork != 0)
    {
        CForkContext ctxt;
        if (dbBlock.RetrieveForkContext(hashNewFork, ctxt, hashPrevBlock))
        {
            StdLog("BlockBase", "Verify fork flag: Fork id existed, fork: %s, symbol: %s, create txid: %s", hashNewFork.GetHex().c_str(), ctxt.strSymbol.c_str(), ctxt.txidEmbedded.ToString().c_str());
            return false;
        }
    }
    if (nChainId != 0)
    {
        uint256 hashTempFork;
        if (dbBlock.GetForkHashByChainId(nChainId, hashTempFork, hashPrevBlock))
        {
            StdLog("BlockBase", "Verify fork flag: Chain id existed, chainid: %d", nChainId);
            return false;
        }
    }
    if (!strForkSymbol.empty())
    {
        CCoinContext ctxCoin;
        if (dbBlock.GetForkCoinCtxByForkSymbol(strForkSymbol, ctxCoin, hashPrevBlock))
        {
            StdLog("BlockBase", "Verify fork flag: Symbol existed, symbol: %s", strForkSymbol.c_str());
            return false;
        }
    }
    if (!strForkName.empty())
    {
        uint256 hashTempFork;
        if (dbBlock.GetForkHashByForkName(strForkName, hashTempFork, hashPrevBlock))
        {
            StdLog("BlockBase", "Verify fork flag: Fork name existed, name: %s", strForkName.c_str());
            return false;
        }
    }
    return true;
}

bool CBlockBase::VerifyOriginBlock(const CBlock& block, const CProfile& parentProfile)
{
    CProfile forkProfile;
    if (!block.GetForkProfile(forkProfile))
    {
        StdLog("BlockBase", "Verify origin block: load profile error");
        return false;
    }
    if (forkProfile.IsNull())
    {
        StdLog("BlockBase", "Verify origin block: invalid profile");
        return false;
    }
    if (forkProfile.nChainId == 0 || forkProfile.nChainId != block.txMint.GetChainId())
    {
        StdLog("BlockBase", "Verify origin block: invalid chainid");
        return false;
    }
    if (!MoneyRange(forkProfile.nAmount))
    {
        StdLog("BlockBase", "Verify origin block: invalid fork amount");
        return false;
    }
    if (!RewardRange(forkProfile.nMintReward))
    {
        StdLog("BlockBase", "Verify origin block: invalid fork reward");
        return false;
    }
    if (block.txMint.GetToAddress() != forkProfile.destOwner)
    {
        StdLog("BlockBase", "Verify origin block: invalid fork to");
        return false;
    }
    // if (parentProfile.IsPrivate())
    // {
    //     if (!forkProfile.IsPrivate() || parentProfile.destOwner != forkProfile.destOwner)
    //     {
    //         StdLog("BlockBase", "Verify origin block: permission denied");
    //         return false;
    //     }
    // }
    return true;
}

bool CBlockBase::ListForkContext(std::map<uint256, CForkContext>& mapForkCtxt, const uint256& hashBlock)
{
    return dbBlock.ListForkContext(mapForkCtxt, hashBlock);
}

bool CBlockBase::RetrieveForkContext(const uint256& hashFork, CForkContext& ctxt, const uint256& hashMainChainRefBlock)
{
    return dbBlock.RetrieveForkContext(hashFork, ctxt, hashMainChainRefBlock);
}

bool CBlockBase::GetForkCtxStatus(const uint256& hashFork, CForkCtxStatus& forkStatus, const uint256& hashMainChainRefBlock)
{
    return dbBlock.GetForkCtxStatus(hashFork, forkStatus, hashMainChainRefBlock);
}

bool CBlockBase::RetrieveForkLast(const uint256& hashFork, uint256& hashLastBlock)
{
    return dbBlock.RetrieveForkLast(hashFork, hashLastBlock);
}

bool CBlockBase::GetForkCoinCtxByForkSymbol(const std::string& strForkSymbol, CCoinContext& ctxCoin, const uint256& hashMainChainRefBlock)
{
    return dbBlock.GetForkCoinCtxByForkSymbol(strForkSymbol, ctxCoin, hashMainChainRefBlock);
}

bool CBlockBase::GetForkHashByForkName(const std::string& strForkName, uint256& hashFork, const uint256& hashMainChainRefBlock)
{
    return dbBlock.GetForkHashByForkName(strForkName, hashFork, hashMainChainRefBlock);
}

bool CBlockBase::GetForkHashByChainId(const CChainId nChainId, uint256& hashFork, const uint256& hashMainChainRefBlock)
{
    return dbBlock.GetForkHashByChainId(nChainId, hashFork, hashMainChainRefBlock);
}

bool CBlockBase::ListCoinContext(std::map<std::string, CCoinContext>& mapSymbolCoin, const uint256& hashMainChainRefBlock)
{
    return dbBlock.ListCoinContext(mapSymbolCoin, hashMainChainRefBlock);
}

bool CBlockBase::GetDexCoinPairBySymbolPair(const std::string& strSymbol1, const std::string& strSymbol2, uint32& nCoinPair, const uint256& hashMainChainRefBlock)
{
    return dbBlock.GetDexCoinPairBySymbolPair(strSymbol1, strSymbol2, nCoinPair, hashMainChainRefBlock);
}

bool CBlockBase::GetSymbolPairByDexCoinPair(const uint32 nCoinPair, std::string& strSymbol1, std::string& strSymbol2, const uint256& hashMainChainRefBlock)
{
    return dbBlock.GetSymbolPairByDexCoinPair(nCoinPair, strSymbol1, strSymbol2, hashMainChainRefBlock);
}

bool CBlockBase::ListDexCoinPair(const uint32 nCoinPair, const std::string& strCoinSymbol, std::map<uint32, std::pair<std::string, std::string>>& mapDexCoinPair, const uint256& hashMainChainRefBlock)
{
    return dbBlock.ListDexCoinPair(nCoinPair, strCoinSymbol, mapDexCoinPair, hashMainChainRefBlock);
}

bool CBlockBase::IsTimeVaultWhitelistAddressExist(const CDestination& address, const uint256& hashMainChainRefBlock)
{
    return dbBlock.IsTimeVaultWhitelistAddressExist(address, hashMainChainRefBlock);
}

bool CBlockBase::ListTimeVaultWhitelist(std::set<CDestination>& setTimeVaultWhitelist, const uint256& hashMainChainRefBlock)
{
    return dbBlock.ListTimeVaultWhitelist(setTimeVaultWhitelist, hashMainChainRefBlock);
}

int CBlockBase::GetForkCreatedHeight(const uint256& hashFork, const uint256& hashRefBlock)
{
    CForkContext ctxFork;
    if (!dbBlock.RetrieveForkContext(hashFork, ctxFork, hashRefBlock))
    {
        return -1;
    }
    return ctxFork.nJointHeight;
}

bool CBlockBase::GetPrimaryForkOwnerAddress(CDestination& destForkOwner, const uint256& hashRefBlock)
{
    CForkContext ctxFork;
    if (!dbBlock.RetrieveForkContext(hashGenesisBlock, ctxFork, hashRefBlock))
    {
        return false;
    }
    destForkOwner = ctxFork.destOwner;
    return true;
}

bool CBlockBase::GetBlockIndexHashByNumberNoLock(const uint256& hashFork, const uint256& hashLastBlock, const uint64 nBlockNumber, uint256& hashBlock)
{
    if (hashLastBlock != 0)
    {
        BlockIndexPtr pForkLastIndex = GetForkLastIndex(hashFork);
        if (pForkLastIndex && hashLastBlock != pForkLastIndex->GetBlockHash()
            && pForkLastIndex->GetBlockNumber() <= nBlockNumber + 1024)
        {
            BlockIndexPtr pIndex = GetIndex(hashLastBlock);
            while (pIndex)
            {
                if (pIndex->GetBlockNumber() == nBlockNumber)
                {
                    hashBlock = pIndex->GetBlockHash();
                    return true;
                }
                if (pIndex->IsOrigin())
                {
                    break;
                }
                pIndex = GetPrevBlockIndex(pIndex);
            }
            return false;
        }
    }
    return dbBlock.RetrieveBlockHashByNumber(hashFork, nBlockNumber, hashBlock);
}

bool CBlockBase::GetBlockIndexHashByNumberLock(const uint256& hashFork, const uint64 nBlockNumber, uint256& hashBlock)
{
    CReadLock rlock(rwAccess);
    return GetBlockIndexHashByNumberNoLock(hashFork, 0, nBlockNumber, hashBlock);
}

bool CBlockBase::GetForkBlockLocator(const uint256& hashFork, CBlockLocator& locator, uint256& hashDepth, int nIncStep)
{
    CReadLock rlock(rwAccess);

    BlockIndexPtr pIndex = GetForkLastIndex(hashFork);
    if (!pIndex)
    {
        StdLog("BlockBase", "Get Fork Block Locator: Get frok last index failed, fork: %s", hashFork.ToString().c_str());
        return false;
    }

    if (hashDepth != 0)
    {
        BlockIndexPtr pStartIndex = GetIndex(hashDepth);
        if (pStartIndex && IsLongchainBlock(hashFork, hashDepth))
        {
            pIndex = pStartIndex;
        }
    }

    while (pIndex)
    {
        if (pIndex->GetOriginHash() != hashFork)
        {
            hashDepth = 0;
            break;
        }
        locator.vBlockHash.push_back(pIndex->GetBlockHash());
        if (pIndex->IsOrigin())
        {
            hashDepth = 0;
            break;
        }
        if (locator.vBlockHash.size() >= nIncStep / 2)
        {
            pIndex = GetPrevBlockIndex(pIndex);
            if (!pIndex)
            {
                hashDepth = 0;
            }
            else
            {
                hashDepth = pIndex->GetBlockHash();
            }
            break;
        }
        // for (int i = 0; i < nIncStep && !pIndex->IsOrigin(); i++)
        // {
        //     pIndex = GetPrevBlockIndex(pIndex);
        //     if (!pIndex)
        //     {
        //         hashDepth = 0;
        //         break;
        //     }
        // }

        uint256 hashPrevStepBlock;
        if (pIndex->GetBlockNumber() <= nIncStep || !GetBlockIndexHashByNumberNoLock(hashFork, 0, pIndex->GetBlockNumber() - nIncStep, hashPrevStepBlock))
        {
            hashDepth = 0;
            break;
        }
        pIndex = GetIndex(hashPrevStepBlock);
        if (!pIndex)
        {
            hashDepth = 0;
            break;
        }
    }

    return true;
}

bool CBlockBase::GetForkBlockInv(const uint256& hashFork, const CBlockLocator& locator, vector<uint256>& vBlockHash, size_t nMaxCount)
{
    CReadLock rlock(rwAccess);

    BlockIndexPtr pIndexLast = GetForkLastIndex(hashFork);
    if (!pIndexLast)
    {
        StdLog("BlockBase", "Get Fork Block Inv: Retrieve fork failed, fork: %s", hashFork.ToString().c_str());
        return false;
    }

    BlockIndexPtr pIndex;
    for (const uint256& hash : locator.vBlockHash)
    {
        pIndex = GetIndex(hash);
        if (pIndex && (pIndex->GetBlockHash() == pIndexLast->GetBlockHash() || GetNextBlockIndex(pIndex)))
        {
            if (pIndex->GetOriginHash() != hashFork)
            {
                StdTrace("BlockBase", "GetForkBlockInv GetOriginHash error, fork: %s", hashFork.ToString().c_str());
                return false;
            }
            break;
        }
        pIndex = nullptr;
    }

    if (pIndex)
    {
        pIndex = GetNextBlockIndex(pIndex);
        while (pIndex && vBlockHash.size() < nMaxCount)
        {
            vBlockHash.push_back(pIndex->GetBlockHash());
            pIndex = GetNextBlockIndex(pIndex);
        }
    }
    return true;
}

BlockIndexPtr CBlockBase::GetOriginBlockIndex(const BlockIndexPtr pIndex)
{
    return (pIndex ? GetIndex(pIndex->GetOriginHash()) : nullptr);
}

BlockIndexPtr CBlockBase::GetPrevBlockIndex(const BlockIndexPtr pIndex)
{
    return (pIndex ? GetIndex(pIndex->GetPrevHash()) : nullptr);
}

BlockIndexPtr CBlockBase::GetNextBlockIndex(const BlockIndexPtr pIndex)
{
    if (pIndex)
    {
        uint256 hashNextBlock;
        if (GetBlockIndexHashByNumberNoLock(pIndex->GetOriginHash(), 0, pIndex->GetBlockNumber() + 1, hashNextBlock))
        {
            BlockIndexPtr pNextIndex = GetIndex(hashNextBlock);
            if (pNextIndex && pNextIndex->GetPrevHash() == pIndex->GetBlockHash())
            {
                return pNextIndex;
            }
        }
    }
    return nullptr;
}

BlockIndexPtr CBlockBase::GetParentBlockIndex(const BlockIndexPtr pIndex)
{
    if (!pIndex || pIndex->IsPrimary())
    {
        return nullptr;
    }
    BlockIndexPtr pOriginIndex = GetOriginBlockIndex(pIndex);
    if (!pOriginIndex)
    {
        return nullptr;
    }
    BlockIndexPtr pOriPrevIndex = GetPrevBlockIndex(pOriginIndex);
    if (!pOriPrevIndex)
    {
        return nullptr;
    }
    return GetOriginBlockIndex(pOriPrevIndex);
}

bool CBlockBase::IsBlockIndexEquivalent(const BlockIndexPtr pIndex, const BlockIndexPtr pIndexCompare)
{
    if (pIndex && pIndexCompare)
    {
        BlockIndexPtr pCurIndex = pIndex;
        while (pCurIndex)
        {
            if (pCurIndex->GetBlockHash() == pIndexCompare->GetBlockHash())
            {
                return true;
            }
            if (pCurIndex->nType != CBlock::BLOCK_VACANT
                || pCurIndex->GetBlockHeight() <= pIndexCompare->GetBlockHeight())
            {
                break;
            }
            pCurIndex = GetPrevBlockIndex(pCurIndex);
        }
    }
    return false;
}

bool CBlockBase::GetForkBlockMintListByHeight(const uint256& hashFork, const uint32 nHeight, std::map<uint256, CBlockHeightIndex>& mapBlockMint)
{
    std::vector<uint256> vBlockHash;
    if (!dbBlock.RetrieveBlockHashByHeight(hashFork, nHeight, vBlockHash))
    {
        return false;
    }
    for (auto& hashBlock : vBlockHash)
    {
        BlockIndexPtr pIndex = GetIndex(hashBlock);
        if (!pIndex)
        {
            return false;
        }
        mapBlockMint.insert(std::make_pair(hashBlock, CBlockHeightIndex(pIndex->GetBlockTime(), pIndex->destMint, pIndex->GetRefBlock())));
    }
    return true;
}

bool CBlockBase::GetForkMaxHeight(const uint256& hashFork, uint32& nMaxHeight)
{
    return dbBlock.GetForkMaxHeight(hashFork, nMaxHeight);
}

bool CBlockBase::IsLongchainBlock(const uint256& hashFork, const uint256& hashBlock)
{
    BlockIndexPtr pIndex = GetIndex(hashBlock);
    if (pIndex)
    {
        uint256 hashNumberBlock;
        if (GetBlockIndexHashByNumberNoLock(hashFork, 0, pIndex->GetBlockNumber(), hashNumberBlock) && hashBlock == hashNumberBlock)
        {
            return true;
        }
    }
    return false;
}

bool CBlockBase::GetBlockHashByHeightSlot(const uint256& hashFork, const uint256& hashRefBlock, const uint32 nHeight, const uint16 nSlot, uint256& hashBlock)
{
    bool fLongchainBlock = false;
    if (hashRefBlock == 0)
    {
        fLongchainBlock = true;
    }
    else
    {
        if (CBlock::GetBlockHeightByHash(hashRefBlock) < nHeight)
        {
            return false;
        }
        if (IsLongchainBlock(hashFork, hashRefBlock))
        {
            fLongchainBlock = true;
        }
    }
    if (!fLongchainBlock && hashRefBlock != 0)
    {
        BlockIndexPtr pIndex = GetIndex(hashRefBlock);
        while (pIndex && pIndex->GetOriginHash() == hashFork && pIndex->GetBlockHeight() >= nHeight)
        {
            if (IsLongchainBlock(hashFork, pIndex->GetBlockHash()))
            {
                fLongchainBlock = true;
                break;
            }
            if (pIndex->GetBlockHeight() == nHeight && pIndex->GetBlockSlot() == nSlot)
            {
                hashBlock = pIndex->GetBlockHash();
                return true;
            }
            pIndex = GetPrevBlockIndex(pIndex);
        }
    }
    if (fLongchainBlock)
    {
        std::vector<uint256> vBlockHash;
        if (dbBlock.RetrieveBlockHashByHeight(hashFork, nHeight, vBlockHash))
        {
            for (const uint256& hashHeightBlock : vBlockHash)
            {
                if (CBlock::GetBlockSlotByHash(hashHeightBlock) == nSlot)
                {
                    if (IsLongchainBlock(hashFork, hashHeightBlock))
                    {
                        hashBlock = hashHeightBlock;
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool CBlockBase::GetBlockHashListByHeight(const uint256& hashFork, const uint32 nHeight, vector<uint256>& vBlockHash)
{
    std::vector<uint256> vHeightBlockHash;
    if (dbBlock.RetrieveBlockHashByHeight(hashFork, nHeight, vHeightBlockHash))
    {
        set<uint256, CustomBlockHashCompare> setBlockHash;
        for (const uint256& hashHeightBlock : vHeightBlockHash)
        {
            if (IsLongchainBlock(hashFork, hashHeightBlock))
            {
                setBlockHash.insert(hashHeightBlock);
            }
        }
        if (setBlockHash.empty())
        {
            return false;
        }
        for (auto& hash : setBlockHash)
        {
            vBlockHash.push_back(hash);
        }
        return true;
    }
    return false;
}

bool CBlockBase::GetDelegateVotes(const uint256& hashGenesis, const uint256& hashRefBlock, const CDestination& destDelegate, uint256& nVotes)
{
    uint256 hashLastBlock = hashRefBlock;
    if (hashRefBlock == 0)
    {
        if (!dbBlock.RetrieveForkLast(hashGenesis, hashLastBlock))
        {
            return false;
        }
    }
    if (!dbBlock.RetrieveDestDelegateVote(hashLastBlock, destDelegate, nVotes))
    {
        return false;
    }
    return true;
}

bool CBlockBase::RetrieveDestVoteContext(const uint256& hashBlock, const CDestination& destVote, CVoteContext& ctxtVote)
{
    return dbBlock.RetrieveDestVoteContext(hashBlock, destVote, ctxtVote);
}

bool CBlockBase::RetrieveDestPledgeVoteContext(const uint256& hashBlock, const CDestination& destVote, CPledgeVoteContext& ctxPledgeVote)
{
    return dbBlock.RetrieveDestPledgeVoteContext(hashBlock, destVote, ctxPledgeVote);
}

bool CBlockBase::ListPledgeFinalHeight(const uint256& hashBlock, const uint32 nFinalHeight, std::map<CDestination, std::pair<uint32, uint32>>& mapPledgeFinalHeight)
{
    return dbBlock.ListPledgeFinalHeight(hashBlock, nFinalHeight, mapPledgeFinalHeight);
}

bool CBlockBase::WalkThroughDayVote(const uint256& hashBeginBlock, const uint256& hashTailBlock, CDayVoteWalker& walker)
{
    return dbBlock.WalkThroughDayVote(hashBeginBlock, hashTailBlock, walker);
}

bool CBlockBase::RetrieveDelegateEnrollStatus(const uint256& hashBlock, std::map<CDestination, std::pair<uint32, uint64>>& mapDelegateEnrollStatus)
{
    //std::map<CDestination, std::pair<uint32, uint64>> mapDelegateEnrollStatus; // key: delegate address, value1: last enroll height, value2: last enroll time
    std::vector<uint256> vBlockRange;
    std::map<uint32, uint64> mapHeightTime;
    BlockIndexPtr pIndex = GetIndex(hashBlock);
    for (int i = 0; pIndex && i < CONSENSUS_ENROLL_INTERVAL; i++)
    {
        vBlockRange.push_back(pIndex->GetBlockHash());
        mapHeightTime[pIndex->GetBlockHeight()] = pIndex->GetBlockTime();
        pIndex = GetPrevBlockIndex(pIndex);
    }
    std::map<CDestination, uint32> mapEnrollHeight; // key: delegate address, value: last enroll height
    if (!dbBlock.RetrieveDelegateEnrollStatus(vBlockRange, mapEnrollHeight))
    {
        return false;
    }
    for (auto& kv : mapEnrollHeight)
    {
        uint64 nTime = 0;
        auto it = mapHeightTime.find(kv.second);
        if (it != mapHeightTime.end())
        {
            nTime = it->second;
        }
        mapDelegateEnrollStatus[kv.first] = std::make_pair(kv.second, nTime);
    }
    return true;
}

bool CBlockBase::RetrieveDelegateRewardApy(const uint256& hashBlock, std::map<CDestination, std::pair<uint256, double>>& mapDelegateRewardApy)
{
    //std::map<CDestination, std::pair<uint256, double>> mapDelegateRewardApy; // key: delegate address, value1: delegate total reward, value2: delegate apy
    std::map<CDestination, uint256> mapDelegateLastReward;

    const uint32 DELEGATE_REWARD_APY_DAY_COUNT = 7;
    BlockIndexPtr pIndex = GetIndex(hashBlock);
    for (size_t i = 0; pIndex && i < DAY_HEIGHT * DELEGATE_REWARD_APY_DAY_COUNT; i++)
    {
        if (pIndex->IsPrimary() && pIndex->IsProofOfStake())
        {
            mapDelegateLastReward[pIndex->destMint] += pIndex->GetBlockReward();
        }
        pIndex = GetPrevBlockIndex(pIndex);
    }

    uint256 nVoteRewardRatio;
    if (REWARD_DISTRIBUTION_RATIO_ENABLE)
    {
        nVoteRewardRatio = REWARD_DISTRIBUTION_PER - REWARD_DISTRIBUTION_RATIO_PROJECT_PARTY - REWARD_DISTRIBUTION_RATIO_FOUNDATION;
    }
    for (auto& kv : mapDelegateLastReward)
    {
        uint256 n = kv.second / DELEGATE_REWARD_APY_DAY_COUNT * 365;
        if (REWARD_DISTRIBUTION_RATIO_ENABLE)
        {
            n = n * nVoteRewardRatio / REWARD_DISTRIBUTION_PER;
        }
        kv.second = n;
    }

    std::multimap<uint256, CDestination> mapVotes;
    GetDelegateList(hashBlock, 0, 0, mapVotes);

    const int64 nPrecision = 10000000000;
    for (auto& kv : mapVotes)
    {
        const uint256& nDelegateVotes = kv.first;
        const CDestination& destDelegateAddress = kv.second;

        if (nDelegateVotes >= DELEGATE_PROOF_OF_STAKE_ENROLL_MINIMUM_AMOUNT)
        {
            uint8 nMintTemplateType = 0;
            uint256 nDelegateTotalReward = 0;
            if (!dbBlock.RetrieveMintReward(GetGenesisBlockHash(), hashBlock, destDelegateAddress, nMintTemplateType, nDelegateTotalReward))
            {
                nDelegateTotalReward = 0;
            }

            double dApy = 0.0;
            auto it = mapDelegateLastReward.find(destDelegateAddress);
            if (it != mapDelegateLastReward.end())
            {
                dApy = (double)((it->second * nPrecision / nDelegateVotes).Get64()) / nPrecision;
            }

            mapDelegateRewardApy[destDelegateAddress] = std::make_pair(nDelegateTotalReward, dApy);
            if (mapDelegateRewardApy.size() >= MAX_DELEGATE_THRESH)
            {
                break;
            }
        }
    }
    return true;
}

bool CBlockBase::RetrieveAllDelegateVote(const uint256& hashBlock, std::map<CDestination, std::map<CDestination, CVoteContext>>& mapDelegateVote)
{
    return dbBlock.RetrieveAllDelegateVote(hashBlock, mapDelegateVote);
}

bool CBlockBase::GetDelegateMintRewardRatio(const uint256& hashBlock, const CDestination& destDelegate, uint32& nRewardRation)
{
    CAddressContext ctxAddress;
    if (!dbBlock.RetrieveAddressContext(GetGenesisBlockHash(), hashBlock, destDelegate, ctxAddress))
    {
        StdLog("CBlockBase", "Get Delegate Mint Reward Ratio: Retrieve address context fail, delegate: %s, block: %s",
               destDelegate.ToString().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    if (!ctxAddress.IsTemplate())
    {
        StdLog("CBlockBase", "Get Delegate Mint Reward Ratio: Address not is template, delegate: %s, block: %s",
               destDelegate.ToString().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    CTemplateAddressContext ctxtTemplate;
    if (!ctxAddress.GetTemplateAddressContext(ctxtTemplate))
    {
        StdLog("CBlockBase", "Get Delegate Mint Reward Ratio: Get template address context fail, delegate: %s, block: %s",
               destDelegate.ToString().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    CTemplatePtr ptr = CTemplate::Import(ctxtTemplate.btData);
    if (ptr == nullptr)
    {
        StdLog("CBlockBase", "Get Delegate Mint Reward Ratio: Create template fail, delegate: %s, block: %s",
               destDelegate.ToString().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    if (ptr->GetTemplateType() != TEMPLATE_DELEGATE)
    {
        StdLog("CBlockBase", "Get Delegate Mint Reward Ratio: Not delegate template, template type: %d, delegate: %s, block: %s",
               ptr->GetTemplateType(), destDelegate.ToString().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    nRewardRation = boost::dynamic_pointer_cast<CTemplateDelegate>(ptr)->nRewardRatio;
    return true;
}

bool CBlockBase::GetDelegateList(const uint256& hashRefBlock, const uint32 nStartIndex, const uint32 nCount, std::multimap<uint256, CDestination>& mapVotes)
{
    uint256 hashLastBlock = hashRefBlock;
    if (hashRefBlock == 0)
    {
        if (!dbBlock.RetrieveForkLast(GetGenesisBlockHash(), hashLastBlock))
        {
            return false;
        }
    }
    std::map<CDestination, uint256> mapAddressVote;
    if (!dbBlock.RetrieveDelegate(hashLastBlock, mapAddressVote))
    {
        return false;
    }
    if (nCount == 0)
    {
        for (const auto& d : mapAddressVote)
        {
            mapVotes.insert(std::make_pair(d.second, d.first));
        }
    }
    else
    {
        std::multimap<uint256, CDestination> mapTempAmountVote;
        for (const auto& d : mapAddressVote)
        {
            mapTempAmountVote.insert(std::make_pair(d.second, d.first));
        }
        auto it = mapTempAmountVote.begin();
        uint32 nJumpIndex = nStartIndex;
        while (it != mapTempAmountVote.end() && nJumpIndex > 0)
        {
            ++it;
            --nJumpIndex;
        }
        uint32 nJumpCount = nCount;
        while (it != mapTempAmountVote.end() && nJumpCount > 0)
        {
            mapVotes.insert(std::make_pair(it->first, it->second));
            ++it;
            --nJumpCount;
        }
    }
    return true;
}

bool CBlockBase::VerifyRepeatBlock(const uint256& hashFork, const uint256& hashBlock, const uint32 height, const CDestination& destMint, const uint16 nBlockType,
                                   const uint64 nBlockTimeStamp, const uint64 nRefBlockTimeStamp, const uint32 nExtendedBlockSpacing)
{
    CWriteLock wlock(rwAccess);

    map<uint256, CBlockHeightIndex> mapBlockMint;
    if (GetForkBlockMintListByHeight(hashFork, height, mapBlockMint))
    {
        for (auto& mt : mapBlockMint)
        {
            const uint256& hashHiBlock = mt.first;
            if (hashBlock == hashHiBlock)
            {
                continue;
            }
            if (mt.second.destMint.IsNull())
            {
                BlockIndexPtr pBlockIndex = GetIndex(hashHiBlock);
                if (pBlockIndex)
                {
                    if (pBlockIndex->IsVacant())
                    {
                        CBlock block;
                        if (Retrieve(pBlockIndex, block) && !block.txMint.GetToAddress().IsNull())
                        {
                            mt.second.destMint = block.txMint.GetToAddress();
                        }
                    }
                    else
                    {
                        CTransaction tx;
                        uint256 hashAtFork;
                        uint256 hashTxAtBlock;
                        CTxIndex txIndex;
                        if (RetrieveTxAndIndex(hashFork, pBlockIndex->txidMint, tx, hashAtFork, hashTxAtBlock, txIndex))
                        {
                            mt.second.destMint = tx.GetToAddress();
                        }
                    }
                }
            }
            if (mt.second.destMint == destMint)
            {
                if (nBlockType == CBlock::BLOCK_SUBSIDIARY || nBlockType == CBlock::BLOCK_EXTENDED)
                {
                    if ((nBlockTimeStamp - nRefBlockTimeStamp) / nExtendedBlockSpacing
                        == (mt.second.nTimeStamp - nRefBlockTimeStamp) / nExtendedBlockSpacing)
                    {
                        StdTrace("BlockBase", "Verify Repeat Block: subsidiary or extended repeat block, block time: %lu, cache block time: %lu, ref block time: %lu, destMint: %s",
                                 nBlockTimeStamp, mt.second.nTimeStamp, mt.second.nTimeStamp, destMint.ToString().c_str());
                        return false;
                    }
                }
                else
                {
                    StdTrace("BlockBase", "Verify Repeat Block: repeat block: %s, destMint: %s", hashHiBlock.GetHex().c_str(), destMint.ToString().c_str());
                    return false;
                }
            }
        }
    }
    return true;
}

bool CBlockBase::GetBlockDelegateVote(const uint256& hashBlock, map<CDestination, uint256>& mapVote)
{
    return dbBlock.RetrieveDelegate(hashBlock, mapVote);
}

bool CBlockBase::GetRangeDelegateEnroll(int height, const vector<uint256>& vBlockRange, map<CDestination, CDiskPos>& mapEnrollTxPos)
{
    return dbBlock.RetrieveRangeEnroll(height, vBlockRange, mapEnrollTxPos);
}

bool CBlockBase::VerifyRefBlock(const uint256& hashGenesis, const uint256& hashRefBlock)
{
    const BlockIndexPtr pIndexGenesisLast = GetForkLastIndex(hashGenesis);
    if (!pIndexGenesisLast)
    {
        return false;
    }
    return IsValidBlock(pIndexGenesisLast, hashRefBlock);
}

BlockIndexPtr CBlockBase::GetForkValidLast(const uint256& hashGenesis, const uint256& hashFork)
{
    CReadLock rlock(rwAccess);

    BlockIndexPtr pIndexGenesisLast = GetForkLastIndex(hashGenesis);
    if (!pIndexGenesisLast)
    {
        return nullptr;
    }

    BlockIndexPtr pForkLast = GetForkLastIndex(hashFork);
    if (!pForkLast || pForkLast->IsOrigin())
    {
        return nullptr;
    }

    set<uint256> setInvalidHash;
    BlockIndexPtr pIndex = pForkLast;
    while (pIndex && !pIndex->IsOrigin())
    {
        if (VerifyValidBlock(pIndexGenesisLast, pIndex))
        {
            break;
        }
        setInvalidHash.insert(pIndex->GetBlockHash());
        pIndex = GetPrevBlockIndex(pIndex);
    }
    if (!pIndex)
    {
        pIndex = GetIndex(hashFork);
    }
    if (pIndex->GetBlockHash() == pForkLast->GetBlockHash())
    {
        return nullptr;
    }
    BlockIndexPtr pIndexValidLast = GetLongChainLastBlock(hashFork, pIndex->GetBlockHeight(), pIndexGenesisLast, setInvalidHash);
    if (!pIndexValidLast)
    {
        return pIndex;
    }
    return pIndexValidLast;
}

bool CBlockBase::VerifySameChain(const uint256& hashPrevBlock, const uint256& hashAfterBlock)
{
    CReadLock rlock(rwAccess);
    return VerifySameChainNoLock(hashPrevBlock, hashAfterBlock);
}

bool CBlockBase::GetLastRefBlockHash(const uint256& hashFork, const uint256& hashBlock, uint256& hashRefBlock, bool& fOrigin)
{
    BlockIndexPtr pIndex = GetIndex(hashBlock);
    if (!pIndex)
    {
        return false;
    }
    hashRefBlock = pIndex->GetRefBlock();
    fOrigin = pIndex->IsOrigin();
    return true;

    // hashRefBlock = 0;
    // fOrigin = false;
    // BlockIndexPtr pIndexUpdateRef;

    // {
    //     CReadLock rlock(rwAccess);

    //     std::map<uint256, CBlockHeightIndex> mapHeightIndex;
    //     if (GetForkBlockMintListByHeight(hashFork, CBlock::GetBlockHeightByHash(hashBlock), mapHeightIndex))
    //     {
    //         auto mt = mapHeightIndex.find(hashBlock);
    //         if (mt != mapHeightIndex.end() && mt->second.hashRefBlock != 0)
    //         {
    //             hashRefBlock = mt->second.hashRefBlock;
    //             return true;
    //         }
    //     }

    //     BlockIndexPtr pIndex = GetIndex(hashBlock);
    //     while (pIndex)
    //     {
    //         if (pIndex->IsOrigin())
    //         {
    //             fOrigin = true;
    //             return true;
    //         }
    //         CBlockEx block;
    //         if (!Retrieve(pIndex, block))
    //         {
    //             return false;
    //         }
    //         CProofOfPiggyback proof;
    //         if (block.GetPiggybackProof(proof) && proof.hashRefBlock != 0)
    //         {
    //             hashRefBlock = proof.hashRefBlock;
    //             pIndexUpdateRef = pIndex;
    //             break;
    //         }
    //         pIndex = GetPrevBlockIndex(pIndex);
    //     }
    // }

    // if (pIndexUpdateRef)
    // {
    //     return true;
    // }
    // return false;
}

bool CBlockBase::GetBlockForRefBlockNoLock(const uint256& hashBlock, uint256& hashRefBlock)
{
    BlockIndexPtr pIndex = GetIndex(hashBlock);
    if (!pIndex)
    {
        return false;
    }
    hashRefBlock = pIndex->GetRefBlock();
    return true;
}

bool CBlockBase::GetPrimaryHeightBlockTime(const uint256& hashLastBlock, int nHeight, uint256& hashBlock, int64& nTime)
{
    CReadLock rlock(rwAccess);

    BlockIndexPtr pIndex = GetIndex(hashLastBlock);
    if (!pIndex || !pIndex->IsPrimary())
    {
        return false;
    }
    // while (pIndex && pIndex->GetBlockHeight() >= nHeight)
    // {
    //     if (pIndex->GetBlockHeight() == nHeight)
    //     {
    //         hashBlock = pIndex->GetBlockHash();
    //         nTime = pIndex->GetBlockTime();
    //         return true;
    //     }
    //     pIndex = GetPrevBlockIndex(pIndex);
    // }
    if (GetBlockHashByHeightSlot(pIndex->GetOriginHash(), pIndex->GetBlockHash(), nHeight, 0, hashBlock))
    {
        pIndex = GetIndex(hashBlock);
        if (pIndex)
        {
            nTime = pIndex->GetBlockTime();
            return true;
        }
    }
    return false;
}

bool CBlockBase::VerifyPrimaryHeightRefBlockTime(const int nHeight, const int64 nTime)
{
    CReadLock rlock(rwAccess);

    std::map<uint256, CBlockHeightIndex> mapHeightIndex;
    if (!GetForkBlockMintListByHeight(GetGenesisBlockHash(), nHeight, mapHeightIndex))
    {
        return false;
    }
    for (const auto& kv : mapHeightIndex)
    {
        if (kv.second.nTimeStamp != nTime)
        {
            return false;
        }
    }
    return true;
}

bool CBlockBase::UpdateForkNext(const uint256& hashFork, BlockIndexPtr pIndexLast, const std::vector<CBlockEx>& vBlockRemove, const std::vector<CBlockEx>& vBlockAddNew)
{
    CWriteLock wlock(rwAccess);

    if (!UpdateBlockLongChain(hashFork, vBlockRemove, vBlockAddNew))
    {
        StdLog("BlockBase", "Update Fork Next: Update block long chain fail, fork: %s, last block: %s", hashFork.ToString().c_str(), pIndexLast->GetBlockHash().ToString().c_str());
        return false;
    }

    return dbBlock.UpdateForkLast(hashFork, pIndexLast->GetBlockHash());
}

bool CBlockBase::RetrieveDestState(const uint256& hashFork, const uint256& hashBlockRoot, const CDestination& dest, CDestState& state)
{
    return dbBlock.RetrieveDestState(hashFork, hashBlockRoot, dest, state);
}

SHP_BLOCK_STATE CBlockBase::CreateBlockStateRoot(const uint256& hashFork, const CForkContext& ctxFork, const CBlock& block, const uint256& hashPrevStateRoot, const uint32 nPrevBlockTime, uint256& hashStateRoot, uint256& hashReceiptRoot,
                                                 uint256& hashCrosschainMerkleRoot, uint256& nBlockGasUsed, bytes& btBloomDataOut, uint256& nTotalMintRewardOut, bool& fMoStatus, const std::map<CDestination, CAddressContext>& mapAddressContext)
{
    SHP_BLOCK_STATE ptrBlockState = MAKE_SHARED_BLOCK_STATE(*this, hashFork, ctxFork, block, hashPrevStateRoot, nPrevBlockTime, mapAddressContext, fCfgTraceDb);

    CBlockRootStatus statusBlockRoot(block.nType, block.GetBlockTime(), block.txMint.GetToAddress());
    if (!dbBlock.CreateCacheStateTrie(hashFork, hashPrevStateRoot, statusBlockRoot, ptrBlockState->mapBlockState, hashStateRoot))
    {
        StdLog("BlockBase", "Create block state root: Create cache state trie fail, prev: %s", block.hashPrev.GetHex().c_str());
        return nullptr;
    }
    return ptrBlockState;
}

bool CBlockBase::ReUpdateBlockTraceData(const uint256& hashFork, const uint256& hashBlock)
{
    BlockIndexPtr pIndex;
    if (!(pIndex = GetIndex(hashBlock)))
    {
        StdTrace("BlockBase", "ReUpdate block trace data: Get index failed, block: %s", hashBlock.ToString().c_str());
        return false;
    }
    CBlockEx block;
    if (!tsBlock.Read(block, pIndex->nFile, pIndex->nOffset, true, true))
    {
        StdTrace("BlockBase", "ReUpdate block trace data: Read block failed, block: %s", hashBlock.ToString().c_str());
        return false;
    }

    CForkContext ctxFork;
    if (block.IsOrigin())
    {
        CProfile profile;
        if (block.GetForkProfile(profile))
        {
            ctxFork = CForkContext(hashFork, block.hashPrev, 0, profile);
        }
    }
    else
    {
        if (!dbBlock.RetrieveForkContext(hashFork, ctxFork, pIndex->GetRefBlock()))
        {
            StdError("BlockBase", "ReUpdate block trace data: Retrieve fork context fail, fork: %s, ref block: %s", hashFork.ToString().c_str(), pIndex->GetRefBlock().GetBhString().c_str());
            return false;
        }
    }
    std::map<CDestination, CAddressContext> mapTempAddressContext;
    if (!GetBlockAddress(hashFork, hashBlock, block, mapTempAddressContext))
    {
        StdError("BlockBase", "ReUpdate block trace data: Get block address failed, block: %s", hashBlock.ToString().c_str());
        return false;
    }
    return true;
}

bool CBlockBase::RetrieveContractKvValue(const uint256& hashFork, const uint256& hashContractRoot, const uint256& key, bytes& value)
{
    return dbBlock.RetrieveContractKvValue(hashFork, hashContractRoot, key, value);
}

bool CBlockBase::CreateCacheContractKvTrie(const uint256& hashFork, const uint256& hashPrevRoot, const std::map<uint256, bytes>& mapContractState, uint256& hashNewRoot)
{
    return dbBlock.CreateCacheContractKvTrie(hashFork, hashPrevRoot, mapContractState, hashNewRoot);
}

bool CBlockBase::RetrieveAddressContext(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, CAddressContext& ctxAddress)
{
    return dbBlock.RetrieveAddressContext(hashFork, hashBlock, dest, ctxAddress);
}

bool CBlockBase::RetrieveTokenContractAddressContext(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, CTokenContractAddressContext& ctxAddress)
{
    uint256 hashLastBlock = hashBlock;
    if (hashLastBlock == 0)
    {
        if (!dbBlock.RetrieveForkLast(hashFork, hashLastBlock))
        {
            return false;
        }
    }
    return dbBlock.RetrieveTokenContractAddressContext(hashFork, hashLastBlock, dest, ctxAddress);
}

bool CBlockBase::ListContractAddress(const uint256& hashFork, const uint256& hashBlock, std::map<CDestination, CContractAddressContext>& mapContractAddress)
{
    return dbBlock.ListContractAddress(hashFork, hashBlock, mapContractAddress);
}

bool CBlockBase::ListTokenContractAddress(const uint256& hashFork, const uint256& hashBlock, std::map<CDestination, CTokenContractAddressContext>& mapTokenContractAddress)
{
    return dbBlock.ListTokenContractAddress(hashFork, hashBlock, mapTokenContractAddress);
}

bool CBlockBase::GetAddressCount(const uint256& hashFork, const uint256& hashBlock, uint64& nAddressCount, uint64& nNewAddressCount)
{
    return dbBlock.GetAddressCount(hashFork, hashBlock, nAddressCount, nNewAddressCount);
}

bool CBlockBase::ListFunctionAddress(const uint256& hashBlock, std::map<uint32, CFunctionAddressContext>& mapFunctionAddress)
{
    uint256 hashLastBlock;
    if (hashBlock == 0)
    {
        if (!dbBlock.RetrieveForkLast(hashGenesisBlock, hashLastBlock))
        {
            return false;
        }
    }
    else
    {
        hashLastBlock = hashBlock;
    }

    if (!dbBlock.ListFunctionAddress(hashGenesisBlock, hashLastBlock, mapFunctionAddress))
    {
        return false;
    }

    if (mapFunctionAddress.find(FUNCTION_ID_PLEDGE_SURPLUS_REWARD_ADDRESS) == mapFunctionAddress.end())
    {
        mapFunctionAddress[FUNCTION_ID_PLEDGE_SURPLUS_REWARD_ADDRESS] = CFunctionAddressContext(PLEDGE_SURPLUS_REWARD_ADDRESS, false);
    }
    if (mapFunctionAddress.find(FUNCTION_ID_TIME_VAULT_TO_ADDRESS) == mapFunctionAddress.end())
    {
        mapFunctionAddress[FUNCTION_ID_TIME_VAULT_TO_ADDRESS] = CFunctionAddressContext(TIME_VAULT_TO_ADDRESS, false);
    }
    if (mapFunctionAddress.find(FUNCTION_ID_PROJECT_PARTY_REWARD_TO_ADDRESS) == mapFunctionAddress.end())
    {
        mapFunctionAddress[FUNCTION_ID_PROJECT_PARTY_REWARD_TO_ADDRESS] = CFunctionAddressContext(PROJECT_PARTY_REWARD_TO_ADDRESS, false);
    }
    if (mapFunctionAddress.find(FUNCTION_ID_FOUNDATION_REWARD_TO_ADDRESS) == mapFunctionAddress.end())
    {
        mapFunctionAddress[FUNCTION_ID_FOUNDATION_REWARD_TO_ADDRESS] = CFunctionAddressContext(FOUNDATION_REWARD_TO_ADDRESS, false);
    }
    return true;
}

bool CBlockBase::RetrieveFunctionAddress(const uint256& hashBlock, const uint32 nFuncId, CFunctionAddressContext& ctxFuncAddress)
{
    if (!dbBlock.RetrieveFunctionAddress(hashGenesisBlock, hashBlock, nFuncId, ctxFuncAddress))
    {
        CDestination destDefFunction;
        if (!GetDefaultFunctionAddress(nFuncId, destDefFunction))
        {
            return false;
        }
        ctxFuncAddress.SetFunctionAddress(destDefFunction);
        ctxFuncAddress.SetDisableModify(false);
    }
    return true;
}

bool CBlockBase::GetDefaultFunctionAddress(const uint32 nFuncId, CDestination& destDefFunction)
{
    switch (nFuncId)
    {
    case FUNCTION_ID_PLEDGE_SURPLUS_REWARD_ADDRESS:
        destDefFunction = PLEDGE_SURPLUS_REWARD_ADDRESS;
        break;
    case FUNCTION_ID_TIME_VAULT_TO_ADDRESS:
        destDefFunction = TIME_VAULT_TO_ADDRESS;
        break;
    case FUNCTION_ID_PROJECT_PARTY_REWARD_TO_ADDRESS:
        destDefFunction = PROJECT_PARTY_REWARD_TO_ADDRESS;
        break;
    case FUNCTION_ID_FOUNDATION_REWARD_TO_ADDRESS:
        destDefFunction = FOUNDATION_REWARD_TO_ADDRESS;
        break;
    default:
        return false;
    }
    return true;
}

bool CBlockBase::VerifyNewFunctionAddress(const uint256& hashBlock, const CDestination& destFrom, const uint32 nFuncId, const CDestination& destNewFunction, std::string& strErr)
{
    CAddressContext ctxNewAddress;
    if (!RetrieveAddressContext(hashGenesisBlock, hashBlock, destNewFunction, ctxNewAddress))
    {
        StdLog("CBlockBase", "Verify new function address: Retrieve new function address context fail, function id: %d, new function address: %s, from: %s",
               nFuncId, destNewFunction.ToString().c_str(), destFrom.ToString().c_str());
        strErr = "Address context error";
        return false;
    }

    CDestination destDefFunc;
    if (!GetDefaultFunctionAddress(nFuncId, destDefFunc))
    {
        StdLog("CBlockBase", "Verify new function address: Get default function address fail, function id: %d, new function address: %s, from: %s",
               nFuncId, destNewFunction.ToString().c_str(), destFrom.ToString().c_str());
        strErr = "Function id error";
        return false;
    }
    if (destFrom != destDefFunc)
    {
        StdLog("CBlockBase", "Verify new function address: From address is not default function address, function id: %d, new function address: %s, default function address: %s, from: %s",
               nFuncId, destNewFunction.ToString().c_str(), destDefFunc.ToString().c_str(), destFrom.ToString().c_str());
        strErr = "From address is not default function address";
        return false;
    }

    std::map<uint32, CFunctionAddressContext> mapFunctionAddress;
    if (!ListFunctionAddress(hashBlock, mapFunctionAddress))
    {
        StdLog("CBlockBase", "Verify new function address: List function address fail, function id: %d, new function address: %s, from: %s",
               nFuncId, destNewFunction.ToString().c_str(), destFrom.ToString().c_str());
        strErr = "List function address fail";
        return false;
    }
    for (auto& kv : mapFunctionAddress)
    {
        if (kv.second.GetFunctionAddress() == destNewFunction)
        {
            StdLog("CBlockBase", "Verify new function address: Function address already exists, function id: %d, new function address: %s, repeat function id: %d, from: %s",
                   nFuncId, destNewFunction.ToString().c_str(), kv.first, destFrom.ToString().c_str());
            strErr = "Function address already exists";
            return false;
        }
    }

    auto it = mapFunctionAddress.find(nFuncId);
    if (it != mapFunctionAddress.end())
    {
        if (it->second.IsDisableModify())
        {
            StdLog("CBlockBase", "Verify new function address: Disable modify, function id: %d, new function address: %s, from: %s",
                   nFuncId, destNewFunction.ToString().c_str(), destFrom.ToString().c_str());
            strErr = "Disable modify";
            return false;
        }
    }
    return true;
}

bool CBlockBase::RetrieveBlsPubkeyContext(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, uint384& blsPubkey)
{
    return dbBlock.RetrieveBlsPubkeyContext(hashFork, hashBlock, dest, blsPubkey);
}

bool CBlockBase::GetOwnerLinkTemplateAddress(const uint256& hashFork, const uint256& hashBlock, const CDestination& destOwner, std::map<CDestination, uint8>& mapTemplateAddress)
{
    return dbBlock.GetOwnerLinkTemplateAddress(hashFork, hashBlock, destOwner, mapTemplateAddress);
}

bool CBlockBase::GetDelegateLinkTemplateAddress(const uint256& hashFork, const uint256& hashBlock, const CDestination& destDelegate, const uint32 nTemplateType, const uint64 nBegin, const uint64 nCount, std::vector<std::pair<CDestination, uint8>>& vTemplateAddress)
{
    return dbBlock.GetDelegateLinkTemplateAddress(hashFork, hashBlock, destDelegate, nTemplateType, nBegin, nCount, vTemplateAddress);
}

bool CBlockBase::CallContract(const uint256& hashFork, const uint256& hashBlock, const CVmCallTx& vmCallTx, CVmCallResult& vmCallResult)
{
    BlockIndexPtr pIndex;
    if (hashBlock == 0)
    {
        pIndex = RetrieveFork(hashFork);
        if (!pIndex)
        {
            StdLog("BlockBase", "Call contract: Retrieve fork fail, fork: %s", hashFork.GetHex().c_str());
            return false;
        }
    }
    else
    {
        pIndex = RetrieveIndex(hashBlock);
        if (!pIndex)
        {
            StdLog("BlockBase", "Call contract: Retrieve index fail, block: %s", hashBlock.GetHex().c_str());
            return false;
        }
    }

    uint256 hashPrimaryBlock;
    if (pIndex->IsPrimary())
    {
        hashPrimaryBlock = pIndex->GetBlockHash();
    }
    else
    {
        hashPrimaryBlock = pIndex->hashRefBlock;
    }

    CForkContext ctxFork;
    if (!RetrieveForkContext(hashFork, ctxFork, hashPrimaryBlock))
    {
        StdLog("BlockBase", "Call contract: Retrieve forkc ontext fail, fork: %s", hashFork.GetHex().c_str());
        return false;
    }

    uint256 nTvGas;
    uint256 nRunGasLimit;
    uint256 nBaseTvGas = TX_BASE_GAS + CTransaction::GetTxDataGasStatic(vmCallTx.btData.size()) + nTvGas;
    if (vmCallTx.nGasLimit == 0)
    {
        // Gas is 0, Calc gas used
        nRunGasLimit = DEF_TX_GAS_LIMIT;
    }
    else
    {
        if (vmCallTx.nGasLimit < nBaseTvGas)
        {
            StdLog("BlockBase", "Call contract: Gas not enough, tv gas: %lu, base+tv gas: %lu, gas limit: %lu, from: %s, fork: %s",
                   nTvGas.Get64(), nBaseTvGas.Get64(), vmCallTx.nGasLimit.Get64(), vmCallTx.destFrom.ToString().c_str(), hashFork.GetHex().c_str());
            return false;
        }
        nRunGasLimit = vmCallTx.nGasLimit - nBaseTvGas;
    }

    CVmCallBlock vmCallBlock;

    vmCallBlock.hashFork = hashFork;
    vmCallBlock.nChainId = CBlock::GetBlockChainIdByHash(hashFork);
    vmCallBlock.nAgreement = pIndex->GetAgreement();
    vmCallBlock.nBlockHeight = pIndex->GetBlockHeight();
    vmCallBlock.nBlockNumber = pIndex->GetBlockNumber();
    vmCallBlock.fPrimaryBlock = pIndex->IsPrimary();
    vmCallBlock.destMint = pIndex->destMint;
    vmCallBlock.nOriBlockGasLimit = pIndex->nGasLimit.Get64();
    vmCallBlock.nBlockGasLimit = MAX_BLOCK_GAS_LIMIT;
    vmCallBlock.nBlockTimestamp = pIndex->GetBlockTime();
    vmCallBlock.hashPrevBlock = pIndex->GetBlockHash();
    vmCallBlock.hashPrevStateRoot = pIndex->GetStateRoot();
    vmCallBlock.nPrevBlockTime = pIndex->GetBlockTime();

    CVmCallTx vmRunCallTx = vmCallTx;
    vmRunCallTx.nGasLimit = nRunGasLimit;

    if (nRunGasLimit.Get64() <= vmCallResult.nGasLeft)
    {
        vmCallResult.nGasUsed = nBaseTvGas.Get64();
    }
    else
    {
        vmCallResult.nGasUsed = (nBaseTvGas + (nRunGasLimit - uint256(vmCallResult.nGasLeft))).Get64();
    }
    return true;
}

bool CBlockBase::GetContractCoinName(const uint256& hashFork, const uint256& hashBlock, const CDestination& destContract, const bool fNeedVerifyConntractAddress, string& strName)
{
    CVmCallTx vmCallTx;
    CVmCallResult vmCallResult;

    vmCallTx.fEthCall = true;
    //vmCallTx.destFrom = 0;
    vmCallTx.destTo = destContract;
    vmCallTx.nTxNonce = 0;
    vmCallTx.nGasPrice = MIN_GAS_PRICE;
    vmCallTx.nGasLimit = DEF_TX_GAS_LIMIT;
    vmCallTx.nAmount = 0;
    vmCallTx.btData = MakeEthTxCallData("name()", {});
    vmCallTx.fNeedVerifyToAddress = fNeedVerifyConntractAddress;

    if (!CallContract(hashFork, hashBlock, vmCallTx, vmCallResult))
    {
        StdLog("BlockBase", "Get contract coin name: Call contract fail, contract address: %s, fork: %s",
               destContract.ToString().c_str(), hashFork.GetHex().c_str());
        return false;
    }
    if (vmCallResult.btResult.size() <= 64)
    {
        StdLog("BlockBase", "Get contract coin name: Result error, result: %s, contract address: %s, fork: %s",
               ToHexString(vmCallResult.btResult).c_str(), destContract.ToString().c_str(), hashFork.GetHex().c_str());
        return false;
    }

    uint256 tempData;
    tempData.FromBigEndian(vmCallResult.btResult.data() + 32, 32);
    std::size_t nSize = tempData.Get64();
    if (nSize == 0 || vmCallResult.btResult.size() < nSize + 64)
    {
        StdLog("BlockBase", "Get contract coin name: Result name size error, result: %s, contract address: %s, fork: %s",
               ToHexString(vmCallResult.btResult).c_str(), destContract.ToString().c_str(), hashFork.GetHex().c_str());
        return false;
    }
    strName = string(vmCallResult.btResult.begin() + 64, vmCallResult.btResult.begin() + 64 + nSize);
    return true;
}

bool CBlockBase::GetContractCoinSymbol(const uint256& hashFork, const uint256& hashBlock, const CDestination& destContract, const bool fNeedVerifyConntractAddress, string& strSymbol)
{
    CVmCallTx vmCallTx;
    CVmCallResult vmCallResult;

    vmCallTx.fEthCall = true;
    //vmCallTx.destFrom = 0;
    vmCallTx.destTo = destContract;
    vmCallTx.nTxNonce = 0;
    vmCallTx.nGasPrice = MIN_GAS_PRICE;
    vmCallTx.nGasLimit = DEF_TX_GAS_LIMIT;
    vmCallTx.nAmount = 0;
    vmCallTx.btData = MakeEthTxCallData("symbol()", {});
    vmCallTx.fNeedVerifyToAddress = fNeedVerifyConntractAddress;

    if (!CallContract(hashFork, hashBlock, vmCallTx, vmCallResult))
    {
        StdLog("BlockBase", "Get contract coin symbol: Call contract fail, contract address: %s, fork: %s",
               destContract.ToString().c_str(), hashFork.GetHex().c_str());
        return false;
    }
    if (vmCallResult.btResult.size() <= 64)
    {
        StdLog("BlockBase", "Get contract coin symbol: Result error, result: %s, contract address: %s, fork: %s",
               ToHexString(vmCallResult.btResult).c_str(), destContract.ToString().c_str(), hashFork.GetHex().c_str());
        return false;
    }

    uint256 tempData;
    tempData.FromBigEndian(vmCallResult.btResult.data() + 32, 32);
    std::size_t nSize = tempData.Get64();
    if (nSize == 0 || vmCallResult.btResult.size() < nSize + 64)
    {
        StdLog("BlockBase", "Get contract coin symbol: Result symbol size error, result: %s, contract address: %s, fork: %s",
               ToHexString(vmCallResult.btResult).c_str(), destContract.ToString().c_str(), hashFork.GetHex().c_str());
        return false;
    }
    strSymbol = string(vmCallResult.btResult.begin() + 64, vmCallResult.btResult.begin() + 64 + nSize);
    return true;
}

bool CBlockBase::GetContractCoinDecimals(const uint256& hashFork, const uint256& hashBlock, const CDestination& destContract, const bool fNeedVerifyConntractAddress, uint8& nDecimals)
{
    CVmCallTx vmCallTx;
    CVmCallResult vmCallResult;

    vmCallTx.fEthCall = true;
    //vmCallTx.destFrom = 0;
    vmCallTx.destTo = destContract;
    vmCallTx.nTxNonce = 0;
    vmCallTx.nGasPrice = MIN_GAS_PRICE;
    vmCallTx.nGasLimit = DEF_TX_GAS_LIMIT;
    vmCallTx.nAmount = 0;
    vmCallTx.btData = MakeEthTxCallData("decimals()", {});
    vmCallTx.fNeedVerifyToAddress = fNeedVerifyConntractAddress;

    if (!CallContract(hashFork, hashBlock, vmCallTx, vmCallResult))
    {
        StdLog("BlockBase", "Get contract coin decimals: Call contract fail, contract address: %s, fork: %s",
               destContract.ToString().c_str(), hashFork.GetHex().c_str());
        return false;
    }
    if (vmCallResult.btResult.size() != 32)
    {
        StdLog("BlockBase", "Get contract coin decimals: Result error, result: %s, contract address: %s, fork: %s",
               ToHexString(vmCallResult.btResult).c_str(), destContract.ToString().c_str(), hashFork.GetHex().c_str());
        return false;
    }

    uint256 tempData;
    tempData.FromBigEndian(vmCallResult.btResult.data(), 32);
    nDecimals = (uint8)tempData.Get64();
    return true;
}

bool CBlockBase::GetContractCoinTotalSupply(const uint256& hashFork, const uint256& hashBlock, const CDestination& destContract, const bool fNeedVerifyConntractAddress, uint256& nTotalSupply)
{
    CVmCallTx vmCallTx;
    CVmCallResult vmCallResult;

    vmCallTx.fEthCall = true;
    //vmCallTx.destFrom = 0;
    vmCallTx.destTo = destContract;
    vmCallTx.nTxNonce = 0;
    vmCallTx.nGasPrice = MIN_GAS_PRICE;
    vmCallTx.nGasLimit = DEF_TX_GAS_LIMIT;
    vmCallTx.nAmount = 0;
    vmCallTx.btData = MakeEthTxCallData("totalSupply()", {});
    vmCallTx.fNeedVerifyToAddress = fNeedVerifyConntractAddress;

    if (!CallContract(hashFork, hashBlock, vmCallTx, vmCallResult))
    {
        StdLog("BlockBase", "Get contract coin totalSupply: Call contract fail, contract address: %s, fork: %s",
               destContract.ToString().c_str(), hashFork.GetHex().c_str());
        return false;
    }
    if (vmCallResult.btResult.size() != 32)
    {
        StdLog("BlockBase", "Get contract coin totalSupply: Result error, result: %s, contract address: %s, fork: %s",
               ToHexString(vmCallResult.btResult).c_str(), destContract.ToString().c_str(), hashFork.GetHex().c_str());
        return false;
    }
    nTotalSupply.FromBigEndian(vmCallResult.btResult.data(), 32);
    return true;
}

bool CBlockBase::GetContractCoinBalance(const uint256& hashFork, const uint256& hashBlock, const CDestination& destContract, const CDestination& destUser, const bool fNeedVerifyConntractAddress, uint256& nBalance)
{
    //function balanceOf(address tokenOwner) external view returns (uint);

    std::vector<bytes> vParamList;
    vParamList.push_back(destUser.ToHash().GetBytes());

    CVmCallTx vmCallTx;
    CVmCallResult vmCallResult;

    vmCallTx.fEthCall = true;
    vmCallTx.destFrom = destUser;
    vmCallTx.destTo = destContract;
    vmCallTx.nTxNonce = 0;
    vmCallTx.nGasPrice = MIN_GAS_PRICE;
    vmCallTx.nGasLimit = DEF_TX_GAS_LIMIT;
    vmCallTx.nAmount = 0;
    vmCallTx.btData = MakeEthTxCallData("balanceOf(address)", vParamList);
    vmCallTx.fNeedVerifyToAddress = fNeedVerifyConntractAddress;

    if (!CallContract(hashFork, hashBlock, vmCallTx, vmCallResult))
    {
        StdLog("BlockBase", "Get contract balance: Call contract fail, contract address: %s, user address: %s, fork: %s",
               destContract.ToString().c_str(), destUser.ToString().c_str(), hashFork.GetHex().c_str());
        return false;
    }
    if (vmCallResult.btResult.size() != 32)
    {
        StdLog("BlockBase", "Get contract balance: Result error, result: %s, contract address: %s, user address: %s, fork: %s",
               ToHexString(vmCallResult.btResult).c_str(), destContract.ToString().c_str(), destUser.ToString().c_str(), hashFork.GetHex().c_str());
        return false;
    }
    nBalance.FromBigEndian(vmCallResult.btResult.data(), 32);
    return true;
}

////////////////////////////////////////////////////////////////////////
BlockIndexPtr CBlockBase::GetIndex(const uint256& hash)
{
    BlockIndexPtr pIndex = GetCacheBlockIndex(hash);
    if (pIndex)
    {
        return pIndex;
    }
    CBlockIndex outline;
    if (!dbBlock.RetrieveBlockIndex(hash, outline))
    {
        return nullptr;
    }
    pIndex = MAKE_SHARED_BLOCK_INDEX(outline);
    if (!pIndex)
    {
        return nullptr;
    }
    AddCacheBlockIndex(pIndex);
    return pIndex;
}

void CBlockBase::AddCacheBlockIndex(const BlockIndexPtr pIndex)
{
    CWriteLock wlock(rwCacheBlockIndexAccess);
    mapCacheBlockIndex.insert(std::make_pair(pIndex->GetBlockHash(), pIndex));
}

void CBlockBase::RemoveCacheBlockIndex(const uint256& hashBlock)
{
    CWriteLock wlock(rwCacheBlockIndexAccess);
    mapCacheBlockIndex.erase(hashBlock);
}

BlockIndexPtr CBlockBase::GetCacheBlockIndex(const uint256& hashBlock)
{
    CReadLock rlock(rwCacheBlockIndexAccess);
    auto it = mapCacheBlockIndex.find(hashBlock);
    if (it != mapCacheBlockIndex.end())
    {
        return it->second;
    }
    return nullptr;
}

BlockIndexPtr CBlockBase::GetForkLastIndex(const uint256& hashFork)
{
    uint256 hashLastBlock;
    if (!dbBlock.RetrieveForkLast(hashFork, hashLastBlock))
    {
        return nullptr;
    }
    return GetIndex(hashLastBlock);
}

BlockIndexPtr CBlockBase::GetBranch(BlockIndexPtr pIndexRef, BlockIndexPtr pIndex, vector<BlockIndexPtr>& vPath)
{
    vPath.clear();
    while (pIndex && pIndexRef && pIndex->GetBlockHash() != pIndexRef->GetBlockHash())
    {
        if (pIndexRef->GetBlockTime() > pIndex->GetBlockTime())
        {
            pIndexRef = GetPrevBlockIndex(pIndexRef);
        }
        else if (pIndex->GetBlockTime() > pIndexRef->GetBlockTime())
        {
            vPath.push_back(pIndex);
            pIndex = GetPrevBlockIndex(pIndex);
        }
        else
        {
            vPath.push_back(pIndex);
            pIndex = GetPrevBlockIndex(pIndex);
            pIndexRef = GetPrevBlockIndex(pIndexRef);
        }
    }
    if (!pIndex || !pIndexRef)
    {
        return nullptr;
    }
    return pIndex;
}

bool CBlockBase::UpdateBlockLongChain(const uint256& hashFork, const std::vector<CBlockEx>& vBlockRemove, const std::vector<CBlockEx>& vBlockAddNew)
{
    std::vector<uint256> vRemoveBlock;
    std::vector<uint256> vNewBlock;
    std::vector<uint256> vRemoveTx;
    std::map<uint256, uint256> mapNewTx;
    std::vector<std::pair<uint64, uint256>> vRemoveNumberBlock;
    std::vector<std::pair<uint64, uint256>> vNewNumberBlock;

    for (const auto& blockex : vBlockAddNew)
    {
        const uint256 hashBlock = blockex.GetHash();
        mapNewTx.insert(std::make_pair(blockex.txMint.GetHash(), hashBlock));
        for (const auto& tx : blockex.vtx)
        {
            mapNewTx.insert(std::make_pair(tx.GetHash(), hashBlock));
        }
        vNewBlock.push_back(hashBlock);
        vNewNumberBlock.push_back(std::make_pair(blockex.GetBlockNumber(), hashBlock));
    }
    for (const auto& blockex : vBlockRemove)
    {
        const uint256 hashBlock = blockex.GetHash();
        uint256 txid = blockex.txMint.GetHash();
        if (mapNewTx.find(txid) == mapNewTx.end())
        {
            vRemoveTx.push_back(txid);
        }
        for (const auto& tx : blockex.vtx)
        {
            uint256 txid = tx.GetHash();
            if (mapNewTx.find(txid) == mapNewTx.end())
            {
                vRemoveTx.push_back(txid);
            }
        }
        vRemoveBlock.push_back(hashBlock);
        vRemoveNumberBlock.push_back(std::make_pair(blockex.GetBlockNumber(), hashBlock));
    }

    if (!dbBlock.UpdateBlockNumberBlockLongChain(hashFork, vRemoveNumberBlock, vNewNumberBlock))
    {
        StdLog("CBlockBase", "Update block long chain: Update block number long chain fail, fork: %s", hashFork.GetBhString().c_str());
        return false;
    }
    if (!dbBlock.UpdateTxIndexBlockLongChain(hashFork, vRemoveTx, mapNewTx))
    {
        StdLog("CBlockBase", "Update block long chain: Update tx index long chain fail, fork: %s", hashFork.GetBhString().c_str());
        return false;
    }
    if (fCfgFullDb)
    {
        if (!dbBlock.UpdateAddressTxInfoBlockLongChain(hashFork, vRemoveBlock, vNewBlock))
        {
            StdLog("CBlockBase", "Update block long chain: Update address tx long chain fail, fork: %s", hashFork.GetBhString().c_str());
            return false;
        }
    }
    return true;
}

BlockIndexPtr CBlockBase::AddNewIndex(const uint256& hashFork, const uint256& hashBlock, const CBlock& block, const uint32 nFile, const uint32 nOffset, const uint32 nCrc, const uint256& nChainTrust, const uint256& hashNewStateRoot)
{
    BlockIndexPtr pIndexNew = GetIndex(hashBlock);
    if (!pIndexNew)
    {
        pIndexNew = MAKE_SHARED_BLOCK_INDEX(hashFork, hashBlock, block, nFile, nOffset, nCrc);
        if (pIndexNew)
        {
            if (!block.GetBlockMint(pIndexNew->nMoneySupply))
            {
                StdError("BlockBase", "Add new index: Get block mint fail, block: %s", hashBlock.GetHex().c_str());
                return nullptr;
            }
            pIndexNew->nMoneyDestroy = block.GetBlockMoneyDestroy();
            pIndexNew->nChainTrust = nChainTrust;
            pIndexNew->hashStateRoot = hashNewStateRoot;

            if (block.hashPrev != 0)
            {
                BlockIndexPtr pIndexPrev = GetIndex(block.hashPrev);
                if (pIndexPrev)
                {
                    if (!pIndexNew->IsOrigin())
                    {
                        pIndexNew->nMoneySupply += pIndexPrev->nMoneySupply;
                        pIndexNew->nMoneyDestroy += pIndexPrev->nMoneyDestroy;
                        pIndexNew->nTxCount += pIndexPrev->nTxCount;
                        pIndexNew->nRewardTxCount += pIndexPrev->nRewardTxCount;
                        pIndexNew->nUserTxCount += pIndexPrev->nUserTxCount;
                    }
                    pIndexNew->nChainTrust += pIndexPrev->nChainTrust;
                }
                else
                {
                    StdError("BlockBase", "Add new index: Get prev block index fail, prev block: %s", block.hashPrev.GetHex().c_str());
                    return nullptr;
                }
            }
            else
            {
                if (!pIndexNew->IsOrigin())
                {
                    StdError("BlockBase", "Add new index: Prev is null, not origin block, block: %s", hashBlock.GetHex().c_str());
                    return nullptr;
                }
            }
            AddCacheBlockIndex(pIndexNew);
        }
    }
    return pIndexNew;
}

bool CBlockBase::LoadForkProfile(const BlockIndexPtr pIndexOrigin, CProfile& profile)
{
    CForkContext ctxt;
    if (!RetrieveForkContext(pIndexOrigin->GetBlockHash(), ctxt))
    {
        return false;
    }
    profile = ctxt.GetProfile();
    return true;
}

bool CBlockBase::UpdateDelegate(const uint256& hashFork, const uint256& hashBlock, const CBlockEx& block, const uint32 nFile, const uint32 nOffset,
                                const uint256& nMinEnrollAmount, const std::map<CDestination, CAddressContext>& mapAddressContext,
                                const std::map<CDestination, CDestState>& mapAccStateIn, uint256& hashDelegateRoot)
{
    //StdTrace("BlockBase", "Update delegate: height: %d, block: %s", block.GetBlockHeight(), hashBlock.GetHex().c_str());

    std::map<CDestination, uint256> mapDelegateVote;
    std::map<int, std::map<CDestination, CDiskPos>> mapEnrollTx;

    uint256 hashPrevStateRoot;
    if (!block.IsOrigin() && block.hashPrev != 0)
    {
        BlockIndexPtr pIndexPrev = GetIndex(block.hashPrev);
        if (!pIndexPrev)
        {
            StdLog("BlockBase", "Update delegate: Get prev index fail, prev: %s", block.hashPrev.GetHex().c_str());
            return false;
        }
        hashPrevStateRoot = pIndexPrev->GetStateRoot();
    }

    auto addVote = [&](const CDestination& destDelegate, const uint256& nVoteAmount, const bool fAdd) -> bool {
        if (nVoteAmount == 0)
        {
            return true;
        }
        auto it = mapDelegateVote.find(destDelegate);
        if (it == mapDelegateVote.end())
        {
            uint256 nGetVoteAmount;
            if (!dbBlock.RetrieveDestDelegateVote(block.hashPrev, destDelegate, nGetVoteAmount))
            {
                nGetVoteAmount = 0;
            }
            it = mapDelegateVote.insert(make_pair(destDelegate, nGetVoteAmount)).first;
        }
        if (fAdd)
        {
            it->second += nVoteAmount;
        }
        else
        {
            it->second -= nVoteAmount;
        }
        return true;
    };

    for (auto& kv : mapAccStateIn)
    {
        const CDestination& dest = kv.first;
        const CDestState& state = kv.second;

        CAddressContext ctxAddress;
        auto it = mapAddressContext.find(dest);
        if (it == mapAddressContext.end())
        {
            if (!RetrieveAddressContext(hashFork, block.hashPrev, dest, ctxAddress))
            {
                StdError("BlockBase", "Update delegate: Find address context fail, dest: %s", dest.ToString().c_str());
                return false;
            }
        }
        else
        {
            ctxAddress = it->second;
        }
        if (ctxAddress.IsTemplate()
            && (ctxAddress.GetTemplateType() == TEMPLATE_DELEGATE
                || ctxAddress.GetTemplateType() == TEMPLATE_VOTE
                || ctxAddress.GetTemplateType() == TEMPLATE_PLEDGE))
        {
            CTemplateAddressContext ctxtTemplate;
            if (!ctxAddress.GetTemplateAddressContext(ctxtTemplate))
            {
                StdLog("BlockBase", "Update delegate: Get template address context fail, dest: %s", dest.ToString().c_str());
                return false;
            }
            CTemplatePtr ptr = CTemplate::Import(ctxtTemplate.btData);
            if (!ptr)
            {
                StdLog("BlockBase", "Update delegate: Import template fail, dest: %s", dest.ToString().c_str());
                return false;
            }
            CDestination destDelegate;
            uint8 nTidType = ptr->GetTemplateType();
            if (nTidType == TEMPLATE_DELEGATE)
            {
                destDelegate = dest;
            }
            else if (nTidType == TEMPLATE_VOTE)
            {
                if (!fEnableStakeVote)
                {
                    continue;
                }
                destDelegate = boost::dynamic_pointer_cast<CTemplateVote>(ptr)->destDelegate;
            }
            else if (nTidType == TEMPLATE_PLEDGE)
            {
                if (!fEnableStakePledge)
                {
                    continue;
                }
                destDelegate = boost::dynamic_pointer_cast<CTemplatePledge>(ptr)->destDelegate;
            }
            if (!destDelegate.IsNull())
            {
                uint256 nPrevBalance;
                CDestState statePrev;
                if (RetrieveDestState(hashFork, hashPrevStateRoot, dest, statePrev))
                {
                    if (statePrev.IsPubkey())
                    {
                        StdLog("BlockBase", "Update delegate: Prev state is pubkey, dest: %s, block: %s",
                               dest.ToString().c_str(), hashBlock.GetHex().c_str());
                        nPrevBalance = 0;
                    }
                    else
                    {
                        nPrevBalance = statePrev.GetBalance();
                    }
                }
                uint256 nChangeAmount;
                bool fAdd = false;
                if (state.GetBalance() > nPrevBalance)
                {
                    nChangeAmount = state.GetBalance() - nPrevBalance;
                    fAdd = true;
                }
                else
                {
                    nChangeAmount = nPrevBalance - state.GetBalance();
                }
                if (!addVote(destDelegate, nChangeAmount, fAdd))
                {
                    StdLog("BlockBase", "Update delegate: Add vote fail, dest: %s, block: %s",
                           dest.ToString().c_str(), hashBlock.GetHex().c_str());
                    return false;
                }
            }
        }
    }

    CBufStream ss;
    CVarInt var(block.vtx.size());
    uint32 nSetOffset = nOffset + block.GetTxSerializedOffset()
                        + ss.GetSerializeSize(block.txMint)
                        + ss.GetSerializeSize(var);
    for (int i = 0; i < block.vtx.size(); i++)
    {
        const CTransaction& tx = block.vtx[i];

        if (tx.GetTxType() == CTransaction::TX_CERT)
        {
            const CDestination& destToDelegateTemplate = tx.GetToAddress();
            uint256 nDelegateVote;
            if (!dbBlock.RetrieveDestDelegateVote(block.hashPrev, destToDelegateTemplate, nDelegateVote))
            {
                StdLog("BlockBase", "Update delegate: TX CERT find from vote fail, delegate address: %s, txid: %s",
                       destToDelegateTemplate.ToString().c_str(), tx.GetHash().GetHex().c_str());
                continue;
            }
            if (nDelegateVote < nMinEnrollAmount)
            {
                StdLog("BlockBase", "Update delegate: TX CERT not enough votes, delegate address: %s, delegate vote: %s, weight ratio: %s, txid: %s",
                       destToDelegateTemplate.ToString().c_str(), CoinToTokenBigFloat(nDelegateVote).c_str(),
                       CoinToTokenBigFloat(nMinEnrollAmount).c_str(), tx.GetHash().GetHex().c_str());
                continue;
            }

            int nCertAnchorHeight = tx.GetNonce();
            bytes btCertData;
            if (!tx.GetTxData(CTransaction::DF_CERTTXDATA, btCertData))
            {
                StdLog("BlockBase", "Update delegate: TX CERT tx data error1, delegate address: %s, delegate vote: %s, weight ratio: %s, txid: %s",
                       destToDelegateTemplate.ToString().c_str(), CoinToTokenBigFloat(nDelegateVote).c_str(),
                       CoinToTokenBigFloat(nMinEnrollAmount).c_str(), tx.GetHash().GetHex().c_str());
                return false;
            }
            mapEnrollTx[nCertAnchorHeight].insert(make_pair(destToDelegateTemplate, CDiskPos(nFile, nSetOffset)));
            StdTrace("BlockBase", "Update delegate: Enroll cert tx, anchor height: %d, nAmount: %s, vote: %s, delegate address: %s, txid: %s",
                     nCertAnchorHeight, CoinToTokenBigFloat(tx.GetAmount()).c_str(), CoinToTokenBigFloat(nDelegateVote).c_str(), destToDelegateTemplate.ToString().c_str(), tx.GetHash().GetHex().c_str());
        }
        nSetOffset += ss.GetSerializeSize(tx);
    }

    for (auto it = mapDelegateVote.begin(); it != mapDelegateVote.end(); ++it)
    {
        StdTrace("BlockBase", "Update delegate: delegate address: %s, votes: %s",
                 it->first.ToString().c_str(), CoinToTokenBigFloat(it->second).c_str());
    }

    if (!dbBlock.UpdateDelegateContext(block.hashPrev, hashBlock, mapDelegateVote, mapEnrollTx, hashDelegateRoot))
    {
        StdError("BlockBase", "Update delegate: Update delegate context failed, block: %s", hashBlock.ToString().c_str());
        return false;
    }
    return true;
}

bool CBlockBase::UpdateVote(const uint256& hashFork, const uint256& hashBlock, const CBlockEx& block,
                            const std::map<CDestination, CAddressContext>& mapAddressContext, const std::map<CDestination, CDestState>& mapAccStateIn,
                            const std::map<CDestination, std::pair<uint32, uint32>>& mapBlockModifyPledgeFinalHeightIn, uint256& hashVoteRoot)
{
    uint256 hashPrevStateRoot;
    if (!block.IsOrigin() && block.hashPrev != 0)
    {
        BlockIndexPtr pIndexPrev = GetIndex(block.hashPrev);
        if (!pIndexPrev)
        {
            StdLog("BlockBase", "Update vote: Get prev index fail, prev: %s", block.hashPrev.GetHex().c_str());
            return false;
        }
        hashPrevStateRoot = pIndexPrev->GetStateRoot();
    }

    auto funcGetMintRewardAmount = [&](const CDestination& dest) -> uint256 {
        uint256 nMintReward;
        if (block.txMint.GetToAddress() == dest)
        {
            nMintReward += block.txMint.GetAmount();
        }
        for (const CTransaction& tx : block.vtx)
        {
            if (tx.IsRewardTx() && tx.GetToAddress() == dest)
            {
                nMintReward += tx.GetAmount();
            }
        }
        return nMintReward;
    };
    auto funcCheckNewVote = [&](const CDestination& dest) -> bool {
        const uint256 nMintReward = funcGetMintRewardAmount(dest);

        auto it = mapAccStateIn.find(dest);
        if (it == mapAccStateIn.end())
        {
            return false;
        }
        const uint256 nNewBalance = it->second.GetBalance();

        uint256 nPrevBalance;
        CDestState statePrev;
        if (RetrieveDestState(hashFork, hashPrevStateRoot, dest, statePrev))
        {
            nPrevBalance = statePrev.GetBalance();
        }

        if (nNewBalance > nPrevBalance + nMintReward)
        {
            return true;
        }
        return false;
    };
    auto funcGetAddressContext = [&](const CDestination& dest, CAddressContext& ctxAddress) -> bool {
        auto it = mapAddressContext.find(dest);
        if (it != mapAddressContext.end())
        {
            ctxAddress = it->second;
        }
        else
        {
            if (!RetrieveAddressContext(hashFork, block.hashPrev, dest, ctxAddress))
            {
                return false;
            }
        }
        return true;
    };

    std::map<CDestination, CVoteContext> mapBlockVote;                         // key: vote address
    std::map<CDestination, std::pair<uint32, uint32>> mapAddPledgeFinalHeight; // key: pledge address, value first: final height, value second: pledge height
    std::map<CDestination, CPledgeVoteContext> mapPledgeVote;                  // key: pledge vote address

    for (auto& kv : mapAccStateIn)
    {
        const CDestination& dest = kv.first;
        const CDestState& state = kv.second;

        CAddressContext ctxAddress;
        if (!funcGetAddressContext(dest, ctxAddress))
        {
            StdError("BlockBase", "Update vote: Get address context failed, dest: %s, block: %s", dest.ToString().c_str(), hashBlock.ToString().c_str());
            return false;
        }

        if (ctxAddress.IsTemplate()
            && (ctxAddress.GetTemplateType() == TEMPLATE_DELEGATE
                || ctxAddress.GetTemplateType() == TEMPLATE_VOTE
                || ctxAddress.GetTemplateType() == TEMPLATE_PLEDGE))
        {
            CVoteContext ctxVote;
            if (!dbBlock.RetrieveDestVoteContext(block.hashPrev, dest, ctxVote))
            {
                ctxVote.SetNull();
                if (ctxAddress.GetTemplateType() == TEMPLATE_VOTE)
                {
                    if (!fEnableStakeVote)
                    {
                        continue;
                    }
                    CTemplateAddressContext ctxTemplate;
                    if (!ctxAddress.GetTemplateAddressContext(ctxTemplate))
                    {
                        StdError("BlockBase", "Update vote: Get template address context failed, dest: %s, block: %s", dest.ToString().c_str(), hashBlock.ToString().c_str());
                        return false;
                    }
                    CTemplatePtr ptr = CTemplate::Import(ctxTemplate.btData);
                    if (!ptr || ptr->GetTemplateType() != TEMPLATE_VOTE)
                    {
                        StdError("BlockBase", "Update vote: Import vote template fail or template type error, dest: %s, block: %s", dest.ToString().c_str(), hashBlock.ToString().c_str());
                        return false;
                    }
                    auto objVote = boost::dynamic_pointer_cast<CTemplateVote>(ptr);
                    ctxVote.destDelegate = objVote->destDelegate;
                    ctxVote.destOwner = objVote->destOwner;
                    ctxVote.SetRewardModeFlag(objVote->nRewardMode);
                    ctxVote.nRewardRate = PLEDGE_REWARD_PER;
                    ctxVote.nFinalHeight = block.GetBlockHeight() + VOTE_REDEEM_HEIGHT;
                }
                else if (ctxAddress.GetTemplateType() == TEMPLATE_PLEDGE)
                {
                    if (!fEnableStakePledge)
                    {
                        continue;
                    }
                    CTemplateAddressContext ctxTemplate;
                    if (!ctxAddress.GetTemplateAddressContext(ctxTemplate))
                    {
                        StdError("BlockBase", "Update vote: Get template address context failed, dest: %s, block: %s", dest.ToString().c_str(), hashBlock.ToString().c_str());
                        return false;
                    }
                    CTemplatePtr ptr = CTemplate::Import(ctxTemplate.btData);
                    if (!ptr || ptr->GetTemplateType() != TEMPLATE_PLEDGE)
                    {
                        StdError("BlockBase", "Update vote: Import pledge template fail or template type error, dest: %s, block: %s", dest.ToString().c_str(), hashBlock.ToString().c_str());
                        return false;
                    }
                    auto objPledge = boost::dynamic_pointer_cast<CTemplatePledge>(ptr);
                    ctxVote.destDelegate = objPledge->destDelegate;
                    ctxVote.destOwner = objPledge->destOwner;
                    if (VERIFY_FHX_HEIGHT_BRANCH_002(block.GetBlockHeight()))
                    {
                        ctxVote.SetRewardModeFlag(CVoteContext::REWARD_MODE_VOTE);
                        ctxVote.SetUnlimitCycesFlag(true);
                    }
                    else
                    {
                        ctxVote.SetRewardModeFlag(CVoteContext::REWARD_MODE_OWNER);
                    }
                    ctxVote.nRewardRate = objPledge->GetRewardRate(block.GetBlockHeight());
                    if (ctxVote.nRewardRate == 0)
                    {
                        StdLog("BlockBase", "Update vote: Get pledge reward rate fail, dest: %s, block: %s", dest.ToString().c_str(), hashBlock.ToString().c_str());
                        continue;
                    }
                    if (!objPledge->GetPledgeFinalHeight(block.GetBlockHeight(), ctxVote.nFinalHeight))
                    {
                        StdLog("BlockBase", "Update vote: Get pledge final height fail, dest: %s, block: %s", dest.ToString().c_str(), hashBlock.ToString().c_str());
                        continue;
                    }
                    mapAddPledgeFinalHeight[dest] = std::make_pair(ctxVote.nFinalHeight, block.GetBlockHeight());

                    if (VERIFY_FHX_HEIGHT_BRANCH_002(block.GetBlockHeight()))
                    {
                        if (state.GetBalance() >= DELEGATE_PROOF_OF_STAKE_MIN_VOTE_AMOUNT)
                        {
                            // first pledge vote
                            CPledgeVoteContext& ctxPledgeVote = mapPledgeVote[dest];

                            ctxPledgeVote.destVote = dest;
                            ctxPledgeVote.destDelegate = objPledge->destDelegate;
                            ctxPledgeVote.destOwner = objPledge->destOwner;
                            ctxPledgeVote.nPledgeType = objPledge->nPledgeType;
                            ctxPledgeVote.nCycles = objPledge->nCycles;
                            ctxPledgeVote.nNonce = objPledge->nNonce;
                            ctxPledgeVote.nVoteAmount = state.GetBalance();
                            ctxPledgeVote.nVoteTime = block.GetBlockTime();
                            ctxPledgeVote.nVoteHeight = block.GetBlockHeight();
                            ctxPledgeVote.nStopHeight = 0;
                            ctxPledgeVote.nRedeemHeight = 0;
                            ctxPledgeVote.nStatus = CPledgeVoteContext::PLEDGE_STATUS_VOTING;
                            ctxPledgeVote.nReVoteRewardAmount = 0;
                            ctxPledgeVote.nStopedRewardAmount = 0;
                            ctxPledgeVote.nTotalRewardAmount = 0;
                        }
                    }
                }
                else
                {
                    ctxVote.destDelegate = dest;
                    ctxVote.destOwner = ctxVote.destDelegate;
                    ctxVote.SetRewardModeFlag(CVoteContext::REWARD_MODE_VOTE);
                    ctxVote.nRewardRate = PLEDGE_REWARD_PER;
                    ctxVote.nFinalHeight = block.GetBlockHeight() + VOTE_REDEEM_HEIGHT;
                }
            }
            else
            {
                // vote data already exists
                if (ctxAddress.GetTemplateType() == TEMPLATE_PLEDGE)
                {
                    bool fModifyPledgeHeight = false;
                    if (VERIFY_FHX_HEIGHT_BRANCH_002(block.GetBlockHeight()))
                    {
                        CPledgeVoteContext ctxPledgeVote;
                        auto mt = mapPledgeVote.find(dest);
                        if (mt != mapPledgeVote.end())
                        {
                            ctxPledgeVote = mt->second;
                        }
                        else
                        {
                            if (!dbBlock.RetrieveDestPledgeVoteContext(block.hashPrev, dest, ctxPledgeVote))
                            {
                                CTemplateAddressContext ctxTemplate;
                                if (!ctxAddress.GetTemplateAddressContext(ctxTemplate))
                                {
                                    StdError("BlockBase", "Update vote: Get template address context failed, dest: %s, block: %s", dest.ToString().c_str(), hashBlock.ToString().c_str());
                                    return false;
                                }
                                CTemplatePtr ptr = CTemplate::Import(ctxTemplate.btData);
                                if (!ptr || ptr->GetTemplateType() != TEMPLATE_PLEDGE)
                                {
                                    StdError("BlockBase", "Update vote: Import pledge template fail or template type error, dest: %s, block: %s", dest.ToString().c_str(), hashBlock.ToString().c_str());
                                    return false;
                                }
                                auto objPledge = boost::dynamic_pointer_cast<CTemplatePledge>(ptr);

                                // first pledge vote (Compatible with original voting)
                                ctxPledgeVote.SetNull();

                                ctxPledgeVote.destVote = dest;
                                ctxPledgeVote.destDelegate = objPledge->destDelegate;
                                ctxPledgeVote.destOwner = objPledge->destOwner;
                                ctxPledgeVote.nPledgeType = objPledge->nPledgeType;
                                ctxPledgeVote.nCycles = objPledge->nCycles;
                                ctxPledgeVote.nNonce = objPledge->nNonce;
                            }
                        }

                        if (state.GetBalance() >= DELEGATE_PROOF_OF_STAKE_MIN_VOTE_AMOUNT)
                        {
                            if (ctxVote.nVoteAmount == 0)
                            {
                                // revote
                                fModifyPledgeHeight = true;
                                ctxVote.SetStopReVoteFlag(false);
                                ctxVote.SetRewardModeFlag(CVoteContext::REWARD_MODE_VOTE);
                                ctxVote.SetUnlimitCycesFlag(true);

                                ctxPledgeVote.nVoteAmount = state.GetBalance();
                                ctxPledgeVote.nVoteTime = block.GetBlockTime();
                                ctxPledgeVote.nVoteHeight = block.GetBlockHeight();
                                ctxPledgeVote.nStopHeight = 0;
                                ctxPledgeVote.nRedeemHeight = 0;
                                ctxPledgeVote.nStatus = CPledgeVoteContext::PLEDGE_STATUS_VOTING;
                                ctxPledgeVote.nReVoteRewardAmount = 0;
                                ctxPledgeVote.nStopedRewardAmount = 0;

                                mapPledgeVote[dest] = ctxPledgeVote;
                            }
                            else
                            {
                                // append vote
                                ctxPledgeVote.nVoteAmount = state.GetBalance();
                                if (ctxPledgeVote.nStatus == CPledgeVoteContext::PLEDGE_STATUS_INIT)
                                {
                                    ctxPledgeVote.nVoteTime = block.GetBlockTime();
                                    ctxPledgeVote.nVoteHeight = block.GetBlockHeight();
                                    ctxPledgeVote.nStopHeight = 0;
                                    ctxPledgeVote.nRedeemHeight = 0;
                                    ctxPledgeVote.nStatus = CPledgeVoteContext::PLEDGE_STATUS_VOTING;
                                    ctxPledgeVote.nReVoteRewardAmount = 0;
                                    ctxPledgeVote.nStopedRewardAmount = 0;

                                    ctxVote.SetStopReVoteFlag(false);
                                    ctxVote.SetRewardModeFlag(CVoteContext::REWARD_MODE_VOTE);
                                    ctxVote.SetUnlimitCycesFlag(true);
                                }
                                mapPledgeVote[dest] = ctxPledgeVote;
                            }
                        }
                        else
                        {
                            if (ctxVote.nVoteAmount > 0 && ctxVote.GetRewardModeFlag() == CVoteContext::REWARD_MODE_VOTE)
                            {
                                // redeem
                                ctxPledgeVote.nVoteAmount = 0;
                                ctxPledgeVote.nVoteTime = 0;
                                ctxPledgeVote.nVoteHeight = 0;
                                ctxPledgeVote.nStopHeight = 0;
                                ctxPledgeVote.nRedeemHeight = 0;
                                ctxPledgeVote.nStatus = CPledgeVoteContext::PLEDGE_STATUS_INIT;
                                // ctxPledgeVote.nReVoteRewardAmount = 0;
                                // ctxPledgeVote.nStopedRewardAmount = 0;

                                mapPledgeVote[dest] = ctxPledgeVote;
                            }
                        }
                    }
                    else
                    {
                        if (ctxVote.nFinalHeight > 0 && block.GetBlockHeight() >= ctxVote.nFinalHeight
                            && state.GetBalance() > ctxVote.nVoteAmount && state.GetBalance() >= DELEGATE_PROOF_OF_STAKE_MIN_VOTE_AMOUNT)
                        {
                            fModifyPledgeHeight = true;
                        }
                    }
                    if (fModifyPledgeHeight)
                    {
                        CTemplateAddressContext ctxTemplate;
                        if (!ctxAddress.GetTemplateAddressContext(ctxTemplate))
                        {
                            StdError("BlockBase", "Update vote: Get template address context failed, dest: %s, block: %s", dest.ToString().c_str(), hashBlock.ToString().c_str());
                            return false;
                        }
                        CTemplatePtr ptr = CTemplate::Import(ctxTemplate.btData);
                        if (!ptr || ptr->GetTemplateType() != TEMPLATE_PLEDGE)
                        {
                            StdError("BlockBase", "Update vote: Import pledge template fail or template type error, dest: %s, block: %s", dest.ToString().c_str(), hashBlock.ToString().c_str());
                            return false;
                        }
                        auto objPledge = boost::dynamic_pointer_cast<CTemplatePledge>(ptr);
                        ctxVote.nRewardRate = objPledge->GetRewardRate(block.GetBlockHeight());
                        if (ctxVote.nRewardRate == 0)
                        {
                            StdLog("BlockBase", "Update vote: Get pledge reward rate fail, dest: %s, block: %s", dest.ToString().c_str(), hashBlock.ToString().c_str());
                            continue;
                        }
                        if (!objPledge->GetPledgeFinalHeight(block.GetBlockHeight(), ctxVote.nFinalHeight))
                        {
                            StdLog("BlockBase", "Update vote: Get pledge final height fail, dest: %s, block: %s", dest.ToString().c_str(), hashBlock.ToString().c_str());
                            continue;
                        }
                        mapAddPledgeFinalHeight[dest] = std::make_pair(ctxVote.nFinalHeight, block.GetBlockHeight());
                    }
                }
                else
                {
                    if (funcCheckNewVote(dest))
                    {
                        ctxVote.nFinalHeight = block.GetBlockHeight() + VOTE_REDEEM_HEIGHT;
                    }
                }
            }
            if (state.GetBalance() < DELEGATE_PROOF_OF_STAKE_MIN_VOTE_AMOUNT)
            {
                if (ctxVote.nVoteAmount != 0)
                {
                    ctxVote.nVoteAmount = 0;
                    mapBlockVote[dest] = ctxVote;
                }
            }
            else
            {
                ctxVote.nVoteAmount = state.GetBalance();
                mapBlockVote[dest] = ctxVote;
            }
        }
    }

    std::map<CDestination, uint32> mapRemovePledgeFinalHeight; // key: pledge address, value: old final height
    for (auto& kv : mapBlockModifyPledgeFinalHeightIn)
    {
        const auto& destPledge = kv.first;
        const uint32 nNewFinalHeight = kv.second.first;
        CVoteContext ctxVote;
        auto it = mapBlockVote.find(destPledge);
        if (it != mapBlockVote.end())
        {
            ctxVote = it->second;
        }
        else
        {
            if (!dbBlock.RetrieveDestVoteContext(block.hashPrev, destPledge, ctxVote))
            {
                continue;
            }
        }
        bool fModify = false;
        if (VERIFY_FHX_HEIGHT_BRANCH_002(block.GetBlockHeight()))
        {
            if (!ctxVote.GetStopReVoteFlag() || ctxVote.nFinalHeight != nNewFinalHeight)
            {
                ctxVote.SetStopReVoteFlag(true);
                fModify = true;

                bool fGetPledge = false;
                CPledgeVoteContext ctxPledgeVote;
                auto mt = mapPledgeVote.find(destPledge);
                if (mt != mapPledgeVote.end())
                {
                    ctxPledgeVote = mt->second;
                    fGetPledge = true;
                }
                else
                {
                    if (dbBlock.RetrieveDestPledgeVoteContext(block.hashPrev, destPledge, ctxPledgeVote))
                    {
                        fGetPledge = true;
                    }
                }
                if (fGetPledge)
                {
                    ctxPledgeVote.nStopHeight = block.GetBlockHeight();
                    ctxPledgeVote.nRedeemHeight = nNewFinalHeight;
                    ctxPledgeVote.nStatus = CPledgeVoteContext::PLEDGE_STATUS_REQSTOPED;

                    mapPledgeVote[destPledge] = ctxPledgeVote;
                }
            }
        }
        else
        {
            if (ctxVote.nFinalHeight != nNewFinalHeight)
            {
                fModify = true;
            }
        }
        if (fModify)
        {
            mapRemovePledgeFinalHeight[destPledge] = ctxVote.nFinalHeight;
            ctxVote.nFinalHeight = nNewFinalHeight;
            mapBlockVote[destPledge] = ctxVote;
            mapAddPledgeFinalHeight[destPledge] = kv.second;
        }
    }

    if (VERIFY_FHX_HEIGHT_BRANCH_002(block.GetBlockHeight()))
    {
        auto funcAddPledgeVoteReward = [&](const CDestination& destVote, const bool fReVote, const uint256& nRewardAmount) -> bool {
            CPledgeVoteContext ctxPledgeVote;
            auto mt = mapPledgeVote.find(destVote);
            if (mt != mapPledgeVote.end())
            {
                ctxPledgeVote = mt->second;
            }
            else
            {
                if (!dbBlock.RetrieveDestPledgeVoteContext(block.hashPrev, destVote, ctxPledgeVote))
                {
                    StdLog("BlockBase", "Update vote: Retrieve vote address context fail, vote address: %s, block: %s",
                           destVote.ToString().c_str(), hashBlock.GetHex().c_str());
                    return false;
                }
            }
            if (fReVote)
            {
                ctxPledgeVote.nReVoteRewardAmount += nRewardAmount;
            }
            else
            {
                if (ctxPledgeVote.nStatus != CPledgeVoteContext::PLEDGE_STATUS_VOTING)
                {
                    ctxPledgeVote.nStopedRewardAmount += nRewardAmount;
                }
            }
            ctxPledgeVote.nTotalRewardAmount += nRewardAmount;

            mapPledgeVote[destVote] = ctxPledgeVote;
            return true;
        };

        for (const auto& tx : block.vtx)
        {
            if (tx.GetTxType() == CTransaction::TX_VOTE_REWARD)
            {
                bytes btRewardListData;
                if (tx.GetTxData(CTransaction::DF_VOTEREWARDLIST, btRewardListData) && btRewardListData.size() > 0)
                {
                    try
                    {
                        std::map<CDestination, pair<uint256, uint8>> mapSubReward; // key: vote address, value1: reward amount, value2: vote address template type
                        CBufStream ss(btRewardListData);
                        ss >> mapSubReward;

                        for (auto& kv : mapSubReward)
                        {
                            if (kv.second.second == TEMPLATE_PLEDGE)
                            {
                                funcAddPledgeVoteReward(kv.first, false, kv.second.first);
                            }
                        }
                    }
                    catch (std::exception& e)
                    {
                        StdLog("BlockBase", "Update vote: Parser tx extdata fail, to: %s, block: %s",
                               tx.GetToAddress().ToString().c_str(), hashBlock.GetHex().c_str());
                        return false;
                    }
                }
                else
                {
                    CAddressContext ctxAddress;
                    if (!tx.GetToAddressData(ctxAddress))
                    {
                        if (!RetrieveAddressContext(hashFork, block.hashPrev, tx.GetToAddress(), ctxAddress))
                        {
                            StdLog("BlockBase", "Update vote: Retrieve reward address context fail, reward address: %s, block: %s",
                                   tx.GetToAddress().ToString().c_str(), hashBlock.GetHex().c_str());
                            return false;
                        }
                    }
                    if (ctxAddress.IsTemplate() && ctxAddress.GetTemplateType() == TEMPLATE_PLEDGE)
                    {
                        funcAddPledgeVoteReward(tx.GetToAddress(), true, tx.GetAmount());
                    }
                }
            }
        }
    }

    // key: pledge address, value first: final height, value second: pledge height
    // for (auto& kv : mapAddPledgeFinalHeight)
    // {
    //     StdDebug("BlockBase", "Update vote: Update address final height, address: %s, final height: %d, block: [%d] %s",
    //              kv.first.ToString().c_str(), kv.second.first, CBlock::GetBlockHeightByHash(hashBlock), hashBlock.ToString().c_str());
    // }

    if (!dbBlock.AddBlockVote(block.hashPrev, hashBlock, mapBlockVote, mapAddPledgeFinalHeight, mapRemovePledgeFinalHeight, mapPledgeVote, hashVoteRoot))
    {
        StdError("BlockBase", "Update vote: Add block vote failed, block: %s", hashBlock.ToString().c_str());
        return false;
    }
    return true;
}

bool CBlockBase::IsValidBlock(const BlockIndexPtr& pForkLast, const uint256& hashBlock)
{
    // if (hashBlock != 0)
    // {
    //     int nBlockHeight = CBlock::GetBlockHeightByHash(hashBlock);
    //     BlockIndexPtr pIndex = pForkLast;
    //     while (pIndex && pIndex->GetBlockHeight() >= nBlockHeight)
    //     {
    //         if (pIndex->GetBlockHeight() == nBlockHeight && pIndex->GetBlockHash() == hashBlock)
    //         {
    //             return true;
    //         }
    //         pIndex = GetPrevBlockIndex(pIndex);
    //     }
    // }

    if (hashBlock != 0)
    {
        uint256 hashObjBlock;
        if (GetBlockHashByHeightSlot(pForkLast->GetOriginHash(), pForkLast->GetBlockHash(),
                                     CBlock::GetBlockHeightByHash(hashBlock),
                                     CBlock::GetBlockSlotByHash(hashBlock), hashObjBlock)
            && hashBlock == hashObjBlock)
        {
            return true;
        }
    }
    return false;
}

bool CBlockBase::VerifyValidBlock(const BlockIndexPtr& pIndexGenesisLast, const BlockIndexPtr& pIndex)
{
    if (pIndex->IsOrigin())
    {
        return true;
    }

    uint256 hashRefBlock;
    std::map<uint256, CBlockHeightIndex> mapHeightIndex;
    if (GetForkBlockMintListByHeight(pIndex->GetOriginHash(), pIndex->GetBlockHeight(), mapHeightIndex))
    {
        auto mt = mapHeightIndex.find(pIndex->GetBlockHash());
        if (mt != mapHeightIndex.end() && mt->second.hashRefBlock != 0)
        {
            hashRefBlock = mt->second.hashRefBlock;
        }
    }
    if (hashRefBlock == 0)
    {
        CBlockEx block;
        if (!Retrieve(pIndex, block))
        {
            return false;
        }
        CProofOfPiggyback proof;
        if (block.GetPiggybackProof(proof) && proof.hashRefBlock != 0)
        {
            hashRefBlock = proof.hashRefBlock;
        }
        if (hashRefBlock == 0)
        {
            return false;
        }
    }
    return IsValidBlock(pIndexGenesisLast, hashRefBlock);
}

bool CBlockBase::VerifyBlockConfirmChain(const BlockIndexPtr& pNewIndex)
{
    if (!pNewIndex)
    {
        return false;
    }
    const uint256 hashFork = pNewIndex->GetOriginHash();
    uint256 hashLastConfirmBlock;
    if (dbBlock.GetLastConfirmBlock(hashFork, hashLastConfirmBlock))
    {
        BlockIndexPtr pIndex = pNewIndex;
        while (pIndex && !pIndex->IsOrigin())
        {
            auto pNextIndex = GetNextBlockIndex(pIndex);
            if (pNextIndex && VerifyBlockConfirmNoLock(hashFork, pNextIndex->GetBlockHash(), hashLastConfirmBlock))
            {
                return false;
            }
            if (VerifyBlockConfirmNoLock(hashFork, pIndex->GetBlockHash(), hashLastConfirmBlock))
            {
                break;
            }
            pIndex = GetPrevBlockIndex(pIndex);
        }
    }
    return true;
}

bool CBlockBase::VerifyBlockConfirmNoLock(const uint256& hashFork, const uint256& hashBlock, const uint256& hashLastConfirmBlock)
{
    if (hashBlock == 0)
    {
        return nullptr;
    }
    CForkHeightIndex& indexHeight = it->second;
    CBlockIndex* pMaxTrustIndex = nullptr;
    while (1)
    {
        std::map<uint256, CBlockHeightIndex>* pHeightIndex = indexHeight.GetBlockMintList(nStartHeight);
        if (pHeightIndex == nullptr)
        {
            break;
        }
        auto mt = pHeightIndex->begin();
        for (; mt != pHeightIndex->end(); ++mt)
        {
            const uint256& hashBlock = mt->first;
            if (setInvalidHash.count(hashBlock) == 0)
            {
                CBlockIndex* pIndex;
                if (!(pIndex = GetIndex(hashBlock)))
                {
                    StdError("BlockBase", "GetLongChainLastBlock GetIndex failed, block: %s", hashBlock.ToString().c_str());
                }
                else if (!pIndex->IsOrigin())
                {
                    if (VerifyValidBlock(pIndexGenesisLast, pIndex))
                    {
                        if (pMaxTrustIndex == nullptr)
                        {
                            pMaxTrustIndex = pIndex;
                        }
                        else if (!(pMaxTrustIndex->nChainTrust > pIndex->nChainTrust
                                   || (pMaxTrustIndex->nChainTrust == pIndex->nChainTrust && !pIndex->IsEquivalent(pMaxTrustIndex))))
                        {
                            pMaxTrustIndex = pIndex;
                        }
                    }
                }
            }
        }
        nStartHeight++;
    }
    return pMaxTrustIndex;
}

// bool CBlockBase::GetBlockDelegateAddress(const uint256& hashFork, const uint256& hashBlock, const CBlock& block, const std::map<CDestination, CAddressContext>& mapAddressContext, std::map<CDestination, CDestination>& mapDelegateAddress)
// {
//     auto getAddress = [&](const CDestination& dest) -> bool
//     {
//         if (dest.IsNull())
//         {
//             return true;
//         }
//         if (mapDelegateAddress.count(dest) == 0)
//         {
//             auto it = mapAddressContext.find(dest);
//             if (it == mapAddressContext.end())
//             {
//                 StdLog("BlockBase", "Get Block Delegate Address: Find address context fail, dest: %s", dest.ToString().c_str());
//                 return false;
//             }
//             const CAddressContext& ctxAddress = it->second;
//             if (!ctxAddress.IsTemplate())
//             {
//                 return true;
//             }
//             CTemplateAddressContext ctxtTemplate;
//             if (!ctxAddress.GetTemplateAddressContext(ctxtTemplate))
//             {
//                 StdLog("BlockBase", "Get Block Delegate Address: Get template address context fail, dest: %s", dest.ToString().c_str());
//                 return false;
//             }
//             CTemplatePtr ptr = CTemplate::Import(ctxtTemplate.btData);
//             if (!ptr)
//             {
//                 StdLog("BlockBase", "Get Block Delegate Address: Import template fail, dest: %s", dest.ToString().c_str());
//                 return false;
//             }
//             uint8 nTidType = ptr->GetTemplateType();
//             if (nTidType == TEMPLATE_DELEGATE)
//             {
//                 mapDelegateAddress.insert(make_pair(dest, dest));
//             }
//             else if (nTidType == TEMPLATE_VOTE)
//             {
//                 auto vote = boost::dynamic_pointer_cast<CTemplateVote>(ptr);
//                 mapDelegateAddress.insert(make_pair(dest, vote->destDelegate));
//             }
//         }
//         return true;
//     };

//     if (!getAddress(block.txMint.GetToAddress()))
//     {
//         StdLog("BlockBase", "Get Block Delegate Address: Get mint to failed, mint to: %s", block.txMint.GetToAddress().ToString().c_str());
//         return false;
//     }
//     for (const CTransaction& tx : block.vtx)
//     {
//         if (!getAddress(tx.GetFromAddress()))
//         {
//             StdLog("BlockBase", "Get Block Delegate Address: Get from failed, from: %s, txid: %s",
//                    tx.GetFromAddress().ToString().c_str(), tx.GetHash().ToString().c_str());
//             return false;
//         }
//         if (!getAddress(tx.GetToAddress()))
//         {
//             StdLog("BlockBase", "Get Block Delegate Address: Get to failed, to: %s, txid: %s",
//                    tx.GetToAddress().ToString().c_str(), tx.GetHash().ToString().c_str());
//             return false;
//         }
//     }
//     return true;
// }

bool CBlockBase::GetTxContractData(const uint32 nTxFile, const uint32 nTxOffset, CTxContractData& txcdCode, uint256& txidCreate)
{
    CTransaction tx;
    if (!tsBlock.Read(tx, nTxFile, nTxOffset, false, true))
    {
        StdLog("BlockBase", "Get tx contract data: Read fail, nTxFile: %d, nTxOffset: %d", nTxFile, nTxOffset);
        return false;
    }
    if (!tx.GetToAddress().IsNull())
    {
        StdLog("BlockBase", "Get tx contract data: Read fail, to not is null contract address, to: %s, nTxFile: %d, nTxOffset: %d",
               tx.GetToAddress().ToString().c_str(), nTxFile, nTxOffset);
        return false;
    }
    uint8 nCodeType;
    CTemplateContext ctxTemplate;
    if (!tx.GetCreateCodeContext(nCodeType, ctxTemplate, txcdCode))
    {
        StdLog("BlockBase", "Get tx contract data: Get create code fail, txid: %s, from: %s",
               tx.GetHash().GetHex().c_str(), tx.GetFromAddress().ToString().c_str());
        return false;
    }
    if (nCodeType != CODE_TYPE_CONTRACT)
    {
        StdLog("BlockBase", "Get tx contract data: Code type error, nTxFile: %d, nTxOffset: %d", nTxFile, nTxOffset);
        return false;
    }
    txidCreate = tx.GetHash();
    return true;
}

bool CBlockBase::GetBlockSourceCodeData(const uint256& hashFork, const uint256& hashBlock, const uint256& hashSourceCode, CTxContractData& txcdCode)
{
    CContractSourceCodeContext ctxtCode;
    if (!dbBlock.RetrieveSourceCodeContext(hashFork, hashBlock, hashSourceCode, ctxtCode))
    {
        StdLog("BlockBase", "Get block source code data: Retrieve source code context fail, code: %s, block: %s",
               hashSourceCode.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    uint32 nFile, nOffset;
    if (!ctxtCode.GetLastPos(nFile, nOffset))
    {
        StdLog("BlockBase", "Get block source code data: Get last pos fail, code: %s, block: %s",
               hashSourceCode.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    uint256 txidCreate;
    if (!GetTxContractData(nFile, nOffset, txcdCode, txidCreate))
    {
        StdLog("BlockBase", "Get block source code data: Get tx contract data fail, code: %s, block: %s",
               hashSourceCode.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CBlockBase::GetBlockContractCreateCodeData(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractCreateCode, CTxContractData& txcdCode)
{
    CContractCreateCodeContext ctxtCode;
    bool fLinkGenesisFork;
    if (!RetrieveLinkGenesisContractCreateCodeContext(hashFork, hashBlock, hashContractCreateCode, ctxtCode, fLinkGenesisFork))
    {
        StdLog("BlockBase", "Get block contract create code data: Retrieve contract create code context fail, code: %s, block: %s",
               hashContractCreateCode.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }

    txcdCode.nMuxType = 1;
    txcdCode.strType = ctxtCode.strType;
    txcdCode.strName = ctxtCode.strName;
    txcdCode.destCodeOwner = ctxtCode.destCodeOwner;
    txcdCode.nCompressDescribe = 0;
    txcdCode.btDescribe.assign(ctxtCode.strDescribe.begin(), ctxtCode.strDescribe.end());
    txcdCode.nCompressCode = 0;
    txcdCode.btCode = ctxtCode.btCreateCode; // contract create code
    txcdCode.nCompressSource = 0;
    txcdCode.btSource.clear(); // source code
    return true;
}

bool CBlockBase::RetrieveForkContractCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractCreateCode, CContractCreateCodeContext& ctxtCode)
{
    return dbBlock.RetrieveContractCreateCodeContext(hashFork, hashBlock, hashContractCreateCode, ctxtCode);
}

bool CBlockBase::RetrieveLinkGenesisContractCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractCreateCode, CContractCreateCodeContext& ctxtCode, bool& fLinkGenesisFork)
{
    fLinkGenesisFork = false;
    if (dbBlock.RetrieveContractCreateCodeContext(hashFork, hashBlock, hashContractCreateCode, ctxtCode))
    {
        return true;
    }
    if (hashFork != GetGenesisBlockHash())
    {
        CBlockIndex* pSubIndex = GetIndex(hashBlock);
        if (pSubIndex && pSubIndex->hashRefBlock != 0)
        {
            if (dbBlock.RetrieveContractCreateCodeContext(GetGenesisBlockHash(), pSubIndex->hashRefBlock, hashContractCreateCode, ctxtCode))
            {
                fLinkGenesisFork = true;
                return true;
            }
        }
    }
    return false;
}

bool CBlockBase::RetrieveContractRunCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractRunCode, CContractRunCodeContext& ctxtCode)
{
    return dbBlock.RetrieveContractRunCodeContext(hashFork, hashBlock, hashContractRunCode, ctxtCode);
}

bool CBlockBase::GetForkContractCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractCreateCode, CContractCodeContext& ctxtContractCode)
{
    CContractCreateCodeContext ctxtCode;
    if (!RetrieveForkContractCreateCodeContext(hashFork, hashBlock, hashContractCreateCode, ctxtCode))
    {
        return false;
    }
    ctxtContractCode.hashCode = ctxtCode.GetContractCreateCodeHash();
    ctxtContractCode.hashSource = ctxtCode.hashSourceCode;
    ctxtContractCode.strType = ctxtCode.strType;
    ctxtContractCode.strName = ctxtCode.strName;
    ctxtContractCode.destOwner = ctxtCode.destCodeOwner;
    ctxtContractCode.strDescribe = ctxtCode.strDescribe;
    ctxtContractCode.hashCreateTxid = ctxtCode.txidCreate;
    return true;
}

bool CBlockBase::ListContractCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& txid, std::map<uint256, CContractCodeContext>& mapCreateCode)
{
    std::map<uint256, CContractCreateCodeContext> mapContractCreateCode;
    if (!dbBlock.ListContractCreateCodeContext(hashFork, hashBlock, mapContractCreateCode))
    {
        return false;
    }
    for (const auto& kv : mapContractCreateCode)
    {
        CContractCodeContext ctxt;

        ctxt.hashCode = kv.second.GetContractCreateCodeHash();
        ctxt.hashSource = kv.second.hashSourceCode;
        ctxt.strType = kv.second.strType;
        ctxt.strName = kv.second.strName;
        ctxt.destOwner = kv.second.destCodeOwner;
        ctxt.strDescribe = kv.second.strDescribe;
        ctxt.hashCreateTxid = kv.second.txidCreate;

        mapCreateCode.insert(make_pair(kv.first, ctxt));
    }
    return true;
}

bool CBlockBase::VerifyContractAddress(const uint256& hashFork, const uint256& hashBlock, const CDestination& destContract)
{
    uint256 hashStateRoot;
    {
        CReadLock rlock(rwAccess);
        CBlockIndex* pIndex = GetIndex(hashBlock);
        if (pIndex == nullptr)
        {
            StdLog("CBlockBase", "Verify contract address: Get index fail, hashBlock: %s", hashBlock.ToString().c_str());
            return false;
        }
        hashStateRoot = pIndex->hashStateRoot;
    }

    CDestState stateDest;
    if (!RetrieveDestState(hashFork, hashStateRoot, destContract, stateDest))
    {
        StdLog("CBlockBase", "Verify contract address: Retrieve dest state fail, state root: %s, destContract: %s, hashBlock: %s",
               hashStateRoot.GetHex().c_str(), destContract.ToString().c_str(), hashBlock.ToString().c_str());
        return false;
    }

    bytes btDestCodeData;
    if (!RetrieveContractKvValue(hashFork, stateDest.GetStorageRoot(), destContract.ToHash(), btDestCodeData))
    {
        StdLog("CBlockBase", "Verify contract address: Retrieve contract state fail, storage root: %s, block: %s, destContract: %s",
               stateDest.GetStorageRoot().GetHex().c_str(), hashBlock.GetHex().c_str(), destContract.ToString().c_str());
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
        StdLog("CBlockBase", "Verify contract address: Parse contract code fail, block: %s, destContract: %s",
               hashBlock.GetHex().c_str(), destContract.ToString().c_str());
        return false;
    }

    CContractRunCodeContext ctxtRunCode;
    if (!RetrieveContractRunCodeContext(hashFork, hashBlock, ctxDestCode.hashContractRunCode, ctxtRunCode))
    {
        StdLog("CBlockBase", "Verify contract address: Retrieve contract run code fail, block: %s, destContract: %s",
               hashBlock.GetHex().c_str(), destContract.ToString().c_str());
        return false;
    }

    // CContractCreateCodeContext ctxtCreateCode;
    // bool fLinkGenesisFork;
    // if (!RetrieveLinkGenesisContractCreateCodeContext(hashFork, hashBlock, ctxDestCode.hashContractCreateCode, ctxtCreateCode, fLinkGenesisFork))
    // {
    //     StdLog("CBlockBase", "Verify contract address: Retrieve contract create code fail, hashContractCreateCode: %s, block: %s, destContract: %s",
    //            ctxDestCode.hashContractCreateCode.ToString().c_str(), hashBlock.GetHex().c_str(), destContract.ToString().c_str());
    //     return false;
    // }
    return true;
}

bool CBlockBase::VerifyCreateCodeTx(const uint256& hashFork, const uint256& hashBlock, const CTransaction& tx)
{
    uint8 nCodeType;
    CTemplateContext ctxTemplate;
    CTxContractData txcdCode;
    if (!tx.GetCreateCodeContext(nCodeType, ctxTemplate, txcdCode))
    {
        StdLog("CBlockBase", "Verify create code tx: Get create code fail, tx: %s", tx.GetHash().GetHex().c_str());
        return false;
    }

    if (nCodeType == CODE_TYPE_TEMPLATE)
    {
        CTemplatePtr ptr = CTemplate::Import(ctxTemplate.GetTemplateData());
        if (!ptr)
        {
            StdLog("CBlockBase", "Verify create code tx: Import template fail, tx: %s", tx.GetHash().GetHex().c_str());
            return false;
        }
    }
    else
    {
        uint256 hashContractCreateCode;
        if (txcdCode.IsCreate() || txcdCode.IsUpcode())
        {
            // txcdCode.UncompressCode();
            // hashContractCreateCode = txcdCode.GetContractCreateCodeHash();

            // if (txcdCode.IsCreate() && fEnableContractCodeVerify)
            // {
            //     CContractCreateCodeContext ctxtContractCreateCode;
            //     bool fLinkGenesisFork;
            //     if (!RetrieveLinkGenesisContractCreateCodeContext(hashFork, hashBlock, hashContractCreateCode, ctxtContractCreateCode, fLinkGenesisFork))
            //     {
            //         StdLog("CBlockState", "Verify create code tx: Code not exist, code: %s, tx: %s, block: %s",
            //                hashContractCreateCode.GetHex().c_str(), tx.GetHash().GetHex().c_str(), hashBlock.GetHex().c_str());
            //         return false;
            //     }
            //     // if (!ctxtContractCreateCode.IsActivated())
            //     // {
            //     //     StdLog("CBlockState", "Verify create code tx: Code not activate, code: %s, tx: %s, block: %s",
            //     //            hashContractCreateCode.GetHex().c_str(), tx.GetHash().GetHex().c_str(), hashBlock.GetHex().c_str());
            //     //     return false;
            //     // }
            // }
        }
        else if (txcdCode.IsSetup())
        {
            CTxContractData txcd;
            hashContractCreateCode = txcdCode.GetContractCreateCodeHash();
            if (!GetBlockContractCreateCodeData(hashFork, hashBlock, hashContractCreateCode, txcd))
            {
                StdLog("CBlockBase", "Verify create code tx: Get create code data fail, code hash: %s, tx: %s, block: %s",
                       hashContractCreateCode.GetHex().c_str(), tx.GetHash().GetHex().c_str(), hashBlock.GetHex().c_str());
                return false;
            }
        }
        else
        {
            StdLog("CBlockBase", "Verify create code tx: Code flag error, flag: %d, tx: %s, block: %s",
                   txcdCode.nMuxType, tx.GetHash().GetHex().c_str(), hashBlock.GetHex().c_str());
            return false;
        }
    }
    return true;
}

bool CBlockBase::GetAddressTxCount(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, uint64& nTxCount)
{
    return dbBlock.GetAddressTxCount(hashFork, hashBlock, dest, nTxCount);
}

bool CBlockBase::RetrieveAddressTxInfo(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, const uint64 nTxIndex, CDestTxInfo& ctxtAddressTxInfo)
{
    return dbBlock.RetrieveAddressTxInfo(hashFork, hashBlock, dest, nTxIndex, ctxtAddressTxInfo);
}

bool CBlockBase::ListAddressTxInfo(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, const uint64 nBeginTxIndex, const uint64 nGetTxCount, const bool fReverse, std::vector<CDestTxInfo>& vAddressTxInfo)
{
    return dbBlock.ListAddressTxInfo(hashFork, hashBlock, dest, nBeginTxIndex, nGetTxCount, fReverse, vAddressTxInfo);
}

bool CBlockBase::GetVoteRewardLockedAmount(const uint256& hashFork, const uint256& hashPrevBlock, const CDestination& dest, uint256& nLockedAmount)
{
    CBlockOutline outline;
    if (!dbBlock.RetrieveBlockIndex(hashPrevBlock, outline))
    {
        StdLog("CBlockBase", "Get Vote Reward Locked Amount: Retrieve fork context failed, prev block: %s", hashPrevBlock.ToString().c_str());
        return false;
    }
    CForkContext ctxFork;
    if (!dbBlock.RetrieveForkContext(hashFork, ctxFork, outline.hashRefBlock))
    {
        StdLog("CBlockBase", "Get Vote Reward Locked Amount: Retrieve fork context failed, prev block: %s", hashPrevBlock.ToString().c_str());
        return false;
    }

    const uint32 nPrevHeight = CBlock::GetBlockHeightByHash(hashPrevBlock);
    const uint32 nVoteRewardLockDays = VOTE_REWARD_LOCK_DAYS;
    std::vector<std::pair<uint32, uint256>> vVoteReward;
    if (!dbBlock.ListVoteReward(ctxFork.nChainId, hashPrevBlock, dest, nVoteRewardLockDays, vVoteReward))
    {
        StdLog("CBlockBase", "Get Vote Reward Locked Amount: List reward lock fail, dest: %s, fork: %s", dest.ToString().c_str(), hashFork.GetHex().c_str());
        return false;
    }
    nLockedAmount = 0;
    if (!vVoteReward.empty())
    {
        for (const auto& vd : vVoteReward)
        {
            const uint32& nRewardHeight = vd.first;
            const uint256& nRewardAmount = vd.second;
            uint32 nElapsedDays = (nPrevHeight + 1 - nRewardHeight) / VOTE_REWARD_LOCK_DAY_HEIGHT;
            if (nElapsedDays == 0)
            {
                nLockedAmount += nRewardAmount;
            }
            else if (nElapsedDays < nVoteRewardLockDays)
            {
                uint32 nLockDays = nVoteRewardLockDays - nElapsedDays;
                nLockedAmount += (nRewardAmount * uint256(nLockDays) / uint256(nVoteRewardLockDays));
            }
        }
    }
    return true;
}

bool CBlockBase::GetBlockAddress(const uint256& hashFork, const uint256& hashBlock, const CBlock& block, std::map<CDestination, CAddressContext>& mapBlockAddress)
{
    auto loadAddress = [&](const CDestination& dest) -> bool {
        auto it = mapBlockAddress.find(dest);
        if (it == mapBlockAddress.end())
        {
            CAddressContext ctxAddress;
            if (!RetrieveAddressContext(hashFork, block.hashPrev, dest, ctxAddress))
            {
                // StdLog("CBlockBase", "Verify block address: Retrieve address context fail, dest: %s, fork: %s, prev: %s",
                //        dest.ToString().c_str(), hashFork.ToString().c_str(), block.hashPrev.ToString().c_str());
                return false;
            }
            mapBlockAddress.insert(make_pair(dest, ctxAddress));
        }
        return true;
    };

    auto verifyAddress = [&](const CTransaction& tx) -> bool {
        if (!tx.GetFromAddress().IsNull())
        {
            if (!loadAddress(tx.GetFromAddress()))
            {
                StdLog("CBlockBase", "Verify block address: Load from address fail, from: %s, txid: %s, block: %s",
                       tx.GetFromAddress().ToString().c_str(), tx.GetHash().GetHex().c_str(), hashBlock.ToString().c_str());
                return false;
            }
        }

        if (tx.GetToAddress().IsNull())
        {
            if (tx.GetFromAddress().IsNull())
            {
                StdLog("CBlockBase", "Verify block address: Create contract from address is null, from: %s, txid: %s, block: %s",
                       tx.GetFromAddress().ToString().c_str(), tx.GetHash().GetHex().c_str(), hashBlock.ToString().c_str());
                return false;
            }
            uint8 nCodeType;
            CTemplateContext ctxTemplate;
            CTxContractData ctxContract;
            if (!tx.GetCreateCodeContext(nCodeType, ctxTemplate, ctxContract))
            {
                StdLog("CBlockBase", "Verify block address: Get create code fail, to: %s, txid: %s, block: %s",
                       tx.GetToAddress().ToString().c_str(), tx.GetHash().GetHex().c_str(), hashBlock.ToString().c_str());
                return false;
            }
            if (nCodeType == CODE_TYPE_TEMPLATE)
            {
                CTemplatePtr ptr = CTemplate::Import(ctxTemplate.GetTemplateData());
                if (!ptr)
                {
                    StdLog("CBlockBase", "Verify block address: Imprt template fail, to: %s, txid: %s, block: %s",
                           tx.GetToAddress().ToString().c_str(), tx.GetHash().GetHex().c_str(), hashBlock.ToString().c_str());
                    return false;
                }
                CAddressContext ctxAddress(CTemplateAddressContext(ctxTemplate.GetName(), std::string(), ptr->GetTemplateType(), ctxTemplate.GetTemplateData()));
                mapBlockAddress[CDestination(ptr->GetTemplateId())] = ctxAddress;
            }
            else if (nCodeType == CODE_TYPE_CONTRACT)
            {
                CDestination destTo;
                //destTo.SetContractId(tx.GetFromAddress(), tx.GetNonce());
                destTo = CreateContractAddressByNonce(tx.GetFromAddress(), tx.GetNonce());
                if (loadAddress(destTo))
                {
                    StdLog("CBlockBase", "Verify block address: Contract address exited, contract address: %s, from: %s, txid: %s, block: %s",
                           destTo.ToString().c_str(), tx.GetFromAddress().ToString().c_str(), tx.GetHash().GetHex().c_str(), hashBlock.ToString().c_str());
                    return false;
                }
                if (ctxContract.IsCreate() || ctxContract.IsSetup())
                {
                    CAddressContext ctxAddress(CContractAddressContext(ctxContract.GetType(), ctxContract.GetCodeOwner(), ctxContract.GetName(), ctxContract.GetDescribe(), tx.GetHash(),
                                                                       ctxContract.GetSourceCodeHash(), ctxContract.GetContractCreateCodeHash(), uint256()));
                    mapBlockAddress[destTo] = ctxAddress;
                }
            }
            else
            {
                StdLog("CBlockBase", "Verify block address: Code type error, to: %s, txid: %s, block: %s",
                       tx.GetToAddress().ToString().c_str(), tx.GetHash().GetHex().c_str(), hashBlock.ToString().c_str());
                return false;
            }
        }
        else
        {
            if (!loadAddress(tx.GetToAddress()))
            {
                CAddressContext ctxAddress;
                if (!tx.GetToAddressData(ctxAddress))
                {
                    StdLog("CBlockBase", "Verify block address: Get tx to address fail, to: %s, txid: %s, block: %s",
                           tx.GetToAddress().ToString().c_str(), tx.GetHash().GetHex().c_str(), hashBlock.ToString().c_str());
                    return false;
                }
                if (ctxAddress.IsContract())
                {
                    StdLog("CBlockBase", "Verify block address: Contract address error, to: %s, txid: %s, block: %s",
                           tx.GetToAddress().ToString().c_str(), tx.GetHash().GetHex().c_str(), hashBlock.ToString().c_str());
                    return false;
                }
                if (ctxAddress.IsTemplate())
                {
                    CTemplateAddressContext ctxtTemplate;
                    if (!ctxAddress.GetTemplateAddressContext(ctxtTemplate))
                    {
                        StdLog("CBlockBase", "Verify block address: Get template context fail, to: %s, txid: %s, block: %s",
                               tx.GetToAddress().ToString().c_str(), tx.GetHash().GetHex().c_str(), hashBlock.ToString().c_str());
                        return false;
                    }
                    CTemplatePtr ptr = CTemplate::Import(ctxtTemplate.btData);
                    if (!ptr || tx.GetToAddress() != CDestination(ptr->GetTemplateId()))
                    {
                        StdLog("CBlockBase", "Verify block address: To template error, to: %s, txid: %s, block: %s, ptr: %s, template data: %s",
                               tx.GetToAddress().ToString().c_str(), tx.GetHash().GetHex().c_str(),
                               hashBlock.ToString().c_str(), (ptr == nullptr ? "has" : "null"), ToHexString(ctxtTemplate.btData).c_str());
                        return false;
                    }
                }
                mapBlockAddress.insert(make_pair(tx.GetToAddress(), ctxAddress));
            }
            else
            {
                // Correct Address
                CAddressContext ctxTxAddress;
                if (tx.GetToAddressData(ctxTxAddress) && ctxTxAddress.IsTemplate())
                {
                    auto it = mapBlockAddress.find(tx.GetToAddress());
                    if (it != mapBlockAddress.end() && it->second.IsPubkey())
                    {
                        it->second = ctxTxAddress;
                    }
                }
            }
        }
        return true;
    };

    if (!verifyAddress(block.txMint))
    {
        StdLog("CBlockBase", "Verify block address: Verify mint address fail, txid: %s, block: %s",
               block.txMint.GetHash().GetHex().c_str(), hashBlock.ToString().c_str());
        return false;
    }

    for (const auto& tx : block.vtx)
    {
        if (!verifyAddress(tx))
        {
            StdLog("CBlockBase", "Verify block address: Verify address fail, txid: %s, block: %s",
                   tx.GetHash().GetHex().c_str(), hashBlock.ToString().c_str());
            return false;
        }
    }
    return true;
}

bool CBlockBase::GetTransactionReceipt(const uint256& hashFork, const uint256& txid, CTransactionReceiptEx& txReceiptex)
{
    CTransactionReceipt receipt;
    if (!dbBlock.RetrieveTxReceipt(hashFork, txid, receipt))
    {
        CTransaction tx;
        uint256 hashAtFork;
        CTxIndex txIndex;
        if (!RetrieveTxAndIndex(hashFork, txid, tx, hashAtFork, txIndex))
        {
            StdLog("CBlockBase", "Get transaction receipt: Retrieve tx fail, txid: %s, fork: %s", txid.GetHex().c_str(), hashFork.GetHex().c_str());
            return false;
        }
        receipt.nTxIndex = txIndex.nTxSeq;
        receipt.txid = txid;
        receipt.nBlockNumber = txIndex.nBlockNumber;
        receipt.from = tx.GetFromAddress();
        receipt.to = tx.GetToAddress();
        receipt.nTxGasUsed = tx.GetGasLimit();
        receipt.nTvGasUsed = 0;
        receipt.nEffectiveGasPrice = tx.GetGasPrice();
    }
    else
    {
        if (receipt.IsCommonReceipt())
        {
            CTransaction tx;
            uint256 hashAtFork;
            CTxIndex txIndex;
            if (!RetrieveTxAndIndex(hashFork, txid, tx, hashAtFork, txIndex))
            {
                StdLog("CBlockBase", "Get transaction receipt: Retrieve tx fail, txid: %s, fork: %s", txid.GetHex().c_str(), hashFork.GetHex().c_str());
                return false;
            }
            receipt.nTxIndex = txIndex.nTxSeq;
            receipt.txid = txid;
            receipt.nBlockNumber = txIndex.nBlockNumber;
            receipt.from = tx.GetFromAddress();
            receipt.to = tx.GetToAddress();
            receipt.nEffectiveGasPrice = tx.GetGasPrice();
        }
    }
    receipt.CalcLogsBloom();
    txReceiptex = CTransactionReceiptEx(receipt);

    uint256 hashLastBlock;
    if (!dbBlock.RetrieveForkLast(hashFork, hashLastBlock))
    {
        StdLog("CBlockBase", "Get transaction receipt: Retrieve fork last fail, txid: %s, fork: %s", txid.GetHex().c_str(), hashFork.GetHex().c_str());
        return false;
    }
    CForkContext ctxFork;
    if (!dbBlock.RetrieveForkContext(hashFork, ctxFork))
    {
        StdLog("CBlockBase", "Get transaction receipt: Retrieve fork context fail, txid: %s, fork: %s", txid.GetHex().c_str(), hashFork.GetHex().c_str());
        return false;
    }

    uint256 hashBlock;
    if (!dbBlock.RetrieveBlockHashByNumber(hashFork, ctxFork.nChainId, hashLastBlock, txReceiptex.nBlockNumber, hashBlock))
    {
        StdLog("CBlockBase", "Get transaction receipt: Retrieve block hash, block number: %lu, fork: %s", txReceiptex.nBlockNumber, hashFork.GetHex().c_str());
        return false;
    }
    txReceiptex.hashBlock = hashBlock;

    CBlockIndex* pBlockIndex = nullptr;
    if (!RetrieveIndex(hashBlock, &pBlockIndex))
    {
        StdLog("CBlockBase", "Get transaction receipt: Retrieve block index fail, block: %s, fork: %s", hashBlock.ToString().c_str(), hashFork.GetHex().c_str());
        return false;
    }
    txReceiptex.nBlockGasUsed = pBlockIndex->GetBlockGasUsed();
    return true;
}

uint256 CBlockBase::AddLogsFilter(const uint256& hashClient, const uint256& hashFork, const CLogsFilter& logsFilter)
{
    return blockFilter.AddLogsFilter(hashClient, hashFork, logsFilter);
}

void CBlockBase::RemoveFilter(const uint256& nFilterId)
{
    blockFilter.RemoveFilter(nFilterId);
}

bool CBlockBase::GetTxReceiptLogsByFilterId(const uint256& nFilterId, const bool fAll, ReceiptLogsVec& receiptLogs)
{
    return blockFilter.GetTxReceiptLogsByFilterId(nFilterId, fAll, receiptLogs);
}

bool CBlockBase::GetTxReceiptsByLogsFilter(const uint256& hashFork, const CLogsFilter& logsFilter, ReceiptLogsVec& vReceiptLogs)
{
    CBlockIndex* pIndexFrom = nullptr;
    CBlockIndex* pIndexTo = nullptr;
    if (logsFilter.hashFromBlock == 0)
    {
        pIndexFrom = GetIndex(hashFork);
    }
    else
    {
        pIndexFrom = GetIndex(logsFilter.hashFromBlock);
    }
    if (logsFilter.hashToBlock == 0)
    {
        if (!RetrieveFork(hashFork, &pIndexTo))
        {
            StdLog("CBlockBase", "Get tx receipts by logs filter: from or to block error, fork: %s", hashFork.ToString().c_str());
            return false;
        }
    }
    else
    {
        pIndexTo = GetIndex(logsFilter.hashToBlock);
    }
    if (!pIndexFrom || !pIndexTo)
    {
        StdLog("CBlockBase", "Get tx receipts by logs filter: from or to block error, from: %s, to: %s",
               logsFilter.hashFromBlock.ToString().c_str(), logsFilter.hashToBlock.ToString().c_str());
        return false;
    }

    std::vector<uint256> vBlockHash;
    while (pIndexTo && pIndexTo->GetBlockNumber() >= pIndexFrom->GetBlockNumber())
    {
        vBlockHash.push_back(pIndexTo->GetBlockHash());
        pIndexTo = pIndexTo->pPrev;
    }

    for (auto& hashBlock : vBlockHash)
    {
        CBlockEx block;
        if (!Retrieve(hashBlock, block))
        {
            StdLog("CBlockBase", "Get tx receipts by logs filter: Retrieve block failed, block: %s", hashBlock.ToString().c_str());
            return false;
        }
        for (size_t i = 0; i < block.vtx.size(); ++i)
        {
            auto& tx = block.vtx[i];
            uint256 txid = tx.GetHash();
            CTransactionReceipt receipt;
            if (!dbBlock.RetrieveTxReceipt(hashFork, txid, receipt))
            {
                StdLog("CBlockBase", "Get tx receipts by logs filter: Retrieve tx receipt failed, txid: %s, block: %s", txid.ToString().c_str(), hashBlock.ToString().c_str());
                return false;
            }
            MatchLogsVec vLogs;
            logsFilter.matchesLogs(receipt, vLogs);
            if (!vLogs.empty())
            {
                CReceiptLogs r(vLogs);
                r.nTxIndex = i + 1;
                r.txid = txid;
                r.nBlockNumber = block.GetBlockNumber();
                r.hashBlock = hashBlock;
                vReceiptLogs.push_back(r);

                if (vReceiptLogs.size() > MAX_FILTER_CACHE_COUNT)
                {
                    StdLog("CBlockBase", "Get tx receipts by logs filter: Query returned more than %lu results", MAX_FILTER_CACHE_COUNT);
                    return false;
                }
            }
        }
    }
    return true;
}

uint256 CBlockBase::AddBlockFilter(const uint256& hashClient, const uint256& hashFork)
{
    return blockFilter.AddBlockFilter(hashClient, hashFork);
}

bool CBlockBase::GetFilterBlockHashs(const uint256& hashFork, const uint256& nFilterId, const bool fAll, std::vector<uint256>& vBlockHash)
{
    CBlockIndex* pLastIndex = nullptr;
    if (!RetrieveFork(hashFork, &pLastIndex))
    {
        return false;
    }
    return blockFilter.GetFilterBlockHashs(nFilterId, pLastIndex->GetBlockHash(), fAll, vBlockHash);
}

uint256 CBlockBase::AddPendingTxFilter(const uint256& hashClient, const uint256& hashFork)
{
    return blockFilter.AddPendingTxFilter(hashClient, hashFork);
}

void CBlockBase::AddPendingTx(const uint256& hashFork, const uint256& txid)
{
    blockFilter.AddPendingTx(hashFork, txid);
}

bool CBlockBase::GetFilterTxids(const uint256& hashFork, const uint256& nFilterId, const bool fAll, std::vector<uint256>& vTxid)
{
    return blockFilter.GetFilterTxids(hashFork, nFilterId, fAll, vTxid);
}

bool CBlockBase::GetCreateForkLockedAmount(const CDestination& dest, const uint256& hashPrevBlock, const bytes& btAddressData, uint256& nLockedAmount)
{
    if (btAddressData.empty())
    {
        StdLog("CBlockBase", "Get fork lock amount: Address data is empty, dest: %s, prev: %s",
               dest.ToString().c_str(), hashPrevBlock.GetHex().c_str());
        return false;
    }
    CTemplatePtr ptr = CTemplate::Import(btAddressData);
    if (!ptr || ptr->GetTemplateType() != TEMPLATE_FORK)
    {
        StdLog("CBlockBase", "Get fork lock amount: Create template fail, dest: %s, prev: %s",
               dest.ToString().c_str(), hashPrevBlock.GetHex().c_str());
        return false;
    }
    uint256 hashForkLocked = boost::dynamic_pointer_cast<CTemplateFork>(ptr)->hashFork;
    int nCreateHeight = GetForkCreatedHeight(hashForkLocked, hashPrevBlock);
    if (nCreateHeight < 0)
    {
        StdLog("CBlockBase", "Get fork lock amount: Get fork created height fail, dest: %s, fork: %s",
               dest.ToString().c_str(), hashForkLocked.GetHex().c_str());
        return false;
    }
    int nForkValidHeight = CBlock::GetBlockHeightByHash(hashPrevBlock) - nCreateHeight;
    if (nForkValidHeight < 0)
    {
        nForkValidHeight = 0;
    }
    nLockedAmount = CTemplateFork::LockedCoin(nForkValidHeight);
    return true;
}

bool CBlockBase::VerifyAddressVoteRedeem(const CDestination& dest, const uint256& hashPrevBlock)
{
    CVoteContext ctxtVote;
    if (!RetrieveDestVoteContext(hashPrevBlock, dest, ctxtVote))
    {
        StdLog("CBlockBase", "Verify Vote Redeem: Retrieve dest vote context fail, dest: %s", dest.ToString().c_str());
        return false;
    }
    if (ctxtVote.nFinalHeight == 0 || (CBlock::GetBlockHeightByHash(hashPrevBlock) + 1) < ctxtVote.nFinalHeight)
    {
        StdDebug("CBlockBase", "Verify Vote Redeem: Vote locked, final vote height: %d, prev: [%d] %s, dest: %s",
                 ctxtVote.nFinalHeight, CBlock::GetBlockHeightByHash(hashPrevBlock), hashPrevBlock.ToString().c_str(), dest.ToString().c_str());
        return false;
    }
    return true;
}

bool CBlockBase::GetAddressLockedAmount(const uint256& hashFork, const uint256& hashPrevBlock, const CDestination& dest, const CAddressContext& ctxAddress, const uint256& nBalance, uint256& nLockedAmount)
{
    if (hashFork == hashGenesisBlock && ctxAddress.IsTemplate())
    {
        switch (ctxAddress.GetTemplateType())
        {
        case TEMPLATE_FORK:
        {
            CTemplateAddressContext ctxTemplate;
            if (!ctxAddress.GetTemplateAddressContext(ctxTemplate))
            {
                StdLog("CBlockState", "Get address loacked amount: Get template address context fail, dest: %s", dest.ToString().c_str());
                nLockedAmount = nBalance;
                return true;
            }
            uint256 nLocked;
            if (!GetCreateForkLockedAmount(dest, hashPrevBlock, ctxTemplate.btData, nLocked))
            {
                nLockedAmount = nBalance;
                return true;
            }
            nLockedAmount += nLocked;
            break;
        }
        case TEMPLATE_DELEGATE:
        case TEMPLATE_VOTE:
            if (!VerifyAddressVoteRedeem(dest, hashPrevBlock))
            {
                nLockedAmount = nBalance;
                return true;
            }
            break;
        case TEMPLATE_PLEDGE:
            nLockedAmount = nBalance;
            return true;
        }
    }

    uint256 nLocked;
    if (!GetVoteRewardLockedAmount(hashFork, hashPrevBlock, dest, nLocked))
    {
        nLockedAmount = nBalance;
        return true;
    }
    nLockedAmount += nLocked;
    return true;
}

bool CBlockBase::AddBlacklistAddress(const CDestination& dest)
{
    return dbBlock.AddBlacklistAddress(dest);
}

void CBlockBase::RemoveBlacklistAddress(const CDestination& dest)
{
    dbBlock.RemoveBlacklistAddress(dest);
}

bool CBlockBase::IsExistBlacklistAddress(const CDestination& dest)
{
    return dbBlock.IsExistBlacklistAddress(dest);
}

void CBlockBase::ListBlacklistAddress(set<CDestination>& setAddressOut)
{
    dbBlock.ListBlacklistAddress(setAddressOut);
}

bool CBlockBase::UpdateForkMintMinGasPrice(const uint256& hashFork, const uint256& nMinGasPrice)
{
    return dbBlock.UpdateForkMintMinGasPrice(hashFork, nMinGasPrice);
}

uint256 CBlockBase::GetForkMintMinGasPrice(const uint256& hashFork)
{
    uint256 nMinGasPrice;
    if (!dbBlock.GetForkMintMinGasPrice(hashFork, nMinGasPrice))
    {
        nMinGasPrice = MIN_GAS_PRICE;
    }
    return nMinGasPrice;
}

//----------------------------------------------------------------------------
bool CBlockBase::GetTxIndex(const uint256& hashFork, const uint256& txid, uint256& hashAtFork, CTxIndex& txIndex)
{
    if (hashFork == 0)
    {
        std::map<uint256, CForkContext> mapForkCtxt;
        if (!ListForkContext(mapForkCtxt))
        {
            return false;
        }
        for (const auto& kv : mapForkCtxt)
        {
            const uint256& hashGetFork = kv.first;
            if (dbBlock.RetrieveTxIndex(hashGetFork, txid, txIndex))
            {
                hashAtFork = hashGetFork;
                return true;
            }
        }
    }
    else
    {
        if (dbBlock.RetrieveTxIndex(hashFork, txid, txIndex))
        {
            hashAtFork = hashFork;
            return true;
        }
    }
    return false;
}

void CBlockBase::ClearCache()
{
    map<uint256, CBlockIndex*>::iterator mi;
    for (mi = mapIndex.begin(); mi != mapIndex.end(); ++mi)
    {
        delete (*mi).second;
    }
    mapIndex.clear();
    mapForkHeightIndex.clear();
}

bool CBlockBase::LoadDB()
{
    CWriteLock wlock(rwAccess);

    ClearCache();

    /*CBlockWalker walker(this);
    if (!dbBlock.WalkThroughBlockIndex(walker))
    {
        StdLog("BlockBase", "LoadDB: Walk Through Block Index fail");
        ClearCache();
        return false;
    }*/
    StdLog("BlockBase", "Start verify db.");
    if (!VerifyDB())
    {
        StdError("BlockBase", "Load DB: Verify DB fail.");
        return false;
    }
    StdLog("BlockBase", "Verify db success!");
    return true;
}

bool CBlockBase::VerifyDB()
{
    const uint32 nNeedVerifyCount = 128;
    bool fAllVerify = false;
    std::map<uint256, uint256> mapForkLast;
    std::size_t nVerifyCount = dbBlock.GetBlockVerifyCount();
    if (nVerifyCount > nNeedVerifyCount)
    {
        for (std::size_t i = nVerifyCount - nNeedVerifyCount; i < nVerifyCount; i++)
        {
            CBlockVerify verifyBlock;
            if (!dbBlock.GetBlockVerify(i, verifyBlock))
            {
                StdError("BlockBase", "Verify DB: Get block verify fail, pos: %ld.", i);
                return false;
            }
            CBlockOutline outline;
            CBlockRoot blockRoot;
            if (!VerifyBlockDB(verifyBlock, outline, blockRoot, true))
            {
                fAllVerify = true;
                break;
            }
        }
    }
    for (std::size_t i = 0; i < nVerifyCount; i++)
    {
        CBlockVerify verifyBlock;
        if (!dbBlock.GetBlockVerify(i, verifyBlock))
        {
            StdError("BlockBase", "Verify DB: Get block verify fail, pos: %ld.", i);
            return false;
        }

        bool fVerify = false;
        if (fAllVerify)
        {
            fVerify = true;
        }
        else
        {
            if (i <= nNeedVerifyCount / 2
                || nVerifyCount <= nNeedVerifyCount
                || i > (nVerifyCount - nNeedVerifyCount))
            {
                fVerify = true;
            }
        }

        CBlockOutline outline;
        CBlockRoot blockRoot;
        CBlockIndex* pIndexNew = nullptr;
        CBlockEx blockex;
        bool fRepairBlock = false;
        if (VerifyBlockDB(verifyBlock, outline, blockRoot, fVerify))
        {
            if (!LoadBlockIndex(outline, &pIndexNew))
            {
                StdError("BlockBase", "Verify DB: Load block index fail, pos: %ld, block: [%d] %s.", i, CBlock::GetBlockHeightByHash(verifyBlock.hashBlock), verifyBlock.hashBlock.GetHex().c_str());
                return false;
            }
        }
        else
        {
            if (!fAllVerify)
            {
                fAllVerify = true;
            }
            StdDebug("BlockBase", "Verify DB: Verify block db fail, pos: %ld, block: [%d] %s.", i, CBlock::GetBlockHeightByHash(verifyBlock.hashBlock), verifyBlock.hashBlock.GetHex().c_str());
            if (!RepairBlockDB(verifyBlock, blockRoot, blockex, &pIndexNew))
            {
                StdError("BlockBase", "Verify DB: Repair block db fail, pos: %ld, block: [%d] %s.", i, CBlock::GetBlockHeightByHash(verifyBlock.hashBlock), verifyBlock.hashBlock.GetHex().c_str());
                return false;
            }
            fRepairBlock = true;
        }

        auto funcUpdateLongChain = [&](const uint256& hashForkIn, const uint256& hashForkLastBlockIn, const uint256& hashBlockIn, const CBlockEx& blockIn, const CBlockIndex* pIndexIn) -> bool {
            CBlockChainUpdate update = CBlockChainUpdate(pIndexIn);
            if (blockIn.IsOrigin())
            {
                update.vBlockAddNew.push_back(blockIn);
            }
            else
            {
                if (!GetBlockBranchListNolock(hashBlockIn, hashForkLastBlockIn, &blockIn, update.vBlockRemove, update.vBlockAddNew))
                {
                    StdLog("BlockBase", "Verify DB: Get block branch list fail, block: [%d] %s", CBlock::GetBlockHeightByHash(hashBlockIn), hashBlockIn.ToString().c_str());
                    return false;
                }
            }
            if (!UpdateBlockLongChain(hashForkIn, update.vBlockRemove, update.vBlockAddNew))
            {
                StdLog("BlockBase", "Verify DB: Update block long chain fail, block: [%d] %s", CBlock::GetBlockHeightByHash(hashBlockIn), hashBlockIn.ToString().c_str());
                return false;
            }
            return true;
        };

        if (pIndexNew->IsOrigin())
        {
            if (fRepairBlock)
            {
                if (!funcUpdateLongChain(pIndexNew->GetBlockHash(), pIndexNew->GetBlockHash(), pIndexNew->GetBlockHash(), blockex, pIndexNew))
                {
                    StdError("BlockBase", "Verify DB: Update origin long chain fail, block: [%d] %s.", CBlock::GetBlockHeightByHash(pIndexNew->GetBlockHash()), pIndexNew->GetBlockHash().GetHex().c_str());
                    return false;
                }
            }
            mapForkLast[pIndexNew->GetBlockHash()] = pIndexNew->GetBlockHash();
        }
        else
        {
            auto it = mapForkLast.find(pIndexNew->GetOriginHash());
            if (it == mapForkLast.end())
            {
                StdError("BlockBase", "Verify DB: Find fork last fail, fork: %s, block: [%d] %s.",
                         pIndexNew->GetOriginHash().GetHex().c_str(), CBlock::GetBlockHeightByHash(verifyBlock.hashBlock), verifyBlock.hashBlock.GetHex().c_str());
                return false;
            }
            CBlockIndex* pIndexFork = GetIndex(it->second);
            if (pIndexFork == nullptr)
            {
                StdError("BlockBase", "Verify DB: Get last index fail, fork: %s, block: [%d] %s.",
                         pIndexNew->GetOriginHash().GetHex().c_str(), CBlock::GetBlockHeightByHash(verifyBlock.hashBlock), verifyBlock.hashBlock.GetHex().c_str());
                return false;
            }
            if (!(pIndexFork->nChainTrust > pIndexNew->nChainTrust
                  || (pIndexFork->nChainTrust == pIndexNew->nChainTrust && !pIndexNew->IsEquivalent(pIndexFork))))
            {
                if (fRepairBlock)
                {
                    if (!funcUpdateLongChain(pIndexNew->GetOriginHash(), it->second, pIndexNew->GetBlockHash(), blockex, pIndexNew))
                    {
                        StdError("BlockBase", "Verify DB: Update fork long chain fail, fork: %s, block: [%d] %s.",
                                 pIndexNew->GetOriginHash().GetHex().c_str(), CBlock::GetBlockHeightByHash(pIndexNew->GetBlockHash()), pIndexNew->GetBlockHash().GetHex().c_str());
                        return false;
                    }
                }
                UpdateBlockNext(pIndexNew);
                it->second = pIndexNew->GetBlockHash();
            }
        }

        if (i % 100000 == 0)
        {
            StdLog("BlockBase", "Verify DB: Verify block count: %ld, block: [%d] %s.",
                   i + 1, CBlock::GetBlockHeightByHash(verifyBlock.hashBlock), verifyBlock.hashBlock.GetHex().c_str());
        }
    }

    for (const auto& kv : mapForkLast)
    {
        uint256 hashLastBlock;
        if (!dbBlock.RetrieveForkLast(kv.first, hashLastBlock) || hashLastBlock != kv.second)
        {
            StdError("BlockBase", "Verify DB: Fork last error, last block: [%d] %s, fork: %s",
                     CBlock::GetBlockHeightByHash(kv.second), kv.second.GetHex().c_str(), kv.first.GetHex().c_str());
            if (!dbBlock.UpdateForkLast(kv.first, kv.second))
            {
                StdError("BlockBase", "Verify DB: Repair fork last fail, last block: [%d] %s, fork: %s",
                         CBlock::GetBlockHeightByHash(kv.second), kv.second.GetHex().c_str(), kv.first.GetHex().c_str());
                return false;
            }
        }
    }
    return true;
}

bool CBlockBase::VerifyBlockDB(const CBlockVerify& verifyBlock, CBlockOutline& outline, CBlockRoot& blockRoot, const bool fVerify)
{
    if (!dbBlock.RetrieveBlockIndex(verifyBlock.hashBlock, outline))
    {
        StdError("BlockBase", "Verify block DB: Retrieve block index fail, block: [%d] %s.", CBlock::GetBlockHeightByHash(verifyBlock.hashBlock), verifyBlock.hashBlock.GetHex().c_str());
        return false;
    }
    if (outline.GetCrc() != verifyBlock.nIndexCrc)
    {
        StdError("BlockBase", "Verify block DB: Block index crc error, db index crc: 0x%8.8x, verify index crc: 0x%8.8x, block: [%d] %s.",
                 outline.GetCrc(), verifyBlock.nIndexCrc, CBlock::GetBlockHeightByHash(verifyBlock.hashBlock), verifyBlock.hashBlock.GetHex().c_str());
        return false;
    }

    if (fVerify || outline.IsOrigin())
    {
        if (!dbBlock.VerifyBlockRoot(outline.IsPrimary(), outline.hashOrigin, outline.hashPrev,
                                     verifyBlock.hashBlock, outline.hashStateRoot, blockRoot, false))
        {
            StdError("BlockBase", "Verify block DB: Verify block root fail, block: [%d] %s.", CBlock::GetBlockHeightByHash(verifyBlock.hashBlock), verifyBlock.hashBlock.GetHex().c_str());
            return false;
        }
        if (blockRoot.GetRootCrc() != verifyBlock.nRootCrc)
        {
            StdError("BlockBase", "Verify block DB: Root crc error, db root crc: 0x%8.8x, verify root crc: 0x%8.8x, block: [%d] %s.",
                     blockRoot.GetRootCrc(), verifyBlock.nRootCrc, CBlock::GetBlockHeightByHash(verifyBlock.hashBlock), verifyBlock.hashBlock.GetHex().c_str());
            return false;
        }
    }
    return true;
}

bool CBlockBase::RepairBlockDB(const CBlockVerify& verifyBlock, CBlockRoot& blockRoot, CBlockEx& block, CBlockIndex** ppIndexNew)
{
    //CBlockEx block;
    if (!tsBlock.Read(block, verifyBlock.nFile, verifyBlock.nOffset, true, true))
    {
        StdError("BlockBase", "Repair block DB: Read block fail, file: %d, offset: %d, block: [%d] %s.",
                 verifyBlock.nFile, verifyBlock.nOffset, CBlock::GetBlockHeightByHash(verifyBlock.hashBlock), verifyBlock.hashBlock.GetHex().c_str());
        return false;
    }
    uint256 hashBlock = block.GetHash();
    if (hashBlock != verifyBlock.hashBlock)
    {
        StdError("BlockBase", "Repair block DB: Block error, file: %d, offset: %d, calc block: [%d] %s, block: [%d] %s.",
                 verifyBlock.nFile, verifyBlock.nOffset, CBlock::GetBlockHeightByHash(hashBlock), hashBlock.GetHex().c_str(),
                 CBlock::GetBlockHeightByHash(verifyBlock.hashBlock), verifyBlock.hashBlock.GetHex().c_str());
        return false;
    }

    uint256 hashFork;
    if (block.IsOrigin())
    {
        hashFork = hashBlock;
    }
    else
    {
        CBlockIndex* pPrevIndex = GetIndex(block.hashPrev);
        if (!pPrevIndex)
        {
            StdError("BlockBase", "Repair block DB: Get prev index fail, block: [%d] %s, prev block: [%d] %s.",
                     CBlock::GetBlockHeightByHash(verifyBlock.hashBlock), verifyBlock.hashBlock.GetHex().c_str(),
                     CBlock::GetBlockHeightByHash(block.hashPrev), block.hashPrev.GetHex().c_str());
            return false;
        }
        hashFork = pPrevIndex->GetOriginHash();
    }

    if (!SaveBlock(hashFork, hashBlock, block, ppIndexNew, blockRoot, true))
    {
        StdError("BlockBase", "Repair block DB: Save block failed, block: [%d] %s", CBlock::GetBlockHeightByHash(hashBlock), hashBlock.ToString().c_str());
        return false;
    }
    return true;
}

bool CBlockBase::LoadBlockIndex(CBlockOutline& outline, CBlockIndex** ppIndexNew)
{
    uint256 hashBlock = outline.GetBlockHash();

    if (mapIndex.find(hashBlock) != mapIndex.end())
    {
        StdError("BlockBase", "Load block index: Block index exist, block: %s", hashBlock.ToString().c_str());
        return false;
    }

    CBlockIndex* pIndexNew = new CBlockIndex(static_cast<CBlockIndex&>(outline));
    if (pIndexNew == nullptr)
    {
        StdError("BlockBase", "Load block index: New block index fail, block: %s", hashBlock.ToString().c_str());
        return false;
    }
    auto mi = mapIndex.insert(make_pair(hashBlock, pIndexNew)).first;
    if (mi == mapIndex.end())
    {
        StdError("BlockBase", "Load block index: Block index insert fail, block: %s", hashBlock.ToString().c_str());
        return false;
    }

    pIndexNew->phashBlock = &(mi->first);
    pIndexNew->pPrev = nullptr;
    pIndexNew->pOrigin = nullptr;

    if (outline.hashPrev != 0)
    {
        pIndexNew->pPrev = GetIndex(outline.hashPrev);
        if (pIndexNew->pPrev == nullptr)
        {
            StdError("BlockBase", "Load block index: Get prev index fail, block: %s, prev block: %s",
                     hashBlock.ToString().c_str(), outline.hashPrev.GetHex().c_str());
            return false;
        }
    }

    if (pIndexNew->IsOrigin())
    {
        pIndexNew->pOrigin = pIndexNew;
    }
    else
    {
        pIndexNew->pOrigin = GetIndex(outline.hashOrigin);
        if (pIndexNew->pOrigin == nullptr)
        {
            StdError("BlockBase", "Load block index: Get origin index fail, block: %s, origin block: %s",
                     hashBlock.ToString().c_str(), outline.hashOrigin.GetHex().c_str());
            return false;
        }
    }

    UpdateBlockHeightIndex(pIndexNew->GetOriginHash(), hashBlock, pIndexNew->nTimeStamp, CDestination(), pIndexNew->GetRefBlock());

    *ppIndexNew = pIndexNew;
    return true;
}

} // namespace storage
} // namespace hashahead
