// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_BLOCKSTATE_H
#define STORAGE_BLOCKSTATE_H

#include <boost/range/adaptor/reversed.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <list>
#include <map>
#include <numeric>

#include "block.h"
#include "forkcontext.h"
#include "hnbase.h"
#include "param.h"

namespace hashahead
{
namespace storage
{

//////////////////////////////
// CBlockState

class CBlockState
{
    typedef bool (CBlockState::*FuncContractCall)(const CFuncInputParam& funcInputParam, CFuncOutputParam& funcOutParam);

public:
    CBlockState(CBlockBase& dbBlockBaseIn, const uint256& hashForkIn, const CForkContext& ctxForkIn, const CBlock& block, const uint256& hashPrevStateRootIn, const uint32 nPrevBlockTimeIn,
                const std::map<CDestination, CAddressContext>& mapAddressContext, const bool fBtTraceDbIn);

protected:
    CBlockBase& dbBlockBase;
    const uint16 nBlockType;
    const CChainId nLocalChainId;
    const uint256 hashFork;
    const CForkContext ctxFork;
    const uint256 hashPrevBlock;
    const uint256 hashPrevStateRoot;
    const uint32 nPrevBlockTime;
    const uint64 nOriBlockGasLimit;
    const uint64 nBlockTimestamp;
    const int nBlockHeight;
    const uint64 nBlockNumber;
    const CDestination destMint;
    const CTransaction mintTx;
    const uint256 txidMint;
    const bool fPrimaryBlock;
    const bool fBtTraceDb;
    const std::map<CChainId, CBlockProve> mapBlockProve; // key: peer chainid
};

typedef std::shared_ptr<CBlockState> SHP_BLOCK_STATE;
#define MAKE_SHARED_BLOCK_STATE std::make_shared<CBlockState>

} // namespace storage
} // namespace hashahead

#endif //STORAGE_BLOCKSTATE_H
