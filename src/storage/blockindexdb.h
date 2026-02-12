// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_BLOCKINDEXDB_H
#define STORAGE_BLOCKINDEXDB_H

#include <map>

#include "block.h"
#include "destination.h"
#include "hnbase.h"
#include "transaction.h"
#include "triedb.h"
#include "uint256.h"

namespace hashahead
{
namespace storage
{

class CBlockDBWalker
{
public:
    virtual bool Walk(CBlockIndex& outline) = 0;
};

class CBlockIndexDB : public hnbase::CKVDB
{
public:
    CBlockIndexDB();
    ~CBlockIndexDB();

    bool Initialize(const boost::filesystem::path& pathData);
    void Deinitialize();
    bool Clear();

    bool AddNewBlockIndex(const CBlockIndex& outline);
    bool RemoveBlockIndex(const uint256& hashBlock);
    bool RetrieveBlockIndex(const uint256& hashBlock, CBlockIndex& outline);
    bool WalkThroughBlockIndex(CBlockDBWalker& walker);
    bool RetrieveBlockHashByHeight(const uint256& hashFork, const uint32 nBlockHeight, std::vector<uint256>& vBlockHash);
    bool GetForkMaxHeight(const uint256& hashFork, uint32& nMaxHeight);

    bool UpdateBlockNumberBlockLongChain(const uint256& hashFork, const std::vector<std::pair<uint64, uint256>>& vRemoveNumberBlock, const std::vector<std::pair<uint64, uint256>>& mapNewNumberBlock);
    bool RetrieveBlockHashByNumber(const uint256& hashFork, const uint64 nBlockNumber, uint256& hashBlock);

    bool AddBlockVoteResult(const uint256& hashBlock, const bool fLongChain, const bytes& btBitmap, const bytes& btAggSig, const bool fAtChain, const uint256& hashAtBlock);
    bool RemoveBlockVoteResult(const uint256& hashBlock);
    bool RetrieveBlockVoteResult(const uint256& hashBlock, bytes& btBitmap, bytes& btAggSig, bool& fAtChain, uint256& hashAtBlock);
    bool GetLastBlockVoteResult(const uint256& hashFork, uint256& hashLastBlock, bytes& btBitmap, bytes& btAggSig, bool& fAtChain, uint256& hashAtBlock);
    bool GetLastConfirmBlock(const uint256& hashFork, uint256& hashLastConfirmBlock);
    bool AddBlockLocalVoteSignFlag(const uint256& hashBlock);
    bool GetBlockLocalSignFlag(const CChainId nChainId, const uint32 nHeight, const uint16 nSlot, uint256& hashBlock);

    bool GetSnapshotBlockVoteData(const std::vector<uint256>& vBlockHash, bytes& btSnapData);
    bool RecoverySnapshotBlockVoteData(bytes& btSnapData);

protected:
    bool AddBlockLocalSignFlagDb(const uint256& hashBlock);
    bool GetBlockLocalSignFlagDb(const CChainId nChainId, const uint32 nHeight, const uint16 nSlot, uint256& hashBlock);

protected:
    hnbase::CRWAccess rwAccess;
};

} // namespace storage
} // namespace hashahead

#endif // STORAGE_BLOCKINDEXDB_H
