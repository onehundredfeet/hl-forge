#!/bin/sh


#!/bin/sh
ARCH=x86_64
#ARCH=arm64
BUILDER="ninja"
TARGET=hl
#TARGET=jvm
CONFIG=Debug
PATH_TO_IDL="ext/idl"

while getopts p:b:c:a:t:i: flag
do
    case "${flag}" in
        b) BUILDER=${OPTARG};;
        c) CONFIG=${OPTARG};;
        a) ARCH=${OPTARG};;
        t) TARGET=${OPTARG};;
        i) PATH_TO_IDL=${OPTARG};;
    esac
done


GENERATOR=${BUILDER}

if [ ${BUILDER} = "make" ]; then
    GENERATOR="Unix Makefiles"
fi

if [ ${BUILDER} = "ninja" ]; then
    GENERATOR="Ninja"
fi



mkdir -p build/${TARGET}/${ARCH}/${CONFIG}
mkdir -p installed/${TARGET}/${ARCH}/${CONFIG}

mkdir -p build/${TARGET}/${ARCH}/${CONFIG}
#pushd build/${TARGET}/${ARCH}/${CONFIG}


cmake -G${GENERATOR} -DCMAKE_BUILD_TYPE=${CONFIG} -DCMAKE_INSTALL_PREFIX=installed/${TARGET}/${ARCH}/${CONFIG}/shaderc -B build/${TARGET}/${ARCH}/${CONFIG}/shaderc -DSHADERC_SKIP_INSTALL=ON -DSHADERC_SKIP_TESTS=ON -DSHADERC_SKIP_EXAMPLES=ON -DSHADERC_SKIP_COPYRIGHT_CHECK=ON -DSHADERC_SPIRV_HEADERS_DIR=${PWD}/ext/SPIRV-Headers -DSHADERC_SPIRV_TOOLS_DIR=${PWD}/ext/spirv -DSHADERC_GLSLANG_DIR=${PWD}/ext/glslang ext/shaderc
cmake --build build/${TARGET}/${ARCH}/${CONFIG}/shaderc

cmake -G${GENERATOR} -DCMAKE_BUILD_TYPE=${CONFIG} -DCMAKE_INSTALL_PREFIX=installed/${TARGET}/${ARCH}/${CONFIG}/shaderc -B build/${TARGET}/${ARCH}/${CONFIG}/spirv-cross  -DSPIRV_CROSS_STATIC=ON -DSPIRV_CROSS_SHARED=OFF -DSPIRV_CROSS_CLI=OFF -DSPIRV_CROSS_ENABLE_TESTS=OFF -DSPIRV_CROSS_SKIP_INSTALL=ON ext/spirv-cross
cmake --build build/${TARGET}/${ARCH}/${CONFIG}/spirv-cross



cmake -G${GENERATOR} -DTARGET_ARCH=${ARCH} -DTARGET_HOST=${TARGET} -DCMAKE_BUILD_TYPE=${CONFIG}  -DCMAKE_INSTALL_PREFIX=installed/${TARGET}/${ARCH}/${CONFIG} -DPATH_TO_IDL=${PATH_TO_IDL} -B build/${TARGET}/${ARCH}/${CONFIG}
cmake --build build/${TARGET}/${ARCH}/${CONFIG}

echo Run the following to rebuild or run rebuild.sh
echo "cmake --build build/${TARGET}/${ARCH}/${CONFIG}/shaderc"
echo "cmake --build build/${TARGET}/${ARCH}/${CONFIG}/spirv-cross"
echo "cmake --build build/${TARGET}/${ARCH}/${CONFIG}"