// Copyright (c) 2021-2023 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "profile.h"

#include <compacttv.h>
#include <util.h>

using namespace std;
using namespace hnbase;

namespace hashahead
{

//////////////////////////////
// CProfile

bool CProfile::Save(std::vector<unsigned char>& vchProfile) const
{
    hnbase::CBufStream ss;
    ss << *this;
    ss.GetData(vchProfile);
    return true;
}

bool CProfile::Load(const std::vector<unsigned char>& vchProfile)
{
    try
    {
        hnbase::CBufStream ss(vchProfile);
        ss >> *this;
    }
    catch (exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

void CProfile::Serialize(hnbase::CStream& s, hnbase::SaveType&) const
{
    bytes btAmount = nAmount.ToValidBigEndianData();
    bytes btMintReward = nMintReward.ToValidBigEndianData();
    bytes btMinTxFee = nMinTxFee.ToValidBigEndianData();
    s << nVersion << nType << strName << strSymbol << CVarInt((uint64)nChainId) << btAmount << btMintReward << btMinTxFee << CVarInt((uint64)nHalveCycle) << destOwner << hashParent << CVarInt((uint64)nJointHeight);
}

void CProfile::Serialize(hnbase::CStream& s, hnbase::LoadType&)
{
    bytes btAmount;
    bytes btMintReward;
    bytes btMinTxFee;
    CVarInt varChainId;
    CVarInt varHalveCycle;
    CVarInt varJointHeight;
    s >> nVersion >> nType >> strName >> strSymbol >> varChainId >> btAmount >> btMintReward >> btMinTxFee >> varHalveCycle >> destOwner >> hashParent >> varJointHeight;
    nChainId = (CChainId)(varChainId.nValue);
    nHalveCycle = (uint32)(varHalveCycle.nValue);
    nJointHeight = (int)(varJointHeight.nValue);
    nAmount.FromValidBigEndianData(btAmount);
    nMintReward.FromValidBigEndianData(btMintReward);
    nMinTxFee.FromValidBigEndianData(btMinTxFee);
}

void CProfile::Serialize(hnbase::CStream& s, std::size_t& serSize) const
{
    (void)s;
    bytes btAmount = nAmount.ToValidBigEndianData();
    bytes btMintReward = nMintReward.ToValidBigEndianData();
    bytes btMinTxFee = nMinTxFee.ToValidBigEndianData();
    hnbase::CBufStream ss;
    ss << nVersion << nType << strName << strSymbol << CVarInt((uint64)nChainId) << btAmount << btMintReward << btMinTxFee << CVarInt((uint64)nHalveCycle) << destOwner << hashParent << CVarInt((uint64)nJointHeight);
    serSize = ss.GetSize();
}

} // namespace hashahead
