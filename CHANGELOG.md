# Version 0.3.9 (2025-11-13)

### Features
- Add support for new EVM instructions, including TLOAD, TSTORE, MCOPY, BLOBHASH, BLOBBASEFEE instructions.
- Add state data pruning functionality.
- Add blockchain snapshot generation and restoration features.
- Add support for NAT UPnP and NAT-PMP.
- Add the ability to modify and cancel transactions in the transaction pool.
- Add new RPC APIs, including listtokenaddress and listtokentransaction.

### Fixs
- Fix some bugs in RPC API.
- Fix bug related to WETH9 transfers under Uniswap v3.
- Modified the ETH RPC block transaction list to include mint transactions, ensuring that every block contains at least one transaction.


# Version 0.3.8 (2025-05-10)

### Features
- Optimize the storage code of tracedb to reduce storage capacity.
- Optimize the loading efficiency of blockindex.
- Optimize the storage code for address transaction lists.
- Added listuservotebydelegate RPC API.
- Added makefork RPC API.
- Modified the sendfrom and getbalance RPC APIs.


# Version 0.3.7 (2025-02-28)

### Features
- Modify apy and scale precision.
- Modify the calculation method of tv gas.
- Modify recovery db.


# Version 0.3.6 (2025-01-18)

### Features
- Modify pledge vote function.
- Modify contract exec block number.
- Modify vmhost delegate call.
- Optimize contract call code.
- Add pledge voteq query RPC.
- Add deploy contract proxy.

### Fixs
- Fix owner code reward calc bug.
- Fix contract exec block number bug.


# Version 0.3.5 (2024-11-17)

### Features
- Modify eth rpc out

### Fixs
- Fix contract call value bug
- Fix get contract code bug
- Fix gas not enough without not save debug receipt bug


# Version 0.3.4 (2024-08-23)

### Features
- Add the following RPC API:
- txpool_content
- txpool_inspect
- txpool_contentFrom
- txpool_status
- eth_blobBaseFee
- eth_feeHistory
- eth_getAccount
- eth_getBlockReceipts
- eth_maxPriorityFeePerGas

### Fixs
- Fix the following RPC API for rsv bug:
- eth_getBlockByHash
- eth_getBlockByNumber
- eth_getTransactionByBlockHashAndIndex
- eth_getTransactionByBlockNumberAndIndex
- eth_getTransactionByHash


# Version 0.3.3 (2024-07-11)

### Features
- Add the following RPC API:
- debug_getBadBlocks
- debug_storageRangeAt
- debug_getTrieFlushInterval
- debug_traceBlock
- debug_traceBlockByHash
- debug_traceBlockByNumber
- debug_traceCall
- debug_traceTransaction
- getblockencode
- getblockdecode


# Version 0.3.2 (2024-06-03)

### Features
- Enable user fork and cancel user fork timevault.
- Add filter data file.
- Add timevault whitelist.
- AAdd fork stop and block create and locked.
- Modify fork redeem height.
- Disable no user fork stop.
- Modify fork context cache.


# Version 0.3.1 (2024-04-18)

### Features
- Modify estimate time vault gas.
- Modify block confirm verify.
- Modify filter block param.
- Add delegate multisig owner address.


# Version 0.3.0 (2024-03-04)

### Features
- Add cross chain transfer function.
- Add decentralized exchange functionality.
- Modify rpc get transaction receipt number type.

### Fixs
- Fix get transaction rpc v bug.


# Version 0.2.3 (2023-09-01)

### Features
- Modify min gas price.
- Add function address change.
- Add multiple address control pledge voting.
- Add delegate multisig owner address.

### Fixs
- Fix poa chain trust equal to 0 bug


# Version 0.2.2 (2023-08-09)

### Features
- Add proof data for verify blocks.
- Add address quantity statistics function.
- Add checkatbloomfilter rpc.
- Modify block hash struct to chainid height slot.
- Modify reward distribution programme.
- Modify mint reward and add reward tx statistics.

### Fixs
- Fix create tx gas bug


# Version 0.2.1 (2023-06-28)

### Features
- Support network channel function
- Optimize transaction pool algorithm

### Fixs
- Fix estimate gas bug
- Fix integer division zero bug


# Version 0.2.0 (2023-06-15)

### Features
- Support for EVM virtual machines
- Compatible with other public chain smart contracts and RPC Web3 development package, supported by Remix tools
- Support for new mortgage voting function
- Support account negative interest rate function

# Version 0.1.0 (2022-08-17)

### Features

- Implement POS consensus mechanism
- Support account model
- Support for WASM virtual machines
- Implement a decentralized smart contract library
- Smart Contract Code Author Incentive
- Implement a multi chain structure with limited extension, providing a consensus base chain and a functional ship chain to form a multi chain structure, which can be extended to a limited functional ship chain

# Version 0.0.1 (2021-09-20)

### Features
* baseline
