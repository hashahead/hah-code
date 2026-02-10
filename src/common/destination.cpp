// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "destination.h"

#include "base32.h"
//#include "block.h"
#include "key.h"
#include "template/mint.h"
#include "template/template.h"
#include "transaction.h"
#include "util.h"

using namespace std;
using namespace hnbase;
using namespace hashahead::crypto;

namespace hashahead
{

//////////////////////////////
// CDestination

CDestination::CDestination()
{
    SetNull();
}

CDestination::CDestination(const uint160& data)
  : uint160(data)
{
}

CDestination::CDestination(const uint256& hash)
{
    SetHash(hash);
}

CDestination::CDestination(const uint8* pData, std::size_t nSize)
  : uint160(pData, nSize)
{
}

CDestination::CDestination(const bytes& btData)
  : uint160(btData.data(), btData.size())
{
}

CDestination::CDestination(const std::string& str)
{
    ParseString(str);
}

CDestination::CDestination(const CPubKey& pubkey)
{
    SetPubKey(pubkey);
}

CDestination::CDestination(const CTemplateId& tid)
{
    SetTemplateId(tid);
}

CDestination::CDestination(const CContractId& cid)
{
    SetContractId(cid);
}

void CDestination::SetHash(const uint256& hashData)
{
    memcpy(begin(), hashData.begin() + 12, size());
}

uint256 CDestination::ToHash() const
{
    uint256 n;
    memcpy(n.begin() + 12, begin(), size());
    return n;
}

bytes CDestination::GetCompressData() const
{
    return ToValidBigEndianData();
}

void CDestination::SetCompressData(const bytes& btCompressData)
{
    FromValidBigEndianData(btCompressData);
}

CDestination& CDestination::SetPubKey(const CPubKey& pubkey)
{
    *this = CryptoGetPubkeyAddress(pubkey);
    return *this;
}

CDestination& CDestination::SetTemplateId(const CTemplateId& tid)
{
    SetHash(tid);
    return *this;
}

// contract id
CDestination& CDestination::SetContractId(const CContractId& wid)
{
    SetHash(wid);
    return *this;
}

// CDestination& CDestination::SetContractId(const CDestination& from, const uint64 nTxNonce)
// {
//     hnbase::CBufStream ss;
//     ss << from << nTxNonce;
//     uint256 hash = CryptoHash(ss.GetData(), ss.GetSize());
//     SetHash(hash);
//     return *this;
// }

bool CDestination::ParseString(const string& str)
{
    SetHex(str);
    return true;
}

string CDestination::ToString() const
{
    return GetHex();
}

////////////////////////////////////////////////////
// CTimeVault

void CTimeVault::SettlementTimeVault(const uint32 nHeight, const uint64 nBlockTime, const bool fEstimate)
{
    uint64 nCalcTime = nBlockTime;
    if (VERIFY_FHX_HEIGHT_BRANCH_003(nHeight))
    {
        nCalcTime = EstimateBlockTime(nBlockTime, fEstimate);
    }
    if (nPrevSettlementTime > 0 && nCalcTime > nPrevSettlementTime)
    {
        uint256 nTvs = (nBalanceAmount * (nCalcTime - nPrevSettlementTime)) / TIME_VAULT_RATE;
        if (fSurplus)
        {
            if (nTvAmount > nTvs)
            {
                nTvAmount -= nTvs;
            }
            else
            {
                nTvAmount = nTvs - nTvAmount;
                fSurplus = false;
            }
        }
        else
        {
            nTvAmount += nTvs;
        }
    }
    if (VERIFY_FHX_HEIGHT_BRANCH_003(nHeight))
    {
        if (nCalcTime > nPrevSettlementTime)
        {
            nPrevSettlementTime = nCalcTime;
        }
    }
    else
    {
        nPrevSettlementTime = nCalcTime;
    }
}

void CTimeVault::ModifyBalance(const uint256& nBalance)
{
    nBalanceAmount = nBalance;
}

uint256 CTimeVault::CalcTransTvGasFee(const uint256& nTransAmount)
{
    if (fSurplus || nTvAmount == 0)
    {
        return 0;
    }
    if (nBalanceAmount == 0 || nBalanceAmount <= nTransAmount)
    {
        return nTvAmount;
    }
    return (nTvAmount * nTransAmount) / nBalanceAmount;
}

uint256 CTimeVault::EstimateTransTvGasFee(const uint64 nEstimateBlockTime, const uint256& nTransAmount)
{
    SettlementTimeVault(nEstimateBlockTime);
    return CalcTransTvGasFee(nTransAmount == 0 ? COIN : nTransAmount);
}

void CTimeVault::PayTvGasFee(const uint256& nTvGasFee)
{
    if (fSurplus)
    {
        nTvAmount += nTvGasFee;
    }
    else
    {
        if (nTvAmount > nTvGasFee)
        {
            nTvAmount -= nTvGasFee;
        }
        else
        {
            fSurplus = true;
            nTvAmount = nTvGasFee - nTvAmount;
        }
    }
}

uint256 CTimeVault::CalcGiveTvFee(const uint256& nAmount)
{
    return ((nAmount * GIVE_TIME_VAULT_TS) / TIME_VAULT_RATE);
}

void CTimeVault::CalcRealityTvGasFee(const uint256& nGasPrice, uint256& nTvGasFee, uint256& nTvGas)
{
    if (nTvGasFee > 0 && nGasPrice > 0)
    {
        nTvGas = nTvGasFee / nGasPrice;
        if (nTvGas > 0)
        {
            if (nTvGas * nGasPrice < nTvGasFee)
            {
                nTvGas++;
                nTvGasFee = nGasPrice * nTvGas;
            }
        }
        else
        {
            nTvGas = 1;
            nTvGasFee = nGasPrice;
        }
    }
    else
    {
        nTvGasFee = 0;
        nTvGas = 0;
    }
}

} // namespace hashahead
