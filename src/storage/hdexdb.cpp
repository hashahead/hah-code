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

} // namespace storage
} // namespace hashahead
