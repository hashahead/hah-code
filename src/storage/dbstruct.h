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

} // namespace storage
} // namespace hashahead

#endif // STORAGE_DBSTRUCT_H
