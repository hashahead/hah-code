// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_DBSTRUCT_H
#define STORAGE_DBSTRUCT_H

#include <map>

#include "block.h"
#include "destination.h"
#include "hnbase.h"
#include "timeseries.h"
#include "transaction.h"
#include "uint256.h"

namespace hashahead
{
namespace storage
{

//////////////////////////////
// CSnapForkRootKv

class CSnapForkRootKv
{
    friend class hnbase::CStream;

public:
    CSnapForkRootKv() {}
    CSnapForkRootKv(const std::map<uint256, uint256>& mapForkLastBlockIn, const std::vector<uint256>& vBlockHashIn)
      : mapForkLastBlock(mapForkLastBlockIn), vBlockHash(vBlockHashIn) {}

public:
    std::map<uint256, uint256> mapForkLastBlock;
    std::vector<uint256> vBlockHash;
    std::vector<std::pair<uint256, bytesmap>> vKv; // v1: root, v2: inc kv

protected:
    template <typename O>
    void Serialize(hnbase::CStream& s, O& opt)
    {
        s.Serialize(mapForkLastBlock, opt);
        s.Serialize(vBlockHash, opt);
        s.Serialize(vKv, opt);
    }
};

} // namespace storage
} // namespace hashahead

#endif // STORAGE_DBSTRUCT_H
