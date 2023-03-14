cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0091 NEW)
project (forge C CXX )

if(APPLE)
    message("Enabling Apple languages")
    enable_language(OBJC)
    enable_language(OBJCXX)
endif()


if (NOT FORGE_LIB_DIR)
    set(FORGE_LIB_DIR "ext/forge")
    set(FORGE_LIB_INC "${FORGE_LIB_DIR}/Common_3")
    set(FORGE_LIB_SRC "${FORGE_LIB_DIR}/Common_3")
    set(FORGE_OS_SRC "${FORGE_LIB_DIR}/Common_3/OS")
    set(FORGE_TOOLS_SRC_DIR "${FORGE_LIB_DIR}/Common_3/Tools")
    set(FORGE_TP_SRC "${FORGE_LIB_DIR}/Common_3/ThirdParty/OpenSource")
    set(FORGE_MW_SRC "${FORGE_LIB_DIR}/Middleware_3")
    set(FORGE_RDR_SRC "${FORGE_LIB_DIR}/Common_3/Renderer")
    set(FORGE_EASTL_DIR "${FORGE_TP_SRC}/EASTL")
    set(FORGE_TP_INC ${FORGE_TP_SRC} )
endif()

if (UNIX)
if (NOT HL_LIB_DIR) 
    set(HL_LIB_DIR "/usr/local/lib")
endif()

if (NOT COMMON_LIB_DIR) 
    set(COMMON_LIB_DIR "/usr/local/lib")
endif()

if (NOT HDLL_DESTINATION) 
    set(HDLL_DESTINATION "/usr/local/lib")
endif()

if (NOT HL_INCLUDE_DIR) 
    set(HL_INCLUDE_DIR "/usr/local/include")
endif()


elseif(WIN32)

if (NOT HL_LIB_DIR) 
set(HL_LIB_DIR "./ext/lib")
endif()

if (NOT COMMON_LIB_DIR) 
set(COMMON_LIB_DIR "./ext/lib")
endif()

if (NOT HDLL_DESTINATION) 
set(HDLL_DESTINATION "./installed/lib")
endif()

if (NOT HL_INCLUDE_DIR) 
    set(HL_INCLUDE_DIR "./ext/include")
endif()

if (NOT VULKAN_VERSION)
    set(VULKAN_VERSION "1.3.239.0")
endif()

if (NOT VULKAN_ROOT)
    set(VULKAN_ROOT "C:/VulkanSDK/${VULKAN_VERSION}")
    set(VULKAN_INC_DIR "${VULKAN_ROOT}/include")
    set(VULKAN_LIB_DIR "${VULKAN_ROOT}/lib")
endif() # vulkan root

if (NOT SDL_ROOT)
    set(SDL_ROOT "./ext/sdl2")
    set(SDL_INC_DIR "${SDL_ROOT}/include")
    set(SDL_LIB_DIR "${SDL_ROOT}/lib/x64")
endif()

if (NOT SPIRV_ROOT)
    set( SPIRV_ROOT "${FORGE_TP_SRC}/SPRIV_Cross")
    set( SPIRV_INC_DIR "${SPIRV_ROOT}")
endif()

if (NOT SHADERC_ROOT)
    #set(SHADERC_ROOT "./ext/shaderc")
    #set(SHADERC_INC 
    #"${SHADERC_ROOT}/libshaderc/include"
    #"${SHADERC_ROOT}/libshaderc_util/include"
   # )
    #set(SHADERC_SRC "${SHADERC_ROOT}/libshaderc/src")
endif()
endif() # OS specific



if (NOT FORGE_ROOT) 
set(FORGE_ROOT "../forge")
endif()

if (NOT FORGE_SRC_DIR) 
set(FORGE_SRC_DIR "${FORGE_ROOT}/source")
endif()
if (NOT FORGE_INC_DIR) 
set(FORGE_INC_DIR "${FORGE_ROOT}/include")
endif()

set(FP16_INC_DIR "ext/FP16/include")


if (NOT XXHASH_ROOT_DIR) 
    set(XXHASH_ROOT_DIR "ext/xxHash")
endif()



#platform specific OS
if(APPLE)
set( FORGE_OS_C_SRCS 
    ${FORGE_OS_SRC}/Darwin/DarwinThread.c
    ${FORGE_OS_SRC}/Darwin/DarwinLog.c
)

set( FORGE_OS_CPP_SRCS
${FORGE_OS_SRC}/Darwin/CocoaFileSystem.mm
${FORGE_OS_SRC}/FileSystem/UnixFileSystem.cpp
)
elseif(WIN32)

set( FORGE_OS_C_SRCS 
${FORGE_OS_SRC}/Windows/WindowsLog.c
${FORGE_OS_SRC}/Windows/WindowsThread.c
${FORGE_OS_SRC}/Windows/WindowsTime.c
)

set( FORGE_OS_CPP_SRCS
#${FORGE_OS_SRC}/Windows/WindowsBase.cpp
${FORGE_OS_SRC}/Windows/WindowsFileSystem.cpp
${FORGE_OS_SRC}/Windows/WindowsWindow.cpp
)
endif()

#common OS
set( FORGE_OS_C_SRCS ${FORGE_OS_C_SRCS}
${FORGE_OS_SRC}/MemoryTracking/MemoryTracking.c
${FORGE_OS_SRC}/Logging/Log.c
)

set( FORGE_OS_CPP_SRCS ${FORGE_OS_CPP_SRCS}
${FORGE_OS_SRC}/Core/Screenshot.cpp
${FORGE_OS_SRC}/FileSystem/FileSystem.cpp
${FORGE_OS_SRC}/FileSystem/SystemRun.cpp
#${FORGE_OS_SRC}/UI/UI.cpp
)

# meshes
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

if (APPLE)
set( FORGE_OS_RENDER_SRCS
${FORGE_RDR_SRC}/Metal/MetalRenderer.mm
${FORGE_RDR_SRC}/Metal/MetalRaytracing.mm
${FORGE_RDR_SRC}/Metal/MetalShaderReflection.mm
)
elseif(WIN32)
set( FORGE_OS_RENDER_SRCS
#${SHADERC_SRC}/shaderc.cc
${FORGE_RDR_SRC}/Vulkan/Vulkan.cpp
${FORGE_RDR_SRC}/Vulkan/VulkanRayTracing.cpp
${FORGE_RDR_SRC}/Vulkan/VulkanShaderReflection.cpp
${FORGE_TOOLS_SRC_DIR}/SpirvTools/SpirvTools.cpp
)

endif()

set (FORGE_RENDERER_SRCS
    ${FORGE_OS_RENDER_SRCS}
    ${FORGE_RDR_SRC}/Renderer.cpp
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

set(ALL_C_SOURCES
${XXHASH_ROOT_DIR}/xxhash.c
${FORGE_OS_C_SRCS}
)

if (APPLE)
set(HL_OS_SRCS
src/hl-forge-metal.mm
)
elseif(WIN32)
set(HL_OS_SRCS
)
endif()

set(ALL_CPP_SOURCES
${FORGE_OS_CPP_SRCS}
${FORGE_TP_SRCS}
${FORGE_RENDERER_SRCS}
${FORGE_MW_SRCS}
${FORGE_EASTL_SRCS}
src/hl-forge.cpp
src/hl-forge-shaders.cpp
src/idl_hl.cpp
${HL_OS_SRCS}
)

if (APPLE)
set_source_files_properties(${ALL_C_SOURCES}  PROPERTIES COMPILE_FLAGS "-x objective-c")
set_source_files_properties(${ALL_CPP_SOURCES}  PROPERTIES COMPILE_FLAGS "-x objective-c++")
endif()

add_library(forge.hdll SHARED
    ${ALL_C_SOURCES}
    ${ALL_CPP_SOURCES}
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
${VULKAN_INC_DIR}
${SHADERC_INC}
${FORGE_INC_DIR}
${FORGE_LIB_INC}
${HL_INCLUDE_DIR}
${FORGE_TP_INC}
${FP16_INC_DIR}
${SDL_INC_DIR}
"${XXHASH_ROOT_DIR}"
)
if (APPLE)
find_library( METAL_LIB Metal)
find_library( METALKIT_LIB MetalKit)
find_library( QUARTZ_LIB QuartzCore)
find_library( FOUNDATION_LIB Foundation)
find_library( IOKIT_LIB IOKit)
find_library( METAL_PERF_SHADER_LIB MetalPerformanceShaders)
find_library( APPKITLIB AppKit)
find_library( COREGRAPHICSLIB CoreGraphics)
find_library( SPIRVCROSS_METALLIB spirv-cross-msl HINTS ${VULKAN_LIB_DIR})



set( OS_LIBS  ${METAL_LIB} ${QUARTZ_LIB} ${FOUNDATION_LIB} ${IOKIT_LIB} ${METAL_PERF_SHADER_LIB} ${APPKITLIB} ${COREGRAPHICSLIB} ${METALKIT_LIB}   )
elseif(WIN32)
set(CMAKE_MSVC_RUNTIME_LIBRARY  "MultiThreadedDLL")
set_property(TARGET forge.hdll PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MD")

find_library( VULKAN_LIB vulkan-1 HINTS ${VULKAN_LIB_DIR})
find_library( VOLK_LIB volk HINTS ${VULKAN_LIB_DIR})
set( OS_LIBS ${VULKAN_LIB} ${VOLK_LIB})
endif()

find_library( SDL_LIB SDL2 HINTS ${SDL_LIB_DIR})
find_library( SPIRV_LIB SPIRV HINTS ${VULKAN_LIB_DIR})
find_library( SPIRVCROSS_CORELIB spirv-cross-core HINTS ${VULKAN_LIB_DIR})
find_library( SPIRVCROSS_UTIL_LIB spirv-cross-util HINTS ${VULKAN_LIB_DIR})
find_library( SPIRVCROSS_GLSLLIB spirv-cross-glsl HINTS ${VULKAN_LIB_DIR})
find_library( SPIRVCROSS_HLSL_LIB spirv-cross-hlsl HINTS ${VULKAN_LIB_DIR})
find_library( SPIRVCROSS_REFLECT_LIB spirv-cross-reflect HINTS ${VULKAN_LIB_DIR})
find_library( SPIRVCROSS_CPP_LIB spirv-cross-cpp HINTS ${VULKAN_LIB_DIR})
find_library( SPIRVTOOLS_LIB SPIRV-Tools HINTS ${VULKAN_LIB_DIR})
find_library( SPIRVTOOLS_OPT_LIB SPIRV-Tools-opt HINTS ${VULKAN_LIB_DIR})

set(SPRIV_LIBS
${SPIRVTOOLS_OPT_LIB}
${SPIRVTOOLS_LIB} 
${SPIRVCROSS_REFLECT_LIB}
${SPIRVCROSS_METALLIB}
${SPIRVCROSS_GLSLLIB}
${SPIRVCROSS_UTIL_LIB}
${SPIRVCROSS_CPP_LIB}
${SPIRVCROSS_HLSL_LIB}
${SPIRV_LIB}
${SPIRVCROSS_CORELIB}
)

find_library( SHADERC_LIB shaderc_combined HINTS ${VULKAN_LIB_DIR})


if (UNIX)
    # Some special flags are needed for GNU GCC compiler
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -fpermissive")
    #not sure why the ${HL_LIB_DIR} is necessary given the above.
    SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -shared  ")

    find_library(LIBHL NAMES hl HINTS ${HL_LIB_DIR} )
else()
    find_library(LIBHL NAMES libhl HINTS ${HL_LIB_DIR} )
endif (UNIX)

if (APPLE)
set_property (TARGET forge.hdll APPEND_STRING PROPERTY COMPILE_FLAGS " -mpclmul -fobjc-arc")
elseif(WIN32)
set_property (TARGET forge.hdll APPEND_STRING PROPERTY COMPILE_FLAGS "")
endif() 


target_link_libraries(forge.hdll ${LIBHL} ${OS_LIBS}   ${SDL_LIB} ${SHADERC_LIB} ${SPRIV_LIBS})

install(TARGETS forge.hdll )
