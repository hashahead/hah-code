// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HASHAHEAD_MODE_MODULE_TYPE_H
#define HASHAHEAD_MODE_MODULE_TYPE_H

namespace hashahead
{
// module type
enum class EModuleType
{
    LOCK,             // lock file
    BLOCKMAKER,       // CBlockMaker
    COREPROTOCOL,     // CCoreProtocol
    DISPATCHER,       // CDispatcher
    HTTPGET,          // CHttpGet
    HTTPSERVER,       // CHttpServer
    NETCHANNEL,       // CNetChannel
    DELEGATEDCHANNEL, // CDelegatedChannel
    BLOCKCHANNEL,     // CBlockChannel
    CERTTXCHANNEL,    // CCertTxChannel
    USERTXCHANNEL,    // CUserTxChannel
    BLOCKVOTECHANNEL, // CBlockVoteChannel
    BLOCKCROSSPROVE,  // CBlockCrossProveChannel
    SNAPSHOTDOWN,     // CSnapshotDownChannel
    NETWORK,          // CNetwork
    RPCCLIENT,        // CRPCClient
    RPCMODE,          // CRPCMod
    SERVICE,          // CService
    TXPOOL,           // CTxPool
    WALLET,           // CWallet
    BLOCKCHAIN,       // CBlockChain
    CONSENSUS,        // CConsensus
    FORKMANAGER,      // CForkManager
    DATASTAT,         // CDataStat
    RECOVERY,         // CRecovery
    WSSERVICE,        // CWsService
    BLOCKFILTER,      // CBlockFilter
    PRUNEDB,          // CPruneDb
    CHAINSNAPSHOT,    // CChainSnapshot
    NATPORTMAPPING,   // CNatPortMapping
};

} // namespace hashahead

#endif // HASHAHEAD_MODE_MODULE_TYPE_H
