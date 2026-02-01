// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "mthbase.h"

#include "util.h"

using namespace std;

namespace hnbase
{

//----------------------------------------------------------------------------------------
uint32 CBaseUniqueId::uiIdCreate = 0;
boost::mutex CBaseUniqueId::lockCreate;

} // namespace hnbase
