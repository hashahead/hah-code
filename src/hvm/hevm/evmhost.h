// Copyright (c) 2021-2025 The HashAhead developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HVM_EVMHOST_H
#define HVM_EVMHOST_H

#include <evmc/evmc.hpp>
#include <map>

#include "vmhostface.h"

namespace hashahead
{
namespace hvm
{

void onExOperation(const uint64_t steps,
                   const uint64_t pc,
                   const uint8_t instr,
                   const uint64_t newMemSize,
                   const uint64_t gasCost,
                   const uint64_t gas,
                   const evmc_bytes32 stack[],
                   const size_t stack_count,
                   const evmc_bytes32 mem[],
                   const size_t mem_count,
                   void* pUser);

class CHostCacheKv
{
public:
    CHostCacheKv() {}

    void SetCacheValue(const CDestination& dest, const uint256& key, const bytes& value)
    {
        cacheKv[dest][key] = value;
    }
    void SetTraceValue(const CDestination& dest, const uint256& key, const bytes& value)
    {
        traceKv[dest][key] = value;
    }
    bool GetValue(const CDestination& dest, const uint256& key, bytes& value) const
    {
        auto it = cacheKv.find(dest);
        if (it != cacheKv.end())
        {
            auto mt = it->second.find(key);
            if (mt != it->second.end())
            {
                value = mt->second;
                return true;
            }
        }
        return false;
    }
    bool GetDestCacheKv(const CDestination& dest, std::map<uint256, bytes>& out)
    {
        auto it = cacheKv.find(dest);
        if (it != cacheKv.end())
        {
            out = it->second;
            cacheKv.erase(it);
            return true;
        }
        return false;
    }
    bool GetDestTraceKv(const CDestination& dest, std::map<uint256, bytes>& out)
    {
        auto it = traceKv.find(dest);
        if (it != traceKv.end())
        {
            out = it->second;
            traceKv.erase(it);
            return true;
        }
        return false;
    }

protected:
    std::map<CDestination, std::map<uint256, bytes>> cacheKv;
    std::map<CDestination, std::map<uint256, bytes>> traceKv;
};
using SHP_HOST_CACHE_KV = shared_ptr<CHostCacheKv>;

class CEvmHost : public evmc::Host
{
private:
    evmc_tx_context tx_context;
    CVmHostFaceDB& dbHost;
    bool fTraceVmLog = false;

public:
    uint32 nDepth = 0;
    SHP_HOST_CACHE_KV pCacheKv;

    std::vector<CTransactionLogs> vLogs;
    std::map<CDestination, std::map<uint256, bytes>> mapOldKeyValue; // key1: address, key2: key, value: old value

    bool fFhxHeightBranch001 = false;
    bool fFhxHeightBranch002 = false;

public:
    static CDestination AddressToDestination(const evmc::address& addr);
    static evmc::address DestinationToAddress(const CDestination& dest);

    bool GetCacheKv(std::map<uint256, bytes>& out)
    {
        return pCacheKv->GetDestCacheKv(dbHost.GetStorageContractAddress(), out);
    }
    bool GetTraceKv(std::map<uint256, bytes>& out)
    {
        return pCacheKv->GetDestTraceKv(dbHost.GetStorageContractAddress(), out);
    }
    void AddContractFirstReceipt(const CDestination& from, const CDestination& to, const CDestination& destContractIn, const uint64 nTxGasLimit,
                                 const uint256& nTxAmount, const bytes& btInput, const bool fCreateContract, const evmc::result& result);

public:
    CEvmHost(const evmc_tx_context& _tx_context, CVmHostFaceDB& dbHostIn, SHP_HOST_CACHE_KV pCacheKvIn = nullptr);

    bool account_exists(const evmc::address& addr) const noexcept final;

    evmc::bytes32 get_storage(const evmc::address& addr, const evmc::bytes32& key) const noexcept final;

    evmc_storage_status set_storage(const evmc::address& addr,
                                    const evmc::bytes32& key,
                                    const evmc::bytes32& value) noexcept final;

    evmc::uint256be get_balance(const evmc::address& addr) const noexcept final;

    size_t get_code_size(const evmc::address& addr) const noexcept;

    evmc::bytes32 get_code_hash(const evmc::address& addr) const noexcept;

    size_t copy_code(const evmc::address& addr,
                     size_t code_offset,
                     uint8_t* buffer_data,
                     size_t buffer_size) const noexcept;

    void selfdestruct(const evmc::address& addr, const evmc::address& beneficiary) noexcept;

    evmc::result call(const evmc_message& msg) noexcept final;

    evmc_tx_context get_tx_context() const noexcept final;

    evmc::bytes32 get_block_hash(int64_t number) const noexcept final;

    void emit_log(const evmc::address& addr,
                  const uint8_t* data,
                  size_t data_size,
                  const evmc::bytes32 topics[],
                  size_t topics_count) noexcept final;

protected:
    evmc::result Create(const evmc_message& msg);
    evmc::result Call(const evmc_message& msg);
};

} // namespace hvm
} // namespace hashahead

#endif // HVM_EVMHOST_H
