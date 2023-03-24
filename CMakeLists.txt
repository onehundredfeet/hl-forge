cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0091 NEW)
project (forge C CXX )

include(config.cmake)

IF(CMAKE_BUILD_TYPE MATCHES Debug)
message("Building DEBUG config")
set (GENERATE_DEBUG 1)
else()
message("Building ${CMAKE_BUILD_TYPE} config")
endif()

if(APPLE)
    message("Enabling Apple languages")
    enable_language(OBJC)
    enable_language(OBJCXX)
endif()


set(FORGE_LIB_INC "${FORGE_LIB_DIR}/Common_3")
set(FORGE_LIB_SRC "${FORGE_LIB_DIR}/Common_3")
set(FORGE_OS_SRC "${FORGE_LIB_DIR}/Common_3/OS")
set(FORGE_INC_SRC "${FORGE_LIB_DIR}/Common_3/OS/Interfaces")
set(FORGE_APP_SRC "${FORGE_LIB_DIR}/Common_3/Application")
set(FORGE_APP_INC "${FORGE_APP_SRC}/Interfaces")
set(FORGE_UTIL_SRC "${FORGE_LIB_DIR}/Common_3/Utilities")
set(FORGE_UITL_INC "${FORGE_UTIL_SRC}/Interfaces")
set(FORGE_TOOLS_SRC_DIR "${FORGE_LIB_DIR}/Common_3/Tools")
set(FORGE_TP_SRC "${FORGE_LIB_DIR}/Common_3/Utilities/ThirdParty/OpenSource")
set(FORGE_RES_SRC "${FORGE_LIB_DIR}/Common_3/Resources")
set(FORGE_RES_INC "${FORGE_RES_SRC}/ResourceLoader/Interfaces")
set(FORGE_RES_TP_SRC "${FORGE_RES_SRC}/ResourceLoader/ThirdParty/OpenSource")
set(FORGE_MW_SRC "${FORGE_LIB_DIR}/Middleware_3")
set(FORGE_RDR_SRC "${FORGE_LIB_DIR}/Common_3/Renderer")
set(FORGE_GFX_SRC "${FORGE_LIB_DIR}/Common_3/Graphics")
set(FORGE_GFX_INC "${FORGE_GFX_SRC}/Interfaces")
set(FORGE_EASTL_DIR "${FORGE_TP_SRC}/EASTL")
set(FORGE_TP_INC ${FORGE_TP_SRC} )

set(FORGE_INC_DIR
${FORGE_APP_INC}
${FORGE_GFX_INC}
${FORGE_TP_INC}
${FORGE_RES_INC}
${FORGE_UITL_INC}
${FORGE_RES_INC}
${FORGE_INC_SRC}
)



#SPIRV-CROSS
set (SPIRV_CROSS_SRCS
    #${SPIRV_CROSS_SRC_DIR}/spirv_cross.cpp
   # ${SPIRV_CROSS_SRC_DIR}/spirv_glsl.cpp
   # ${SPIRV_CROSS_SRC_DIR}/spirv_parser.cpp
    #${SPIRV_CROSS_SRC_DIR}/spirv_reflect.cpp
)

set (SPIRV_TOOLS_SRCS
    ${SPIRV_REFLECT_SRC_DIR}/spirv_reflect.c
#    ${SPIRV_TOOLS_SRC_DIR}/spirv_optimizer_options.cpp
 #   ${SPIRV_TOOLS_SRC_DIR}/opt/optimizer.cpp
)

set (SHADERC_SRCS
    ${LIB_SHADERC_SRC_DIR}/shaderc.cc
    ${LIB_SHADERC_UTIL_SRC_DIR}/compiler.cc
    ${LIB_SHADERC_UTIL_SRC_DIR}/shader_stage.cc
    ${LIB_SHADERC_UTIL_SRC_DIR}/spirv_tools_wrapper.cc

)

if (UNIX)


elseif(WIN32)


endif() # OS specific


#platform specific OS
if(APPLE)
set( FORGE_OS_C_SRCS 
    ${FORGE_OS_SRC}/Darwin/DarwinThread.c
    ${FORGE_OS_SRC}/Darwin/DarwinLog.c
)

set( FORGE_OS_CPP_SRCS
${FORGE_OS_SRC}/Darwin/CocoaFileSystem.mm
${FORGE_UTIL_SRC}/FileSystem/UnixFileSystem.cpp
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
${FORGE_UTIL_SRC}/MemoryTracking/MemoryTracking.c
${FORGE_UTIL_SRC}/Log/Log.c
${FORGE_UTIL_SRC}/Math/StbDs.c
${FORGE_UTIL_SRC}/Math/algorithms.c
${FORGE_TP_SRC}/bstrlib/bstrlib.c
)

set( FORGE_OS_CPP_SRCS ${FORGE_OS_CPP_SRCS}
${FORGE_APP_SRC}/Screenshot.cpp
${FORGE_UTIL_SRC}/FileSystem/FileSystem.cpp
${FORGE_UTIL_SRC}/FileSystem/SystemRun.cpp
#${FORGE_OS_SRC}/UI/UI.cpp
)

# meshes
set ( FORGE_MW_SRCS
    ${FORGE_MW_SRC}/ParallelPrimitives/ParallelPrimitives.cpp
)

set (FORGE_TP_SRCS
    ${FORGE_TP_SRC}/basis_universal/transcoder/basisu_transcoder.cpp
    ${FORGE_RES_TP_SRC}/meshoptimizer/src/vcacheoptimizer.cpp
    ${FORGE_RES_TP_SRC}/meshoptimizer/src/vfetchoptimizer.cpp
    ${FORGE_RES_TP_SRC}/meshoptimizer/src/indexgenerator.cpp
    ${FORGE_RES_TP_SRC}/meshoptimizer/src/overdrawoptimizer.cpp
    ${FORGE_RES_TP_SRC}/meshoptimizer/src/allocator.cpp
#    ${FORGE_TP_SRC}/FluidStudios/MemoryManager/mmgr.c
)


if (APPLE)
set( FORGE_OS_RENDER_SRCS
${FORGE_GFX_SRC}/Metal/MetalRenderer.mm
${FORGE_GFX_SRC}/Metal/MetalRaytracing.mm
${FORGE_GFX_SRC}/Metal/MetalShaderReflection.mm
)
elseif(WIN32)
set( FORGE_OS_RENDER_SRCS
${FORGE_GFX_SRC}/Vulkan/Vulkan.cpp
${FORGE_GFX_SRC}/Vulkan/VulkanRayTracing.cpp
${FORGE_GFX_SRC}/Vulkan/VulkanShaderReflection.cpp
#${FORGE_TOOLS_SRC_DIR}/SpirvTools/SpirvTools.cpp
${FORGE_GFX_SRC}/ThirdParty/OpenSource/SPIRV_Cross/SpirvTools.cpp

)

endif()

set (FORGE_RENDERER_SRCS
    ${FORGE_OS_RENDER_SRCS}
    ${FORGE_GFX_SRC}/Graphics.cpp
    ${FORGE_GFX_SRC}/GPUConfig.cpp
    ${FORGE_GFX_SRC}/DependencyTracking.cpp
    ${FORGE_RES_SRC}/ResourceLoader/ResourceLoader.cpp
    ${FORGE_GFX_SRC}/CommonShaderReflection.cpp
    
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
#${SPIRV_CROSS_SRCS}
#${SPIRV_TOOLS_SRCS}
#${SHADERC_SRCS}
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
OUTPUT_NAME forge${CONFIG_POSTFIX}
SUFFIX .hdll
)

set_property(TARGET forge.hdll PROPERTY CXX_STANDARD 17)

cmake_policy(SET CMP0015 NEW)

target_include_directories(forge.hdll
PRIVATE
${SPIRV_HEADER_DIRS}
${SHADERC_INC_DIR}
${VULKAN_INC_DIR}
${FORGE_INC_DIR}
${FORGE_APP_INC}
${FORGE_GFX_INC}
${FORGE_RES_INC}
${FORGE_LIB_INC}
${HL_INC_DIR}
${FORGE_TP_INC}
${FP16_INC_DIR}
${SDL_INC_DIR}
${FORGE_RES_TP_SRC}
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



set( OS_LIBS  ${METAL_LIB} ${QUARTZ_LIB} ${FOUNDATION_LIB} ${IOKIT_LIB} ${METAL_PERF_SHADER_LIB} ${APPKITLIB} ${COREGRAPHICSLIB} ${METALKIT_LIB}   )
elseif(WIN32)
#set_property(TARGET forge.hdll PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

find_library( VULKAN_LIB vulkan-1 HINTS ${VULKAN_LIB_DIR})
find_library( VOLK_LIB volk HINTS ${VULKAN_LIB_DIR})
set( OS_LIBS ${VULKAN_LIB} ${VOLK_LIB})
endif()

find_library( SDL_LIB SDL2 HINTS ${SDL_LIB_DIR})
find_library( SPIRV_TOOLS_LIB  SPIRV-Tools${CONFIG_POSTFIX} HINTS ${SPIRV_TOOLS_LIB_DIR})

if(CMAKE_BUILD_TYPE MATCHES Debug)

if (WIN32)
set(SPIRV_LIB_POSTFIX "d")
endif()
endif()

find_library( SPIRV_CROSS_GLSL_LIB  spirv-cross-glsl${SPIRV_LIB_POSTFIX} HINTS ${SPIRV_CROSS_LIB_DIR})
find_library( SPIRV_CROSS_HLSL_LIB  spirv-cross-hlsl${SPIRV_LIB_POSTFIX} HINTS ${SPIRV_CROSS_LIB_DIR})
find_library( SPIRV_CROSS_CPP_LIB  spirv-cross-cpp${SPIRV_LIB_POSTFIX} HINTS ${SPIRV_CROSS_LIB_DIR})
find_library( SPIRV_CROSS_REFLECT_LIB  spirv-cross-reflect${SPIRV_LIB_POSTFIX} HINTS ${SPIRV_CROSS_LIB_DIR})
find_library( SPIRV_CROSS_MSL_LIB  spirv-cross-msl${SPIRV_LIB_POSTFIX} HINTS ${SPIRV_CROSS_LIB_DIR})
find_library( SPIRV_CROSS_UTIL_LIB  spirv-cross-util${SPIRV_LIB_POSTFIX} HINTS ${SPIRV_CROSS_LIB_DIR})
find_library( SPIRV_CROSS_CORE_LIB  spirv-cross-core${SPIRV_LIB_POSTFIX} HINTS ${SPIRV_CROSS_LIB_DIR})


#message("SHADERC_LIB_DIR is '${SHADERC_LIB_DIR}' looking for 'shaderc_combined${SPIRV_LIB_POSTFIX}")
find_library( SHADERC_LIB shaderc_combined${CONFIG_POSTFIX} HINTS ${SHADERC_LIB_DIR})

set( SPIRV_CROSS_LIBS 
${SPIRV_CROSS_GLSL_LIB}
${SPIRV_CROSS_HLSL_LIB}
${SPIRV_CROSS_CPP_LIB}
${SPIRV_CROSS_REFLECT_LIB}
${SPIRV_CROSS_MSL_LIB}
${SPIRV_CROSS_UTIL_LIB}
${SPIRV_CROSS_CORE_LIB}

)






#find_library( SPIRV_LIB SPIRV HINTS ${VULKAN_LIB_DIR})
#find_library( SPIRVCROSS_CORELIB spirv-cross-core HINTS ${VULKAN_LIB_DIR})
#find_library( SPIRVCROSS_UTIL_LIB spirv-cross-util HINTS ${VULKAN_LIB_DIR})
#find_library( SPIRVCROSS_GLSLLIB spirv-cross-glsl HINTS ${VULKAN_LIB_DIR})
#find_library( SPIRVCROSS_HLSL_LIB spirv-cross-hlsl HINTS ${VULKAN_LIB_DIR})
#find_library( SPIRVCROSS_REFLECT_LIB spirv-cross-reflect HINTS ${VULKAN_LIB_DIR})
#find_library( SPIRVCROSS_CPP_LIB spirv-cross-cpp HINTS ${VULKAN_LIB_DIR})
#find_library( SPIRVTOOLS_LIB SPIRV-Tools HINTS ${VULKAN_LIB_DIR})
#find_library( SPIRVTOOLS_OPT_LIB SPIRV-Tools-opt HINTS ${VULKAN_LIB_DIR})
#find_library( SPIRVCROSS_METALLIB spirv-cross-msl HINTS ${VULKAN_LIB_DIR})

set(SPRIV_LIBS
${SHADERC_LIB}
#${SPIRV_TOOLS_LIB}
${SPIRV_CROSS_LIBS}
#${SPIRVTOOLS_OPT_LIB}
#${SPIRVTOOLS_LIB} 
#${SPIRVCROSS_REFLECT_LIB}
#${SPIRVCROSS_METALLIB}
#${SPIRVCROSS_GLSLLIB}
#${SPIRVCROSS_UTIL_LIB}
#${SPIRVCROSS_CPP_LIB}
#${SPIRVCROSS_HLSL_LIB}
#${SPIRV_LIB}
#${SPIRVCROSS_CORELIB}
)



if(GENERATE_DEBUG)
target_compile_definitions(forge.hdll PRIVATE -DFORGE_DEBUG)
else()
endif()

target_compile_definitions(forge.hdll PRIVATE -DENABLE_HLSL)


if (UNIX)
    # Some special flags are needed for GNU GCC compiler
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -fpermissive")
    SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -shared  ")
else()

endif (UNIX)

find_library(LIBHL NAMES ${HL_LIB_NAME}${CONFIG_POSTFIX} HINTS ${HL_LIB_DIR} )

if (APPLE)
set_property (TARGET forge.hdll APPEND_STRING PROPERTY COMPILE_FLAGS " -mpclmul -fobjc-arc")
elseif(WIN32)
endif()  


target_link_libraries(forge.hdll ${LIBHL} ${OS_LIBS}   ${SDL_LIB}  ${SPRIV_LIBS})

install(TARGETS forge.hdll )
