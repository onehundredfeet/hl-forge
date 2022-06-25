cmake_minimum_required(VERSION 3.10)
project (forge C CXX )

if(APPLE)
    enable_language(OBJC)
    enable_language(OBJCXX)
endif()

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

if (NOT CLHASH_ROOT_DIR) 
    set(CLHASH_ROOT_DIR "ext/clhash")
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
    set(FORGE_MW_SRC "${FORGE_LIB_DIR}/Middleware_3")
    set(FORGE_RDR_SRC "${FORGE_LIB_DIR}/Common_3/Renderer")
    set(FORGE_EASTL_DIR "${FORGE_TP_SRC}/EASTL")
endif()


set( FORGE_OS_SRCS 
    ${FORGE_OS_SRC}/Logging/Log.c
    ${FORGE_OS_SRC}/Darwin/DarwinThread.c
    ${FORGE_OS_SRC}/Darwin/DarwinLog.c
    ${FORGE_OS_SRC}/Darwin/CocoaFileSystem.mm
    ${FORGE_OS_SRC}/FileSystem/FileSystem.cpp
    ${FORGE_OS_SRC}/FileSystem/UnixFileSystem.cpp
    ${FORGE_OS_SRC}/FileSystem/SystemRun.cpp
    ${FORGE_OS_SRC}/MemoryTracking/MemoryTracking.c
)

set ( FORGE_MW_SRCS
    ${FORGE_MW_SRC}/ParallelPrimitives/ParallelPrimitives.cpp
)

set (FORGE_TP_SRCS
    ${FORGE_TP_SRC}/basis_universal/transcoder/basisu_transcoder.cpp
    ${FORGE_TP_SRC}/meshoptimizer/src/vcacheoptimizer.cpp
    ${FORGE_TP_SRC}/meshoptimizer/src/vfetchoptimizer.cpp
    ${FORGE_TP_SRC}/meshoptimizer/src/indexgenerator.cpp
    ${FORGE_TP_SRC}/meshoptimizer/src/overdrawoptimizer.cpp
    ${FORGE_TP_SRC}/meshoptimizer/src/allocator.cpp
#    ${FORGE_TP_SRC}/FluidStudios/MemoryManager/mmgr.c
)

set (FORGE_RENDERER_SRCS
    ${FORGE_RDR_SRC}/Renderer.cpp
    ${FORGE_RDR_SRC}/Metal/MetalRenderer.mm
    ${FORGE_RDR_SRC}/Metal/MetalRaytracing.mm
    ${FORGE_RDR_SRC}/Metal/MetalShaderReflection.mm
    ${FORGE_RDR_SRC}/ResourceLoader.cpp
    ${FORGE_RDR_SRC}/CommonShaderReflection.cpp
    
)

set(FORGE_EASTL_SRCS
    ${FORGE_EASTL_DIR}/eastl.cpp
   # ${FORGE_EASTL_DIR}/EAStdC/EASprintf.cpp
  #  ${FORGE_EASTL_DIR}/EAStdC/EAMemory.cpp
 #   ${FORGE_EASTL_DIR}/allocator_forge.cpp
#    ${FORGE_EASTL_DIR}/allocator_eastl.cpp
)

add_library(forge.hdll SHARED
src/hl-idl-helpers.cpp
src/forge.cpp
src/hl-forge.cpp
src/hl-forge-shaders.cpp
src/hl-forge-metal.mm
${FORGE_OS_SRCS}
${FORGE_TP_SRCS}
${FORGE_RENDERER_SRCS}
${FORGE_MW_SRCS}
${FORGE_EASTL_SRCS}
${CLHASH_ROOT_DIR}/src/clhash.c
)

set_source_files_properties(${FORGE_RENDERER_SRCS} ${FORGE_MW_SRCS} src/forge.cpp src/hl-forge.cpp PROPERTIES COMPILE_FLAGS "-x objective-c++")

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
"${CLHASH_ROOT_DIR}/include"
)

find_library( METAL_LIB Metal)
find_library( METALKIT_LIB MetalKit)
find_library( QUARTZ_LIB QuartzCore)
find_library( FOUNDATION_LIB Foundation)
find_library( IOKIT_LIB IOKit)
find_library( METAL_PERF_SHADER_LIB MetalPerformanceShaders)
find_library( SHADERC_LIB shaderc_combined)
find_library( SDL_LIB SDL2)
find_library( APPKITLIB AppKit)
find_library( COREGRAPHICSLIB CoreGraphics)
find_library( SPIRVCROSS_CORELIB spirv-cross-core)
find_library( SPIRVCROSS_METALLIB spirv-cross-msl)
find_library( SPIRVCROSS_GLSLLIB spirv-cross-glsl)

if (UNIX)
    # Some special flags are needed for GNU GCC compiler
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fPIC -fpermissive")
    #not sure why the ${HL_LIB_DIR} is necessary given the above.
    SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -shared  ")

    find_library(LIBHL NAMES hl HINTS ${HL_LIB_DIR} )
else()
    find_library(LIBHL NAMES libhl HINTS ${HL_LIB_DIR} )
endif (UNIX)

set_property (TARGET forge.hdll APPEND_STRING PROPERTY COMPILE_FLAGS " -mpclmul -fobjc-arc")
 
target_link_libraries(forge.hdll ${LIBHL} ${METAL_LIB} ${QUARTZ_LIB} ${FOUNDATION_LIB} ${IOKIT_LIB} ${METAL_PERF_SHADER_LIB} ${SHADERC_LIB} ${SDL_LIB} ${APPKITLIB} ${COREGRAPHICSLIB} ${METALKIT_LIB} ${SPIRVCROSS_CORELIB} ${SPIRVCROSS_METALLIB} ${SPIRVCROSS_GLSLLIB})

install(TARGETS forge.hdll DESTINATION ${HDLL_DESTINATION})
