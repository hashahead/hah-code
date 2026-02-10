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

uint64 CBaseUniqueId::CreateUniqueId(const uint8 ucPortType, const uint8 ucDirection, const uint8 ucType, const uint8 ucSubType)
{
    boost::unique_lock<boost::mutex> writeLock(lockCreate);

    if (uiIdCreate == 0)
    {
        srand(time(NULL));
        uiIdCreate = rand();
        if (uiIdCreate == 0)
        {
            uiIdCreate = 1;
        }
    }

    uint64 ui64UniqueId = 0;
    ((PBASE_UNIQUE_ID)&ui64UniqueId)->uiId = uiIdCreate;
    ((PBASE_UNIQUE_ID)&ui64UniqueId)->ucPortType = ucPortType;
    ((PBASE_UNIQUE_ID)&ui64UniqueId)->ucDirection = ucDirection;
    ((PBASE_UNIQUE_ID)&ui64UniqueId)->ucType = ucType;
    ((PBASE_UNIQUE_ID)&ui64UniqueId)->ucSubType = ucSubType;
    ((PBASE_UNIQUE_ID)&ui64UniqueId)->ucRand = GetTimeMillis() & 0xFF;

    if (++uiIdCreate == 0)
    {
        uiIdCreate = 1;
    }

    return ui64UniqueId;
}

} // namespace hnbase
