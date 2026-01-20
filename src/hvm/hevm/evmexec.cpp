// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "evmexec.h"

#include "../heth/libaleth-interpreter/interpreter.h"
#include "../heth/libevm/PithyEvmc.h"
#include "block.h"
#include "evmc/evmc.h"
#include "evmhost.h"
#include "hutil.h"

using namespace std;
using namespace hnbase;
using namespace dev::eth;
using namespace dev;

namespace hashahead
{
namespace hvm
{

//////////////////////////////
CDestination CreateContractAddressByNonce(const CDestination& _sender, const uint256& _nonce)
{
    Address _out = createContractAddress(addressFromDest(_sender), u256(_nonce.Get64()));
    return destFromAddress(_out);
}

} // namespace hvm
} // namespace hashahead
