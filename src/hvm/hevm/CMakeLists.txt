# ------------------------------------------------------------------------------
# CMake file for HashAhead
#
# Copyright (c) 2021-2025 The HashAhead developers
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
# ------------------------------------------------------------------------------

set(sources
    evmexec.cpp evmexec.h
    evmhost.cpp evmhost.h
    precompiled.cpp precompiled.h
    hutil.cpp hutil.h
)

add_library(hevm ${sources})

include_directories(../../hnbase ../../crypto ../../common ../../heth/evmc/include ../vface)

target_link_libraries(hevm
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    ${Boost_DATE_TIME_LIBRARY}
    ${Boost_TIMER_LIBRARY}
    ${Boost_CHRONO_LIBRARY}
    ${Boost_REGEX_LIBRARY}
    hnbase
    crypto
    common
    snappy
    devcrypto
    aleth-interpreter
)
