// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "hdexdb.h"

#include <boost/range/adaptor/reversed.hpp>

#include "block.h"
#include "param.h"

using namespace std;
using namespace hnbase;

namespace hashahead
{
namespace storage
{

// #define HDEX_OUT_TEST_LOG
#define HDEX_CACHE_DEX_ENABLE

#define MAX_FETCH_DEX_ORDER_COUNT 1000

const string DB_HDEX_KEY_ID_PREVROOT("prevroot");

const uint8 DB_HDEX_ROOT_TYPE_TRIE = 0x10;
const uint8 DB_HDEX_ROOT_TYPE_EXT = 0x20;
const uint8 DB_HDEX_ROOT_TYPE_BLOCK_ROOT = 0x30;

// const uint8 DB_HDEX_KEY_TYPE_TRIE_HEIGHT_BLOCK = DB_HDEX_ROOT_TYPE_TRIE | 0x01;
const uint8 DB_HDEX_KEY_TYPE_TRIE_LOCAL_DEX_ORDER = DB_HDEX_ROOT_TYPE_TRIE | 0x02;
const uint8 DB_HDEX_KEY_TYPE_TRIE_PEER_DEX_ORDER = DB_HDEX_ROOT_TYPE_TRIE | 0x03;
const uint8 DB_HDEX_KEY_TYPE_TRIE_CROSS_TRANSFER_SEND = DB_HDEX_ROOT_TYPE_TRIE | 0x04;
const uint8 DB_HDEX_KEY_TYPE_TRIE_CROSS_TRANSFER_RECV = DB_HDEX_ROOT_TYPE_TRIE | 0x05;
const uint8 DB_HDEX_KEY_TYPE_TRIE_DEX_ORDER_MAX_NUMBER = DB_HDEX_ROOT_TYPE_TRIE | 0x06;
//const uint8 DB_HDEX_KEY_TYPE_TRIE_DEX_ORDER_COMPLETE_AMOUNT = DB_HDEX_ROOT_TYPE_TRIE | 0x07;
const uint8 DB_HDEX_KEY_TYPE_TRIE_DEX_ORDER_COMPLETE_RECORD = DB_HDEX_ROOT_TYPE_TRIE | 0x08;
const uint8 DB_HDEX_KEY_TYPE_TRIE_DEX_ORDER_COMPLETE_PRICE = DB_HDEX_ROOT_TYPE_TRIE | 0x09;
const uint8 DB_HDEX_KEY_TYPE_TRIE_CROSS_LAST_PROVE_BLOCK = DB_HDEX_ROOT_TYPE_TRIE | 0x0A;
const uint8 DB_HDEX_KEY_TYPE_TRIE_CROSS_SEND_LAST_PROVE_BLOCK = DB_HDEX_ROOT_TYPE_TRIE | 0x0B;
const uint8 DB_HDEX_KEY_TYPE_TRIE_CROSS_RECV_CONFIRM_BLOCK = DB_HDEX_ROOT_TYPE_TRIE | 0x0C;

const uint8 DB_HDEX_KEY_TYPE_EXT_BLOCK_CROSSCHAIN_PROVE = DB_HDEX_ROOT_TYPE_EXT | 0x01;
const uint8 DB_HDEX_KEY_TYPE_EXT_RECV_CROSSCHAIN_PROVE = DB_HDEX_ROOT_TYPE_EXT | 0x02;
const uint8 DB_HDEX_KEY_TYPE_EXT_SEND_CHAIN_LAST_PROVE_BLOCK = DB_HDEX_ROOT_TYPE_EXT | 0x03;
const uint8 DB_HDEX_KEY_TYPE_EXT_BLOCK_FOR_FIRST_PREV_BLOCK = DB_HDEX_ROOT_TYPE_EXT | 0x04;

const uint8 DB_HDEX_UNCOMPLETED_MATCH_ORDER = 0x01;
const uint8 DB_HDEX_COMPLETED_MATCH_ORDER = 0x02;

///////////////////////////////////
// CCacheBlockDexOrder

bool CCacheBlockDexOrder::AddDexOrderCache(const uint256& hashDexOrder, const CDestination& destOrder, const uint64 nOrderNumber, const CDexOrderBody& dexOrder,
                                           const CChainId nOrderAtChainId, const uint256& hashOrderAtBlock, const uint256& nPrevCompletePrice)
{
    return ptrMatchDex->AddMatchDexOrder(hashDexOrder, destOrder, nOrderNumber, dexOrder, nOrderAtChainId, hashOrderAtBlock, nPrevCompletePrice);
}

bool CCacheBlockDexOrder::UpdateCompleteOrderCache(const uint256& hashCoinPair, const uint256& hashDexOrder, const uint256& nCompleteAmount, const uint64 nCompleteCount)
{
    return ptrMatchDex->UpdateMatchCompleteOrder(hashCoinPair, hashDexOrder, nCompleteAmount, nCompleteCount);
}

void CCacheBlockDexOrder::UpdatePeerProveLastBlock(const CChainId nPeerChainId, const uint256& hashLastProveBlock)
{
    ptrMatchDex->UpdatePeerProveLastBlock(nPeerChainId, hashLastProveBlock);
}

void CCacheBlockDexOrder::UpdateCompletePrice(const uint256& hashCoinPair, const uint256& nCompletePrice)
{
    ptrMatchDex->UpdateCompletePrice(hashCoinPair, nCompletePrice);
}

bool CCacheBlockDexOrder::GetMatchDexResult(std::map<uint256, CMatchOrderResult>& mapMatchResult)
{
    return ptrMatchDex->MatchDex(mapMatchResult);
}

bool CCacheBlockDexOrder::ListMatchDexOrder(const std::string& strCoinSymbolSell, const std::string& strCoinSymbolBuy, const uint64 nGetCount, CRealtimeDexOrder& realDexOrder)
{
    return ptrMatchDex->ListDexOrder(strCoinSymbolSell, strCoinSymbolBuy, nGetCount, realDexOrder);
}

////////////////////////////////////////////
// CHdexDB

bool CHdexDB::Initialize(const boost::filesystem::path& pathData, const bool fPruneIn)
{
    if (!dbTrie.Initialize(pathData / "hdex"))
    {
        return false;
    }
    fPrune = fPruneIn;
    return true;
}

void CHdexDB::Deinitialize()
{
    dbTrie.Deinitialize();
}

void CHdexDB::Clear()
{
    dbTrie.Clear();
}

bool CHdexDB::AddDexOrder(const uint256& hashFork, const uint256& hashRefBlock, const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDexOrderHeader, CDexOrderBody>& mapDexOrder, const std::map<CChainId, std::vector<CBlockCoinTransferProve>>& mapCrossTransferProve,
                          const std::map<uint256, uint256>& mapCoinPairCompletePrice, const std::set<CChainId>& setPeerCrossChainId, const std::map<CDexOrderHeader, std::vector<CCompDexOrderRecord>>& mapCompDexOrderRecord, const std::map<CChainId, CBlockProve>& mapBlockProve, uint256& hashNewRoot)
{
    CWriteLock wlock(rwAccess);

    uint256 hashPrevRoot;
    if (hashBlock != hashFork)
    {
        if (!ReadTrieRoot(DB_HDEX_ROOT_TYPE_TRIE, hashPrevBlock, hashPrevRoot))
        {
            StdLog("CHdexDB", "Add dex order: Read trie root fail, prev block: %s, block: %s",
                   hashPrevBlock.GetBhString().c_str(), hashBlock.GetBhString().c_str());
            return false;
        }
    }
    const CChainId nChainId = CBlock::GetBlockChainIdByHash(hashBlock);
    const uint32 nHeight = CBlock::GetBlockHeightByHash(hashBlock);
    const uint16 nSlot = CBlock::GetBlockSlotByHash(hashBlock);

    std::map<CDexOrderHeader, CDexOrderSave> mapDbDexOrder;
    std::map<std::tuple<CDestination, uint256, uint8>, uint64> mapMaxOrderNumber;                  // key: address, coin pair hash, owner coin flag, value: max order number
    std::map<CDexOrderHeader, CDexOrderBody> mapAddNewOrder;                                       // key: order header, value: order body
    std::map<CDexOrderHeader, std::tuple<CDexOrderBody, CChainId, uint256>> mapAddBlockProveOrder; // key: order header, value: 1: order body, 2: at chainid, 3: at block
    std::map<uint256, std::tuple<uint256, uint256, uint64>> mapUpdateCompOrder;                    // key: dex order hash, value: 1: coin pair hash, 2: complete amount, 3: complete count
    std::map<CChainId, uint256> mapPeerProveLastBlock;                                             // key: peer chain id, value: last block

    std::set<bytes> setRemoveKeys;
    bytesmap mapKv;

    auto funcAddLocalDexOrder = [&](const bool fCompleted, const CDexOrderHeader& orderHeader, const CDexOrderSave& dexOrderBody) {
        {
            hnbase::CBufStream ssKey;
            bytes btKey;

            ssKey << DB_HDEX_KEY_TYPE_TRIE_LOCAL_DEX_ORDER << (fCompleted ? DB_HDEX_UNCOMPLETED_MATCH_ORDER : DB_HDEX_COMPLETED_MATCH_ORDER) << BSwap32(orderHeader.GetChainId()) << orderHeader.GetOrderAddress() << orderHeader.GetCoinPairHash() << orderHeader.GetOwnerCoinFlag() << BSwap64(orderHeader.GetOrderNumber());
            ssKey.GetData(btKey);

            setRemoveKeys.insert(btKey);
            mapKv.erase(btKey);
        }

        {
            hnbase::CBufStream ssKey, ssValue;
            bytes btKey, btValue;

            ssKey << DB_HDEX_KEY_TYPE_TRIE_LOCAL_DEX_ORDER << (fCompleted ? DB_HDEX_COMPLETED_MATCH_ORDER : DB_HDEX_UNCOMPLETED_MATCH_ORDER) << BSwap32(orderHeader.GetChainId()) << orderHeader.GetOrderAddress() << orderHeader.GetCoinPairHash() << orderHeader.GetOwnerCoinFlag() << BSwap64(orderHeader.GetOrderNumber());
            ssKey.GetData(btKey);

            ssValue << dexOrderBody;
            ssValue.GetData(btValue);

            mapKv[btKey] = btValue;
        }
    };

    auto funcAddPeerDexOrder = [&](const bool fCompleted, const CDexOrderHeader& orderHeader, const CDexOrderSave& dexOrderBody) {
        {
            hnbase::CBufStream ssKey;
            bytes btKey;

            ssKey << DB_HDEX_KEY_TYPE_TRIE_PEER_DEX_ORDER << (fCompleted ? DB_HDEX_UNCOMPLETED_MATCH_ORDER : DB_HDEX_COMPLETED_MATCH_ORDER) << BSwap32(orderHeader.GetChainId()) << orderHeader.GetOrderAddress() << orderHeader.GetCoinPairHash() << orderHeader.GetOwnerCoinFlag() << BSwap64(orderHeader.GetOrderNumber());
            ssKey.GetData(btKey);

            setRemoveKeys.insert(btKey);
            mapKv.erase(btKey);
        }

        {
            hnbase::CBufStream ssKey, ssValue;
            bytes btKey, btValue;

            ssKey << DB_HDEX_KEY_TYPE_TRIE_PEER_DEX_ORDER << (fCompleted ? DB_HDEX_COMPLETED_MATCH_ORDER : DB_HDEX_UNCOMPLETED_MATCH_ORDER) << BSwap32(orderHeader.GetChainId()) << orderHeader.GetOrderAddress() << orderHeader.GetCoinPairHash() << orderHeader.GetOwnerCoinFlag() << BSwap64(orderHeader.GetOrderNumber());
            ssKey.GetData(btKey);

            ssValue << dexOrderBody;
            ssValue.GetData(btValue);

            mapKv[btKey] = btValue;
        }
    };

    for (const auto& kv : mapCompDexOrderRecord)
    {
        const CDexOrderHeader& orderHeader = kv.first;
        const std::vector<CCompDexOrderRecord>& vDexOrderRecord = kv.second;

        CDexOrderSave dexOrderBody;
        if (orderHeader.GetChainId() == nChainId)
        {
            if (!GetDexOrderDb(hashPrevRoot, orderHeader.GetChainId(), orderHeader.GetOrderAddress(), orderHeader.GetCoinPairHash(), orderHeader.GetOwnerCoinFlag(), orderHeader.GetOrderNumber(), dexOrderBody))
            {
                StdLog("CHdexDB", "Add dex order: Get dex order fail, at chainid: %d, order address: %s, owner coin flag: %d, order number: %lu, prev block: %s, block: %s",
                       orderHeader.GetChainId(), orderHeader.GetOrderAddress().ToString().c_str(), orderHeader.GetOwnerCoinFlag(), orderHeader.GetOrderNumber(), hashPrevBlock.GetBhString().c_str(), hashBlock.GetBhString().c_str());
                return false;
            }
        }
        else
        {
            if (!GetPeerDexOrderProveDb(hashPrevRoot, orderHeader.GetChainId(), orderHeader.GetOrderAddress(), orderHeader.GetCoinPairHash(), orderHeader.GetOwnerCoinFlag(), orderHeader.GetOrderNumber(), dexOrderBody))
            {
                StdLog("CHdexDB", "Add dex order: Get peer dex order prove fail, at chainid: %d, order address: %s, owner coin flag: %d, order number: %lu, prev block: %s, block: %s",
                       orderHeader.GetChainId(), orderHeader.GetOrderAddress().ToString().c_str(), orderHeader.GetOwnerCoinFlag(), orderHeader.GetOrderNumber(), hashPrevBlock.GetBhString().c_str(), hashBlock.GetBhString().c_str());
                return false;
            }
        }

        for (const CCompDexOrderRecord& orderRecord : vDexOrderRecord)
        {
            dexOrderBody.dexOrder.nCompleteOrderAmount += orderRecord.nCompleteAmount;
        }
        dexOrderBody.dexOrder.nCompleteOrderCount += vDexOrderRecord.size();

        if (orderHeader.GetChainId() == nChainId)
        {
            funcAddLocalDexOrder(dexOrderBody.dexOrder.nCompleteOrderAmount >= dexOrderBody.dexOrder.nOrderAmount, orderHeader, dexOrderBody);
        }
        else
        {
            funcAddPeerDexOrder(dexOrderBody.dexOrder.nCompleteOrderAmount >= dexOrderBody.dexOrder.nOrderAmount, orderHeader, dexOrderBody);
        }

        mapDbDexOrder.insert(std::make_pair(orderHeader, dexOrderBody));
        mapUpdateCompOrder.insert(std::make_pair(orderHeader.GetDexOrderHash(), std::make_tuple(orderHeader.GetCoinPairHash(), dexOrderBody.dexOrder.nCompleteOrderAmount, dexOrderBody.dexOrder.nCompleteOrderCount)));
    }

    for (const auto& kv : mapDexOrder)
    {
        const CDexOrderHeader& orderHeader = kv.first;
        const CDexOrderBody& orderBody = kv.second;

        const CDestination& destOrder = orderHeader.GetOrderAddress();
        const uint256& hashCoinPair = orderHeader.GetCoinPairHash();
        const uint8 nOwnerCoinFlag = orderHeader.GetOwnerCoinFlag();
        const uint64 nOrderNumber = orderHeader.GetOrderNumber();

        auto it = mapDbDexOrder.find(orderHeader);
        if (it == mapDbDexOrder.end())
        {
            CDexOrderSave dexOrderBody;
            if (orderHeader.GetChainId() == nChainId)
            {
                if (GetDexOrderDb(hashPrevRoot, orderHeader.GetChainId(), orderHeader.GetOrderAddress(), orderHeader.GetCoinPairHash(), orderHeader.GetOwnerCoinFlag(), orderHeader.GetOrderNumber(), dexOrderBody))
                {
                    it = mapDbDexOrder.insert(std::make_pair(orderHeader, dexOrderBody)).first;
                }
            }
            else
            {
                if (GetPeerDexOrderProveDb(hashPrevRoot, orderHeader.GetChainId(), orderHeader.GetOrderAddress(), orderHeader.GetCoinPairHash(), orderHeader.GetOwnerCoinFlag(), orderHeader.GetOrderNumber(), dexOrderBody))
                {
                    it = mapDbDexOrder.insert(std::make_pair(orderHeader, dexOrderBody)).first;
                }
            }
        }
        if (it != mapDbDexOrder.end())
        {
            CDexOrderSave& dexOrderSave = it->second;
            CDexOrderBody& dbDexOrder = dexOrderSave.dexOrder;
            if (orderBody.nOrderAmount != dbDexOrder.nOrderAmount && orderBody.nOrderAmount >= dbDexOrder.nCompleteOrderAmount)
            {
                dbDexOrder.nOrderAmount = orderBody.nOrderAmount;
                dexOrderSave.nAtChainId = nChainId;
                dexOrderSave.hashAtBlock = hashBlock;

                funcAddLocalDexOrder(dbDexOrder.nCompleteOrderAmount >= dbDexOrder.nOrderAmount, orderHeader, dexOrderSave);

                mapAddNewOrder[orderHeader] = dbDexOrder;
            }
        }
        else
        {
            CDexOrderSave dexOrderSave(orderBody, nChainId, hashBlock);

            funcAddLocalDexOrder(false, orderHeader, dexOrderSave);

            mapDbDexOrder[orderHeader] = dexOrderSave;
            mapAddNewOrder[orderHeader] = orderBody;
        }

        // Update max order number
        auto nt = mapMaxOrderNumber.find(std::make_tuple(destOrder, hashCoinPair, nOwnerCoinFlag));
        if (nt == mapMaxOrderNumber.end())
        {
            uint64 nMaxOrderNumber;
            if (!GetDexOrderMaxNumberDb(hashPrevRoot, nChainId, destOrder, hashCoinPair, nOwnerCoinFlag, nMaxOrderNumber))
            {
                nMaxOrderNumber = 0;
            }
            if (nMaxOrderNumber < nOrderNumber)
            {
                mapMaxOrderNumber.insert(std::make_pair(std::make_tuple(destOrder, hashCoinPair, nOwnerCoinFlag), nOrderNumber));
            }
        }
        else
        {
            if (nt->second < nOrderNumber)
            {
                nt->second = nOrderNumber;
            }
        }

#ifdef HDEX_OUT_TEST_LOG
        StdDebug("TEST", "HdexDB Add DexOrder: Dex order: chainid: %d, order address: %s, coin pair: %s, symble owner: %s, symble peer: %s, order number: %ld, order amount: %s, order price: %s, at block: %s",
                 nChainId, destOrder.ToString().c_str(), hashCoinPair.ToString().c_str(), orderBody.strCoinSymbolOwner.c_str(), orderBody.strCoinSymbolPeer.c_str(), nOrderNumber,
                 CoinToTokenBigFloat(orderBody.nOrderAmount).c_str(), CoinToTokenBigFloat(orderBody.nOrderPrice).c_str(), hashBlock.GetBhString().c_str());
#endif
    }

    for (const auto& kv : mapCrossTransferProve)
    {
        const CChainId nPeerChainId = kv.first;
        uint32 nIndex = 0;
        for (const CBlockCoinTransferProve& transProve : kv.second)
        {
            hnbase::CBufStream ssKey, ssValue;
            bytes btKey, btValue;

            ssKey << DB_HDEX_KEY_TYPE_TRIE_CROSS_TRANSFER_SEND << BSwap32(nPeerChainId) << transProve.destTransfer << hashBlock << BSwap32(nIndex);
            nIndex++;
            ssKey.GetData(btKey);

            ssValue << transProve;
            ssValue.GetData(btValue);

            mapKv[btKey] = btValue;

#ifdef HDEX_OUT_TEST_LOG
            StdDebug("TEST", "HdexDB Add DexOrder: Block prove: cross transfer send: local chainid: %d, peer chainid: %d, address: %s, coin symbol: %s, ori chainid: %d, dest chainid: %d, amount: %s, at block: %s",
                     nChainId, nPeerChainId, transProve.destTransfer.ToString().c_str(), transProve.strCoinSymbol.c_str(), transProve.nOriChainId,
                     transProve.nDestChainId, CoinToTokenBigFloat(transProve.nTransferAmount).c_str(), hashBlock.GetBhString().c_str());
#endif
        }
    }

    // {
    //     hnbase::CBufStream ssKey, ssValue;
    //     bytes btKey, btValue;

    //     ssKey << DB_HDEX_KEY_TYPE_TRIE_HEIGHT_BLOCK << BSwap32(nHeight) << BSwap16(nSlot);
    //     ssKey.GetData(btKey);

    //     ssValue << hashBlock << hashRefBlock << hashPrevBlock;
    //     ssValue.GetData(btValue);

    //     mapKv[btKey] = btValue;
    // }

    for (const auto& kv : mapMaxOrderNumber)
    {
        const CDestination destOrder = std::get<0>(kv.first);
        const uint256 hashCoinPair = std::get<1>(kv.first);
        const uint8 nOwnerCoinFlag = std::get<2>(kv.first);

        hnbase::CBufStream ssKey, ssValue;
        bytes btKey, btValue;

        ssKey << DB_HDEX_KEY_TYPE_TRIE_DEX_ORDER_MAX_NUMBER << BSwap32(nChainId) << destOrder << hashCoinPair << nOwnerCoinFlag;
        ssKey.GetData(btKey);

        ssValue << kv.second;
        ssValue.GetData(btValue);

        mapKv[btKey] = btValue;

#ifdef HDEX_OUT_TEST_LOG
        StdDebug("TEST", "HdexDB Add DexOrder: Max order number: chainid: %d, order address: %s, coin pair: %s, owner coin flag: %d, max number: %ld, at block: %s",
                 nChainId, destOrder.ToString().c_str(), hashCoinPair.ToString().c_str(), nOwnerCoinFlag, kv.second, hashBlock.GetBhString().c_str());
#endif
    }

    for (const auto& kv : mapCoinPairCompletePrice)
    {
        const uint256& hashCoinPair = kv.first;
        const uint256& nCompletePrice = kv.second;

        hnbase::CBufStream ssKey, ssValue;
        bytes btKey, btValue;

        ssKey << DB_HDEX_KEY_TYPE_TRIE_DEX_ORDER_COMPLETE_PRICE << hashCoinPair;
        ssKey.GetData(btKey);

        ssValue << nCompletePrice;
        ssValue.GetData(btValue);

        mapKv[btKey] = btValue;

#ifdef HDEX_OUT_TEST_LOG
        StdDebug("TEST", "HdexDB Add DexOrder: Complete price: coin pair: %s, complete price: %s, at block: %s",
                 hashCoinPair.ToString().c_str(), CoinToTokenBigFloat(nCompletePrice).c_str(), hashBlock.GetBhString().c_str());
#endif
    }

    for (const CChainId nPeerChainId : setPeerCrossChainId)
    {
        hnbase::CBufStream ssKey, ssValue;
        bytes btKey, btValue;

        ssKey << DB_HDEX_KEY_TYPE_TRIE_CROSS_SEND_LAST_PROVE_BLOCK << BSwap32(nPeerChainId);
        ssKey.GetData(btKey);

        ssValue << hashBlock;
        ssValue.GetData(btValue);

        mapKv[btKey] = btValue;

#ifdef HDEX_OUT_TEST_LOG
        StdDebug("TEST", "HdexDB Add DexOrder: Peer cross chainid: peer chainid: %u, block: %s",
                 nPeerChainId, hashBlock.GetBhString().c_str());
#endif
    }

    //     for (const auto& kv : mapCompDexOrderRecord)
    //     {
    //         const CDexOrderHeader& orderHeader = kv.first;

    //         const CChainId nAtChainId = orderHeader.GetChainId();
    //         const CDestination& destOrder = orderHeader.GetOrderAddress();
    //         const uint256& hashCoinPair = orderHeader.GetCoinPairHash();
    //         const uint8 nOwnerCoinFlag = orderHeader.GetOwnerCoinFlag();
    //         const uint64 nOrderNumber = orderHeader.GetOrderNumber();

    //         uint64 nCompRecordNumber = 0;
    //         auto it = mapDbDexOrder.find(orderHeader);
    //         if (it != mapDbDexOrder.end())
    //         {
    //             nCompRecordNumber = it->second.dexOrder.nCompleteOrderCount - kv.second.size();
    //         }

    //         for (const CCompDexOrderRecord& orderRecord : kv.second)
    //         {
    //             hnbase::CBufStream ssKey, ssValue;
    //             bytes btKey, btValue;

    //             ssKey << DB_HDEX_KEY_TYPE_TRIE_DEX_ORDER_COMPLETE_RECORD << BSwap32(nAtChainId) << destOrder << hashCoinPair << nOwnerCoinFlag << BSwap64(nOrderNumber) << BSwap64(nCompRecordNumber);
    //             ssKey.GetData(btKey);

    //             ssValue << orderRecord << nHeight << nSlot;
    //             ssValue.GetData(btValue);

    //             mapKv[btKey] = btValue;

    // #ifdef HDEX_OUT_TEST_LOG
    //             StdDebug("TEST", "HdexDB Add DexOrder: Complete record: at chainid: %d, order address: %s, coin pair: %s, owner coin flag: %d, order number: %lu, record number: %lu, peer address: %s, complete amount: %s, complete price: %s, at block: %s",
    //                      nAtChainId, destOrder.ToString().c_str(), hashCoinPair.ToString().c_str(), nOwnerCoinFlag, nOrderNumber, nCompRecordNumber,
    //                      orderRecord.destPeerOrder.ToString().c_str(), CoinToTokenBigFloat(orderRecord.nCompleteAmount).c_str(),
    //                      CoinToTokenBigFloat(orderRecord.nCompletePrice).c_str(), hashBlock.GetBhString().c_str());
    // #endif

    //             nCompRecordNumber++;
    //         }
    //     }

    for (const auto& kv : mapBlockProve)
    {
        auto funcAddCrossTransferRecord = [&](const CBlockCoinTransferProve& transProve, const CChainId nPeerChainId, const uint256& hashPeerAtBlock, const uint32 nIndex) {
            hnbase::CBufStream ssKey, ssValue;
            bytes btKey, btValue;

            ssKey << DB_HDEX_KEY_TYPE_TRIE_CROSS_TRANSFER_RECV << BSwap32(nPeerChainId) << transProve.destTransfer << hashPeerAtBlock << BSwap32(nIndex);
            ssKey.GetData(btKey);

            ssValue << transProve;
            ssValue.GetData(btValue);

            mapKv[btKey] = btValue;

#ifdef HDEX_OUT_TEST_LOG
            StdDebug("TEST", "HdexDB Add DexOrder: Block prove: cross transfer recv: local chainid: %d, peer chainid: %d, address: %s, coin symbol: %s, ori chainid: %d, dest chainid: %d, amount: %s, peer at block: %s",
                     nChainId, nPeerChainId, transProve.destTransfer.ToString().c_str(), transProve.strCoinSymbol.c_str(), transProve.nOriChainId,
                     transProve.nDestChainId, CoinToTokenBigFloat(transProve.nTransferAmount).c_str(), hashPeerAtBlock.GetBhString().c_str());
#endif
        };
        auto funcAddOrderProve = [&](const CBlockDexOrderProve& orderProve, const CChainId nPeerChainId, const uint256& hashPeerAtBlock) {
            const CDestination& destOrder = orderProve.destOrder;
            const CChainId nChainIdPeer = orderProve.nChainIdPeer;
            const uint256 hashCoinPair = CDexOrderHeader::GetCoinPairHashStatic(orderProve.strCoinSymbolOwner, orderProve.strCoinSymbolPeer);
            const uint8 nOwnerCoinFlag = CDexOrderHeader::GetOwnerCoinFlagStatic(orderProve.strCoinSymbolOwner, orderProve.strCoinSymbolPeer);
            const uint64 nOrderNumber = orderProve.nOrderNumber;

            CDexOrderHeader orderHeader(nPeerChainId, destOrder, hashCoinPair, nOwnerCoinFlag, nOrderNumber);
            CDexOrderBody orderBody(orderProve.strCoinSymbolOwner, orderProve.strCoinSymbolPeer, nChainIdPeer, orderProve.nOrderAmount, orderProve.nOrderPrice);

            auto mt = mapDbDexOrder.find(orderHeader);
            if (mt == mapDbDexOrder.end())
            {
                CDexOrderSave dexOrderBodyDb;
                if (GetPeerDexOrderProveDb(hashPrevRoot, orderHeader.GetChainId(), orderHeader.GetOrderAddress(), orderHeader.GetCoinPairHash(), orderHeader.GetOwnerCoinFlag(), orderHeader.GetOrderNumber(), dexOrderBodyDb))
                {
                    mt = mapDbDexOrder.insert(std::make_pair(orderHeader, dexOrderBodyDb)).first;
                }
            }
            if (mt != mapDbDexOrder.end())
            {
                CDexOrderSave& dexOrderSave = mt->second;
                CDexOrderBody& dbDexOrder = dexOrderSave.dexOrder;
                if (orderBody.nOrderAmount != dbDexOrder.nOrderAmount && orderBody.nOrderAmount >= dbDexOrder.nCompleteOrderAmount)
                {
                    dbDexOrder.nOrderAmount = orderBody.nOrderAmount;
                    dexOrderSave.nAtChainId = nPeerChainId;
                    dexOrderSave.hashAtBlock = hashPeerAtBlock;

                    funcAddPeerDexOrder(dbDexOrder.nCompleteOrderAmount >= dbDexOrder.nOrderAmount, orderHeader, dexOrderSave);

                    mapAddBlockProveOrder[orderHeader] = std::make_tuple(dbDexOrder, nPeerChainId, hashPeerAtBlock);
                }
            }
            else
            {
                CDexOrderSave dexOrderSave(orderBody, nPeerChainId, hashPeerAtBlock);

                funcAddPeerDexOrder(false, orderHeader, dexOrderSave);

                mapDbDexOrder[orderHeader] = dexOrderSave;

                //key: order header, value: 1: order body, 2: complete amount, 3: complete count, 4: at chainid, 5: at block
                //std::map<CDexOrderHeader, std::tuple<CDexOrderBody, CChainId, uint256>> mapAddBlockProveOrder;
                mapAddBlockProveOrder[orderHeader] = std::make_tuple(orderBody, nPeerChainId, hashPeerAtBlock);
            }

#ifdef HDEX_OUT_TEST_LOG
            StdDebug("TEST", "HdexDB Add DexOrder: Block prove: local chainid: %d, peer chainid: %d, order address: %s, symbol owner: %s, symbol peer: %s, owner coin flag: %d, order number: %lu, order amount: %s, order price: %s, at block: %s",
                     nChainId, nPeerChainId, destOrder.ToString().c_str(), orderProve.strCoinSymbolOwner.c_str(), orderProve.strCoinSymbolPeer.c_str(),
                     nOwnerCoinFlag, nOrderNumber, CoinToTokenBigFloat(orderProve.nOrderAmount).c_str(),
                     CoinToTokenBigFloat(orderProve.nOrderPrice).c_str(), hashPeerAtBlock.GetBhString().c_str());
#endif
        };
        auto funcAddConfirmRecvProve = [&](const uint256& hashPeerLastConfirmBlock, const CChainId nPeerChainId, const uint256& hashPeerAtBlock) {
            hnbase::CBufStream ssKey, ssValue;
            bytes btKey, btValue;

            ssKey << DB_HDEX_KEY_TYPE_TRIE_CROSS_RECV_CONFIRM_BLOCK << BSwap32(nPeerChainId);
            ssKey.GetData(btKey);

            ssValue << hashPeerAtBlock << hashPeerLastConfirmBlock;
            ssValue.GetData(btValue);

            mapKv[btKey] = btValue;

#ifdef HDEX_OUT_TEST_LOG
            StdDebug("TEST", "HdexDB Add DexOrder: Block prove: Confirm recv prove block: peer chainid: %d, hashPeerAtBlock: %s, hashPeerLastConfirmBlock: %s, hashBlock: %s",
                     nPeerChainId, hashPeerAtBlock.GetBhString().c_str(), hashPeerLastConfirmBlock.GetBhString().c_str(), hashBlock.GetBhString().c_str());
#endif
        };

        const CChainId nPeerChainId = kv.first;
        const CBlockProve& blockProve = kv.second;

        uint256 hashLastProveBlock;
        if (blockProve.hashPrevBlock != 0 && !blockProve.vPrevBlockCcProve.empty())
        {
            std::vector<std::pair<uint256, CBlockPrevProve>> vCacheProve;
            uint256 hashAtBlock = blockProve.hashPrevBlock;

            for (const CBlockPrevProve& prevProve : blockProve.vPrevBlockCcProve)
            {
                vCacheProve.push_back(std::make_pair(hashAtBlock, prevProve));
                hashAtBlock = prevProve.hashPrevBlock;
            }

            for (const auto& vd : boost::adaptors::reverse(vCacheProve))
            {
                const uint256& hashPeerAtBlock = vd.first;
                const CBlockPrevProve& prevProve = vd.second;
                if (!prevProve.proveCrosschain.IsNull())
                {
                    hashLastProveBlock = hashPeerAtBlock;

#ifdef HDEX_OUT_TEST_LOG
                    StdDebug("TEST", "HdexDB Add DexOrder: Block prove: prove block: prev prove, prev prove block: %s, peer at block: %s",
                             prevProve.proveCrosschain.GetPrevProveBlock().GetBhString().c_str(), hashPeerAtBlock.GetBhString().c_str());
#endif

                    auto& transProve = prevProve.proveCrosschain.GetCoinTransferProve();
                    for (uint32 i = 0; i < transProve.size(); i++)
                    {
                        funcAddCrossTransferRecord(transProve[i], nPeerChainId, hashPeerAtBlock, i);
                    }
                    for (const auto& kv : prevProve.proveCrosschain.GetDexOrderProve())
                    {
                        funcAddOrderProve(kv.second, nPeerChainId, hashPeerAtBlock);
                    }
                    for (const uint256& hashPeerLastConfirmBlock : prevProve.proveCrosschain.GetCrossConfirmRecvBlock())
                    {
                        funcAddConfirmRecvProve(hashPeerLastConfirmBlock, nPeerChainId, hashPeerAtBlock);
                    }
                }
            }
        }

        if (!blockProve.proveCrosschain.IsNull())
        {
            hashLastProveBlock = blockProve.hashBlock;

#ifdef HDEX_OUT_TEST_LOG
            StdDebug("TEST", "HdexDB Add DexOrder: Block prove: prove block: local prove, confirm block size: %lu, first prove block: %s, prev prove block: %s, local block: %s",
                     blockProve.proveCrosschain.GetCrossConfirmRecvBlock().size(), blockProve.GetFirstPrevBlockHash().GetBhString().c_str(),
                     blockProve.proveCrosschain.GetPrevProveBlock().GetBhString().c_str(), blockProve.hashBlock.GetBhString().c_str());
#endif

            auto& transProve = blockProve.proveCrosschain.GetCoinTransferProve();
            for (uint32 i = 0; i < transProve.size(); i++)
            {
                funcAddCrossTransferRecord(transProve[i], nPeerChainId, blockProve.hashBlock, i);
            }
            for (const auto& kv : blockProve.proveCrosschain.GetDexOrderProve())
            {
                funcAddOrderProve(kv.second, nPeerChainId, blockProve.hashBlock);
            }
            for (const uint256& hashPeerLastConfirmBlock : blockProve.proveCrosschain.GetCrossConfirmRecvBlock())
            {
                funcAddConfirmRecvProve(hashPeerLastConfirmBlock, nPeerChainId, blockProve.hashBlock);
            }
        }

        if (hashLastProveBlock != 0)
        {
            hnbase::CBufStream ssKey, ssValue;
            bytes btKey, btValue;

            ssKey << DB_HDEX_KEY_TYPE_TRIE_CROSS_LAST_PROVE_BLOCK << BSwap32(nPeerChainId);
            ssKey.GetData(btKey);

            ssValue << hashLastProveBlock;
            ssValue.GetData(btValue);

            mapKv[btKey] = btValue;

            mapPeerProveLastBlock[nPeerChainId] = hashLastProveBlock;

#ifdef HDEX_OUT_TEST_LOG
            StdDebug("TEST", "HdexDB Add DexOrder: Block prove: write last prove block: peer chainid: %d, last prove block: %s, prove block: %s",
                     nPeerChainId, hashLastProveBlock.GetBhString().c_str(), blockProve.hashBlock.GetBhString().c_str());
#endif
        }
    }

    if (hashPrevRoot == 0 && mapKv.empty())
    {
        AddPrevRoot(DB_HDEX_ROOT_TYPE_TRIE, hashPrevRoot, hashBlock, mapKv);
    }

    if (!dbTrie.AddNewTrie(hashPrevRoot, mapKv, hashNewRoot))
    {
        StdLog("CHdexDB", "Add dex order: Add new trie fail, prev block: %s, block: %s",
               hashPrevBlock.GetBhString().c_str(), hashBlock.GetBhString().c_str());
        return false;
    }

    if (!WriteTrieRoot(DB_HDEX_ROOT_TYPE_TRIE, hashBlock, hashNewRoot))
    {
        StdLog("CHdexDB", "Add dex order: Write trie root fail, prev block: %s, block: %s",
               hashPrevBlock.GetHex().c_str(), hashBlock.GetBhString().c_str());
        return false;
    }

    if (!UpdateDexOrderCache(hashPrevBlock, hashBlock, mapAddNewOrder, mapAddBlockProveOrder, mapCoinPairCompletePrice, mapUpdateCompOrder, mapPeerProveLastBlock))
    {
        StdLog("CHdexDB", "Add dex order: Update dex order cache fail,  block: %s", hashBlock.GetBhString().c_str());
        return false;
    }
    return true;
}

bool CHdexDB::GetDexOrder(const uint256& hashBlock, const CDestination& destOrder, const CChainId nChainIdOwner, const std::string& strCoinSymbolOwner, const std::string& strCoinSymbolPeer, const uint64 nOrderNumber, CDexOrderBody& dexOrder)
{
    CReadLock rlock(rwAccess);

    uint256 hashRoot;
    if (!ReadTrieRoot(DB_HDEX_ROOT_TYPE_TRIE, hashBlock, hashRoot))
    {
        StdLog("CHdexDB", "Get dex order: Read trie root fail, block: %s", hashBlock.GetBhString().c_str());
        return false;
    }

    const uint256 hashCoinPair = CDexOrderHeader::GetCoinPairHashStatic(strCoinSymbolOwner, strCoinSymbolPeer);
    const uint8 nOwnerCoinFlag = CDexOrderHeader::GetOwnerCoinFlagStatic(strCoinSymbolOwner, strCoinSymbolPeer);

    CDexOrderSave dexOrderDb;
    if (!GetDexOrderDb(hashRoot, nChainIdOwner, destOrder, hashCoinPair, nOwnerCoinFlag, nOrderNumber, dexOrderDb))
    {
        return false;
    }
    dexOrder = dexOrderDb.dexOrder;
    return true;
}

bool CHdexDB::GetDexCompletePrice(const uint256& hashBlock, const uint256& hashCoinPair, uint256& nCompletePrice)
{
    CReadLock rlock(rwAccess);

    uint256 hashRoot;
    if (!ReadTrieRoot(DB_HDEX_ROOT_TYPE_TRIE, hashBlock, hashRoot))
    {
        StdLog("CHdexDB", "Get dex complete price: Read trie root fail, block: %s", hashBlock.GetBhString().c_str());
        return false;
    }

    return GetDexCompletePriceDb(hashRoot, hashCoinPair, nCompletePrice);
}

bool CHdexDB::GetCompleteOrder(const uint256& hashBlock, const CDestination& destOrder, const CChainId nChainIdOwner, const std::string& strCoinSymbolOwner, const std::string& strCoinSymbolPeer, const uint64 nOrderNumber, uint256& nCompleteAmount, uint64& nCompleteOrderCount)
{
    CReadLock rlock(rwAccess);

    uint256 hashRoot;
    if (!ReadTrieRoot(DB_HDEX_ROOT_TYPE_TRIE, hashBlock, hashRoot))
    {
        StdLog("CHdexDB", "Get complete order: Read trie root fail, block: %s", hashBlock.GetBhString().c_str());
        return false;
    }

    const CChainId nChainId = CBlock::GetBlockChainIdByHash(hashBlock);
    const uint256 hashCoinPair = CDexOrderHeader::GetCoinPairHashStatic(strCoinSymbolOwner, strCoinSymbolPeer);
    const uint8 nOwnerCoinFlag = CDexOrderHeader::GetOwnerCoinFlagStatic(strCoinSymbolOwner, strCoinSymbolPeer);

    CDexOrderSave dexOrder;
    if (!GetDexOrderDb(hashRoot, nChainId, destOrder, hashCoinPair, nOwnerCoinFlag, nOrderNumber, dexOrder))
    {
        return false;
    }
    nCompleteAmount = dexOrder.dexOrder.nCompleteOrderAmount;
    nCompleteOrderCount = dexOrder.dexOrder.nCompleteOrderCount;
    return true;
}

bool CHdexDB::ListAddressDexOrder(const uint256& hashBlock, const CDestination& destOrder, const std::string& strCoinSymbolOwner, const std::string& strCoinSymbolPeer,
                                  const uint64 nBeginOrderNumber, const uint8 nGetStatus, const uint32 nGetCount, std::map<CDexOrderHeader, CDexOrderSave>& mapDexOrder)
{
    CReadLock rlock(rwAccess);

    switch (nGetStatus)
    {
    case CDexCompleteStatus::DCS_DEX_ORDER_UNCOMPLETED:
    case CDexCompleteStatus::DCS_DEX_ORDER_COMPLETED:
        if (!ListAddressDexOrderDb(hashBlock, destOrder, strCoinSymbolOwner, strCoinSymbolPeer, nBeginOrderNumber, nGetStatus, nGetCount, mapDexOrder))
        {
            StdLog("CHdexDB", "List address dex order: Get order fail, address: %s, block: %s", destOrder.ToString().c_str(), hashBlock.GetBhString().c_str());
            return false;
        }
        break;
    case CDexCompleteStatus::DCS_DEX_ORDER_ALL:
        if (!ListAddressDexOrderDb(hashBlock, destOrder, strCoinSymbolOwner, strCoinSymbolPeer, nBeginOrderNumber, CDexCompleteStatus::DCS_DEX_ORDER_UNCOMPLETED, nGetCount, mapDexOrder))
        {
            StdLog("CHdexDB", "List address dex order: Get uncompleted order fail, address: %s, block: %s", destOrder.ToString().c_str(), hashBlock.GetBhString().c_str());
            return false;
        }
        if (!ListAddressDexOrderDb(hashBlock, destOrder, strCoinSymbolOwner, strCoinSymbolPeer, nBeginOrderNumber, CDexCompleteStatus::DCS_DEX_ORDER_COMPLETED, nGetCount, mapDexOrder))
        {
            StdLog("CHdexDB", "List address dex order: Get completed order fail, address: %s, block: %s", destOrder.ToString().c_str(), hashBlock.GetBhString().c_str());
            return false;
        }
        break;
    default:
        StdLog("CHdexDB", "List address dex order: Get status error, status: %d, address: %s, block: %s", nGetStatus, destOrder.ToString().c_str(), hashBlock.GetBhString().c_str());
        return false;
    }
    return true;
}

bool CHdexDB::GetDexOrderMaxNumber(const uint256& hashBlock, const CDestination& destOrder, const std::string& strCoinSymbolOwner, const std::string& strCoinSymbolPeer, uint64& nMaxOrderNumber)
{
    CReadLock rlock(rwAccess);

    uint256 hashRoot;
    if (!ReadTrieRoot(DB_HDEX_ROOT_TYPE_TRIE, hashBlock, hashRoot))
    {
        StdLog("CHdexDB", "Get dex order max number: Read trie root fail, block: %s", hashBlock.GetBhString().c_str());
        return false;
    }

    const uint256 hashCoinPair = CDexOrderHeader::GetCoinPairHashStatic(strCoinSymbolOwner, strCoinSymbolPeer);
    const uint8 nOwnerCoinFlag = CDexOrderHeader::GetOwnerCoinFlagStatic(strCoinSymbolOwner, strCoinSymbolPeer);

    return GetDexOrderMaxNumberDb(hashRoot, CBlock::GetBlockChainIdByHash(hashBlock), destOrder, hashCoinPair, nOwnerCoinFlag, nMaxOrderNumber);
}

bool CHdexDB::GetPeerCrossLastBlock(const uint256& hashBlock, const CChainId nPeerChainId, uint256& hashLastProveBlock)
{
    CReadLock rlock(rwAccess);

    uint256 hashRoot;
    if (!ReadTrieRoot(DB_HDEX_ROOT_TYPE_TRIE, hashBlock, hashRoot))
    {
        StdLog("CHdexDB", "Get peer cross last block: Read trie root fail, block: %s", hashBlock.GetBhString().c_str());
        return false;
    }
    return GetPeerCrossLastBlockDb(hashRoot, nPeerChainId, hashLastProveBlock);
}

const SHP_MATCH_DEX CHdexDB::GetBlockMatchDex(const uint256& hashBlock)
{
    CReadLock rlock(rwAccess);

    SHP_CACHE_BLOCK_DEX_ORDER ptrDexOrderCache = LoadBlockDexOrderCache(hashBlock);
    if (!ptrDexOrderCache)
    {
        return nullptr;
    }
    return ptrDexOrderCache->GetMatchDex();
}

bool CHdexDB::GetMatchDexData(const uint256& hashBlock, std::map<uint256, CMatchOrderResult>& mapMatchResult)
{
    CReadLock rlock(rwAccess);

    SHP_CACHE_BLOCK_DEX_ORDER ptrDexOrderCache = LoadBlockDexOrderCache(hashBlock);
    if (!ptrDexOrderCache)
    {
        return false;
    }
    return ptrDexOrderCache->GetMatchDexResult(mapMatchResult);
}


    CDexOrderSave dexOrderDb;
    if (!GetDexOrderDb(hashRoot, nChainIdOwner, destOrder, hashCoinPair, nOwnerCoinFlag, nOrderNumber, dexOrderDb))
    {
        return false;
    }
    dexOrder = dexOrderDb.dexOrder;
    return true;
}

} // namespace storage
} // namespace hashahead
