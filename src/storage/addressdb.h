// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_ADDRESSDB_H
#define STORAGE_ADDRESSDB_H

#include <map>

#include "block.h"
#include "dbstruct.h"
#include "destination.h"
#include "hnbase.h"
#include "transaction.h"
#include "triedb.h"
#include "uint256.h"

namespace hashahead
{
namespace storage
{

class CListAddressTrieDBWalker : public CTrieDBWalker
{
public:
    CListAddressTrieDBWalker(std::map<CDestination, CAddressContext>& mapAddressn)
      : mapAddress(mapAddressn) {}

    bool Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver) override;

public:
    std::map<CDestination, CAddressContext>& mapAddress;
};

class CListContractAddressTrieDBWalker : public CTrieDBWalker
{
public:
    CListContractAddressTrieDBWalker(std::map<CDestination, CContractAddressContext>& mapContractAddressIn)
      : mapContractAddress(mapContractAddressIn) {}

    bool Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver) override;

public:
    std::map<CDestination, CContractAddressContext>& mapContractAddress;
};

class CListTokenContractAddressTrieDBWalker : public CTrieDBWalker
{
public:
    CListTokenContractAddressTrieDBWalker(std::map<CDestination, CTokenContractAddressContext>& mapTokenContractAddressIn)
      : mapTokenContractAddress(mapTokenContractAddressIn) {}

    bool Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver) override;

public:
    std::map<CDestination, CTokenContractAddressContext>& mapTokenContractAddress;
};

class CListFunctionAddressTrieDBWalker : public CTrieDBWalker
{
public:
    CListFunctionAddressTrieDBWalker(std::map<uint32, CFunctionAddressContext>& mapFunctionAddressIn)
      : mapFunctionAddress(mapFunctionAddressIn) {}

    bool Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver) override;

public:
    std::map<uint32, CFunctionAddressContext>& mapFunctionAddress;
};

class CListAddressCreateCodeTrieDBWalker : public CTrieDBWalker
{
public:
    CListAddressCreateCodeTrieDBWalker(std::map<uint256, CContractCreateCodeContext>& mapWasmCreateCodeIn)
      : mapContractCreateCode(mapWasmCreateCodeIn) {}

    bool Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver) override;

public:
    std::map<uint256, CContractCreateCodeContext>& mapContractCreateCode;
};

class CCacheAddressData
{
public:
    CCacheAddressData() {}

public:
    bool AddBlockAddress(const uint256& hashBlock, const std::map<CDestination, CAddressContext>& mapAddress);
    bool AddBlockContractAddress(const uint256& hashBlock, const std::map<CDestination, CContractAddressContext>& mapContractAddress);
    bool AddBlockTokenContractAddress(const uint256& hashBlock, const std::map<CDestination, CTokenContractAddressContext>& mapTokenContractAddress);

    bool GetBlockAddress(const uint256& hashBlock, std::map<CDestination, CAddressContext>& mapAddress);
    bool GetBlockContractAddress(const uint256& hashBlock, std::map<CDestination, CContractAddressContext>& mapContractAddress);
    bool GetBlockTokenContractAddress(const uint256& hashBlock, std::map<CDestination, CTokenContractAddressContext>& mapTokenContractAddress);

protected:
    hnbase::CRWAccess rwAccess;
    std::map<uint256, std::map<CDestination, CAddressContext>, CustomBlockHashCompare> mapBlockAddress;
    std::map<uint256, std::map<CDestination, CContractAddressContext>, CustomBlockHashCompare> mapBlockContractAddress;
    std::map<uint256, std::map<CDestination, CTokenContractAddressContext>, CustomBlockHashCompare> mapBlockTokenContractAddress;
};

class CForkAddressDB
{
public:
    CForkAddressDB();
    ~CForkAddressDB();

    bool Initialize(const uint256& hashForkIn, const boost::filesystem::path& pathData, const bool fNeedPrune);
    void Deinitialize();
    bool RemoveAll();

    bool AddAddressContext(const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDestination, CAddressContext>& mapAddress, const uint64 nNewAddressCount,
                           const std::map<CDestination, CTimeVault>& mapTimeVault, const std::map<uint32, CFunctionAddressContext>& mapFunctionAddress,
                           const std::map<CDestination, uint384>& mapBlsPubkeyContext, uint256& hashNewRoot);
    bool AddTokenContractAddressContext(const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDestination, CTokenContractAddressContext>& mapTokenContractAddressContext, const bool fAll);
    bool RetrieveAddressContext(const uint256& hashBlock, const CDestination& dest, CAddressContext& ctxAddress);
    bool RetrieveTokenContractAddressContext(const uint256& hashBlock, const CDestination& dest, CTokenContractAddressContext& ctxAddress);
    bool ListAddress(const uint256& hashBlock, std::map<CDestination, CAddressContext>& mapAddress);
    bool ListContractAddress(const uint256& hashBlock, std::map<CDestination, CContractAddressContext>& mapContractAddress);
    bool ListTokenContractAddress(const uint256& hashBlock, std::map<CDestination, CTokenContractAddressContext>& mapTokenContractAddress);
    bool GetAddressCount(const uint256& hashBlock, uint64& nAddressCount, uint64& nNewAddressCount);
    bool ListFunctionAddress(const uint256& hashBlock, std::map<uint32, CFunctionAddressContext>& mapFunctionAddress);
    bool RetrieveFunctionAddress(const uint256& hashBlock, const uint32 nFuncId, CFunctionAddressContext& ctxFuncAddress);
    bool RetrieveBlsPubkeyContext(const uint256& hashBlock, const CDestination& dest, uint384& blsPubkey);
    bool GetOwnerLinkTemplateAddress(const uint256& hashBlock, const CDestination& destOwner, std::map<CDestination, uint8>& mapTemplateAddress);
    bool GetDelegateLinkTemplateAddress(const uint256& hashBlock, const CDestination& destDelegate, const uint32 nTemplateType, const uint64 nBegin, const uint64 nCount, std::vector<std::pair<CDestination, uint8>>& vTemplateAddress);
    bool VerifyAddressContext(const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode = true);

    bool AddCodeContext(const uint256& hashPrevBlock, const uint256& hashBlock,
                        const std::map<uint256, CContractSourceCodeContext>& mapSourceCode,
                        const std::map<uint256, CContractCreateCodeContext>& mapContractCreateCode,
                        const std::map<uint256, CContractRunCodeContext>& mapContractRunCode,
                        const std::map<uint256, CTemplateContext>& mapTemplateData,
                        uint256& hashCodeRoot);
    bool RetrieveSourceCodeContext(const uint256& hashBlock, const uint256& hashSourceCode, CContractSourceCodeContext& ctxtCode);
    bool RetrieveContractCreateCodeContext(const uint256& hashBlock, const uint256& hashContractCreateCode, CContractCreateCodeContext& ctxtCode);
    bool ListContractCreateCodeContext(const uint256& hashBlock, std::map<uint256, CContractCreateCodeContext>& mapContractCreateCode);
    bool VerifyCodeContext(const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode = true);

    bool ClearAddressUnavailableNode(const uint32 nClearRefHeight);
    bool GetSnapshotAddressData(const std::vector<uint256>& vBlockHash, CForkAddressRootKv& addressRootKv);
    bool RecoveryAddressData(const CForkAddressRootKv& addressRootKv);

protected:
    bool WriteTrieRoot(const uint8 nRootType, const uint256& hashBlock, const uint256& hashTrieRoot);
    bool ReadTrieRoot(const uint8 nRootType, const uint256& hashBlock, uint256& hashTrieRoot);
    bool RemoveTrieRoot(const uint8 nRootType, const uint256& hashBlock);
    void AddPrevRoot(const uint8 nRootType, const uint256& hashPrevRoot, const uint256& hashBlock, bytesmap& mapKv);
    void AddPrevRoot(const uint256& hashPrevRoot, const uint256& hashBlock, bytesmap& mapKv);
    bool GetPrevRoot(const uint256& hashRoot, uint256& hashPrevRoot, uint256& hashBlock);
    bool AddAddressCount(const uint256& hashBlock, const uint64 nAddressCount, const uint64 nNewAddressCount);
    bool ListContractAddressDb(const uint256& hashBlock, std::map<CDestination, CContractAddressContext>& mapContractAddress);

protected:
    enum
    {
        MAX_CACHE_COUNT = 16
    };
    bool fCache;
    uint256 hashFork;
    CTrieDB dbTrie;
};

class CAddressDB
{
public:
    CAddressDB(const bool fCacheIn = true)
      : fCache(fCacheIn) {}
    bool Initialize(const boost::filesystem::path& pathData);
    void Deinitialize();

    bool ExistFork(const uint256& hashFork);
    bool LoadFork(const uint256& hashFork);
    void RemoveFork(const uint256& hashFork);
    bool AddNewFork(const uint256& hashFork);
    void Clear();

    bool AddAddressContext(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDestination, CAddressContext>& mapAddress, const uint64 nNewAddressCount,
                           const std::map<CDestination, CTimeVault>& mapTimeVault, const std::map<uint32, CFunctionAddressContext>& mapFunctionAddress, uint256& hashNewRoot);
    bool RetrieveAddressContext(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, CAddressContext& ctxAddress);
    bool ListContractAddress(const uint256& hashFork, const uint256& hashBlock, std::map<CDestination, CContractAddressContext>& mapContractAddress);
    bool RetrieveTimeVault(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, CTimeVault& tv);
    bool GetAddressCount(const uint256& hashFork, const uint256& hashBlock, uint64& nAddressCount, uint64& nNewAddressCount);
    bool ListFunctionAddress(const uint256& hashFork, const uint256& hashBlock, std::map<uint32, CFunctionAddressContext>& mapFunctionAddress);
    bool RetrieveFunctionAddress(const uint256& hashFork, const uint256& hashBlock, const uint32 nFuncId, CFunctionAddressContext& ctxFuncAddress);

    bool CheckAddressContext(const uint256& hashFork, const std::vector<uint256>& vCheckBlock);
    bool CheckAddressContext(const uint256& hashFork, const uint256& hashLastBlock, const uint256& hashLastRoot);
    bool VerifyAddressContext(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode = true);

protected:
    bool fCache;
    boost::filesystem::path pathAddress;
    hnbase::CRWAccess rwAccess;
    std::map<uint256, std::shared_ptr<CForkAddressDB>> mapAddressDB;
};

} // namespace storage
} // namespace hashahead

#endif // STORAGE_ADDRESSDB_H
