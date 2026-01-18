// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COMMON_TEMPLATE_ACTIVATECODE_H
#define COMMON_TEMPLATE_ACTIVATECODE_H

#include "destination.h"
#include "template.h"

namespace hashahead
{

class CTemplateActivateCode : virtual public CTemplate
{
public:
    CTemplateActivateCode(const CDestination& destGrantIn = CDestination());
    virtual CTemplateActivateCode* clone() const;
    virtual void GetTemplateData(rpc::CTemplateResponse& obj) const;

protected:
    virtual bool ValidateParam() const;
    virtual bool SetTemplateData(const std::vector<uint8>& vchDataIn);
    virtual bool SetTemplateData(const rpc::CTemplateRequest& obj);
    virtual void BuildTemplateData();
    virtual bool GetSignDestination(const CTransaction& tx, CDestination& destSign) const;

public:
    CDestination destGrant;
};

} // namespace hashahead

#endif // COMMON_TEMPLATE_VOTE_H
