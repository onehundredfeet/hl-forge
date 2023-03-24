#!/bin/sh
PROJECT=forge

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

cmake --install build/${TARGET}/${ARCH}/${CONFIG}

rm -f ${PROJECT}.hdll
cp -f installed/${TARGET}/${ARCH}/${CONFIG}/lib/${PROJECT}.* .

