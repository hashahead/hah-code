// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COMMON_TEMPLATE_MINT_H
#define COMMON_TEMPLATE_MINT_H

#include <boost/shared_ptr.hpp>

#include "template.h"

namespace hashahead
{

class CTemplateMint;
typedef boost::shared_ptr<CTemplateMint> CTemplateMintPtr;

class CBlock;

class CTemplateMint : virtual public CTemplate
{
public:
    // Construct by CTemplateMint pointer.
    static const CTemplateMintPtr CreateTemplatePtr(CTemplateMint* ptr);

    virtual bool GetBlockSignDestination(CDestination& destSign) const = 0;
};

} // namespace hashahead

#endif // COMMON_TEMPLATE_MINT_H
