// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "poa.h"

#include "key.h"
#include "rpc/auto_protocol.h"
#include "template.h"
#include "util.h"

using namespace std;
using namespace hnbase;
using namespace hashahead::crypto;

namespace hashahead
{

//////////////////////////////
// CTemplatePoa

CTemplatePoa::CTemplatePoa(const CDestination& destMintIn, const CDestination& destSpendIn)
  : CTemplate(TEMPLATE_POA), destMint(destMintIn), destSpend(destSpendIn)
{
}

CTemplatePoa* CTemplatePoa::clone() const
{
    return new CTemplatePoa(*this);
}

void CTemplatePoa::GetTemplateData(rpc::CTemplateResponse& obj) const
{
    obj.mint.strMint = destMint.ToString();
    obj.mint.strSpent = destSpend.ToString();
}

bool CTemplatePoa::ValidateParam() const
{
    if (destMint.IsNull() /*|| !destMint.IsPubKey()*/)
    {
        return false;
    }
    if (destSpend.IsNull() || !IsTxSpendable(destSpend))
    {
        return false;
    }
    return true;
}

bool CTemplatePoa::SetTemplateData(const vector<uint8>& vchDataIn)
{
    // CBufStream is(vchDataIn);
    // try
    // {
    //     is >> destMint >> destSpend;
    // }
    // catch (exception& e)
    // {
    //     StdError(__PRETTY_FUNCTION__, e.what());
    //     return false;
    // }
    if (vchDataIn.size() != destMint.size() + destSpend.size())
    {
        return false;
    }
    destMint.SetBytes(vchDataIn.data(), destMint.size());
    destSpend.SetBytes(vchDataIn.data() + destMint.size(), destSpend.size());
    return true;
}

bool CTemplatePoa::SetTemplateData(const rpc::CTemplateRequest& obj)
{
    if (obj.strType != GetTypeName(TEMPLATE_POA))
    {
        return false;
    }

    destMint.ParseString(obj.mint.strMint);
    if (destMint.IsNull())
    {
        return false;
    }

    destSpend.ParseString(obj.mint.strSpent);
    if (destSpend.IsNull())
    {
        return false;
    }
    return true;
}

void CTemplatePoa::BuildTemplateData()
{
    vchData.clear();
    vchData.reserve(destMint.size() + destSpend.size());
    vchData.insert(vchData.end(), (char*)(destMint.begin()), (char*)(destMint.end()));
    vchData.insert(vchData.end(), (char*)(destSpend.begin()), (char*)(destSpend.end()));

    // vchData.clear();
    // CBufStream os;
    // os << destMint << destSpend;
    // os.GetData(vchData);
}

bool CTemplatePoa::GetSignDestination(const CTransaction& tx, CDestination& destSign) const
{
    destSign = destSpend;
    return true;
}

bool CTemplatePoa::GetBlockSignDestination(CDestination& destSign) const
{
    destSign = destMint;
    return true;
}

} // namespace hashahead
