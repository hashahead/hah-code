// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_DBSTRUCT_H
#define STORAGE_DBSTRUCT_H

#include <map>

#include "block.h"
#include "destination.h"
#include "hnbase.h"
#include "timeseries.h"
#include "transaction.h"
#include "uint256.h"

namespace hashahead
{
namespace storage
{

#define BLOCKFILE_PREFIX "block"
#define SNAPSHOTFILE_PREFIX "snapshot"

//////////////////////////////
// Snapshot data type

const uint8 SNAP_DATA_TYPE_FORK_BLOCK_INDEX = 0x01;
const uint8 SNAP_DATA_TYPE_FORK_FORK_KV = 0x02;
const uint8 SNAP_DATA_TYPE_FORK_VOTE_KV = 0x03;
const uint8 SNAP_DATA_TYPE_FORK_STATE_KV = 0x04;
const uint8 SNAP_DATA_TYPE_FORK_ADDRESS_KV = 0x05;
const uint8 SNAP_DATA_TYPE_FORK_HDEX_KV = 0x06;
const uint8 SNAP_DATA_TYPE_FORK_TRACE_KV = 0x07;
const uint8 SNAP_DATA_TYPE_FORK_TX_INDEX = 0x08;
const uint8 SNAP_DATA_TYPE_FORK_ADDRESS_TX = 0x09;

const uint8 SNAP_SUB_TYPE_ADDRESS_TX_INDEX = 0x01;
const uint8 SNAP_SUB_TYPE_ADDRESS_TX_COUNT = 0x02;
const uint8 SNAP_SUB_TYPE_TOKEN_TX_INDEX = 0x03;
const uint8 SNAP_SUB_TYPE_TOKEN_TX_COUNT = 0x04;

//////////////////////////////
// Walker function

typedef boost::function<bool(const bytes& btKey, const bytes& btValue)> WalkerTxIndexKvFunc;
typedef boost::function<bool(const CDestination& address, const uint64 nTxIndex, const bytes& btKey, const bytes& btValue)> WalkerAddressTxKvFunc;
typedef boost::function<bool(const CDestination& destContract, const CDestination& destUserAddress, const uint64 nTxIndex, const bytes& btKey, const bytes& btValue)> WalkerTokenTxKvFunc;

//////////////////////////////
// CSnapForkRootKv

class CSnapForkRootKv
{
    friend class hnbase::CStream;

public:
    CSnapForkRootKv() {}
    CSnapForkRootKv(const std::map<uint256, uint256>& mapForkLastBlockIn, const std::vector<uint256>& vBlockHashIn)
      : mapForkLastBlock(mapForkLastBlockIn), vBlockHash(vBlockHashIn) {}

public:
    std::map<uint256, uint256> mapForkLastBlock;
    std::vector<uint256> vBlockHash;
    std::vector<std::pair<uint256, bytesmap>> vKv; // v1: root, v2: inc kv

protected:
    template <typename O>
    void Serialize(hnbase::CStream& s, O& opt)
    {
        s.Serialize(mapForkLastBlock, opt);
        s.Serialize(vBlockHash, opt);
        s.Serialize(vKv, opt);
    }
};

//////////////////////////////
// CVoteRootKv

class CVoteRootKv
{
    friend class hnbase::CStream;

public:
    CVoteRootKv() {}
    CVoteRootKv(const uint256& hashForkIn, const bool fPrimaryChainIn, const std::vector<uint256>& vBlockHashIn)
      : hashFork(hashForkIn), fPrimaryChain(fPrimaryChainIn), vBlockHash(vBlockHashIn) {}

public:
    uint256 hashFork;
    bool fPrimaryChain = false;
    std::vector<uint256> vBlockHash;
    std::vector<std::pair<uint256, bytesmap>> vUserVoteKv;     // v1: root, v2: inc kv
    std::vector<std::pair<uint256, bytesmap>> vDelegateVoteKv; // v1: root, v2: inc kv
    std::vector<std::pair<uint256, bytesmap>> vVoteRewardKv;   // v1: root, v2: inc kv
    std::vector<std::map<int, std::map<CDestination, CDiskPos>>> vDelegateEnroll;

protected:
    template <typename O>
    void Serialize(hnbase::CStream& s, O& opt)
    {
        s.Serialize(hashFork, opt);
        s.Serialize(fPrimaryChain, opt);
        s.Serialize(vBlockHash, opt);
        s.Serialize(vUserVoteKv, opt);
        s.Serialize(vDelegateVoteKv, opt);
        s.Serialize(vVoteRewardKv, opt);
        s.Serialize(vDelegateEnroll, opt);
    }
};

//////////////////////////////
// CSnapBlockVoteData

class CSnapBlockVoteData
{
    friend class hnbase::CStream;

public:
    CSnapBlockVoteData()
      : fAtChain(false) {}
    CSnapBlockVoteData(const uint256& hashBlockIn)
      : hashBlock(hashBlockIn), fAtChain(false) {}

public:
    uint256 hashBlock;
    bytes btBitmap;
    bytes btAggSig;
    bool fAtChain;
    uint256 hashAtBlock;

protected:
    template <typename O>
    void Serialize(hnbase::CStream& s, O& opt)
    {
        s.Serialize(hashBlock, opt);
        s.Serialize(btBitmap, opt);
        s.Serialize(btAggSig, opt);
        s.Serialize(fAtChain, opt);
        s.Serialize(hashAtBlock, opt);
    }
};

//////////////////////////////
// CForkStateRootKv & CForkUserStateRootKv & CForkContractRootKv

class CForkUserStateRootKv
{
    friend class hnbase::CStream;

public:
    CForkUserStateRootKv() {}
    CForkUserStateRootKv(const uint256& hashBlockIn, const uint256& hashRootIn)
      : hashBlock(hashBlockIn), hashRoot(hashRootIn) {}
    CForkUserStateRootKv(const uint256& hashBlockIn, const uint256& hashRootIn, const bytesmap& mapKvIn)
      : hashBlock(hashBlockIn), hashRoot(hashRootIn), mapKv(mapKvIn) {}

public:
    uint256 hashBlock;
    uint256 hashRoot;
    bytesmap mapKv;

protected:
    template <typename O>
    void Serialize(hnbase::CStream& s, O& opt)
    {
        s.Serialize(hashBlock, opt);
        s.Serialize(hashRoot, opt);
        s.Serialize(mapKv, opt);
    }
};

class CForkContractRootKv
{
    friend class hnbase::CStream;

public:
    CForkContractRootKv() {}

public:
    uint256 hashRoot;
    uint256 hashPrevRoot;
    bytesmap mapKv;
    uint32 nBlockHeight = 0;
    uint64 nBlockNumber = 0;

protected:
    template <typename O>
    void Serialize(hnbase::CStream& s, O& opt)
    {
        s.Serialize(hashRoot, opt);
        s.Serialize(hashPrevRoot, opt);
        s.Serialize(mapKv, opt);
        s.Serialize(nBlockHeight, opt);
        s.Serialize(nBlockNumber, opt);
    }
};

class CForkStateRootKv
{
    friend class hnbase::CStream;

public:
    CForkStateRootKv() {}
    CForkStateRootKv(const uint256& hashForkIn, const uint256& hashLastBlockIn)
      : hashFork(hashForkIn), hashLastBlock(hashLastBlockIn) {}

public:
    uint256 hashFork;
    uint256 hashLastBlock;
    std::vector<CForkUserStateRootKv> vStateKv;                   // v1: root, v2: inc kv
    std::map<CDestination, CForkContractRootKv> mapLastStorageKv; // key: contract address

protected:
    template <typename O>
    void Serialize(hnbase::CStream& s, O& opt)
    {
        s.Serialize(hashFork, opt);
        s.Serialize(hashLastBlock, opt);
        s.Serialize(vStateKv, opt);
        s.Serialize(mapLastStorageKv, opt);
    }
};

//////////////////////////////
// CForkAddressRootKv

class CForkAddressRootKv
{
    friend class hnbase::CStream;

public:
    CForkAddressRootKv() {}
    CForkAddressRootKv(const uint256& hashForkIn, const std::vector<uint256>& vBlockHashIn)
      : hashFork(hashForkIn), vBlockHash(vBlockHashIn) {}

public:
    uint256 hashFork;
    std::vector<uint256> vBlockHash;
    std::vector<std::pair<uint256, bytesmap>> vAddressKv; // v1: root, v2: inc kv
    std::vector<std::pair<uint256, bytesmap>> vTokenKv;   // v1: root, v2: inc kv
    std::vector<std::pair<uint256, bytesmap>> vCodeKv;    // v1: root, v2: inc kv
    std::vector<std::pair<uint64, uint64>> vAddreeCount;  // v1: address count, v2: inc address count

protected:
    template <typename O>
    void Serialize(hnbase::CStream& s, O& opt)
    {
        s.Serialize(hashFork, opt);
        s.Serialize(vBlockHash, opt);
        s.Serialize(vAddressKv, opt);
        s.Serialize(vTokenKv, opt);
        s.Serialize(vCodeKv, opt);
        s.Serialize(vAddreeCount, opt);
    }
};

//////////////////////////////
// CHdexFirstPrevBlock & CHdexLastProveBlock & CHdexRecvCrosschainProve

class CHdexFirstPrevBlock
{
    friend class hnbase::CStream;

public:
    CHdexFirstPrevBlock()
      : nRecvChainId(0), nSendChainId(0) {}
    CHdexFirstPrevBlock(const uint32 nRecvChainIdIn, const uint32 nSendChainIdIn, const uint256& hashBlockIn, const uint256& hashFirstPrevBlockIn)
      : nRecvChainId(nRecvChainIdIn), nSendChainId(nSendChainIdIn), hashBlock(hashBlockIn), hashFirstPrevBlock(hashFirstPrevBlockIn) {}

public:
    uint32 nRecvChainId;
    uint32 nSendChainId;
    uint256 hashBlock;
    uint256 hashFirstPrevBlock;

protected:
    template <typename O>
    void Serialize(hnbase::CStream& s, O& opt)
    {
        s.Serialize(nRecvChainId, opt);
        s.Serialize(nSendChainId, opt);
        s.Serialize(hashBlock, opt);
        s.Serialize(hashFirstPrevBlock, opt);
    }
};
} // namespace storage
} // namespace hashahead

#endif // STORAGE_DBSTRUCT_H
