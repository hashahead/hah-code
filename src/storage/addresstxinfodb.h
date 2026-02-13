// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_ADDRESSTXINFODB_H
#define STORAGE_ADDRESSTXINFODB_H

#include <map>

#include "dbstruct.h"
#include "destination.h"
#include "hnbase.h"
#include "transaction.h"
#include "uint256.h"

#define CACHE_ROLLBACK_HEIGHT (1024)

namespace hashahead
{
namespace storage
{

class CForkAddressTxInfoDB : public hnbase::CKVDB
{
public:
    CForkAddressTxInfoDB();
    ~CForkAddressTxInfoDB();

    bool Initialize(const uint256& hashForkIn, const boost::filesystem::path& pathData);
    void Deinitialize();

    bool AddAddressTxInfo(const uint256& hashPrevBlock, const uint256& hashBlock, const uint64 nBlockNumber,
                          const std::map<CDestination, std::vector<CDestTxInfo>>& mapAddressTxInfo,
                          const std::map<CDestination, std::vector<CTokenTransRecord>>& mapTokenRecord);
    bool UpdateAddressTxInfoBlockLongChain(const std::vector<uint256>& vRemoveBlock, const std::vector<uint256>& vAddBlock);

    bool GetAddressTxCount(const CDestination& address, uint64& nTxCount);
    bool RetrieveAddressTxInfo(const CDestination& address, const uint64 nTxIndex, CDestTxInfo& ctxAddressTxInfo);
    bool ListAddressTxInfo(const CDestination& address, const uint64 nBeginTxIndex, const uint64 nGetTxCount, const bool fReverse, std::vector<CDestTxInfo>& vAddressTxInfo);
    bool ListTokenTx(const CDestination& destContractAddress, const CDestination& destUserAddress, const uint64 nPageNumber, const uint64 nPageSize,
                     const bool fReverse, uint64& nTotalRecordCount, uint64& nPageCount, std::vector<std::pair<uint64, CTokenTransRecord>>& vTokenTxRecord);

    bool WalkThroughSnapshotAddressTxKv(const uint64 nLastBlockNumber, WalkerAddressTxKvFunc fnWalker);
    bool WalkThroughSnapshotTokenTxKv(const uint64 nLastBlockNumber, WalkerTokenTxKvFunc fnWalker);
    bool WriteSnapshotAddressTxKvData(const bytes& btKey, const bytes& btValue);
    bool WriteSnapshotAddressTxCount(const uint256& hashLastBlock, const std::map<CDestination, uint64>& mapAddressTxCount);
    bool WriteSnapshotTokenTxCount(const std::map<CDestination, std::map<CDestination, uint64>>& mapTokenTxCount);

protected:
    bool GetBlockAddressTxInfo(const uint256& hashBlock, std::map<CDestination, std::vector<CDestTxInfo>>& mapAddressTxInfo, std::map<CDestination, std::vector<CTokenTransRecord>>& mapTokenRecord);
    bool GetBlockPrevBlock(const uint256& hashBlock, uint256& hashPrevBlock);
    bool WalkThroughAllBlockHash(std::set<uint256>& setBlockHash);
    bool WalkThroughAddressTxInfo(const CDestination& address, const uint64 nBeginTxIndex, const uint64 nGetTxCount, std::vector<CDestTxInfo>& vAddressTxInfo);
    bool WalkThroughTokenTxInfo(const CDestination& destContractAddress, const CDestination& destUserAddress, const uint64 nBeginTxIndex, const uint64 nGetTxCount, std::vector<std::pair<uint64, CTokenTransRecord>>& vTokenTxInfo);
    void RemoveOldBlockData(const uint256& hashBlock);
    void ClearOldBlockData();
    bool GetPrevRoot(const uint256& hashRoot, uint256& hashPrevRoot, uint256& hashBlock);
    void WriteAddressLast(const CDestination& dest, const uint64 nTxCount, bytesmap& mapKv);
    bool ReadAddressLast(const uint256& hashRoot, const CDestination& dest, uint64& nTxCount);

protected:
    uint256 hashFork;
    hnbase::CRWAccess rwAccess;

    std::set<uint256> setCacheBlockHash;
};

class CAddressTxInfoDB
{
public:
    CAddressTxInfoDB(const bool fCacheIn = true)
      : fCache(fCacheIn) {}
    bool Initialize(const boost::filesystem::path& pathData);
    void Deinitialize();

    bool ExistFork(const uint256& hashFork);
    bool LoadFork(const uint256& hashFork);
    void RemoveFork(const uint256& hashFork);
    bool AddNewFork(const uint256& hashFork);
    void Clear();

    bool AddAddressTxInfo(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, const uint64 nBlockNumber, const std::map<CDestination, std::vector<CDestTxInfo>>& mapAddressTxInfo, uint256& hashNewRoot);
    bool GetAddressTxCount(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, uint64& nTxCount);
    bool RetrieveAddressTxInfo(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, const uint64 nTxIndex, CDestTxInfo& ctxtAddressTxInfo);
    bool ListAddressTxInfo(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, const uint64 nBeginTxIndex, const uint64 nGetTxCount, const bool fReverse, std::vector<CDestTxInfo>& vAddressTxInfo);

    bool VerifyAddressTxInfo(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode = true);

protected:
    bool fCache;
    boost::filesystem::path pathAddress;
    hnbase::CRWAccess rwAccess;
    std::map<uint256, std::shared_ptr<CForkAddressTxInfoDB>> mapAddressTxInfoDB;
};

} // namespace storage
} // namespace hashahead

#endif // STORAGE_ADDRESSTXINFODB_H
