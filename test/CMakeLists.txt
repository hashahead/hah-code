# ------------------------------------------------------------------------------
# CMake file for HashAhead
#
# Copyright (c) 2021-2023 The HashAhead developers
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
# ------------------------------------------------------------------------------

include_directories(${CMAKE_CURRENT_BINARY_DIR}/../src/blockchain)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/../src/jsonrpc ../src/jsonrpc)
include_directories(../src/wvm/wasm-edge/include)

set(sources
    test_big_main.cpp
    test_big.h test_big.cpp
    uint256_tests.cpp
    rpc_tests.cpp
    version_tests.cpp
    mpvss_tests.cpp
    ipv6_tests.cpp
    crypto_tests.cpp
    delegate_tests.cpp
    eth_test.cpp
    storage_tests.cpp
    structure_tests.cpp
    util_tests.cpp
    slowhash_tests.cpp
    triedb_tests.cpp
    bloomfilter_tests.cpp
    evmc/evmcTest.cpp
    evmc/example_host.cpp
)

add_executable(test_big ${sources})

include_directories(../src/blockchain ../src/hnbase ../src/crypto ../src/common ../src/storage ../src/network ../src/mpvss ../src/delegate ../src/wvm/utils ../src/heth ../src/hvm/hevm ../src/hvm/hwvm ../src/hvm/vface)

target_link_libraries(test_big
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    OpenSSL::SSL
    OpenSSL::Crypto
    mpvss
    delegate
    crypto
    common
    blockchain
    hnbase
    storage
    devcore
    devcrypto
    ethcore
    devcommon
    evm
    ssvmCommon
    ssvmEVMCUtilEVMCLoader
    ssvm-evmc
    hevm
    hwvm
    vface
    ${Boost_LOG_LIBRARY}
)

add_executable(wasm_test evmc/test.cpp evmc/example_host.cpp)

target_link_libraries(wasm_test
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    OpenSSL::SSL
    OpenSSL::Crypto
    mpvss
    delegate
    crypto
    common
    blockchain
    hnbase
    storage
    ssvmCommon
    ssvmEVMCUtilEVMCLoader
    ssvm-evmc
    ${Boost_LOG_LIBRARY}
)

add_executable(wasm_erc20 evmc/erc20.cpp evmc/example_host.cpp)

target_link_libraries(wasm_erc20
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    OpenSSL::SSL
    OpenSSL::Crypto
    mpvss
    delegate
    crypto
    common
    blockchain
    hnbase
    storage
    ssvmCommon
    ssvmEVMCUtilEVMCLoader
    ssvm-evmc
    ${Boost_LOG_LIBRARY}
)

add_executable(test_ctsdb test_big_main.cpp test_big.h test_big.cpp ctsdb_test.cpp)

target_link_libraries(test_ctsdb
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    OpenSSL::SSL
    OpenSSL::Crypto
    mpvss
    common
    crypto
    storage
    blockchain
)
