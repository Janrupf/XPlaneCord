#!/usr/bin/env bash

set -e

if [[ -x "$(command -v clang)" ]] && [[ -x "$(command -v clang++)" ]]; then
    clang_available=true
    echo "-- Will use clang and clang++ for linux build."
else
    clang_available=false
fi

if [[ -x "$(command -v ninja)" ]] && [[ $(grep Ninja <<< `cmake -G 2>&1`) ]]; then
    ninja_available=true
    echo "-- Will use ninja generator for faster build"
fi

function build {
    mkdir "build-$1"
    cd "build-$1"
    if (( $# < 4 )); then
        bit_32=$2
        arch=""
        name=$3
        linux_build=true
    else
        arch="$2-"
        bit_32=$3
        name=$4
        linux_build=false
    fi

    if [[ "$bit_32" = true ]]; then
        bit_option="-D32_BIT=ON"
    else
        bit_option="-D32_BIT=OFF"
    fi

    if [[ "${ninja_available}" = true ]]; then
        generator='-GNinja'
    fi

    if [[ "${linux_build}" = true ]] && [[ "${clang_available}" = true ]]; then
        c_compiler="-DCMAKE_C_COMPILER="$(which clang)
        cxx_compiler="-DCMAKE_CXX_COMPILER="$(which clang++)
    fi

    "${arch}cmake" ../.. ${bit_option} "-DCMAKE_BUILD_TYPE=Release" ${c_compiler} ${cxx_compiler} ${generator}

    if [[ "${ninja_available}" = true ]]; then
        ninja
    else
        make -j4
    fi

    strip_command="${arch}strip"
    if [[ -x "$(command -v ${strip_command})" ]]; then
        ${strip_command} XPlaneCord.xpl
    fi

    cd ..

    if [[ "${bit_32}" = true ]]; then
        mv "build-$1/XPlaneCord.xpl" "XPlaneCord/32/${name}.xpl"
    else
        mv "build-$1/XPlaneCord.xpl" "XPlaneCord/64/${name}.xpl"
    fi
}


cd deploy
rm -rf *

mkdir XPlaneCord
mkdir XPlaneCord/64
mkdir XPlaneCord/32

build linux-64 false lin
build linux-32 true lin
build windows-64 x86_64-w64-mingw32 false win
build windows-32 i686-w64-mingw32 true win
build osx-64 x86_64-apple-darwin15 false mac
build osx-32 i386-apple-darwin15 true mac

zip -r XPlaneCord.zip XPlaneCord/