// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "block.h"

#include "crypto.h"
#include "hnbase.h"

using namespace std;
using namespace hnbase;
using namespace hashahead::crypto;

namespace hashahead
{

//////////////////////////////
// CBlock

void CBlock::SetNull()
{
    nVersion = 1;
    nType = 0;
    nTimeStamp = 0;
    nNumber = 0;
    nHeight = 0;
    nSlot = 0;
    hashPrev = 0;
    hashMerkleRoot = 0;
    hashStateRoot = 0;
    hashReceiptsRoot = 0;
    hashCrosschainMerkleRoot = 0;
    nGasLimit = 0;
    nGasUsed = 0;
    mapProof.clear();
    txMint.SetNull();
    vchSig.clear();

    btBloomData.clear();
    vtx.clear();
    mapProve.clear();
}

bool CBlock::IsNull() const
{
    return (nType == 0 || nTimeStamp == 0 || txMint.IsNull());
}

bool CBlock::IsGenesis() const
{
    return (nType == BLOCK_GENESIS);
}

bool CBlock::IsOrigin() const
{
    return (nType == BLOCK_GENESIS || nType == BLOCK_ORIGIN);
}

bool CBlock::IsPrimary() const
{
    return ((nType >> 4) == 0);
}

bool CBlock::IsSubsidiary() const
{
    return (nType == BLOCK_SUBSIDIARY);
}

bool CBlock::IsExtended() const
{
    return (nType == BLOCK_EXTENDED);
}

bool CBlock::IsVacant() const
{
    return (nType == BLOCK_VACANT);
}

bool CBlock::IsProofOfPoa() const
{
    return (txMint.GetTxType() == CTransaction::TX_POA);
}

bool CBlock::IsProofEmpty() const
{
    return mapProof.empty();
}

uint256 CBlock::GetHash() const
{
    // hnbase::CBufStream ss;
    // ss << nVersion << nType << nTimeStamp << nNumber << nHeight << nSlot << hashPrev << hashMerkleRoot << hashStateRoot << hashReceiptsRoot << hashCrosschainMerkleRoot << nGasLimit << nGasUsed << mapProof << txMint;
    // return uint256(GetChainId(), GetBlockHeight(), nSlot, crypto::CryptoHash(ss.GetData(), ss.GetSize()));

    return uint256(GetChainId(), GetBlockHeight(), nSlot, CalcMerkleTreeRoot());
}

std::size_t CBlock::GetTxSerializedOffset() const
{
    bytes btMerkleRoot;
    bytes btReceiptsRoot;
    bytes btCrosschainMerkleRoot;
    bytes btGasLimit = nGasLimit.ToValidBigEndianData();
    bytes btGasUsed = nGasUsed.ToValidBigEndianData();
    if (!hashMerkleRoot.IsNull())
    {
        btMerkleRoot = hashMerkleRoot.GetBytes();
    }
    if (!hashReceiptsRoot.IsNull())
    {
        btReceiptsRoot = hashReceiptsRoot.GetBytes();
    }
    if (!hashCrosschainMerkleRoot.IsNull())
    {
        btCrosschainMerkleRoot = hashCrosschainMerkleRoot.GetBytes();
    }
    hnbase::CBufStream ss;
    ss << nVersion << nType << CVarInt(nTimeStamp) << CVarInt(nNumber) << nHeight << nSlot << hashPrev << btMerkleRoot << hashStateRoot << btReceiptsRoot << btCrosschainMerkleRoot << btBloomData << btGasLimit << btGasUsed << mapProof;
    return ss.GetSize();
}

void CBlock::GetSerializedProofOfWorkData(std::vector<unsigned char>& vchProofOfWork) const
{
    hnbase::CBufStream ss;
    ss << nVersion << nType << nTimeStamp << nNumber << nHeight << nSlot << hashPrev << mapProof;
    vchProofOfWork.assign(ss.GetData(), ss.GetData() + ss.GetSize());
}

uint32 CBlock::GetChainId() const
{
    if (IsOrigin())
    {
        CProfile profile;
        if (!GetForkProfile(profile))
        {
            return 0;
        }
        return profile.nChainId;
    }
    return hashPrev.GetB1();
}

uint64 CBlock::GetBlockTime() const
{
    return nTimeStamp;
}

uint64 CBlock::GetBlockNumber() const
{
    return nNumber;
}

uint32 CBlock::GetBlockSlot() const
{
    return nSlot;
}

uint32 CBlock::GetBlockHeight() const
{
    // if (IsGenesis())
    // {
    //     return 0;
    // }
    // else if (IsExtended())
    // {
    //     return hashPrev.GetB2();
    // }
    // else
    // {
    //     return hashPrev.GetB2() + 1;
    // }
    return nHeight;
}

uint256 CBlock::GetRefBlock() const
{
    uint256 hashRefBlock;
    if (!IsPrimary() && !IsOrigin())
    {
        CProofOfPiggyback proof;
        if (GetPiggybackProof(proof))
        {
            hashRefBlock = proof.hashRefBlock;
        }
    }
    return hashRefBlock;
}

uint64 CBlock::GetBlockBeacon(int idx) const
{
    if (mapProof.empty())
    {
        return hashPrev.Get64(idx & 3);
    }
    return 0;
}

bool CBlock::GetBlockMint(uint256& nMintCoin) const
{
    if (nType == BLOCK_EXTENDED || nType == BLOCK_VACANT)
    {
        return true;
    }
    return GetMintCoinProof(nMintCoin);
}

uint256 CBlock::GetBlockTotalReward() const
{
    uint256 nTotalReward;
    if (!GetMintRewardProof(nTotalReward))
    {
        return 0;
    }
    return nTotalReward;
}

uint256 CBlock::GetBlockMoneyDestroy() const
{
    uint256 nDestroyAmount;
    for (auto& tx : vtx)
    {
        if (tx.GetToAddress() == FUNCTION_BLACKHOLE_ADDRESS)
        {
            nDestroyAmount += tx.GetAmount();
        }
    }
    return nDestroyAmount;
}

void CBlock::SetBlockTime(const uint64 nTime)
{
    nTimeStamp = nTime;
}

void CBlock::SetSignData(const bytes& btSigData)
{
    vchSig = btSigData;
}

void CBlock::UpdateMerkleRoot()
{
    hashMerkleRoot = CalcMerkleTreeRoot();
}

bool CBlock::VerifyBlockHeight() const
{
    uint32 nCalcHeight = 0;
    if (IsGenesis())
    {
        nCalcHeight = 0;
    }
    else if (IsExtended())
    {
        nCalcHeight = hashPrev.GetB2();
    }
    else
    {
        nCalcHeight = hashPrev.GetB2() + 1;
    }
    return (nHeight == nCalcHeight);
}

bool CBlock::VerifyBlockMerkleTreeRoot() const
{
    return (hashMerkleRoot == CalcMerkleTreeRoot());
}

bool CBlock::VerifyBlockSignature(const CDestination& destBlockSign) const
{
    uint256 hash = GetHash();
    uint512 pub = CrytoGetSignPubkey(hash, vchSig);
    if (destBlockSign != CDestination(CPubKey(pub)))
    {
        return false;
    }
    return true;
}

bool CBlock::VerifyBlockProof() const
{
    for (auto& kv : mapProof)
    {
        std::size_t nDataSize = 0;
        switch (kv.first)
        {
        case BP_FORK_PROFILE:
        {
            if (!IsOrigin())
            {
                return false;
            }
            CProfile profile;
            if (!profile.Load(kv.second))
            {
                return false;
            }
            bytes btData;
            profile.Save(btData);
            nDataSize = btData.size();
            break;
        }
        case BP_DELEGATE:
        {
            if (nType != BLOCK_PRIMARY || txMint.GetTxType() != CTransaction::TX_STAKE)
            {
                return false;
            }
            CProofOfDelegate proof;
            if (!proof.Load(kv.second))
            {
                return false;
            }
            bytes btData;
            proof.Save(btData);
            nDataSize = btData.size();
            break;
        }
        case BP_PIGGYBACK:
        {
            if (nType != BLOCK_SUBSIDIARY && nType != BLOCK_EXTENDED && nType != BLOCK_VACANT)
            {
                return false;
            }
            CProofOfPiggyback proof;
            if (!proof.Load(kv.second))
            {
                return false;
            }
            bytes btData;
            proof.Save(btData);
            nDataSize = btData.size();
            break;
        }
        case BP_POA_PROOF:
        {
            if (nType != BLOCK_PRIMARY || txMint.GetTxType() != CTransaction::TX_POA)
            {
                return false;
            }
            CProofOfPoa proof;
            if (!proof.Load(kv.second))
            {
                return false;
            }
            bytes btData;
            proof.Save(btData);
            nDataSize = btData.size();
            break;
        }
        case BP_MINTCOIN:
        case BP_MINTREWARD:
        {
            nDataSize = uint256().size();
            break;
        }
        case BP_BLOCK_VOTE_SIG:
        {
            CBlockVoteSig proof;
            if (!proof.Load(kv.second))
            {
                return false;
            }
            bytes btData;
            proof.Save(btData);
            nDataSize = btData.size();
            break;
        }
        default:
            return false;
        }
        if (kv.second.size() != nDataSize)
        {
            return false;
        }
    }
    return true;
}

//------------------------------------------------
void CBlock::AddProofData(const uint8 nProofId, const bytes& btData)
{
    mapProof[nProofId] = btData;
}

bool CBlock::GetProofData(const uint8 nProofId, bytes& btData) const
{
    auto it = mapProof.find(nProofId);
    if (it == mapProof.end())
    {
        return false;
    }
    btData = it->second;
    return true;
}

//------------------------------------------------
void CBlock::AddForkProfile(const CProfile& profile)
{
    bytes btPf;
    profile.Save(btPf);
    mapProof[BP_FORK_PROFILE] = btPf;
}

void CBlock::AddDelegateProof(const CProofOfDelegate& proof)
{
    bytes btPf;
    proof.Save(btPf);
    mapProof[BP_DELEGATE] = btPf;
}

void CBlock::AddPiggybackProof(const CProofOfPiggyback& proof)
{
    bytes btPf;
    proof.Save(btPf);
    mapProof[BP_PIGGYBACK] = btPf;
}

void CBlock::AddPoaProof(const CProofOfPoa& proof)
{
    bytes btPf;
    proof.Save(btPf);
    mapProof[BP_POA_PROOF] = btPf;
}

void CBlock::AddMintCoinProof(const uint256& nMintCoin)
{
    mapProof[BP_MINTCOIN] = nMintCoin.GetBytes();
}

void CBlock::AddMintRewardProof(const uint256& nMintReward)
{
    mapProof[BP_MINTREWARD] = nMintReward.GetBytes();
}

void CBlock::AddBlockVoteSig(const CBlockVoteSig& proof)
{
    bytes btPf;
    proof.Save(btPf);
    mapProof[BP_BLOCK_VOTE_SIG] = btPf;
}

//------------------------------------------------
bool CBlock::GetForkProfile(CProfile& profile) const
{
    auto it = mapProof.find(BP_FORK_PROFILE);
    if (it == mapProof.end())
    {
        return false;
    }
    return profile.Load(it->second);
}

bool CBlock::GetDelegateProof(CProofOfDelegate& proof) const
{
    auto it = mapProof.find(BP_DELEGATE);
    if (it == mapProof.end())
    {
        return false;
    }
    return proof.Load(it->second);
}

bool CBlock::GetPiggybackProof(CProofOfPiggyback& proof) const
{
    auto it = mapProof.find(BP_PIGGYBACK);
    if (it == mapProof.end())
    {
        return false;
    }
    return proof.Load(it->second);
}

bool CBlock::GetPoaProof(CProofOfPoa& proof) const
{
    auto it = mapProof.find(BP_POA_PROOF);
    if (it == mapProof.end())
    {
        return false;
    }
    return proof.Load(it->second);
}

bool CBlock::GetMintCoinProof(uint256& nMintCoin) const
{
    auto it = mapProof.find(BP_MINTCOIN);
    if (it == mapProof.end())
    {
        return false;
    }
    return (nMintCoin.SetBytes(it->second) == nMintCoin.size());
}

bool CBlock::GetMintRewardProof(uint256& nMintReward) const
{
    auto it = mapProof.find(BP_MINTREWARD);
    if (it == mapProof.end())
    {
        return false;
    }
    return (nMintReward.SetBytes(it->second) == nMintReward.size());
}

bool CBlock::GetBlockVoteSig(CBlockVoteSig& proof) const
{
    auto it = mapProof.find(BP_BLOCK_VOTE_SIG);
    if (it == mapProof.end())
    {
        return false;
    }
    return proof.Load(it->second);
}

bool CBlock::GetMerkleProve(SHP_MERKLE_PROVE_DATA ptrMerkleProvePrevBlock, SHP_MERKLE_PROVE_DATA ptrMerkleProveRefBlock, SHP_MERKLE_PROVE_DATA ptrCrossMerkleProve) const
{
    return (CalcMerkleTreeRootAndProve(ptrMerkleProvePrevBlock, ptrMerkleProveRefBlock, ptrCrossMerkleProve) != 0);
}

//-------------------------------------------------------
CChainId CBlock::GetBlockChainIdByHash(const uint256& hash)
{
    return hash.GetB1();
}

uint32 CBlock::GetBlockHeightByHash(const uint256& hash)
{
    return hash.GetB2();
}

uint16 CBlock::GetBlockSlotByHash(const uint256& hash)
{
    return hash.GetB3();
}

uint256 CBlock::CreateBlockHash(const CChainId nChainId, const uint32 nHeight, const uint16 nSlot, const uint256& hash)
{
    return uint256(nChainId, nHeight, nSlot, hash);
}

bool CBlock::BlockHashCompare(const uint256& a, const uint256& b) // return true: a < b, false: a >= b
{
    CChainId nChainIdA = CBlock::GetBlockChainIdByHash(a);
    CChainId nChainIdB = CBlock::GetBlockChainIdByHash(b);
    if (nChainIdA < nChainIdB)
    {
        return true;
    }
    else if (nChainIdA == nChainIdB)
    {
        uint32 nHeightA = CBlock::GetBlockHeightByHash(a);
        uint32 nHeightB = CBlock::GetBlockHeightByHash(b);
        if (nHeightA < nHeightB)
        {
            return true;
        }
        else if (nHeightA == nHeightB)
        {
            uint16 nSlotA = CBlock::GetBlockSlotByHash(a);
            uint16 nSlotB = CBlock::GetBlockSlotByHash(b);
            if (nSlotA < nSlotB)
            {
                return true;
            }
            else if (nSlotA == nSlotB && a < b)
            {
                return true;
            }
        }
    }
    return false;
}

bool CBlock::VerifyBlockMerkleProve(const uint256& hashBlock, const hnbase::MERKLE_PROVE_DATA& merkleProve, const uint256& hashVerify)
{
    return (hashBlock == uint256(GetBlockChainIdByHash(hashBlock), GetBlockHeightByHash(hashBlock), GetBlockSlotByHash(hashBlock), CMerkleTree::GetMerkleRootByProve(merkleProve, hashVerify)));
}

//-------------------------------------------------------
uint256 CBlock::CalcMerkleTreeRoot() const
{
    return CalcMerkleTreeRootAndProve(nullptr, nullptr, nullptr);
}

uint256 CBlock::CalcMerkleTreeRootAndProve(SHP_MERKLE_PROVE_DATA ptrMerkleProvePrevBlock, SHP_MERKLE_PROVE_DATA ptrMerkleProveRefBlock, SHP_MERKLE_PROVE_DATA ptrCrossMerkleProve) const
{
    const uint256 hashBaseMerkleRoot = CalcBlockBaseMerkleTreeRoot(ptrMerkleProvePrevBlock, ptrMerkleProveRefBlock);
    const uint256 hashTxMerkleRoot = CalcTxMerkleTreeRoot();

    std::vector<uint256> vMerkleTree;

    vMerkleTree.push_back(hashBaseMerkleRoot);
    vMerkleTree.push_back(hashStateRoot);
    vMerkleTree.push_back(hashCrosschainMerkleRoot);
    vMerkleTree.push_back(hashTxMerkleRoot);

    const std::size_t nLeafCount = vMerkleTree.size();

    uint256 hashMerkleRoot = CMerkleTree::BuildMerkleTree(vMerkleTree);

    if (ptrMerkleProvePrevBlock || ptrMerkleProveRefBlock)
    {
        SHP_MERKLE_PROVE_DATA ptrMerkleProveBase = MAKE_SHARED_MERKLE_PROVE_DATA();
        if (!ptrMerkleProveBase || !CMerkleTree::BuildMerkleProve(hashBaseMerkleRoot, vMerkleTree, nLeafCount, *ptrMerkleProveBase))
        {
            return 0;
        }
        if (ptrMerkleProvePrevBlock)
        {
            (*ptrMerkleProvePrevBlock).insert((*ptrMerkleProvePrevBlock).end(), (*ptrMerkleProveBase).begin(), (*ptrMerkleProveBase).end());
        }
        if (ptrMerkleProveRefBlock)
        {
            (*ptrMerkleProveRefBlock).insert((*ptrMerkleProveRefBlock).end(), (*ptrMerkleProveBase).begin(), (*ptrMerkleProveBase).end());
        }
    }

    if (ptrCrossMerkleProve)
    {
        if (!CMerkleTree::BuildMerkleProve(hashCrosschainMerkleRoot, vMerkleTree, nLeafCount, *ptrCrossMerkleProve))
        {
            return 0;
        }
    }
    return hashMerkleRoot;
}

uint256 CBlock::CalcBlockBaseMerkleTreeRoot(SHP_MERKLE_PROVE_DATA ptrMerkleProvePrevBlock, SHP_MERKLE_PROVE_DATA ptrMerkleProveRefBlock) const
{
    // hnbase::CBufStream ss;
    // ss << nVersion << nType << nTimeStamp << nNumber << nHeight << nSlot << hashPrev << hashMerkleRoot << hashStateRoot << hashReceiptsRoot << hashCrosschainMerkleRoot << nGasLimit << nGasUsed << mapProof << txMint;
    // return uint256(GetChainId(), GetBlockHeight(), nSlot, crypto::CryptoHash(ss.GetData(), ss.GetSize()));

    uint256 hashBase;
    uint256 hashProof;
    uint256 hashRefBlock;
    uint256 hashBloom;

    {
        hnbase::CBufStream ss;
        ss << nVersion << nType << nTimeStamp << nNumber << nHeight << nSlot << nGasLimit << nGasUsed;
        hashBase = crypto::CryptoHash(ss.GetData(), ss.GetSize());
    }

    if (!mapProof.empty())
    {
        hnbase::CBufStream ss;
        ss << mapProof;
        hashProof = crypto::CryptoHash(ss.GetData(), ss.GetSize());
    }

    hashRefBlock = GetRefBlock();

    if (!btBloomData.empty())
    {
        hashBloom = crypto::CryptoHash(btBloomData.data(), btBloomData.size());
    }

    std::vector<uint256> vMerkleTree;

    vMerkleTree.push_back(hashBase);
    vMerkleTree.push_back(hashProof);
    vMerkleTree.push_back(hashRefBlock);
    vMerkleTree.push_back(hashPrev);
    vMerkleTree.push_back(hashReceiptsRoot);
    vMerkleTree.push_back(hashBloom);

    const std::size_t nLeafCount = vMerkleTree.size();

    uint256 hashMerkleRoot = CMerkleTree::BuildMerkleTree(vMerkleTree);
    if (hashMerkleRoot == 0)
    {
        return 0;
    }

    if (ptrMerkleProvePrevBlock)
    {
        if (!CMerkleTree::BuildMerkleProve(hashPrev, vMerkleTree, nLeafCount, *ptrMerkleProvePrevBlock))
        {
            return 0;
        }
    }
    if (ptrMerkleProveRefBlock)
    {
        if (!CMerkleTree::BuildMerkleProve(hashRefBlock, vMerkleTree, nLeafCount, *ptrMerkleProveRefBlock))
        {
            return 0;
        }
    }
    return hashMerkleRoot;
}

uint256 CBlock::CalcTxMerkleTreeRoot() const
{
    std::vector<uint256> vMerkleTree;
    vMerkleTree.push_back(txMint.GetHash());
    for (const CTransaction& tx : vtx)
    {
        vMerkleTree.push_back(tx.GetHash());
    }
    return CMerkleTree::BuildMerkleTree(vMerkleTree);
}

//-------------------------------------------------------
void CBlock::Serialize(hnbase::CStream& s, hnbase::SaveType&) const
{
    bytes btMerkleRoot;
    bytes btReceiptsRoot;
    bytes btCrosschainMerkleRoot;
    bytes btGasLimit = nGasLimit.ToValidBigEndianData();
    bytes btGasUsed = nGasUsed.ToValidBigEndianData();
    if (!hashMerkleRoot.IsNull())
    {
        btMerkleRoot = hashMerkleRoot.GetBytes();
    }
    if (!hashReceiptsRoot.IsNull())
    {
        btReceiptsRoot = hashReceiptsRoot.GetBytes();
    }
    if (!hashCrosschainMerkleRoot.IsNull())
    {
        btCrosschainMerkleRoot = hashCrosschainMerkleRoot.GetBytes();
    }
    s << nVersion << nType << CVarInt(nTimeStamp) << CVarInt(nNumber) << nHeight << nSlot << hashPrev << btMerkleRoot << hashStateRoot << btReceiptsRoot << btCrosschainMerkleRoot << btBloomData << btGasLimit << btGasUsed << mapProof << txMint << vtx << mapProve << vchSig;
}

void CBlock::Serialize(hnbase::CStream& s, hnbase::LoadType&)
{
    CVarInt varTimeStamp;
    CVarInt varNumber;
    bytes btMerkleRoot;
    bytes btReceiptsRoot;
    bytes btCrosschainMerkleRoot;
    bytes btGasLimit;
    bytes btGasUsed;
    s >> nVersion >> nType >> varTimeStamp >> varNumber >> nHeight >> nSlot >> hashPrev >> btMerkleRoot >> hashStateRoot >> btReceiptsRoot >> btCrosschainMerkleRoot >> btBloomData >> btGasLimit >> btGasUsed >> mapProof >> txMint >> vtx >> mapProve >> vchSig;
    nTimeStamp = varTimeStamp.nValue;
    nNumber = varNumber.nValue;
    nGasLimit.FromValidBigEndianData(btGasLimit);
    nGasUsed.FromValidBigEndianData(btGasUsed);
    hashMerkleRoot.SetBytes(btMerkleRoot);
    hashReceiptsRoot.SetBytes(btReceiptsRoot);
    hashCrosschainMerkleRoot.SetBytes(btCrosschainMerkleRoot);
}

void CBlock::Serialize(hnbase::CStream& s, std::size_t& serSize) const
{
    (void)s;
    bytes btMerkleRoot;
    bytes btReceiptsRoot;
    bytes btCrosschainMerkleRoot;
    bytes btGasLimit = nGasLimit.ToValidBigEndianData();
    bytes btGasUsed = nGasUsed.ToValidBigEndianData();
    if (!hashMerkleRoot.IsNull())
    {
        btMerkleRoot = hashMerkleRoot.GetBytes();
    }
    if (!hashReceiptsRoot.IsNull())
    {
        btReceiptsRoot = hashReceiptsRoot.GetBytes();
    }
    if (!hashCrosschainMerkleRoot.IsNull())
    {
        btCrosschainMerkleRoot = hashCrosschainMerkleRoot.GetBytes();
    }
    hnbase::CBufStream ss;
    ss << nVersion << nType << CVarInt(nTimeStamp) << CVarInt(nNumber) << nHeight << nSlot << hashPrev << btMerkleRoot << hashStateRoot << btReceiptsRoot << btCrosschainMerkleRoot << btBloomData << btGasLimit << btGasUsed << mapProof << txMint << vtx << mapProve << vchSig;
    serSize = ss.GetSize();
}

///////////////////////////////////////////////
// CBlockIndex

CBlockIndex::CBlockIndex()
{
    hashOrigin = 0;
    hashBlock = 0;
    hashPrev = 0;
    txidMint = 0;
    nMintType = 0;
    destMint.SetNull();
    nVersion = 0;
    nType = 0;
    nTimeStamp = 0;
    nNumber = 0;
    nTxCount = 0;
    nRewardTxCount = 0;
    nUserTxCount = 0;
    nAgreement = 0;
    hashRefBlock = 0;
    hashStateRoot = 0;
    nGasLimit = 0;
    nGasUsed = 0;
    nChainTrust = uint64(0);
    nBlockReward = 0;
    nMoneySupply = 0;
    nMoneyDestroy = 0;
    nFile = 0;
    nOffset = 0;
    nBlockCrc = 0;
}

CBlockIndex::CBlockIndex(const uint256& hashOriginIn, const uint256& hashBlockIn, const CBlock& block, const uint32 nFileIn, const uint32 nOffsetIn, const uint32 nCrcIn)
{
    hashOrigin = hashOriginIn;
    hashBlock = hashBlockIn;
    hashPrev = block.hashPrev;
    txidMint = (block.IsVacant() ? uint64(0) : block.txMint.GetHash());
    nMintType = block.txMint.GetTxType();
    destMint = block.txMint.GetToAddress();
    nVersion = block.nVersion;
    nType = block.nType;
    nTimeStamp = block.GetBlockTime();
    nNumber = block.GetBlockNumber();
    nTxCount = block.vtx.size() + 1;
    nRewardTxCount = 0;
    nUserTxCount = 0;
    nAgreement = 0;
    hashRefBlock = 0;
    hashStateRoot = block.hashStateRoot;
    nGasLimit = block.nGasLimit;
    nGasUsed = block.nGasUsed;
    nChainTrust = uint64(0);
    nBlockReward = block.GetBlockTotalReward();
    nMoneySupply = 0;
    nMoneyDestroy = 0;
    nFile = nFileIn;
    nOffset = nOffsetIn;
    nBlockCrc = nCrcIn;

    for (auto& tx : block.vtx)
    {
        if (tx.GetTxType() == CTransaction::TX_GENESIS || tx.GetTxType() == CTransaction::TX_VOTE_REWARD)
        {
            nRewardTxCount++;
        }
        else if (tx.IsUserTx())
        {
            nUserTxCount++;
        }
    }

    if (IsPrimary())
    {
        if (IsProofOfPoa())
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
        hashRefBlock = hashBlock;
    }
    else
    {
        CProofOfPiggyback proof;
        if (block.GetPiggybackProof(proof) && proof.hashRefBlock != 0)
        {
            nAgreement = proof.nAgreement;
            hashRefBlock = proof.hashRefBlock;
        }
    }
}

} // namespace hashahead
