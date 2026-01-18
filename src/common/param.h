// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HASHAHEAD_PARAM_H
#define HASHAHEAD_PARAM_H

#include "destination.h"
#include "uint256.h"

namespace hashahead
{

///////////////////////////////////
extern bool TESTNET_FLAG;
extern bool FASTTEST_FLAG;
extern bool TESTMAINNET_FLAG;
extern CChainId GENESIS_CHAINID;
extern uint32 NETWORK_NETID;

#define GET_PARAM(MAINNET_PARAM, TESTNET_PARAM) (TESTNET_FLAG ? TESTNET_PARAM : MAINNET_PARAM)
#define GET_FAST_PARAM(MAINNET_PARAM, TESTNET_PARAM, FASTTEST_PARAM) (TESTNET_FLAG ? (FASTTEST_FLAG ? FASTTEST_PARAM : TESTNET_PARAM) : MAINNET_PARAM)
#define GET_TESTMAINNET_PARAM(MAINNET_PARAM, TESTMAINNET_PARAM, TESTNET_PARAM) (TESTNET_FLAG ? TESTNET_PARAM : (TESTMAINNET_FLAG ? TESTMAINNET_PARAM : MAINNET_PARAM))

///////////////////////////////////
static const uint32 TOKEN_DECIMAL_DIGIT = 18;
static const uint256 COIN(10, TOKEN_DECIMAL_DIGIT);
static const uint256 MIN_GAS_PRICE_0(10, 12);
static const uint256 MIN_GAS_PRICE(10, 10);
static const uint256 TX_BASE_GAS(21000);
static const uint256 DATA_GAS_PER_BYTE(50);
static const uint256 DEF_TX_GAS_LIMIT(9000000);
static const uint256 MAX_MONEY = COIN * uint256(1000000000000L);
inline bool MoneyRange(const uint256& nValue)
{
    return (nValue <= MAX_MONEY);
}
static const uint256 MAX_REWARD_MONEY = COIN * 1000000;
inline bool RewardRange(const uint256& nValue)
{
    return (nValue <= MAX_REWARD_MONEY);
}

static const unsigned int MAX_BLOCK_SIZE = 1024 * 1024 * 2;
static const unsigned int MAX_TX_SIZE = (MAX_BLOCK_SIZE / 20);
static const unsigned int MAX_SIGNATURE_SIZE = 2048;
static const unsigned int MAX_TX_INPUT_COUNT = (MAX_TX_SIZE - MAX_SIGNATURE_SIZE - 4) / 33;
static const int64 MAX_BLOCK_GAS_LIMIT = 10000 * 10000 * 10;

#define BLOCK_TARGET_SPACING GET_FAST_PARAM(10, 5, 1)
#define EXTENDED_BLOCK_SPACING GET_PARAM(1, 1)

#define DAY_HEIGHT_MAINNET (3600 * 24 / BLOCK_TARGET_SPACING)
#define DAY_HEIGHT_TESTNET (360 / BLOCK_TARGET_SPACING)
#define DAY_HEIGHT GET_PARAM(DAY_HEIGHT_MAINNET, DAY_HEIGHT_TESTNET)

#define WAIT_AGREEMENT_PUBLISH_TIMEOUT GET_FAST_PARAM(10, 5, 1)
#define MAX_SUBMIT_POW_TIMEOUT GET_FAST_PARAM(30, 20, 2)

#define DEF_GENESIS_CHAINID GET_PARAM(100, 101) // 0x64, 0x65

#define MAX_FILTER_CACHE_COUNT GET_PARAM(10000, 100)
#define FILTER_DEFAULT_TIMEOUT GET_PARAM(600, 3600)

///////////////////////////////////
// DELEGATE
#define CONSENSUS_DISTRIBUTE_INTERVAL GET_PARAM(5, 3) //15
#define CONSENSUS_ENROLL_INTERVAL GET_PARAM(10, 6)    //30
#define CONSENSUS_INTERVAL (CONSENSUS_DISTRIBUTE_INTERVAL + CONSENSUS_ENROLL_INTERVAL + 1)

#define MAX_DELEGATE_THRESH (21)

///////////////////////////////////
// FORK
const uint256 MORTGAGE_BASE = 100000 * COIN;
#define MORTGAGE_DECAY_CYCLE_MAINNET (DAY_HEIGHT_MAINNET * 90)
#define MORTGAGE_DECAY_CYCLE_TESTNET (DAY_HEIGHT_TESTNET * 10)
#define MORTGAGE_DECAY_CYCLE GET_PARAM(MORTGAGE_DECAY_CYCLE_MAINNET, MORTGAGE_DECAY_CYCLE_TESTNET)
const double MORTGAGE_DECAY_QUANTITY = 0.5; // decay quantity

#define MIN_CREATE_FORK_INTERVAL_HEIGHT GET_PARAM(30, 0)
#define MAX_JOINT_FORK_INTERVAL_HEIGHT GET_PARAM(DAY_HEIGHT_MAINNET, 0x7FFFFFFF)

///////////////////////////////////
// CORE
static const int64 MAX_CLOCK_DRIFT = 80;

static const uint256 BBCP_TOKEN_INIT = 100000 * 10000 * COIN;
static const uint256 BBCP_REWARD_INIT = 357 * COIN;
#define BBCP_REWARD_HALVE_CYCLE (DAY_HEIGHT * 365 * 4)

static const uint256 DELEGATE_PROOF_OF_STAKE_ENROLL_MINIMUM_AMOUNT = 100 * 10000 * COIN;
static const uint256 DELEGATE_PROOF_OF_STAKE_ENROLL_MAXIMUM_AMOUNT_MAINNET = 30000 * 10000 * COIN;
#define DELEGATE_PROOF_OF_STAKE_ENROLL_MAXIMUM_AMOUNT GET_PARAM(DELEGATE_PROOF_OF_STAKE_ENROLL_MAXIMUM_AMOUNT_MAINNET, BBCP_TOKEN_INIT)
static const uint256 DELEGATE_PROOF_OF_STAKE_UNIT_AMOUNT = COIN;
static const uint256 DELEGATE_PROOF_OF_STAKE_MIN_VOTE_AMOUNT = COIN;

#define VOTE_REWARD_DISTRIBUTE_HEIGHT DAY_HEIGHT

#define VOTE_REDEEM_HEIGHT (DAY_HEIGHT * 3)

#define VOTE_REWARD_LOCK_DAY_HEIGHT DAY_HEIGHT
#define VOTE_REWARD_LOCK_DAYS GET_PARAM(180, 6)

#define FUNCTION_TX_GAS_BASE (200)
#define FUNCTION_TX_GAS_TRANS (2300)
#define REWARD_TX_GAS_LIMIT (300000)
#define PLEDGE_REDEEM_TX_GAS_LIMIT (300000)

static const int64 MINT_REWARD_PER = 10000;
static const uint256 CODE_REWARD_USED(50);
static const uint256 CODE_REWARD_PER(100);

static const bool fEnableContractCodeVerify = false;
static const bool fEnableStakeVote = false;
static const bool fEnableStakePledge = true;

// pledge reward rule: start height, rule type, days, reward rate(base: 10000)
// mainnet
static const std::map<uint32, std::map<uint16, std::pair<uint32, uint32>>> mapPledgeRewardRuleMainnet = {
    { 0,
      { { 1, { 365, 10000 } },
        { 2, { 180, 9000 } },
        { 3, { 90, 8000 } },
        { 4, { 30, 7000 } },
        { 5, { 7, 6000 } } } }
};
// testnet
static const std::map<uint32, std::map<uint16, std::pair<uint32, uint32>>> mapPledgeRewardRuleTestnet = {
    { 0,
      { { 1, { 12, 10000 } },
        { 2, { 9, 9000 } },
        { 3, { 6, 8000 } },
        { 4, { 3, 7000 } },
        { 5, { 1, 6000 } } } }
};
#define PLEDGE_REWARD_RULE GET_PARAM(mapPledgeRewardRuleMainnet, mapPledgeRewardRuleTestnet)
static const uint32 PLEDGE_REWARD_PER = 10000;

#define PLEDGE_SURPLUS_REWARD_ADDRESS GET_TESTMAINNET_PARAM(PLEDGE_SURPLUS_REWARD_ADDRESS_MAINNET, PLEDGE_SURPLUS_REWARD_ADDRESS_TESTNET, PLEDGE_SURPLUS_REWARD_ADDRESS_TESTNET)
#define TIME_VAULT_TO_ADDRESS GET_TESTMAINNET_PARAM(TIME_VAULT_TO_ADDRESS_MAINNET, TIME_VAULT_TO_ADDRESS_TESTNET, TIME_VAULT_TO_ADDRESS_TESTNET)
#define PROJECT_PARTY_REWARD_TO_ADDRESS GET_TESTMAINNET_PARAM(PROJECT_PARTY_REWARD_TO_ADDRESS_MAINNET, PROJECT_PARTY_REWARD_TO_ADDRESS_TESTNET, PROJECT_PARTY_REWARD_TO_ADDRESS_TESTNET)
#define FOUNDATION_REWARD_TO_ADDRESS GET_TESTMAINNET_PARAM(FOUNDATION_REWARD_TO_ADDRESS_MAINNET, FOUNDATION_REWARD_TO_ADDRESS_TESTNET, FOUNDATION_REWARD_TO_ADDRESS_TESTNET)

#define FUNCTION_ID_PLEDGE_SURPLUS_REWARD_ADDRESS 0x01
#define FUNCTION_ID_TIME_VAULT_TO_ADDRESS 0x02
#define FUNCTION_ID_PROJECT_PARTY_REWARD_TO_ADDRESS 0x03
#define FUNCTION_ID_FOUNDATION_REWARD_TO_ADDRESS 0x04

static const std::map<uint32, std::string> mapFunctionAddressName = {
    { FUNCTION_ID_PLEDGE_SURPLUS_REWARD_ADDRESS, std::string("Pledge surplus reward address") },
    { FUNCTION_ID_TIME_VAULT_TO_ADDRESS, std::string("Time vault to address") },
    { FUNCTION_ID_PROJECT_PARTY_REWARD_TO_ADDRESS, std::string("Project party reward address") },
    { FUNCTION_ID_FOUNDATION_REWARD_TO_ADDRESS, std::string("Foundation reward address") },
};

// TimeVault
static const uint256 TIME_VAULT_RATE = (uint64)365 * 24 * 3600 * 100;
static const uint32 ESTIMATE_TIME_VAULT_TS = 3600;
static const uint32 GIVE_TIME_VAULT_TS = 3600 * 24;

// Reward distribution ratio
static const bool REWARD_DISTRIBUTION_RATIO_ENABLE = true;
static const uint32 REWARD_DISTRIBUTION_RATIO_PROJECT_PARTY = 4200;
static const uint32 REWARD_DISTRIBUTION_RATIO_FOUNDATION = 4000;
static const uint32 REWARD_DISTRIBUTION_PER = 10000;

///////////////////////////////////
enum ConsensusMethod
{
    CM_MPVSS = 0,
    CM_CRYPTONIGHT = 1,
    CM_MAX
};

///////////////////////////////////
// FUNCTOIN
static const std::map<uint256, std::map<int, uint256>> mapCheckPointsList_Mainnet = {
    { uint256("0xf90329f7f7f6915e588970aeb25e5d83e1ad1670e6fc839ed6322f231e0ae589"), //Genesis
      { { 0, uint256("0xf90329f7f7f6915e588970aeb25e5d83e1ad1670e6fc839ed6322f231e0ae589") } } }
};
static const std::map<uint256, std::map<int, uint256>> mapCheckPointsList_Testnet = {
    { uint256("0xc9f7f7948af1520cec5eebebb91c5ade8d3c061abaa54e9819ed0b77904f1ded"), //Genesis
      { { 0, uint256("0xc9f7f7948af1520cec5eebebb91c5ade8d3c061abaa54e9819ed0b77904f1ded") } } }
};
#define mapCheckPointsList GET_PARAM(mapCheckPointsList_Mainnet, mapCheckPointsList_Testnet)

///////////////////////////////////
static const CDestination FUNCTION_BLACKHOLE_ADDRESS("0x0000000000000000000000000000000000000001");
static const CDestination FUNCTION_CONTRACT_ADDRESS("0x00000000000000000000000000000000000000A1");

inline bool isFunctionContractAddress(const CDestination& _addr)
{
    return (_addr == FUNCTION_CONTRACT_ADDRESS);
}
bytes getFunctionContractCreateCode();
bytes getFunctionContractRuntimeCode();

static const uint32 DATA_PAGE_SIZE = 32;

///////////////////////////////////
std::string CoinToTokenBigFloat(const uint256& nCoin);
bool TokenBigFloatToCoin(const std::string& strToken, uint256& nCoin);

} // namespace hashahead

#endif //HASHAHEAD_PARAM_H
