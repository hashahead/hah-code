// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "templateid.h"

namespace hashahead
{

//////////////////////////////
// CTemplateId

CTemplateId::CTemplateId() {}

CTemplateId::CTemplateId(const uint256& data)
  : uint256(data)
{
}

CTemplateId& CTemplateId::operator=(uint64 b)
{
    *((uint256*)this) = b;
    return *this;
}

} // namespace hashahead
