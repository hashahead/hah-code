// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_TXINDEXDB_H
#define STORAGE_TXINDEXDB_H

#include <boost/thread/thread.hpp>

#include "dbstruct.h"
#include "hnbase.h"
#include "transaction.h"

namespace hashahead
{
namespace storage
{

class CForkTxIndexDB : public hnbase::CKVDB
{
public:
    CForkTxIndexDB();
    ~CForkTxIndexDB();

    bool Initialize(const uint256& hashForkIn, const boost::filesystem::path& pathData);
    void Deinitialize();

    bool AddBlockTxIndexReceipt(const uint256& hashBlock, const std::map<uint256, CTxIndex>& mapBlockTxIndex, const std::vector<CTransactionReceipt>& vTxReceipts);
    bool UpdateTxIndexBlockLongChain(const std::vector<uint256>& vRemoveTx, const std::map<uint256, uint256>& mapNewTx);
    bool RetrieveTxIndex(const uint256& txid, uint256& hashTxAtBlock, CTxIndex& txIndex);
    bool RetrieveTxReceipt(const uint256& txid, CTransactionReceipt& txReceipt);
    bool VerifyTxIndex(const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode = true);
    bool WalkThroughSnapshotTxIndex(const uint256& hashLastBlock, WalkerTxIndexKvFunc fnWalker);
    bool WriteTxIndexKvData(const bytes& btKey, const bytes& btValue);

protected:
    uint256 hashFork;
    hnbase::CRWAccess rwAccess;
};

class CTxIndexDB
{
public:
    CTxIndexDB() {}
    bool Initialize(const boost::filesystem::path& pathData);
    void Deinitialize();

    bool ExistFork(const uint256& hashFork);
    bool LoadFork(const uint256& hashFork);
    void RemoveFork(const uint256& hashFork);
    bool AddNewFork(const uint256& hashFork);
    void Clear();

    bool AddBlockTxIndexReceipt(const uint256& hashFork, const uint256& hashBlock, const std::map<uint256, CTxIndex>& mapBlockTxIndex, const std::vector<CTransactionReceipt>& vTxReceipts);
    bool UpdateTxIndexBlockLongChain(const uint256& hashFork, const std::vector<uint256>& vRemoveTx, const std::map<uint256, uint256>& mapNewTx);
    bool RetrieveTxIndex(const uint256& hashFork, const uint256& txid, uint256& hashTxAtBlock, CTxIndex& txIndex);
    bool RetrieveTxReceipt(const uint256& hashFork, const uint256& txid, CTransactionReceipt& txReceipt);
    bool VerifyTxIndex(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, bool fVerifyAllNode = true);
    bool WalkThroughSnapshotTxIndex(const uint256& hashFork, const uint256& hashLastBlock, WalkerTxIndexKvFunc fnWalker);
    bool WriteTxIndexKvData(const uint256& hashFork, const bytes& btKey, const bytes& btValue);

protected:
    boost::filesystem::path pathTxIndex;
    hnbase::CRWAccess rwAccess;
    std::map<uint256, std::shared_ptr<CForkTxIndexDB>> mapTxIndexDB;
};

} // namespace storage
} // namespace hashahead

#endif //STORAGE_TXINDEXDB_H
