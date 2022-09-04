#!/bin/sh

if [ -z "$1" ]
then
      CONFIG=Debug
else
      CONFIG=$1
fi

PROJECT=forge

pushd build/${CONFIG}/hl-${PROJECT}
ninja install
popd
