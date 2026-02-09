// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_STATEDB_H
#define STORAGE_STATEDB_H

#include <map>

#include "block.h"
#include "destination.h"
#include "hnbase.h"
#include "timeseries.h"
#include "transaction.h"
#include "triedb.h"
#include "uint256.h"

namespace hashahead
{
namespace storage
{

class CListStateTrieDBWalker : public CTrieDBWalker
{
public:
    CListStateTrieDBWalker(std::map<CDestination, CDestState>& mapStateIn)
      : mapState(mapStateIn) {}

    bool Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver) override;

public:
    std::map<CDestination, CDestState>& mapState;
};

class CForkStateDB
{
public:
    CForkStateDB(const uint256& hashForkIn);
    ~CForkStateDB();
    bool Initialize(const boost::filesystem::path& pathData, const bool fPruneIn);
    void Deinitialize();
    bool RemoveAll();
    bool AddBlockState(const uint32 nBlockHeight, const uint256& hashPrevRoot, const CBlockRootStatus& statusBlockRoot, const std::map<CDestination, CDestState>& mapBlockState, uint256& hashBlockRoot);
    bool CreateCacheStateTrie(const uint256& hashPrevRoot, const CBlockRootStatus& statusBlockRoot, const std::map<CDestination, CDestState>& mapBlockState, uint256& hashBlockRoot);
    bool RetrieveDestState(const uint256& hashBlockRoot, const CDestination& dest, CDestState& state);
    bool ListDestState(const uint256& hashBlockRoot, std::map<CDestination, CDestState>& mapBlockState);
    bool VerifyState(const uint256& hashRoot, const bool fVerifyAllNode = true);
    bool ClearStateUnavailableNode(const uint32 nClearRefHeight);

    bool ListStateRootKv(std::vector<std::pair<uint256, bytesmap>>& vRootKv);
    bool AddStateKvTrie(const uint32 nBlockHeight, const uint256& hashPrevRoot, const bytesmap& mapKv, uint256& hashNewRoot);

protected:
    void AddPrevRoot(const uint256& hashPrevRoot, const CBlockRootStatus& statusBlockRoot, bytesmap& mapKv);
    bool GetPrevRoot(const uint256& hashRoot, uint256& hashPrevRoot, CBlockRootStatus& statusBlockRoot);

protected:
    uint256 hashFork;
    bool fPrune;
    hnbase::CRWAccess rwAccess;
    CTrieDB dbTrie;
};

class CStateDB
{
public:
    CStateDB()
      : fPruneState(false) {}
    bool Initialize(const boost::filesystem::path& pathData, const bool fPruneStateIn);
    void Deinitialize();

    bool ExistFork(const uint256& hashFork);
    bool LoadFork(const uint256& hashFork);
    void RemoveFork(const uint256& hashFork);
    bool AddNewFork(const uint256& hashFork);
    void Clear();

    bool AddBlockState(const uint256& hashFork, const uint32 nBlockHeight, const uint256& hashPrevRoot, const CBlockRootStatus& statusBlockRoot, const std::map<CDestination, CDestState>& mapBlockState, uint256& hashBlockRoot);
    bool CreateCacheStateTrie(const uint256& hashFork, const uint256& hashPrevRoot, const CBlockRootStatus& statusBlockRoot, const std::map<CDestination, CDestState>& mapBlockState, uint256& hashBlockRoot);
    bool RetrieveDestState(const uint256& hashFork, const uint256& hashBlockRoot, const CDestination& dest, CDestState& state);
    bool ListDestState(const uint256& hashFork, const uint256& hashBlockRoot, std::map<CDestination, CDestState>& mapBlockState);
    bool VerifyState(const uint256& hashFork, const uint256& hashRoot, const bool fVerifyAllNode = true);
    bool ClearStateUnavailableNode(const uint256& hashFork, const uint32 nClearRefHeight);

    bool ListStateRootKv(const uint256& hashFork, std::vector<std::pair<uint256, bytesmap>>& vRootKv);
    bool AddStateKvTrie(const uint256& hashFork, const uint32 nBlockHeight, const uint256& hashPrevRoot, const bytesmap& mapKv, uint256& hashNewRoot);

    static bool CreateStaticStateRoot(const CBlockRootStatus& statusBlockRoot, const std::map<CDestination, CDestState>& mapBlockState, uint256& hashStateRoot);

protected:
    boost::filesystem::path pathState;
    hnbase::CRWAccess rwAccess;
    std::map<uint256, std::shared_ptr<CForkStateDB>> mapStateDB;
    bool fPruneState;
};

} // namespace storage
} // namespace hashahead

#endif // STORAGE_STATEDB_H
