# ------------------------------------------------------------------------------
# CMake file for HashAhead
#
# Copyright (c) 2021-2025 The HashAhead developers
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
# ------------------------------------------------------------------------------

set(sources
    memvmhost.cpp memvmhost.h
    vmhostface.h
)

add_library(vface ${sources})

include_directories(../../hnbase ../../crypto ../../common)

target_link_libraries(vface
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
)
