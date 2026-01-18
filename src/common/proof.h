// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COMMON_PROOF_H
#define COMMON_PROOF_H

#include "destination.h"
#include "key.h"
#include "uint256.h"
#include "util.h"

namespace hashahead
{

class CProofOfDelegate
{
    friend class hnbase::CStream;

public:
    CProofOfDelegate()
      : nWeight(0) {}

    unsigned char nWeight;
    uint256 nAgreement;
    bytes btDelegateData;

public:
    void Save(std::vector<unsigned char>& btProof) const
    {
        hnbase::CBufStream ss;
        ss << *this;
        ss.GetData(btProof);
    }
    bool Load(const std::vector<unsigned char>& btProof)
    {
        hnbase::CBufStream ss(btProof);
        try
        {
            ss >> *this;
        }
        catch (const std::exception& e)
        {
            hnbase::StdError(__PRETTY_FUNCTION__, e.what());
            return false;
        }
        return true;
    }

protected:
    template <typename O>
    void Serialize(hnbase::CStream& s, O& opt)
    {
        s.Serialize(nWeight, opt);
        s.Serialize(nAgreement, opt);
        s.Serialize(btDelegateData, opt);
    }
};

class CProofOfPiggyback
{
    friend class hnbase::CStream;

public:
    CProofOfPiggyback()
      : nWeight(0) {}

    unsigned char nWeight;
    uint256 nAgreement;
    uint256 hashRefBlock;

public:
    void Save(std::vector<unsigned char>& btProof) const
    {
        hnbase::CBufStream ss;
        ss << *this;
        ss.GetData(btProof);
    }
    bool Load(const std::vector<unsigned char>& btProof)
    {
        hnbase::CBufStream ss(btProof);
        try
        {
            ss >> *this;
        }
        catch (const std::exception& e)
        {
            hnbase::StdError(__PRETTY_FUNCTION__, e.what());
            return false;
        }
        return true;
    }

protected:
    template <typename O>
    void Serialize(hnbase::CStream& s, O& opt)
    {
        s.Serialize(nWeight, opt);
        s.Serialize(nAgreement, opt);
        s.Serialize(hashRefBlock, opt);
    }
};

class CProofOfHashWork
{
    friend class hnbase::CStream;

public:
    CProofOfHashWork()
      : nWeight(0), nAlgo(0), nBits(0), nNonce(0) {}

    unsigned char nWeight;
    uint256 nAgreement;
    unsigned char nAlgo;
    unsigned char nBits;
    CDestination destMint;
    uint64_t nNonce;

public:
    void Save(std::vector<unsigned char>& btProof) const
    {
        hnbase::CBufStream ss;
        ss << *this;
        ss.GetData(btProof);
    }
    bool Load(const std::vector<unsigned char>& btProof)
    {
        hnbase::CBufStream ss(btProof);
        try
        {
            ss >> *this;
        }
        catch (const std::exception& e)
        {
            hnbase::StdError(__PRETTY_FUNCTION__, e.what());
            return false;
        }
        return true;
    }

protected:
    template <typename O>
    void Serialize(hnbase::CStream& s, O& opt)
    {
        s.Serialize(nWeight, opt);
        s.Serialize(nAgreement, opt);
        s.Serialize(nAlgo, opt);
        s.Serialize(nBits, opt);
        s.Serialize(destMint, opt);
        s.Serialize(nNonce, opt);
    }
};

class CConsensusParam
{
public:
    CConsensusParam()
      : nPrevTime(0), nPrevHeight(0), nPrevNumber(0), nPrevMintType(0), nWaitTime(0), fPow(false), ret(false) {}

    uint256 hashPrev;
    int64 nPrevTime;
    int nPrevHeight;
    uint32 nPrevNumber;
    uint16 nPrevMintType;
    int64 nWaitTime;
    bool fPow;
    bool ret;
};

} // namespace hashahead

#endif //COMMON_PROOF_H
