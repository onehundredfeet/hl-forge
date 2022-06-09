cmake_minimum_required(VERSION 3.10)
project (forge C CXX OBJC OBJCXX)


if (NOT FORGE_ROOT) 
set(FORGE_ROOT "../forge")
endif()

if (NOT FORGE_SRC_DIR) 
set(FORGE_SRC_DIR "${FORGE_ROOT}/source")
endif()
if (NOT FORGE_INC_DIR) 
set(FORGE_INC_DIR "${FORGE_ROOT}/include")
endif()

if (NOT HL_INCLUDE_DIR) 
    set(HL_INCLUDE_DIR "/usr/local/include")
endif()

if (NOT HL_LIB_DIR) 
    set(HL_LIB_DIR "/usr/local/lib")
endif()

if (NOT COMMON_LIB_DIR) 
    set(COMMON_LIB_DIR "/usr/local/lib")
endif()

if (NOT HDLL_DESTINATION) 
    set(HDLL_DESTINATION "/usr/local/lib")
endif()

if (NOT FORGE_LIB_DIR)
    set(FORGE_LIB_DIR "ext/forge")
    set(FORGE_LIB_INC "${FORGE_LIB_DIR}/Common_3")
    set(FORGE_LIB_SRC "${FORGE_LIB_DIR}/Common_3")
    set(FORGE_OS_SRC "${FORGE_LIB_DIR}/Common_3/OS")
    set(FORGE_TP_SRC "${FORGE_LIB_DIR}/Common_3/ThirdParty/OpenSource")
endif()


set( FORGE_OS_SRCS 
    ${FORGE_OS_SRC}/Logging/Log.c
    ${FORGE_OS_SRC}/Darwin/DarwinThread.c
    ${FORGE_OS_SRC}/Darwin/DarwinLog.c
    ${FORGE_OS_SRC}/Darwin/CocoaFileSystem.mm
    ${FORGE_OS_SRC}/FileSystem/FileSystem.cpp
    ${FORGE_OS_SRC}/FileSystem/UnixFileSystem.cpp
    ${FORGE_OS_SRC}/MemoryTracking/MemoryTracking.c
)

set (FORGE_TP_SRCS
#    ${FORGE_TP_SRC}/FluidStudios/MemoryManager/mmgr.c
)

add_library(forge.hdll SHARED
src/hl-idl-helpers.cpp
src/forge.cpp
src/hl-forge.mm
${FORGE_OS_SRCS}
${FORGE_TP_SRCS}
)

set_target_properties(forge.hdll
PROPERTIES
PREFIX ""
OUTPUT_NAME forge
SUFFIX .hdll
)

set_property(TARGET forge.hdll PROPERTY CXX_STANDARD 17)

cmake_policy(SET CMP0015 NEW)

target_include_directories(forge.hdll
PRIVATE
${FORGE_INC_DIR}
${FORGE_LIB_INC}
${HL_INCLUDE_DIR}
)

find_library( METAL_LIB Metal)
find_library( QUARTZ_LIB QuartzCore)
find_library( FOUNDATION_LIB Foundation)

if (UNIX)
    # Some special flags are needed for GNU GCC compiler
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fPIC  -O3  -fpermissive")
    #not sure why the ${HL_LIB_DIR} is necessary given the above.
    SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -shared  ")

    find_library(LIBHL NAMES hl HINTS ${HL_LIB_DIR} )
else()
    find_library(LIBHL NAMES libhl HINTS ${HL_LIB_DIR} )
endif (UNIX)

target_link_libraries(forge.hdll ${LIBHL} ${METAL_LIB} ${QUARTZ_LIB} ${FOUNDATION_LIB})

install(TARGETS forge.hdll DESTINATION ${HDLL_DESTINATION})
