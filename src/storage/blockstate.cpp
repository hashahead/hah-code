// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blockstate.h"

#include <boost/timer/timer.hpp>
#include <cstdio>

#include "blockbase.h"
#include "bloomfilter/bloomfilter.h"
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
using namespace boost::filesystem;
using namespace hnbase;
using namespace hashahead::hvm;

namespace hashahead
{
namespace storage
{

//////////////////////////////
// CBlockState

CBlockState::CBlockState(CBlockBase& dbBlockBaseIn, const uint256& hashForkIn, const CForkContext& ctxForkIn, const CBlock& block, const uint256& hashPrevStateRootIn, const uint32 nPrevBlockTimeIn, const std::map<CDestination, CAddressContext>& mapAddressContext, const bool fBtTraceDbIn)
  : dbBlockBase(dbBlockBaseIn), nBlockType(block.nType), nLocalChainId(CBlock::GetBlockChainIdByHash(hashForkIn)), hashFork(hashForkIn), ctxFork(ctxForkIn), hashPrevBlock(block.hashPrev),
    hashPrevStateRoot(hashPrevStateRootIn), nPrevBlockTime(nPrevBlockTimeIn), nOriBlockGasLimit(block.nGasLimit.Get64()), nSurplusBlockGasLimit(block.nGasLimit.Get64()),
    nBlockTimestamp(block.GetBlockTime()), nBlockHeight(block.GetBlockHeight()), nBlockNumber(block.GetBlockNumber()), destMint(block.txMint.GetToAddress()),
    mintTx(block.txMint), txidMint(block.txMint.GetHash()), fPrimaryBlock(block.IsPrimary()), mapBlockAddressContext(mapAddressContext), mapBlockProve(block.mapProve), fBtTraceDb(fBtTraceDbIn)
{
    hashRefBlock = 0;
    if (fPrimaryBlock)
    {
        if (block.IsProofOfPoa())
        {
            CProofOfPoa proof;
            if (block.GetPoaProof(proof))
            {
                nAgreement = proof.nAgreement;
            }
        }
        else
        {
            CProofOfDelegate proof;
            if (block.GetDelegateProof(proof))
            {
                nAgreement = proof.nAgreement;
            }
        }
    }
    else
    {
        CProofOfPiggyback proof;
        if (block.GetPiggybackProof(proof))
        {
            hashRefBlock = proof.hashRefBlock;
            nAgreement = proof.nAgreement;
        }
    }
    for (auto& tx : block.vtx)
    {
        setBlockBloomData.insert(tx.GetHash().GetBytes());
    }
    uint256 nMintCoint;
    if (!block.GetMintCoinProof(nMintCoint))
    {
        nMintCoint = 0;
    }
    uint256 nTotalTxFee;
    for (auto& tx : block.vtx)
    {
        nTotalTxFee += tx.GetTxFee();
    }
    nOriginalBlockMintReward = nMintCoint + nTotalTxFee;
}

} // namespace storage
} // namespace hashahead
