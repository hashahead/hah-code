// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_CONTRACTDB_H
#define STORAGE_CONTRACTDB_H

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

class CForkContractDB
{
public:
    CForkContractDB(const uint256& hashForkIn);
    ~CForkContractDB();

    bool Initialize(const boost::filesystem::path& pathData, const bool fPrune);
    void Deinitialize();
    bool RemoveAll();

    bool AddBlockContractKvValue(const uint32 nBlockHeight, const uint64 nBlockNumber, const CDestination& destContract, const uint256& hashPrevRoot, const std::map<uint256, bytes>& mapContractState, uint256& hashContractRoot);
    bool CreateCacheContractKvTrie(const uint256& hashPrevRoot, const std::map<uint256, bytes>& mapContractState, uint256& hashNewRoot);
    bool RetrieveContractKvValue(const uint256& hashContractRoot, const uint256& key, bytes& value);
    bool ClearContractKvRootUnavailableNode(const uint32 nRemoveLastHeight, bool& fExit);

    bool GetContractAddressRoot(const CDestination& destContract, const uint256& hashRoot, uint256& hashPrevRoot, uint32& nBlockHeight, uint64& nBlockNumber);
    bool CreateCacheContractKvRoot(const uint256& hashPrevRoot, const bytesmap& mapKv, uint256& hashNewRoot);
    bool AddContractKvTrie(const uint32 nBlockHeight, const uint256& hashPrevRoot, const bytesmap& mapKv, uint256& hashNewRoot);

protected:
    bool WriteAddressRoot(const CDestination& destContract, const uint256& hashPrevRoot, const uint256& hashNewRoot, const uint32 nBlockHeight, const uint64 nBlockNumber);
    bool RemoveAddressRoot(const CDestination& destContract, const uint256& hashRoot);
    bool WalkThroughHeightRoot(std::map<CDestination, std::map<uint256, std::pair<uint32, uint64>>>& mapAddressRoot, std::map<uint256, std::set<uint256>>& mapRootNext);

protected:
    const uint256 hashFork;
    const CChainId nChainId;
    CTrieDB dbTrie;
    bool fPruneData;
};

class CContractDB
{
public:
    CContractDB()
      : fPruneState(false) {}

    bool Initialize(const boost::filesystem::path& pathData, const bool fPruneStateIn);
    void Deinitialize();

    bool ExistFork(const uint256& hashFork);
    bool LoadFork(const uint256& hashFork);
    void RemoveFork(const uint256& hashFork);
    bool AddNewFork(const uint256& hashFork);
    void Clear();

    bool AddBlockContractKvValue(const uint256& hashFork, const uint32 nBlockHeight, const uint64 nBlockNumber, const CDestination& destContract, const uint256& hashPrevRoot, const std::map<uint256, bytes>& mapContractState, uint256& hashContractRoot);
    bool CreateCacheContractKvTrie(const uint256& hashFork, const uint256& hashPrevRoot, const std::map<uint256, bytes>& mapContractState, uint256& hashNewRoot);
    bool RetrieveContractKvValue(const uint256& hashFork, const uint256& hashContractRoot, const uint256& key, bytes& value);
    bool ClearContractKvRootUnavailableNode(const uint256& hashFork, const uint32 nRemoveLastHeight, bool& fExit);

    bool GetContractAddressRoot(const uint256& hashFork, const CDestination& destContract, const uint256& hashRoot, uint256& hashPrevRoot, uint32& nBlockHeight, uint64& nBlockNumber);
    bool CreateCacheContractKvRoot(const uint256& hashFork, const uint256& hashPrevRoot, const bytesmap& mapKv, uint256& hashNewRoot);
    bool AddContractKvTrie(const uint256& hashFork, const uint32 nBlockHeight, const uint256& hashPrevRoot, const bytesmap& mapKv, uint256& hashNewRoot);

    static bool CreateStaticContractStateRoot(const std::map<uint256, bytes>& mapContractState, uint256& hashStateRoot);

protected:
    boost::filesystem::path pathContract;
    hnbase::CRWAccess rwAccess;
    std::map<uint256, std::shared_ptr<CForkContractDB>> mapContractDB;
    bool fPruneState;
};

} // namespace storage
} // namespace hashahead

#endif // STORAGE_CONTRACTDB_H
