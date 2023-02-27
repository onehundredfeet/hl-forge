#!/bin/sh
ARCH=x86_64
#ARCH=arm64
BUILDER="make"
TARGET=hl
#TARGET=jvm
CONFIG=Debug

while getopts b:c:a:t: flag
do
    case "${flag}" in
        b) BUILDER=${OPTARG};;
        c) CONFIG=${OPTARG};;
        a) ARCH=${OPTARG};;
        t) TARGET=${OPTARG};;
    esac
done


pushd build/${TARGET}/${ARCH}/${CONFIG}
${BUILDER}
popd

