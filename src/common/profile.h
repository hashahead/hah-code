// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COMMON_PROFILE_H
#define COMMON_PROFILE_H

#include <stream/stream.h>
#include <string>
#include <vector>

#include "destination.h"

namespace hashahead
{

class CProfile
{
    friend class hnbase::CStream;

public:
    int nVersion;
    uint8 nType;
    std::string strName;
    std::string strSymbol;
    CChainId nChainId;
    uint256 nAmount;
    uint256 nMintReward;
    uint256 nMinTxFee;
    uint32 nHalveCycle;
    CDestination destOwner;
    uint256 hashParent;
    int nJointHeight;
    uint8 nAttachExtdataType;

public:
    enum
    {
        PROFILE_FORK_TYPE_NON = 0,

        PROFILE_FORK_TYPE_MAIN = 1,
        PROFILE_FORK_TYPE_CLONEMAP = 2,
        PROFILE_FORK_TYPE_USER = 3,

        PROFILE_FORK_TYPE_MAX
    };
    enum
    {
        PROFILE_ATTACH_EXTD_TYPE_COMMON = 0,

        PROFILE_ATTACH_EXTD_TYPE_BTCBRC20 = 1,

        PROFILE_ATTACH_EXTD_TYPE_MAX
    };

    CProfile()
    {
        SetNull();
    }
    virtual void SetNull()
    {
        nVersion = 1;
        nType = PROFILE_FORK_TYPE_NON;
        nAmount = 0;
        nMintReward = 0;
        nMinTxFee = 0;
        nHalveCycle = 0;
        hashParent = 0;
        nJointHeight = -1;
        destOwner.SetNull();
        strName.clear();
        strSymbol.clear();
        nChainId = 0;
        nAttachExtdataType = PROFILE_ATTACH_EXTD_TYPE_COMMON;
    }
    bool IsNull() const
    {
        return (nType == PROFILE_FORK_TYPE_NON);
    }
    bool IsMainFork() const
    {
        return (nType == PROFILE_FORK_TYPE_MAIN);
    }
    bool IsClonemapFork() const
    {
        return (nType == PROFILE_FORK_TYPE_CLONEMAP);
    }
    bool IsUserFork() const
    {
        return (nType == PROFILE_FORK_TYPE_USER);
    }
    bool IsAttachExtdataBtcbrc20() const
    {
        return (nAttachExtdataType == PROFILE_ATTACH_EXTD_TYPE_BTCBRC20);
    }

    bool Save(std::vector<unsigned char>& vchProfile) const;
    bool Load(const std::vector<unsigned char>& vchProfile);

    static bool VerifyType(const uint8 nTypeIn)
    {
        if (nTypeIn == PROFILE_FORK_TYPE_NON || nTypeIn >= PROFILE_FORK_TYPE_MAX)
        {
            return false;
        }
        return true;
    }
    static bool VerifySubforkType(const uint8 nTypeIn)
    {
        if (nTypeIn <= PROFILE_FORK_TYPE_MAIN || nTypeIn >= PROFILE_FORK_TYPE_MAX)
        {
            return false;
        }
        return true;
    }
    static bool VerifyAttachExtdataType(const uint8 nAeTypeIn)
    {
        if (nAeTypeIn >= PROFILE_ATTACH_EXTD_TYPE_MAX)
        {
            return false;
        }
        return true;
    }
    static std::string GetTypeStr(const uint8 nTypeIn)
    {
        switch (nTypeIn)
        {
        case PROFILE_FORK_TYPE_MAIN:
            return "main";
        case PROFILE_FORK_TYPE_CLONEMAP:
            return "clonemap";
        case PROFILE_FORK_TYPE_USER:
            return "user";
        }
        return "non";
    }
    static uint8 ParseTypeStr(const std::string& strTypeIn)
    {
        if (strTypeIn == "main")
        {
            return PROFILE_FORK_TYPE_MAIN;
        }
        else if (strTypeIn == "clonemap")
        {
            return PROFILE_FORK_TYPE_CLONEMAP;
        }
        else if (strTypeIn == "user")
        {
            return PROFILE_FORK_TYPE_USER;
        }
        return 0;
    }
    static std::string GetAttachExtdataTypeStr(const uint8 nAeTypeIn)
    {
        switch (nAeTypeIn)
        {
        case PROFILE_ATTACH_EXTD_TYPE_COMMON:
            return "common";
        case PROFILE_ATTACH_EXTD_TYPE_BTCBRC20:
            return "btcbrc20";
        }
        return "non";
    }

protected:
    void Serialize(hnbase::CStream& s, hnbase::SaveType&) const;
    void Serialize(hnbase::CStream& s, hnbase::LoadType&);
    void Serialize(hnbase::CStream& s, std::size_t& serSize) const;
};

} // namespace hashahead

#endif //COMMON_PROFILE_H
