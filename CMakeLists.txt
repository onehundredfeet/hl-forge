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



set(
        INSTANT_MESH_SRC
        ${INSTANT_MESH_SRC_DIR}/meshstats.cpp
        ${INSTANT_MESH_SRC_DIR}/hierarchy.cpp
        ${INSTANT_MESH_SRC_DIR}/field.cpp
        ${INSTANT_MESH_SRC_DIR}/dedge.cpp
        ${INSTANT_MESH_SRC_DIR}/normal.cpp
        ${INSTANT_MESH_SRC_DIR}/adjacency.cpp
        ${INSTANT_MESH_SRC_DIR}/subdivide.cpp
        ${INSTANT_MESH_SRC_DIR}/bvh.cpp
        ${INSTANT_MESH_SRC_DIR}/extract.cpp
        ${INSTANT_MESH_SRC_DIR}/reorder.cpp
        
)

add_library(forge.hdll SHARED
${FORGE_SRC_DIR}/quadrangulate.cxx
${FORGE_SRC_DIR}/forge.cxx
${FORGE_SRC_DIR}/forge_cells.cxx
${FORGE_SRC_DIR}/forge_regex.cxx
${INSTANT_MESH_SRC}
src/hl-idl-helpers.cpp
src/forge.cpp
src/hl-forge.cpp
)

set_target_properties(forge.hdll
PROPERTIES
PREFIX ""
OUTPUT_NAME forge
SUFFIX .hdll
)

cmake_policy(SET CMP0015 NEW)

target_include_directories(forge.hdll
PRIVATE
${FORGE_INC_DIR}
${CINOLIB_INC_DIR}
${EIGEN_INC_DIR}
${CDT_INC_DIR}
${INSTANT_INC_DIR}
${HL_INCLUDE_DIR}
${PSS_INC_DIR}
${DSET_INC_DIR}
${TBB_INC_DIR}
${PCG32_INC_DIR}
${RAPIDJSON_INC_DIR}
${RANDOM_INC_DIR}
${EXPRTK_INC_DIR}
${ONIGURUMA_INC_DIR}
${MESHOPTIMIZER_INC_DIR}
)


if (UNIX)
    # Some special flags are needed for GNU GCC compiler
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fPIC  -O3  -fpermissive")
    #not sure why the ${HL_LIB_DIR} is necessary given the above.
    SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -shared  ")

    find_library(LIBHL NAMES hl HINTS ${HL_LIB_DIR} )
else()
    find_library(LIBHL NAMES libhl HINTS ${HL_LIB_DIR} )
endif (UNIX)

target_link_libraries(forge.hdll ${LIBHL} ${LIBTBB} ${LIBFMT} ${LIBBOOSTFILESYSTEM} ${LIBONIGURUMA} )

install(TARGETS forge.hdll DESTINATION ${HDLL_DESTINATION})
