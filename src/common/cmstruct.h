// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COMMON_CMSTRUCT_H
#define COMMON_CMSTRUCT_H

#include <set>
#include <stream/stream.h>
#include <vector>

#include "block.h"
#include "crypto.h"
#include "destination.h"
#include "param.h"
#include "transaction.h"
#include "uint256.h"

namespace hashahead
{

//////////////////////////////
// CVmCallTx
class CVmCallTx
{
public:
    CVmCallTx()
      : fEthCall(false), nTxNonce(0), fNeedVerifyToAddress(true) {}

public:
    bool fEthCall;
    CDestination destFrom;
    CDestination destTo;
    uint64 nTxNonce;
    uint256 nGasPrice;
    uint256 nGasLimit;
    uint256 nAmount;
    bytes btData;
    bool fNeedVerifyToAddress;
};

//////////////////////////////
// CVmCallBlock

class CVmCallBlock
{
public:
    CVmCallBlock()
      : nChainId(0), nBlockHeight(0), nBlockNumber(0), fPrimaryBlock(false), nOriBlockGasLimit(0), nBlockTimestamp(0), nPrevBlockTime(0) {}

public:
    uint256 hashFork;
    CChainId nChainId;
    uint256 nAgreement;
    uint32 nBlockHeight;
    uint64 nBlockNumber;
    bool fPrimaryBlock;
    CDestination destMint;
    uint64 nOriBlockGasLimit;
    uint256 nBlockGasLimit;
    uint64 nBlockTimestamp;
    uint256 hashPrevBlock;
    uint256 hashPrevStateRoot;
    uint64 nPrevBlockTime;
};

//////////////////////////////
// CVmCallResult

class CVmCallResult
{
public:
    CVmCallResult()
      : nStatus(0), nGasUsed(0), nGasLeft(0), ptrContractTraceResult(nullptr) {}

public:
    int nStatus;
    bytes btResult;
    uint64 nGasUsed;
    uint64 nGasLeft;
    SHP_CONTRACT_TRACE_RESULT ptrContractTraceResult;
};

//////////////////////////////
// CBlockLogsFilter

class CBlockLogsFilter
{
    friend class hnbase::CStream;

public:
    CBlockLogsFilter() {}
    CBlockLogsFilter(const uint256& hashForkIn, const CLogsFilter& logFilterIn)
      : hashFork(hashForkIn), logFilter(logFilterIn), nLogsCount(0), nHisLogsCount(0), nPrevGetChangesTime(hnbase::GetTime()) {}

    bool isTimeout();

    bool AddTxReceipt(const uint256& hashFork, const uint256& hashBlock, const CTransactionReceipt& receipt);
    void GetTxReceiptLogs(const bool fAll, ReceiptLogsVec& receiptLogs);

protected:
    uint256 hashFork;
    CLogsFilter logFilter;

    int64 nPrevGetChangesTime;
    uint64 nLogsCount;
    uint64 nHisLogsCount;

    ReceiptLogsVec vReceiptLogs;
    ReceiptLogsVec vHisReceiptLogs;

protected:
    template <typename O>
    void Serialize(hnbase::CStream& s, O& opt)
    {
        s.Serialize(hashFork, opt);
        s.Serialize(logFilter, opt);
        s.Serialize(nPrevGetChangesTime, opt);
        s.Serialize(nLogsCount, opt);
        s.Serialize(nHisLogsCount, opt);
        s.Serialize(vReceiptLogs, opt);
        s.Serialize(vHisReceiptLogs, opt);
    }
};

//////////////////////////////
// CBlockMakerFilter

class CBlockMakerFilter
{
    friend class hnbase::CStream;

public:
    CBlockMakerFilter() {}
    CBlockMakerFilter(const uint256& hashForkIn)
      : hashFork(hashForkIn), nPrevGetChangesTime(hnbase::GetTime()) {}

    bool isTimeout();
    bool AddBlockHash(const uint256& hashFork, const uint256& hashBlock, const uint256& hashPrev);
    bool GetFilterBlockHashs(const uint256& hashLastBlock, const bool fAll, std::vector<uint256>& vBlockhash);

protected:
    uint256 hashFork;
    int64 nPrevGetChangesTime;

    std::map<uint256, uint256> mapBlockHash;
    std::map<uint256, uint256, CustomBlockHashCompare> mapHisBlockHash;

protected:
    template <typename O>
    void Serialize(hnbase::CStream& s, O& opt)
    {
        s.Serialize(hashFork, opt);
        s.Serialize(nPrevGetChangesTime, opt);
        s.Serialize(mapBlockHash, opt);
        s.Serialize(mapHisBlockHash, opt);
    }
};

//////////////////////////////
// CBlockPendingTxFilter

class CBlockPendingTxFilter
{
    friend class hnbase::CStream;

public:
    CBlockPendingTxFilter() {}
    CBlockPendingTxFilter(const uint256& hashForkIn)
      : hashFork(hashForkIn), nPrevGetChangesTime(hnbase::GetTime()), nSeqCreate(0) {}

    bool isTimeout();
    bool AddPendingTx(const uint256& hashFork, const uint256& txid);
    bool GetFilterTxids(const uint256& hashFork, const bool fAll, std::vector<uint256>& vTxid);

protected:
    uint256 hashFork;
    int64 nPrevGetChangesTime;
    uint64 nSeqCreate;

    std::set<uint256> setTxid;
    std::set<uint256> setHisTxid;
    std::map<uint64, uint256> mapHisSeq;

protected:
    template <typename O>
    void Serialize(hnbase::CStream& s, O& opt)
    {
        s.Serialize(hashFork, opt);
        s.Serialize(nPrevGetChangesTime, opt);
        s.Serialize(nSeqCreate, opt);
        s.Serialize(setTxid, opt);
        s.Serialize(setHisTxid, opt);
        s.Serialize(mapHisSeq, opt);
    }
};
} // namespace hashahead

#endif // COMMON_CMSTRUCT_H
