#!/bin/sh
ARCH=x86_64
#ARCH=arm64
TARGET=hl
#TARGET=jvm
CONFIG=Debug

while getopts c:a:t: flag
do
    case "${flag}" in
        c) CONFIG=${OPTARG};;
        a) ARCH=${OPTARG};;
        t) TARGET=${OPTARG};;
    esac
done

cmake --build build/${TARGET}/${ARCH}/${CONFIG}/shaderc
cmake --build build/${TARGET}/${ARCH}/${CONFIG}/spirv-cross
cmake --build build/${TARGET}/${ARCH}/${CONFIG}
