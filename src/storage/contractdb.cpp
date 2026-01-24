// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "contractdb.h"

#include <boost/range/adaptor/reversed.hpp>

#include "leveldbeng.h"

#include "block.h"

using namespace std;
using namespace hnbase;

namespace hashahead
{
namespace storage
{

const uint8 DB_CONTRACT_KEY_TYPE_CONTRACTKV = 0x11;
const uint8 DB_CONTRACT_KEY_TYPE_ADDRESS_ROOT = 0x12;

//////////////////////////////
// CForkContractDB

CForkContractDB::CForkContractDB(const uint256& hashForkIn)
  : hashFork(hashForkIn), nChainId(CBlock::GetBlockChainIdByHash(hashForkIn))
{
    fPruneData = false;
}

CForkContractDB::~CForkContractDB()
{
    dbTrie.Deinitialize();
}

bool CForkContractDB::Initialize(const boost::filesystem::path& pathData, const bool fPrune)
{
    if (!dbTrie.Initialize(pathData))
    {
        return false;
    }
    fPruneData = fPrune;
    return true;
}

void CForkContractDB::Deinitialize()
{
    dbTrie.Deinitialize();
}

bool CForkContractDB::RemoveAll()
{
    dbTrie.RemoveAll();
    return true;
}

bool CForkContractDB::AddBlockContractKvValue(const uint32 nBlockHeight, const uint64 nBlockNumber, const CDestination& destContract, const uint256& hashPrevRoot, const std::map<uint256, bytes>& mapContractState, uint256& hashBlockRoot)
{
    bytesmap mapKv;
    for (const auto& kv : mapContractState)
    {
        hnbase::CBufStream ssKey, ssValue;
        bytes btKey, btValue;

        ssKey << DB_CONTRACT_KEY_TYPE_CONTRACTKV << kv.first;
        ssKey.GetData(btKey);

        ssValue << kv.second;
        ssValue.GetData(btValue);

        mapKv.insert(make_pair(btKey, btValue));
    }
    if (!dbTrie.AddNewTrie(hashPrevRoot, mapKv, hashBlockRoot))
    {
        StdLog("CForkContractDB", "Add block contract kv value: Add new trie failed, height: %d, prev root: %s, contract address: %s",
               nBlockHeight, hashPrevRoot.ToString().c_str(), destContract.ToString().c_str());
        return false;
    }
    if (fPruneData && hashBlockRoot != hashPrevRoot)
    {
        if (!WriteAddressRoot(destContract, hashPrevRoot, hashBlockRoot, nBlockHeight, nBlockNumber))
        {
            StdLog("CForkContractDB", "Add block contract kv value: Write address root failed, height: %d, prev root: %s, contract address: %s",
                   nBlockHeight, hashPrevRoot.ToString().c_str(), destContract.ToString().c_str());
            return false;
        }
    }
    return true;
}

bool CForkContractDB::CreateCacheContractKvTrie(const uint256& hashPrevRoot, const std::map<uint256, bytes>& mapContractState, uint256& hashNewRoot)
{
    bytesmap mapKv;
    for (const auto& kv : mapContractState)
    {
        hnbase::CBufStream ssKey, ssValue;
        bytes btKey, btValue;

        ssKey << DB_CONTRACT_KEY_TYPE_CONTRACTKV << kv.first;
        ssKey.GetData(btKey);

        ssValue << kv.second;
        ssValue.GetData(btValue);

        mapKv.insert(make_pair(btKey, btValue));
    }

    std::map<uint256, CTrieValue> mapCacheNode;
    return dbTrie.CreateCacheTrie(hashPrevRoot, mapKv, hashNewRoot, mapCacheNode);
}

bool CForkContractDB::RetrieveContractKvValue(const uint256& hashContractRoot, const uint256& key, bytes& value)
{
    hnbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_CONTRACT_KEY_TYPE_CONTRACTKV << key;
    ssKey.GetData(btKey);
    if (!dbTrie.Retrieve(hashContractRoot, btKey, btValue))
    {
        StdLog("CForkContractDB", "Retrieve contract kv value: Retrieve failed, root: %s, key: %s", hashContractRoot.ToString().c_str(), key.ToString().c_str());
        return false;
    }
    try
    {
        ssValue.Write((char*)(btValue.data()), btValue.size());
        ssValue >> value;
    }
    catch (std::exception& e)
    {
        hnbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CForkContractDB::ClearContractKvRootUnavailableNode(const uint32 nRemoveLastHeight, bool& fExit)
{
    if (!fPruneData)
    {
        StdLog("CForkContractDB", "Clear contract kv root unavailable node: Prune data is false, last height: %d, fork chainid: %d", nRemoveLastHeight, nChainId);
        return false;
    }

    std::map<CDestination, std::map<uint256, std::pair<uint32, uint64>>> mapAddressRoot; // key1: contract address, key2: root, v1: height, v2: number
    std::map<uint256, std::set<uint256>> mapRootNext;                                    // key: root, value: next root
    if (!WalkThroughHeightRoot(mapAddressRoot, mapRootNext))
    {
        StdLog("CForkContractDB", "Clear contract kv root unavailable node: Walk through height root failed, last height: %d, fork chainid: %d", nRemoveLastHeight, nChainId);
        return false;
    }

    std::map<uint64, std::map<uint256, CDestination>> mapRemoveRoot; // key1: block number, key2: root, value: address
    for (const auto& kv : mapAddressRoot)
    {
        const CDestination& destAddress = kv.first;
        std::map<uint64, std::set<uint256>> mapLastRootNumber; // key: block number, value: root
        for (const auto& kv2 : kv.second)
        {
            const uint256& hashRoot = kv2.first;
            const uint32 nBlockHeight = kv2.second.first;
            const uint64 nBlockNumber = kv2.second.second;
            if (nBlockHeight < nRemoveLastHeight)
            {
                auto it = mapRootNext.find(hashRoot);
                if (it == mapRootNext.end() || it->second.empty())
                {
                    mapLastRootNumber[nBlockNumber].insert(hashRoot);
                }
                else
                {
                    mapRemoveRoot[nBlockNumber].insert(std::make_pair(hashRoot, destAddress));
                }
            }
        }
        if (mapLastRootNumber.size() > 1)
        {
            auto mt = mapLastRootNumber.rbegin();
            ++mt;
            for (; mt != mapLastRootNumber.rend(); ++mt)
            {
                const uint64 nBlockNumber = mt->first;
                for (const auto& hashRoot : mt->second)
                {
                    mapRemoveRoot[nBlockNumber].insert(std::make_pair(hashRoot, destAddress));
                }
            }
        }
    }

    std::set<uint256> setNeedRemovedHash;
    for (const auto& kv : mapRemoveRoot)
    {
        for (const auto& kv2 : kv.second)
        {
            const uint256& hashRoot = kv2.first;
            if (setNeedRemovedHash.count(hashRoot) == 0)
            {
                setNeedRemovedHash.insert(hashRoot);
            }
        }
    }

    StdDebug("CForkContractDB", "Clear contract kv root unavailable node: Remove root begin, total root count: %lu, last height: %d, fork chainid: %d",
             setNeedRemovedHash.size(), nRemoveLastHeight, nChainId);

    int64 t1 = GetTimeMillis();
    int64 m1 = t1;

    uint64 nTotalRemoveNodeCount = 0, nTotalLinkerCount = 0;
    std::set<uint256> setRemovedHash;
    for (const auto& kv : mapRemoveRoot)
    {
        const uint64 nBlockNumber = kv.first;
        for (const auto& kv2 : kv.second)
        {
            const uint256& hashRoot = kv2.first;
            const CDestination& destAddress = kv2.second;
            if (setRemovedHash.count(hashRoot) == 0)
            {
                setRemovedHash.insert(hashRoot);

                if (setRemovedHash.size() % 100 == 0)
                {
                    int64 m2 = GetTimeMillis();
                    int64 sm = m2 - m1;
                    m1 = m2;
                    StdDebug("CForkContractDB", "Clear contract kv root unavailable node: Remove part success, elapsed time: %lu ms (%lu ms), average: %.3f ms/root, remove node count: %lu, remove linker count: %lu, remove root count: %lu, total root count: %lu, block number: %lu, last height: %d, fork chainid: %d",
                             sm, m2 - t1, (double)sm / 100, nTotalRemoveNodeCount, nTotalLinkerCount, setRemovedHash.size(), setNeedRemovedHash.size(), nBlockNumber, nRemoveLastHeight, nChainId);
                }
            }
            if (!RemoveAddressRoot(destAddress, hashRoot))
            {
                StdLog("CForkContractDB", "Clear contract kv root unavailable node: Remove address root failed, address: %s, root: %s, block number: %lu, last height: %d, fork chainid: %d",
                       destAddress.ToString().c_str(), hashRoot.ToString().c_str(), nBlockNumber, nRemoveLastHeight, nChainId);
                return false;
            }
            if (fExit)
            {
                break;
            }
        }
        if (fExit)
        {
            break;
        }
    }

    int64 t2 = GetTimeMillis();
    int64 et = t2 - t1;
    double dPerBlockTime = 0.0;
    double dPerRootTime = 0.0;
    if (mapRemoveRoot.size() > 0 && et > 0)
    {
        dPerBlockTime = (double)et / mapRemoveRoot.size();
    }
    if (setRemovedHash.size() > 0 && et > 0)
    {
        dPerRootTime = (double)et / setRemovedHash.size();
    }

    StdDebug("CForkContractDB", "Clear contract kv root unavailable node: Remove success, elapsed time: %lu ms, per block time: %.2f ms, per root time: %.2f ms, remove node count: %lu, remove linker count: %lu, remove root count: %lu, block count: %lu, last height: %d, fork chainid: %d",
             et, dPerBlockTime, dPerRootTime, nTotalRemoveNodeCount, nTotalLinkerCount, setRemovedHash.size(), mapRemoveRoot.size(), nRemoveLastHeight, nChainId);
    return true;
}

bool CForkContractDB::GetContractAddressRoot(const CDestination& destContract, const uint256& hashRoot, uint256& hashPrevRoot, uint32& nBlockHeight, uint64& nBlockNumber)
{
    hnbase::CBufStream ssKey, ssValue;
    ssKey << DB_CONTRACT_KEY_TYPE_ADDRESS_ROOT << destContract << hashRoot;
    if (!dbTrie.ReadExtKv(ssKey, ssValue))
    {
        return false;
    }
    try
    {
        ssValue >> hashPrevRoot >> nBlockHeight >> nBlockNumber;
    }
    catch (std::exception& e)
    {
        hnbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CForkContractDB::CreateCacheContractKvRoot(const uint256& hashPrevRoot, const bytesmap& mapKv, uint256& hashNewRoot)
{
    std::map<uint256, CTrieValue> mapCacheNode;
    return dbTrie.CreateCacheTrie(hashPrevRoot, mapKv, hashNewRoot, mapCacheNode);
}

bool CForkContractDB::AddContractKvTrie(const uint32 nBlockHeight, const uint256& hashPrevRoot, const bytesmap& mapKv, uint256& hashNewRoot)
{
    return dbTrie.AddNewTrie(hashPrevRoot, mapKv, hashNewRoot);
}

//-----------------------------------------
bool CForkContractDB::WriteAddressRoot(const CDestination& destContract, const uint256& hashPrevRoot, const uint256& hashNewRoot, const uint32 nBlockHeight, const uint64 nBlockNumber)
{
    hnbase::CBufStream ssKey, ssValue;
    ssKey << DB_CONTRACT_KEY_TYPE_ADDRESS_ROOT << destContract << hashNewRoot;
    ssValue << hashPrevRoot << nBlockHeight << nBlockNumber;
    return dbTrie.WriteExtKv(ssKey, ssValue);
}

bool CForkContractDB::RemoveAddressRoot(const CDestination& destContract, const uint256& hashRoot)
{
    hnbase::CBufStream ssKey;
    ssKey << DB_CONTRACT_KEY_TYPE_ADDRESS_ROOT << destContract << hashRoot;
    return dbTrie.RemoveExtKv(ssKey);
}

bool CForkContractDB::WalkThroughHeightRoot(std::map<CDestination, std::map<uint256, std::pair<uint32, uint64>>>& mapAddressRoot, std::map<uint256, std::set<uint256>>& mapRootNext)
{
    auto funcWalker = [&](CBufStream& ssKey, CBufStream& ssValue) -> bool {
        try
        {
            uint8 nExtKey;
            uint8 nKeyType;
            ssKey >> nExtKey >> nKeyType;
            if (nKeyType == DB_CONTRACT_KEY_TYPE_ADDRESS_ROOT)
            {
                CDestination destContract;
                uint256 hashRoot, hashPrevRoot;
                uint32 nHeight;
                uint64 nBlockNumber;

                ssKey >> destContract >> hashRoot;
                ssValue >> hashPrevRoot >> nHeight >> nBlockNumber;

                mapAddressRoot[destContract].insert(std::make_pair(hashRoot, std::make_pair(nHeight, nBlockNumber)));
                mapRootNext[hashPrevRoot].insert(hashRoot);
                if (mapRootNext.find(hashRoot) == mapRootNext.end())
                {
                    mapRootNext.insert(std::make_pair(hashRoot, std::set<uint256>()));
                }
            }
            return true;
        }
        catch (std::exception& e)
        {
            hnbase::StdError(__PRETTY_FUNCTION__, e.what());
        }
        return false;
    };

    CBufStream ssKeyBegin, ssKeyPrefix;
    ssKeyPrefix << DB_CONTRACT_KEY_TYPE_ADDRESS_ROOT;

    return dbTrie.WalkThroughExtKv(ssKeyBegin, ssKeyPrefix, funcWalker);
}

//////////////////////////////
// CContractDB

bool CContractDB::Initialize(const boost::filesystem::path& pathData, const bool fPruneStateIn)
{
    pathContract = pathData / "contract";

    if (!boost::filesystem::exists(pathContract))
    {
        boost::filesystem::create_directories(pathContract);
    }

    if (!boost::filesystem::is_directory(pathContract))
    {
        return false;
    }
    fPruneState = fPruneStateIn;
    return true;
}

void CContractDB::Deinitialize()
{
    CWriteLock wlock(rwAccess);
    mapContractDB.clear();
}

bool CContractDB::ExistFork(const uint256& hashFork)
{
    CReadLock rlock(rwAccess);
    return (mapContractDB.find(hashFork) != mapContractDB.end());
}

bool CContractDB::LoadFork(const uint256& hashFork)
{
    CWriteLock wlock(rwAccess);

    auto it = mapContractDB.find(hashFork);
    if (it != mapContractDB.end())
    {
        return true;
    }

    std::shared_ptr<CForkContractDB> spWasm(new CForkContractDB(hashFork));
    if (spWasm == nullptr)
    {
        return false;
    }
    if (!spWasm->Initialize(pathContract / hashFork.GetHex(), fPruneState))
    {
        return false;
    }
    mapContractDB.insert(make_pair(hashFork, spWasm));
    return true;
}

void CContractDB::RemoveFork(const uint256& hashFork)
{
    CWriteLock wlock(rwAccess);

    auto it = mapContractDB.find(hashFork);
    if (it != mapContractDB.end())
    {
        (*it).second->RemoveAll();
        mapContractDB.erase(it);
    }

    boost::filesystem::path forkPath = pathContract / hashFork.GetHex();
    if (boost::filesystem::exists(forkPath))
    {
        boost::filesystem::remove_all(forkPath);
    }
}

bool CContractDB::AddNewFork(const uint256& hashFork)
{
    RemoveFork(hashFork);
    return LoadFork(hashFork);
}

void CContractDB::Clear()
{
    CWriteLock wlock(rwAccess);

    auto it = mapContractDB.begin();
    while (it != mapContractDB.end())
    {
        (*it).second->RemoveAll();
        mapContractDB.erase(it++);
    }
}

bool CContractDB::AddBlockContractKvValue(const uint256& hashFork, const uint32 nBlockHeight, const uint64 nBlockNumber, const CDestination& destContract, const uint256& hashPrevRoot, const std::map<uint256, bytes>& mapContractState, uint256& hashContractRoot)
{
    CReadLock rlock(rwAccess);

    auto it = mapContractDB.find(hashFork);
    if (it != mapContractDB.end())
    {
        return it->second->AddBlockContractKvValue(nBlockHeight, nBlockNumber, destContract, hashPrevRoot, mapContractState, hashContractRoot);
    }
    return false;
}

bool CContractDB::CreateCacheContractKvTrie(const uint256& hashFork, const uint256& hashPrevRoot, const std::map<uint256, bytes>& mapContractState, uint256& hashNewRoot)
{
    CReadLock rlock(rwAccess);

    auto it = mapContractDB.find(hashFork);
    if (it != mapContractDB.end())
    {
        return it->second->CreateCacheContractKvTrie(hashPrevRoot, mapContractState, hashNewRoot);
    }
    return false;
}

bool CContractDB::RetrieveContractKvValue(const uint256& hashFork, const uint256& hashContractRoot, const uint256& key, bytes& value)
{
    CReadLock rlock(rwAccess);

    auto it = mapContractDB.find(hashFork);
    if (it != mapContractDB.end())
    {
        return it->second->RetrieveContractKvValue(hashContractRoot, key, value);
    }
    return false;
}

bool CContractDB::ClearContractKvRootUnavailableNode(const uint256& hashFork, const uint32 nRemoveLastHeight, bool& fExit)
{
    CReadLock rlock(rwAccess);

    auto it = mapContractDB.find(hashFork);
    if (it != mapContractDB.end())
    {
        return it->second->ClearContractKvRootUnavailableNode(nRemoveLastHeight, fExit);
    }
    return false;
}

bool CContractDB::GetContractAddressRoot(const uint256& hashFork, const CDestination& destContract, const uint256& hashRoot, uint256& hashPrevRoot, uint32& nBlockHeight, uint64& nBlockNumber)
{
    CReadLock rlock(rwAccess);

    auto it = mapContractDB.find(hashFork);
    if (it != mapContractDB.end())
    {
        return it->second->GetContractAddressRoot(destContract, hashRoot, hashPrevRoot, nBlockHeight, nBlockNumber);
    }
    return false;
}

bool CContractDB::CreateCacheContractKvRoot(const uint256& hashFork, const uint256& hashPrevRoot, const bytesmap& mapKv, uint256& hashNewRoot)
{
    CReadLock rlock(rwAccess);

    auto it = mapContractDB.find(hashFork);
    if (it != mapContractDB.end())
    {
        return it->second->CreateCacheContractKvRoot(hashPrevRoot, mapKv, hashNewRoot);
    }
    return false;
}

bool CContractDB::AddContractKvTrie(const uint256& hashFork, const uint32 nBlockHeight, const uint256& hashPrevRoot, const bytesmap& mapKv, uint256& hashNewRoot)
{
    CReadLock rlock(rwAccess);

    auto it = mapContractDB.find(hashFork);
    if (it != mapContractDB.end())
    {
        return it->second->AddContractKvTrie(nBlockHeight, hashPrevRoot, mapKv, hashNewRoot);
    }
    return false;
}

bool CContractDB::CreateStaticContractStateRoot(const std::map<uint256, bytes>& mapContractState, uint256& hashStateRoot)
{
    bytesmap mapKv;
    for (const auto& kv : mapContractState)
    {
        hnbase::CBufStream ssKey;
        bytes btKey, btValue;

        ssKey << DB_CONTRACT_KEY_TYPE_CONTRACTKV << kv.first;
        ssKey.GetData(btKey);

        hnbase::CBufStream ssValue;
        ssValue << kv.second;
        ssValue.GetData(btValue);

        mapKv.insert(make_pair(btKey, btValue));
    }

    CTrieDB dbTrieTemp;
    std::map<uint256, CTrieValue> mapCacheNode;
    return dbTrieTemp.CreateCacheTrie(uint256(), mapKv, hashStateRoot, mapCacheNode);
}

////////////////////////////////////////////
// contract code

bool CContractDB::AddCodeContext(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock,
                                 const std::map<uint256, CContractSourceCodeContext>& mapSourceCode,
                                 const std::map<uint256, CContractCreateCodeContext>& mapContractCreateCode,
                                 const std::map<uint256, CContractRunCodeContext>& mapContractRunCode,
                                 const std::map<uint256, CTemplateContext>& mapTemplateData,
                                 uint256& hashCodeRoot)
{
    CReadLock rlock(rwAccess);

    auto it = mapContractDB.find(hashFork);
    if (it != mapContractDB.end())
    {
        return it->second->AddCodeContext(hashPrevBlock, hashBlock, mapSourceCode, mapContractCreateCode, mapContractRunCode, mapTemplateData, hashCodeRoot);
    }
    return false;
}

bool CContractDB::RetrieveSourceCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashSourceCode, CContractSourceCodeContext& ctxtCode)
{
    CReadLock rlock(rwAccess);

    auto it = mapContractDB.find(hashFork);
    if (it != mapContractDB.end())
    {
        return it->second->RetrieveSourceCodeContext(hashBlock, hashSourceCode, ctxtCode);
    }
    return false;
}

bool CContractDB::RetrieveContractCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractCreateCode, CContractCreateCodeContext& ctxtCode)
{
    CReadLock rlock(rwAccess);

    auto it = mapContractDB.find(hashFork);
    if (it != mapContractDB.end())
    {
        return it->second->RetrieveContractCreateCodeContext(hashBlock, hashContractCreateCode, ctxtCode);
    }
    return false;
}

bool CContractDB::RetrieveContractRunCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractRunCode, CContractRunCodeContext& ctxtCode)
{
    CReadLock rlock(rwAccess);

    auto it = mapContractDB.find(hashFork);
    if (it != mapContractDB.end())
    {
        return it->second->RetrieveContractRunCodeContext(hashBlock, hashContractRunCode, ctxtCode);
    }
    return false;
}

bool CContractDB::ListContractCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, std::map<uint256, CContractCreateCodeContext>& mapContractCreateCode)
{
    CReadLock rlock(rwAccess);

    auto it = mapContractDB.find(hashFork);
    if (it != mapContractDB.end())
    {
        return it->second->ListContractCreateCodeContext(hashBlock, mapContractCreateCode);
    }
    return false;
}

bool CContractDB::VerifyCodeContext(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode)
{
    CReadLock rlock(rwAccess);

    auto it = mapContractDB.find(hashFork);
    if (it != mapContractDB.end())
    {
        return it->second->VerifyCodeContext(hashPrevBlock, hashBlock, hashRoot, fVerifyAllNode);
    }
    return false;
}

} // namespace storage
} // namespace hashahead
