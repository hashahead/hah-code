// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "purger.h"

#include "blockdb.h"
#include "delegatevotesave.h"
#include "txpooldata.h"
#include "walletdb.h"

using namespace std;
using namespace boost::filesystem;
using namespace hnbase;

namespace hashahead
{
namespace storage
{

//////////////////////////////
// CPurger

bool CPurger::ResetDB(const boost::filesystem::path& pathDataLocation, const uint256& hashGenesisBlockIn, const bool fFullDbIn, const bool fTraceDbIn, const bool fCacheTraceIn) const
{
    {
        CBlockDB dbBlock;
        if (dbBlock.BdInitialize(pathDataLocation, hashGenesisBlockIn, fFullDbIn, fTraceDbIn, fCacheTraceIn, false))
        {
            dbBlock.RemoveAll();
            dbBlock.BdDeinitialize();
        }
    }

    {
        CTxPoolData datTxPool;
        if (datTxPool.Initialize(pathDataLocation))
        {
            if (!datTxPool.Remove())
            {
                return false;
            }
        }
    }

    {
        CDelegateVoteSave datVoteSave;
        if (datVoteSave.Initialize(pathDataLocation))
        {
            if (!datVoteSave.Remove())
            {
                return false;
            }
        }
    }

    return true;
}

bool CPurger::RemoveBlockFile(const path& pathDataLocation) const
{
    try
    {
        path pathBlock = pathDataLocation / "block";
        if (exists(pathBlock))
        {
            remove_all(pathBlock);
        }
    }
    catch (exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

} // namespace storage
} // namespace hashahead
