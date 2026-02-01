// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "addressdb.h"

#include <boost/range/adaptor/reversed.hpp>

#include "leveldbeng.h"

#include "template/delegate.h"
#include "template/fork.h"
#include "template/pledge.h"
#include "template/poa.h"
#include "template/template.h"
#include "template/vote.h"

using namespace std;
using namespace hnbase;

namespace hashahead
{
namespace storage
{

const uint8 DB_ADDRESS_KEY_TYPE_TRIEROOT = 0x11;
const uint8 DB_ADDRESS_KEY_TYPE_PREVROOT = 0x12;
const uint8 DB_ADDRESS_KEY_TYPE_CONTRACT_ADDRESS_TRIEROOT = 0x13;
const uint8 DB_ADDRESS_KEY_TYPE_ROOT_TYPE_ADDRESS = 0x14;
const uint8 DB_ADDRESS_KEY_TYPE_ROOT_TYPE_CODE = 0x15;

const uint8 DB_ADDRESS_KEY_TYPE_ADDRESS = 0x21;
const uint8 DB_ADDRESS_KEY_TYPE_TIMEVAULT = 0x22;
const uint8 DB_ADDRESS_KEY_TYPE_ADDRESSCOUNT = 0x23;
const uint8 DB_ADDRESS_KEY_TYPE_FUNCTION_ADDRESS = 0x24;
const uint8 DB_ADDRESS_KEY_TYPE_BLSPUBKEY = 0x25;
const uint8 DB_ADDRESS_KEY_TYPE_OWNER_LINK_ADDRESS = 0x26;
const uint8 DB_ADDRESS_KEY_TYPE_DELEGATE_LINK_ADDRESS = 0x27;
const uint8 DB_ADDRESS_KEY_TYPE_TOKEN_CONTRACT_ADDRESS = 0x28;

const uint8 DB_ADDRESS_KEY_TYPE_SOURCE_CODE = 0x31;
const uint8 DB_ADDRESS_KEY_TYPE_CONTRACT_CREATE_CODE = 0x32;

#define DB_ADDRESS_KEY_ID_PREVROOT string("prevroot")
#define DB_ADDRESS_KEY_ID_TOKEN_PREVROOT string("tokenprevroot")

#define MAX_CACHE_ADDRESS_BLOCK_COUNT 256

//////////////////////////////
// CListAddressTrieDBWalker

bool CListAddressTrieDBWalker::Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver)
{
    if (btKey.size() == 0 || btValue.size() == 0)
    {
        StdError("CListAddressTrieDBWalker", "btKey.size() = %ld, btValue.size() = %ld", btKey.size(), btValue.size());
        return false;
    }

    try
    {
        hnbase::CBufStream ssKey(btKey);
        uint8 nKeyType;
        ssKey >> nKeyType;
        if (nKeyType == DB_ADDRESS_KEY_TYPE_ADDRESS)
        {
            CDestination dest;
            CAddressContext ctxAddress;
            hnbase::CBufStream ssValue(btValue);
            ssKey >> dest;
            ssValue >> ctxAddress;
            mapAddress.insert(std::make_pair(dest, ctxAddress));
        }
    }
    catch (std::exception& e)
    {
        hnbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

//////////////////////////////
// CListContractAddressTrieDBWalker

bool CListContractAddressTrieDBWalker::Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver)
{
    if (btKey.size() == 0 || btValue.size() == 0)
    {
        StdError("CListContractAddressTrieDBWalker", "btKey.size() = %ld, btValue.size() = %ld", btKey.size(), btValue.size());
        return false;
    }

    try
    {
        hnbase::CBufStream ssKey(btKey);
        uint8 nKeyType;
        ssKey >> nKeyType;
        if (nKeyType == DB_ADDRESS_KEY_TYPE_ADDRESS)
        {
            CDestination dest;
            CAddressContext ctxAddress;
            hnbase::CBufStream ssValue(btValue);
            ssKey >> dest;
            ssValue >> ctxAddress;
            if (ctxAddress.IsContract())
            {
                CContractAddressContext ctxContract;
                if (!ctxAddress.GetContractAddressContext(ctxContract))
                {
                    StdError("CListContractAddressTrieDBWalker", "GetContractAddressContext fail");
                    return false;
                }
                mapContractAddress.insert(std::make_pair(dest, ctxContract));
            }
        }
    }
    catch (std::exception& e)
    {
        hnbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

//////////////////////////////
// CListTokenContractAddressTrieDBWalker

bool CListTokenContractAddressTrieDBWalker::Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver)
{
    if (btKey.size() == 0 || btValue.size() == 0)
    {
        StdError("CListTokenContractAddressTrieDBWalker", "btKey.size() = %ld, btValue.size() = %ld", btKey.size(), btValue.size());
        return false;
    }

    try
    {
        hnbase::CBufStream ssKey(btKey);
        uint8 nKeyType;
        ssKey >> nKeyType;
        if (nKeyType == DB_ADDRESS_KEY_TYPE_TOKEN_CONTRACT_ADDRESS)
        {
            CDestination dest;
            CTokenContractAddressContext ctxTokenContractAddress;
            hnbase::CBufStream ssValue(btValue);
            ssKey >> dest;
            ssValue >> ctxTokenContractAddress;
            mapTokenContractAddress.insert(std::make_pair(dest, ctxTokenContractAddress));
        }
    }
    catch (std::exception& e)
    {
        hnbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

//////////////////////////////
// CListFunctionAddressTrieDBWalker

bool CListFunctionAddressTrieDBWalker::Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver)
{
    if (btKey.size() == 0 || btValue.size() == 0)
    {
        StdError("CListFunctionAddressTrieDBWalker", "btKey.size() = %ld, btValue.size() = %ld", btKey.size(), btValue.size());
        return false;
    }

    try
    {
        hnbase::CBufStream ssKey(btKey);
        uint8 nKeyType;
        ssKey >> nKeyType;
        if (nKeyType == DB_ADDRESS_KEY_TYPE_FUNCTION_ADDRESS)
        {
            uint32 nFuncId;
            CFunctionAddressContext ctxFuncAddress;
            hnbase::CBufStream ssValue(btValue);
            ssKey >> nFuncId;
            ssValue >> ctxFuncAddress;
            mapFunctionAddress.insert(std::make_pair(nFuncId, ctxFuncAddress));
        }
    }
    catch (std::exception& e)
    {
        hnbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

//////////////////////////////
// CListAddressCreateCodeTrieDBWalker

bool CListAddressCreateCodeTrieDBWalker::Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver)
{
    if (btKey.size() == 0 || btValue.size() == 0)
    {
        StdError("CListAddressCreateCodeTrieDBWalker", "btKey.size() = %ld, btValue.size() = %ld", btKey.size(), btValue.size());
        return false;
    }

    try
    {
        hnbase::CBufStream ssKey(btKey);
        uint8 nKeyType;
        ssKey >> nKeyType;
        if (nKeyType == DB_ADDRESS_KEY_TYPE_CONTRACT_CREATE_CODE)
        {
            uint256 hashCreateCode;
            CContractCreateCodeContext ctxtCreateCode;
            hnbase::CBufStream ssValue(btValue);
            ssKey >> hashCreateCode;
            ssValue >> ctxtCreateCode;
            mapContractCreateCode.insert(std::make_pair(hashCreateCode, ctxtCreateCode));
        }
    }
    catch (std::exception& e)
    {
        hnbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

//////////////////////////////
// CCacheAddressData

bool CCacheAddressData::AddBlockAddress(const uint256& hashBlock, const std::map<CDestination, CAddressContext>& mapAddress)
{
    CWriteLock wlock(rwAccess);

    if (mapBlockAddress.size() > MAX_CACHE_ADDRESS_BLOCK_COUNT)
    {
        mapBlockAddress.erase(mapBlockAddress.begin());
    }
    mapBlockAddress[hashBlock] = mapAddress;
    return true;
}

bool CCacheAddressData::AddBlockContractAddress(const uint256& hashBlock, const std::map<CDestination, CContractAddressContext>& mapContractAddress)
{
    CWriteLock wlock(rwAccess);

    if (mapBlockContractAddress.size() > MAX_CACHE_ADDRESS_BLOCK_COUNT)
    {
        mapBlockContractAddress.erase(mapBlockContractAddress.begin());
    }
    mapBlockContractAddress[hashBlock] = mapContractAddress;
    return true;
}

bool CCacheAddressData::AddBlockTokenContractAddress(const uint256& hashBlock, const std::map<CDestination, CTokenContractAddressContext>& mapTokenContractAddress)
{
    CWriteLock wlock(rwAccess);

    if (mapBlockTokenContractAddress.size() > MAX_CACHE_ADDRESS_BLOCK_COUNT)
    {
        mapBlockTokenContractAddress.erase(mapBlockTokenContractAddress.begin());
    }
    mapBlockTokenContractAddress[hashBlock] = mapTokenContractAddress;
    return true;
}

bool CCacheAddressData::GetBlockAddress(const uint256& hashBlock, std::map<CDestination, CAddressContext>& mapAddress)
{
    CReadLock rlock(rwAccess);

    auto it = mapBlockAddress.find(hashBlock);
    if (it != mapBlockAddress.end())
    {
        mapAddress = it->second;
        return true;
    }
    return false;
}

bool CCacheAddressData::GetBlockContractAddress(const uint256& hashBlock, std::map<CDestination, CContractAddressContext>& mapContractAddress)
{
    CReadLock rlock(rwAccess);

    auto it = mapBlockContractAddress.find(hashBlock);
    if (it != mapBlockContractAddress.end())
    {
        mapContractAddress = it->second;
        return true;
    }
    return false;
}

bool CCacheAddressData::GetBlockTokenContractAddress(const uint256& hashBlock, std::map<CDestination, CTokenContractAddressContext>& mapTokenContractAddress)
{
    CReadLock rlock(rwAccess);

    auto it = mapBlockTokenContractAddress.find(hashBlock);
    if (it != mapBlockTokenContractAddress.end())
    {
        mapTokenContractAddress = it->second;
        return true;
    }
    return false;
}

//////////////////////////////
// CForkAddressDB

CForkAddressDB::CForkAddressDB()
{
    fPrune = false;
}

CForkAddressDB::~CForkAddressDB()
{
    dbTrie.Deinitialize();
}

bool CForkAddressDB::Initialize(const uint256& hashForkIn, const boost::filesystem::path& pathData, const bool fNeedPrune)
{
    if (!dbTrie.Initialize(pathData))
    {
        StdLog("CForkAddressDB", "Initialize: Initialize fail, fork: %s", hashForkIn.GetHex().c_str());
        return false;
    }
    hashFork = hashForkIn;
    fPrune = fNeedPrune;
    return true;
}

void CForkAddressDB::Deinitialize()
{
    dbTrie.Deinitialize();
}

bool CForkAddressDB::RemoveAll()
{
    dbTrie.RemoveAll();
    return true;
}

bool CForkAddressDB::AddAddressContext(const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDestination, CAddressContext>& mapAddress, const uint64 nNewAddressCount,
                                       const std::map<CDestination, CTimeVault>& mapTimeVault, const std::map<uint32, CFunctionAddressContext>& mapFunctionAddress,
                                       const std::map<CDestination, uint384>& mapBlsPubkeyContext, uint256& hashNewRoot)
{
    uint256 hashPrevRoot;
    if (hashBlock != hashFork)
    {
        if (!ReadTrieRoot(DB_ADDRESS_KEY_TYPE_ROOT_TYPE_ADDRESS, hashPrevBlock, hashPrevRoot))
        {
            StdLog("CForkAddressDB", "Add Address Context: Read trie root fail, prev block: %s, block: %s",
                   hashPrevBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
            return false;
        }
    }

    std::map<CDestination, std::map<CDestination, uint8>> mapOwnerLinkTemplate;    // key1: owner address, key2: template address, value: template type
    std::map<CDestination, std::map<CDestination, uint8>> mapDelegateLinkTemplate; // key1: owner address, key2: template address, value: template type
    bytesmap mapKv;

    for (const auto& kv : mapAddress)
    {
        hnbase::CBufStream ssKey, ssValue;
        bytes btKey, btValue;

        ssKey << DB_ADDRESS_KEY_TYPE_ADDRESS << kv.first;
        ssKey.GetData(btKey);

        ssValue << kv.second;
        ssValue.GetData(btValue);

        mapKv.insert(make_pair(btKey, btValue));

        if (kv.second.IsTemplate())
        {
            AddOwnerAddressLinkTemplate(hashPrevBlock, kv.first, kv.second, mapOwnerLinkTemplate, mapKv);
            AddDelegateAddressLinkTemplate(hashPrevBlock, kv.first, kv.second, mapDelegateLinkTemplate, mapKv);
        }
    }
    for (const auto& kv : mapTimeVault)
    {
        hnbase::CBufStream ssKey, ssValue;
        bytes btKey, btValue;

        ssKey << DB_ADDRESS_KEY_TYPE_TIMEVAULT << kv.first;
        ssKey.GetData(btKey);

        ssValue << kv.second;
        ssValue.GetData(btValue);

        mapKv.insert(make_pair(btKey, btValue));
    }
    for (const auto& kv : mapFunctionAddress)
    {
        hnbase::CBufStream ssKey, ssValue;
        bytes btKey, btValue;

        ssKey << DB_ADDRESS_KEY_TYPE_FUNCTION_ADDRESS << kv.first;
        ssKey.GetData(btKey);

        ssValue << kv.second;
        ssValue.GetData(btValue);

        mapKv.insert(make_pair(btKey, btValue));
    }
    for (const auto& kv : mapBlsPubkeyContext)
    {
        hnbase::CBufStream ssKey, ssValue;
        bytes btKey, btValue;

        ssKey << DB_ADDRESS_KEY_TYPE_BLSPUBKEY << kv.first;
        ssKey.GetData(btKey);

        ssValue << kv.second;
        ssValue.GetData(btValue);

        mapKv.insert(make_pair(btKey, btValue));
    }
    if (hashPrevRoot == 0 && mapKv.empty())
    {
        AddPrevRoot(DB_ADDRESS_KEY_TYPE_ROOT_TYPE_ADDRESS, hashPrevRoot, hashBlock, mapKv);
    }

    if (!dbTrie.AddNewTrie(hashPrevRoot, mapKv, hashNewRoot))
    {
        StdLog("CForkAddressDB", "Add Address Context: Add new trie fail, prev block: %s, block: %s",
               hashPrevBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }

    uint64 nPrevAddressCount = 0;
    if (hashBlock != hashFork)
    {
        uint64 nPrevNewAddressCount = 0;
        if (!GetAddressCount(hashPrevBlock, nPrevAddressCount, nPrevNewAddressCount))
        {
            StdLog("CForkAddressDB", "Add Address Context: Get prev address count fail, prev block: %s, block: %s",
                   hashPrevBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
            return false;
        }
    }
    if (!AddAddressCount(hashBlock, nPrevAddressCount + nNewAddressCount, nNewAddressCount))
    {
        StdLog("CForkAddressDB", "Add Address Context: Add address count fail, prev block: %s, block: %s",
               hashPrevBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }

    if (!WriteTrieRoot(DB_ADDRESS_KEY_TYPE_ROOT_TYPE_ADDRESS, hashBlock, hashNewRoot))
    {
        StdLog("CForkAddressDB", "Add Address Context: Write trie root fail, prev block: %s, block: %s",
               hashPrevBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }

    AddCacheAddressData(hashPrevBlock, hashBlock, mapAddress);
    return true;
}

bool CForkAddressDB::AddTokenContractAddressContext(const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDestination, CTokenContractAddressContext>& mapTokenContractAddressContext, const bool fAll)
{
    uint256 hashPrevRoot;
    if (!fAll)
    {
        if (hashBlock != hashFork)
        {
            if (!ReadTrieRoot(DB_ADDRESS_KEY_TYPE_CONTRACT_ADDRESS_TRIEROOT, hashPrevBlock, hashPrevRoot))
            {
                StdLog("CForkAddressDB", "Add token constract address context: Read prev root fail, prev block: %s, block: %s",
                       hashPrevBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
                return false;
            }
        }
    }

    bytesmap mapKv;
    for (const auto& kv : mapTokenContractAddressContext)
    {
        hnbase::CBufStream ssKey, ssValue;
        bytes btKey, btValue;

        ssKey << DB_ADDRESS_KEY_TYPE_TOKEN_CONTRACT_ADDRESS << kv.first;
        ssKey.GetData(btKey);

        ssValue << kv.second;
        ssValue.GetData(btValue);

        mapKv.insert(make_pair(btKey, btValue));
    }
    if (hashPrevRoot == 0 && mapKv.empty())
    {
        AddTokenPrevRoot(hashPrevBlock, hashBlock, mapKv);
    }

    uint256 hashNewRoot;
    if (!dbTrie.AddNewTrie(hashPrevRoot, mapKv, hashNewRoot))
    {
        StdLog("CForkAddressDB", "Add token constract address context: Add new trie fail, prev block: %s, block: %s",
               hashPrevBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }

    if (!WriteTrieRoot(DB_ADDRESS_KEY_TYPE_CONTRACT_ADDRESS_TRIEROOT, hashBlock, hashNewRoot))
    {
        StdLog("CForkAddressDB", "Add token constract address context: Write trie root fail, prev block: %s, block: %s",
               hashPrevBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }

    AddCacheTokenContractAddressData(hashPrevBlock, hashBlock, mapTokenContractAddressContext, fAll);
    return true;
}

bool CForkAddressDB::RetrieveAddressContext(const uint256& hashBlock, const CDestination& dest, CAddressContext& ctxAddress)
{
    uint256 hashRoot;
    if (!ReadTrieRoot(DB_ADDRESS_KEY_TYPE_ROOT_TYPE_ADDRESS, hashBlock, hashRoot))
    {
        StdLog("CForkAddressDB", "Retrieve address context: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    hnbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_ADDRESS_KEY_TYPE_ADDRESS << dest;
    ssKey.GetData(btKey);
    if (!dbTrie.Retrieve(hashRoot, btKey, btValue))
    {
        return false;
    }
    try
    {
        ssValue.Write((char*)(btValue.data()), btValue.size());
        ssValue >> ctxAddress;
    }
    catch (std::exception& e)
    {
        hnbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CForkAddressDB::RetrieveTokenContractAddressContext(const uint256& hashBlock, const CDestination& dest, CTokenContractAddressContext& ctxAddress)
{
    uint256 hashRoot;
    if (!ReadTrieRoot(DB_ADDRESS_KEY_TYPE_CONTRACT_ADDRESS_TRIEROOT, hashBlock, hashRoot))
    {
        StdLog("CForkAddressDB", "Retrieve token contract address context: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    hnbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_ADDRESS_KEY_TYPE_TOKEN_CONTRACT_ADDRESS << dest;
    ssKey.GetData(btKey);
    if (!dbTrie.Retrieve(hashRoot, btKey, btValue))
    {
        return false;
    }
    try
    {
        ssValue.Write((char*)(btValue.data()), btValue.size());
        ssValue >> ctxAddress;
    }
    catch (std::exception& e)
    {
        hnbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CForkAddressDB::ListAddress(const uint256& hashBlock, std::map<CDestination, CAddressContext>& mapAddress)
{
    if (cacheAddressData.GetBlockAddress(hashBlock, mapAddress))
    {
        return true;
    }
    return ListAddressDb(hashBlock, mapAddress);
}

bool CForkAddressDB::ListContractAddress(const uint256& hashBlock, std::map<CDestination, CContractAddressContext>& mapContractAddress)
{
    if (cacheAddressData.GetBlockContractAddress(hashBlock, mapContractAddress))
    {
        return true;
    }
    return ListContractAddressDb(hashBlock, mapContractAddress);
}

bool CForkAddressDB::ListTokenContractAddress(const uint256& hashBlock, std::map<CDestination, CTokenContractAddressContext>& mapTokenContractAddress)
{
    if (cacheAddressData.GetBlockTokenContractAddress(hashBlock, mapTokenContractAddress))
    {
        return true;
    }
    return ListTokenContractAddressDb(hashBlock, mapTokenContractAddress);
}

bool CForkAddressDB::GetAddressCount(const uint256& hashBlock, uint64& nAddressCount, uint64& nNewAddressCount)
{
    if (hashBlock == 0)
    {
        nAddressCount = 0;
        return true;
    }

    hnbase::CBufStream ssKey, ssValue;
    ssKey << DB_ADDRESS_KEY_TYPE_ADDRESSCOUNT << hashBlock;
    if (!dbTrie.ReadExtKv(ssKey, ssValue))
    {
        return false;
    }

    try
    {
        ssValue >> nAddressCount >> nNewAddressCount;
    }
    catch (std::exception& e)
    {
        hnbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CForkAddressDB::ListFunctionAddress(const uint256& hashBlock, std::map<uint32, CFunctionAddressContext>& mapFunctionAddress)
{
    uint256 hashRoot;
    if (!ReadTrieRoot(DB_ADDRESS_KEY_TYPE_ROOT_TYPE_ADDRESS, hashBlock, hashRoot))
    {
        return false;
    }

    hnbase::CBufStream ssKeyPrefix;
    ssKeyPrefix << DB_ADDRESS_KEY_TYPE_FUNCTION_ADDRESS;
    bytes btKeyPrefix;
    ssKeyPrefix.GetData(btKeyPrefix);

    CListFunctionAddressTrieDBWalker walker(mapFunctionAddress);
    if (!dbTrie.WalkThroughTrie(hashRoot, walker, btKeyPrefix))
    {
        StdLog("CForkAddressDB", "List function address: Walk through trie fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CForkAddressDB::RetrieveFunctionAddress(const uint256& hashBlock, const uint32 nFuncId, CFunctionAddressContext& ctxFuncAddress)
{
    uint256 hashRoot;
    if (!ReadTrieRoot(DB_ADDRESS_KEY_TYPE_ROOT_TYPE_ADDRESS, hashBlock, hashRoot))
    {
        //StdLog("CForkAddressDB", "Retrieve function address: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    hnbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_ADDRESS_KEY_TYPE_FUNCTION_ADDRESS << nFuncId;
    ssKey.GetData(btKey);
    if (!dbTrie.Retrieve(hashRoot, btKey, btValue))
    {
        return false;
    }
    try
    {
        ssValue.Write((char*)(btValue.data()), btValue.size());
        ssValue >> ctxFuncAddress;
    }
    catch (std::exception& e)
    {
        hnbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CForkAddressDB::RetrieveBlsPubkeyContext(const uint256& hashBlock, const CDestination& dest, uint384& blsPubkey)
{
    uint256 hashRoot;
    if (!ReadTrieRoot(DB_ADDRESS_KEY_TYPE_ROOT_TYPE_ADDRESS, hashBlock, hashRoot))
    {
        StdLog("CForkAddressDB", "Retrieve bls pubkey context: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    hnbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_ADDRESS_KEY_TYPE_BLSPUBKEY << dest;
    ssKey.GetData(btKey);
    if (!dbTrie.Retrieve(hashRoot, btKey, btValue))
    {
        return false;
    }
    try
    {
        ssValue.Write((char*)(btValue.data()), btValue.size());
        ssValue >> blsPubkey;
    }
    catch (std::exception& e)
    {
        hnbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CForkAddressDB::GetOwnerLinkTemplateAddress(const uint256& hashBlock, const CDestination& destOwner, std::map<CDestination, uint8>& mapTemplateAddress)
{
    class CListTrieDBWalker : public CTrieDBWalker
    {
    public:
        CListTrieDBWalker(std::map<CDestination, uint8>& mapTemplateAddressIn)
          : mapTemplateAddress(mapTemplateAddressIn) {}

        bool Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver)
        {
            if (btKey.size() == 0 || btValue.size() == 0)
            {
                StdError("CListTrieDBWalker", "btKey.size() = %ld, btValue.size() = %ld", btKey.size(), btValue.size());
                return false;
            }

            try
            {
                hnbase::CBufStream ssKey(btKey);
                uint8 nKeyType;
                ssKey >> nKeyType;
                if (nKeyType == DB_ADDRESS_KEY_TYPE_OWNER_LINK_ADDRESS)
                {
                    CDestination destOwner;
                    CDestination destTemplate;
                    uint8 nTemplateType;

                    ssKey >> destOwner >> destTemplate;

                    hnbase::CBufStream ssValue(btValue);
                    ssValue >> nTemplateType;

                    mapTemplateAddress.insert(std::make_pair(destTemplate, nTemplateType));
                }
            }
            catch (std::exception& e)
            {
                hnbase::StdError(__PRETTY_FUNCTION__, e.what());
                return false;
            }
            return true;
        }

    public:
        std::map<CDestination, uint8>& mapTemplateAddress;
    };

    uint256 hashRoot;
    if (!ReadTrieRoot(DB_ADDRESS_KEY_TYPE_ROOT_TYPE_ADDRESS, hashBlock, hashRoot))
    {
        StdLog("CForkAddressDB", "Get owner link template address: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }

    hnbase::CBufStream ssKeyPrefix;
    ssKeyPrefix << DB_ADDRESS_KEY_TYPE_OWNER_LINK_ADDRESS << destOwner;
    bytes btKeyPrefix;
    ssKeyPrefix.GetData(btKeyPrefix);

    CListTrieDBWalker walker(mapTemplateAddress);
    if (!dbTrie.WalkThroughTrie(hashRoot, walker, btKeyPrefix))
    {
        StdLog("CForkAddressDB", "Get owner link template address: Walk through trie fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CForkAddressDB::GetDelegateLinkTemplateAddress(const uint256& hashBlock, const CDestination& destDelegate, const uint32 nTemplateType, const uint64 nBegin, const uint64 nCount, std::vector<std::pair<CDestination, uint8>>& vTemplateAddress)
{
    class CListTrieDBWalker : public CTrieDBWalker
    {
    public:
        CListTrieDBWalker(const uint32 nTemplateTypeIn, const uint64 nBeginIn, const uint64 nCountIn, std::vector<std::pair<CDestination, uint8>>& vTemplateAddressIn)
          : nTemplateType(nTemplateTypeIn), nBegin(nBeginIn), nCount(nCountIn), vTemplateAddress(vTemplateAddressIn), nWalkIndex(0) {}

        bool Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver)
        {
            if (btKey.size() == 0 || btValue.size() == 0)
            {
                StdError("CListTrieDBWalker", "btKey.size() = %ld, btValue.size() = %ld", btKey.size(), btValue.size());
                return false;
            }

            try
            {
                hnbase::CBufStream ssKey(btKey);
                uint8 nKeyType;
                ssKey >> nKeyType;
                if (nKeyType == DB_ADDRESS_KEY_TYPE_DELEGATE_LINK_ADDRESS)
                {
                    CDestination destDelegate;
                    CDestination destTemplate;
                    uint8 nTemplateTypeDb;

                    ssKey >> destDelegate >> destTemplate;

                    hnbase::CBufStream ssValue(btValue);
                    ssValue >> nTemplateTypeDb;

                    if (nTemplateType != 0 && nTemplateType != nTemplateTypeDb)
                    {
                        return true;
                    }
                    if (nWalkIndex < nBegin)
                    {
                        return true;
                    }
                    nWalkIndex++;
                    vTemplateAddress.push_back(std::make_pair(destTemplate, nTemplateTypeDb));
                    if (nCount > 0 && vTemplateAddress.size() >= nCount)
                    {
                        fWalkOver = true;
                        return true;
                    }
                }
            }
            catch (std::exception& e)
            {
                hnbase::StdError(__PRETTY_FUNCTION__, e.what());
                return false;
            }
            return true;
        }

    public:
        const uint32 nTemplateType;
        const uint64 nBegin;
        const uint64 nCount;
        std::vector<std::pair<CDestination, uint8>>& vTemplateAddress;
        uint64 nWalkIndex;
    };

    uint256 hashRoot;
    if (!ReadTrieRoot(DB_ADDRESS_KEY_TYPE_ROOT_TYPE_ADDRESS, hashBlock, hashRoot))
    {
        StdLog("CForkAddressDB", "Get delegate link template address: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }

    hnbase::CBufStream ssKeyPrefix;
    ssKeyPrefix << DB_ADDRESS_KEY_TYPE_DELEGATE_LINK_ADDRESS << destDelegate;
    bytes btKeyPrefix;
    ssKeyPrefix.GetData(btKeyPrefix);

    CListTrieDBWalker walker(nTemplateType, nBegin, nCount, vTemplateAddress);
    if (!dbTrie.WalkThroughTrie(hashRoot, walker, btKeyPrefix))
    {
        StdLog("CForkAddressDB", "Get delegate link template address: Walk through trie fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CForkAddressDB::VerifyAddressContext(const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode)
{
    if (!ReadTrieRoot(DB_ADDRESS_KEY_TYPE_ROOT_TYPE_ADDRESS, hashBlock, hashRoot))
    {
        StdLog("CForkAddressDB", "Verify address context: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }

    if (fVerifyAllNode)
    {
        std::map<uint256, CTrieValue> mapCacheNode;
        if (!dbTrie.CheckTrieNode(hashRoot, mapCacheNode))
        {
            StdLog("CForkAddressDB", "Verify address context: Check trie node fail, root: %s, block: %s", hashRoot.GetHex().c_str(), hashBlock.GetHex().c_str());
            return false;
        }
    }
    return true;
}

//////////////////////////////
// contract code

bool CForkAddressDB::AddCodeContext(const uint256& hashPrevBlock, const uint256& hashBlock,
                                    const std::map<uint256, CContractSourceCodeContext>& mapSourceCode,
                                    const std::map<uint256, CContractCreateCodeContext>& mapContractCreateCode,
                                    const std::map<uint256, CContractRunCodeContext>& mapContractRunCode,
                                    const std::map<uint256, CTemplateContext>& mapTemplateData,
                                    uint256& hashCodeRoot)
{
    uint256 hashPrevRoot;
    if (hashBlock != hashFork)
    {
        if (!ReadTrieRoot(DB_ADDRESS_KEY_TYPE_ROOT_TYPE_CODE, hashPrevBlock, hashPrevRoot))
        {
            StdLog("CForkAddressDB", "Add Code Context: Read trie root fail, hashPrevBlock: %s, hashBlock: %s",
                   hashPrevBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
            return false;
        }
    }

    bytesmap mapKv;
    for (const auto& kv : mapSourceCode)
    {
        hnbase::CBufStream ssKey, ssValue;
        bytes btKey, btValue;

        ssKey << DB_ADDRESS_KEY_TYPE_SOURCE_CODE << kv.first;
        ssKey.GetData(btKey);

        ssValue << kv.second;
        ssValue.GetData(btValue);

        mapKv.insert(make_pair(btKey, btValue));
    }
    for (const auto& kv : mapContractCreateCode)
    {
        hnbase::CBufStream ssKey, ssValue;
        bytes btKey, btValue;

        ssKey << DB_ADDRESS_KEY_TYPE_CONTRACT_CREATE_CODE << kv.first;
        ssKey.GetData(btKey);

        ssValue << kv.second;
        ssValue.GetData(btValue);

        mapKv.insert(make_pair(btKey, btValue));
    }
    if (hashPrevRoot == 0 && mapKv.empty())
    {
        AddPrevRoot(DB_ADDRESS_KEY_TYPE_ROOT_TYPE_CODE, hashPrevRoot, hashBlock, mapKv);
    }

    if (!dbTrie.AddNewTrie(hashPrevRoot, mapKv, hashCodeRoot))
    {
        StdLog("CForkAddressDB", "Add Code Context: Add new trie fail, hashPrevBlock: %s, hashBlock: %s",
               hashPrevBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }

    if (!WriteTrieRoot(DB_ADDRESS_KEY_TYPE_ROOT_TYPE_CODE, hashBlock, hashCodeRoot))
    {
        StdLog("CForkAddressDB", "Add Code Context: Write trie root fail, hashPrevBlock: %s, hashBlock: %s",
               hashPrevBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CForkAddressDB::RetrieveSourceCodeContext(const uint256& hashBlock, const uint256& hashSourceCode, CContractSourceCodeContext& ctxtCode)
{
    uint256 hashCodeRoot;
    if (!ReadTrieRoot(DB_ADDRESS_KEY_TYPE_ROOT_TYPE_CODE, hashBlock, hashCodeRoot))
    {
        return false;
    }
    hnbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_ADDRESS_KEY_TYPE_SOURCE_CODE << hashSourceCode;
    ssKey.GetData(btKey);
    if (!dbTrie.Retrieve(hashCodeRoot, btKey, btValue))
    {
        return false;
    }
    try
    {
        ssValue.Write((char*)(btValue.data()), btValue.size());
        ssValue >> ctxtCode;
    }
    catch (std::exception& e)
    {
        hnbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CForkAddressDB::RetrieveContractCreateCodeContext(const uint256& hashBlock, const uint256& hashContractCreateCode, CContractCreateCodeContext& ctxtCode)
{
    uint256 hashCodeRoot;
    if (!ReadTrieRoot(DB_ADDRESS_KEY_TYPE_ROOT_TYPE_CODE, hashBlock, hashCodeRoot))
    {
        return false;
    }
    hnbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_ADDRESS_KEY_TYPE_CONTRACT_CREATE_CODE << hashContractCreateCode;
    ssKey.GetData(btKey);
    if (!dbTrie.Retrieve(hashCodeRoot, btKey, btValue))
    {
        return false;
    }
    try
    {
        ssValue.Write((char*)(btValue.data()), btValue.size());
        ssValue >> ctxtCode;
    }
    catch (std::exception& e)
    {
        hnbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CForkAddressDB::ListContractCreateCodeContext(const uint256& hashBlock, std::map<uint256, CContractCreateCodeContext>& mapContractCreateCode)
{
    uint256 hashRoot;
    if (!ReadTrieRoot(DB_ADDRESS_KEY_TYPE_ROOT_TYPE_CODE, hashBlock, hashRoot))
    {
        StdLog("CForkAddressDB", "List wasm create code: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }

    bytes btKeyPrefix;
    hnbase::CBufStream ssKeyPrefix;
    ssKeyPrefix << DB_ADDRESS_KEY_TYPE_CONTRACT_CREATE_CODE;
    ssKeyPrefix.GetData(btKeyPrefix);

    CListAddressCreateCodeTrieDBWalker walker(mapContractCreateCode);
    if (!dbTrie.WalkThroughTrie(hashRoot, walker, btKeyPrefix))
    {
        StdLog("CForkAddressDB", "List wasm create code: Walk through trie fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CForkAddressDB::VerifyCodeContext(const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode)
{
    if (!ReadTrieRoot(DB_ADDRESS_KEY_TYPE_ROOT_TYPE_CODE, hashBlock, hashRoot))
    {
        return false;
    }

    if (fVerifyAllNode)
    {
        std::map<uint256, CTrieValue> mapCacheNode;
        if (!dbTrie.CheckTrieNode(hashRoot, mapCacheNode))
        {
            return false;
        }
    }

    uint256 hashPrevRoot;
    if (hashBlock != hashFork)
    {
        if (!ReadTrieRoot(DB_ADDRESS_KEY_TYPE_ROOT_TYPE_CODE, hashPrevBlock, hashPrevRoot))
        {
            return false;
        }
    }

    uint256 hashRootPrevDb;
    uint256 hashBlockLocalDb;
    if (!GetPrevRoot(DB_ADDRESS_KEY_TYPE_ROOT_TYPE_CODE, hashRoot, hashRootPrevDb, hashBlockLocalDb))
    {
        return false;
    }
    if (hashRootPrevDb != hashPrevRoot || hashBlockLocalDb != hashBlock)
    {
        return false;
    }
    return true;
}

/////////////////////////////
bool CForkAddressDB::ClearAddressUnavailableNode(const uint32 nClearRefHeight)
{
    if (!fPrune)
    {
        return false;
    }

    uint64 nRemoveNodeCount = 0;
    uint64 nTotalNodeCount = 0;
    uint64 nReserveNodeCount = 0;
    uint64 nReserveRootCount = 0;

    if (!ClearHeightAuxiliaryData(nClearRefHeight))
    {
        StdLog("CForkAddressDB", "Clear address unavailable node: Clear height auxiliary data failed, height: %d", nClearRefHeight);
        return false;
    }
    return true;
}

bool CForkAddressDB::GetSnapshotAddressData(const std::vector<uint256>& vBlockHash, CForkAddressRootKv& addressRootKv)
{
    return true;
}

bool CForkAddressDB::RecoveryAddressData(const CForkAddressRootKv& addressRootKv)
{
    return true;
}

///////////////////////////////////
bool CForkAddressDB::WriteTrieRoot(const uint256& hashBlock, const uint256& hashTrieRoot)
{
    hnbase::CBufStream ssKey, ssValue;
    ssKey << DB_ADDRESS_KEY_TYPE_TRIEROOT << hashBlock;
    ssValue << hashTrieRoot;
    return dbTrie.WriteExtKv(ssKey, ssValue);
}

bool CForkAddressDB::ReadTrieRoot(const uint256& hashBlock, uint256& hashTrieRoot)
{
    if (hashBlock == 0)
    {
        hashTrieRoot = 0;
        return true;
    }

    hnbase::CBufStream ssKey, ssValue;
    ssKey << DB_ADDRESS_KEY_TYPE_TRIEROOT << hashBlock;
    if (!dbTrie.ReadExtKv(ssKey, ssValue))
    {
        return false;
    }

    try
    {
        ssValue >> hashTrieRoot;
    }
    catch (std::exception& e)
    {
        hnbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

void CForkAddressDB::AddPrevRoot(const uint256& hashPrevRoot, const uint256& hashBlock, bytesmap& mapKv)
{
    hnbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;

    ssKey << DB_ADDRESS_KEY_TYPE_PREVROOT << DB_ADDRESS_KEY_ID_PREVROOT;
    ssKey.GetData(btKey);

    ssValue << hashPrevRoot << hashBlock;
    ssValue.GetData(btValue);

    mapKv.insert(make_pair(btKey, btValue));
}

bool CForkAddressDB::GetPrevRoot(const uint256& hashRoot, uint256& hashPrevRoot, uint256& hashBlock)
{
    hnbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_ADDRESS_KEY_TYPE_PREVROOT << DB_ADDRESS_KEY_ID_PREVROOT;
    ssKey.GetData(btKey);
    if (!dbTrie.Retrieve(hashRoot, btKey, btValue))
    {
        return false;
    }
    try
    {
        ssValue.Write((char*)(btValue.data()), btValue.size());
        ssValue >> hashPrevRoot >> hashBlock;
    }
    catch (std::exception& e)
    {
        hnbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CForkAddressDB::AddAddressCount(const uint256& hashBlock, const uint64 nAddressCount, const uint64 nNewAddressCount)
{
    hnbase::CBufStream ssKey, ssValue;
    ssKey << DB_ADDRESS_KEY_TYPE_ADDRESSCOUNT << hashBlock;
    ssValue << nAddressCount << nNewAddressCount;
    return dbTrie.WriteExtKv(ssKey, ssValue);
}

//////////////////////////////
// CAddressDB

bool CAddressDB::Initialize(const boost::filesystem::path& pathData)
{
    pathAddress = pathData / "address";

    if (!boost::filesystem::exists(pathAddress))
    {
        boost::filesystem::create_directories(pathAddress);
    }

    if (!boost::filesystem::is_directory(pathAddress))
    {
        return false;
    }
    return true;
}

void CAddressDB::Deinitialize()
{
    CWriteLock wlock(rwAccess);
    mapAddressDB.clear();
}

bool CAddressDB::ExistFork(const uint256& hashFork)
{
    CReadLock rlock(rwAccess);
    return (mapAddressDB.find(hashFork) != mapAddressDB.end());
}

bool CAddressDB::LoadFork(const uint256& hashFork)
{
    CWriteLock wlock(rwAccess);

    auto it = mapAddressDB.find(hashFork);
    if (it != mapAddressDB.end())
    {
        return true;
    }

    std::shared_ptr<CForkAddressDB> spAddress(new CForkAddressDB());
    if (spAddress == nullptr)
    {
        return false;
    }
    if (!spAddress->Initialize(hashFork, pathAddress / hashFork.GetHex()))
    {
        return false;
    }
    mapAddressDB.insert(make_pair(hashFork, spAddress));
    return true;
}

void CAddressDB::RemoveFork(const uint256& hashFork)
{
    CWriteLock wlock(rwAccess);

    auto it = mapAddressDB.find(hashFork);
    if (it != mapAddressDB.end())
    {
        it->second->RemoveAll();
        mapAddressDB.erase(it);
    }

    boost::filesystem::path forkPath = pathAddress / hashFork.GetHex();
    if (boost::filesystem::exists(forkPath))
    {
        boost::filesystem::remove_all(forkPath);
    }
}

bool CAddressDB::AddNewFork(const uint256& hashFork)
{
    RemoveFork(hashFork);
    return LoadFork(hashFork);
}

void CAddressDB::Clear()
{
    CWriteLock wlock(rwAccess);

    auto it = mapAddressDB.begin();
    while (it != mapAddressDB.end())
    {
        it->second->RemoveAll();
        mapAddressDB.erase(it++);
    }
}

bool CAddressDB::AddAddressContext(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDestination, CAddressContext>& mapAddress, const uint64 nNewAddressCount,
                                   const std::map<CDestination, CTimeVault>& mapTimeVault, const std::map<uint32, CFunctionAddressContext>& mapFunctionAddress, uint256& hashNewRoot)
{
    CReadLock rlock(rwAccess);

    auto it = mapAddressDB.find(hashFork);
    if (it != mapAddressDB.end())
    {
        return it->second->AddAddressContext(hashPrevBlock, hashBlock, mapAddress, nNewAddressCount, mapTimeVault, mapFunctionAddress, hashNewRoot);
    }
    return false;
}

bool CAddressDB::RetrieveAddressContext(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, CAddressContext& ctxAddress)
{
    CReadLock rlock(rwAccess);

    auto it = mapAddressDB.find(hashFork);
    if (it != mapAddressDB.end())
    {
        return it->second->RetrieveAddressContext(hashBlock, dest, ctxAddress);
    }
    return false;
}

bool CAddressDB::ListContractAddress(const uint256& hashFork, const uint256& hashBlock, std::map<CDestination, CContractAddressContext>& mapContractAddress)
{
    CReadLock rlock(rwAccess);

    auto it = mapAddressDB.find(hashFork);
    if (it != mapAddressDB.end())
    {
        return it->second->ListContractAddress(hashBlock, mapContractAddress);
    }
    return false;
}

bool CAddressDB::RetrieveTimeVault(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, CTimeVault& tv)
{
    CReadLock rlock(rwAccess);

    auto it = mapAddressDB.find(hashFork);
    if (it != mapAddressDB.end())
    {
        return it->second->RetrieveTimeVault(hashBlock, dest, tv);
    }
    return false;
}

bool CAddressDB::GetAddressCount(const uint256& hashFork, const uint256& hashBlock, uint64& nAddressCount, uint64& nNewAddressCount)
{
    CReadLock rlock(rwAccess);

    auto it = mapAddressDB.find(hashFork);
    if (it != mapAddressDB.end())
    {
        return it->second->GetAddressCount(hashBlock, nAddressCount, nNewAddressCount);
    }
    return false;
}

bool CAddressDB::ListFunctionAddress(const uint256& hashFork, const uint256& hashBlock, std::map<uint32, CFunctionAddressContext>& mapFunctionAddress)
{
    CReadLock rlock(rwAccess);

    auto it = mapAddressDB.find(hashFork);
    if (it != mapAddressDB.end())
    {
        return it->second->ListFunctionAddress(hashBlock, mapFunctionAddress);
    }
    return false;
}

bool CAddressDB::RetrieveFunctionAddress(const uint256& hashFork, const uint256& hashBlock, const uint32 nFuncId, CFunctionAddressContext& ctxFuncAddress)
{
    CReadLock rlock(rwAccess);

    auto it = mapAddressDB.find(hashFork);
    if (it != mapAddressDB.end())
    {
        return it->second->RetrieveFunctionAddress(hashBlock, nFuncId, ctxFuncAddress);
    }
    return false;
}

bool CAddressDB::CheckAddressContext(const uint256& hashFork, const std::vector<uint256>& vCheckBlock)
{
    CReadLock rlock(rwAccess);

    auto it = mapAddressDB.find(hashFork);
    if (it != mapAddressDB.end())
    {
        return it->second->CheckAddressContext(vCheckBlock);
    }
    return false;
}

bool CAddressDB::CheckAddressContext(const uint256& hashFork, const uint256& hashLastBlock, const uint256& hashLastRoot)
{
    CReadLock rlock(rwAccess);

    auto it = mapAddressDB.find(hashFork);
    if (it != mapAddressDB.end())
    {
        return it->second->CheckAddressContext(hashLastBlock, hashLastRoot);
    }
    return false;
}

bool CAddressDB::VerifyAddressContext(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode)
{
    CReadLock rlock(rwAccess);

    auto it = mapAddressDB.find(hashFork);
    if (it != mapAddressDB.end())
    {
        return it->second->VerifyAddressContext(hashPrevBlock, hashBlock, hashRoot, fVerifyAllNode);
    }
    return false;
}

} // namespace storage
} // namespace hashahead
