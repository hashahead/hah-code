// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "mint.h"

#include "block.h"
#include "template.h"

using namespace std;

namespace hashahead
{

//////////////////////////////
// CTemplateMint

const CTemplateMintPtr CTemplateMint::CreateTemplatePtr(CTemplateMint* ptr)
{
    return boost::dynamic_pointer_cast<CTemplateMint>(CTemplate::CreateTemplatePtr(ptr));
}

} // namespace hashahead
