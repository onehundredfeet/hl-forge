
################# COMMON
if (NOT PATH_TO_IDL)
set(PATH_TO_IDL "${CMAKE_SOURCE_DIR}/ext/hl-idl")
endif()

IF(CMAKE_BUILD_TYPE MATCHES Debug)
    message("BUILDING DEBUG...")
    set( CONFIG_POSTFIX "_d")
else()
    set (CONFIG_POSTFIX "")
endif()


if (NOT EXISTS ${PATH_TO_IDL})
message( "This library requires library hl-idl https://github.com/onehundredfeet/hl-idl.git ")
message( FATAL_ERROR "Please use -DPATH_TO_IDL=<IDLPATH> to set the path or run `git update submodules --init --recursive`")
endif()

if (NOT FORGE_LIB_DIR)
    set(FORGE_LIB_DIR "${CMAKE_SOURCE_DIR}/ext/forge")
endif()

if (NOT EXISTS ${FORGE_LIB_DIR})
message( "This library requires a specific version of forge library https://github.com/onehundredfeet/The-Forge.git ")
message( FATAL_ERROR "Please run `git update submodules --init --recursive`")
endif()


set(XXHASH_ROOT_DIR "${CMAKE_SOURCE_DIR}/ext/xxHash")
set(FP16_INC_DIR "${CMAKE_SOURCE_DIR}/ext/FP16/include")
set(SPIRV_TOOLS_ROOT_DIR "${CMAKE_SOURCE_DIR}/ext/spirv")
set(SPIRV_CROSS_ROOT_DIR "${CMAKE_SOURCE_DIR}/ext/spirv-cross")
set(SHADERC_ROOT_DIR "${CMAKE_SOURCE_DIR}/ext/shaderc")
set(GLSLANG_ROOT_DIR "${CMAKE_SOURCE_DIR}/ext/glslang")

################# MAC AND LINUX
if (UNIX)

if (NOT COMMON_LIB_DIR) 
    set(COMMON_LIB_DIR "/usr/local/lib")
endif()


if (NOT HL_LIB_DIR) 
    set(HL_LIB_DIR ${COMMON_LIB_DIR})
endif()

if (NOT HDLL_DESTINATION) 
    set(HDLL_DESTINATION ${COMMON_LIB_DIR})
endif()

if (NOT HL_INC_DIR) 
    set(HL_INC_DIR "/usr/local/include")
endif()

################# WINDOWS
elseif(WIN32)

if (NOT COMMON_LIB_DIR) 
    set(COMMON_LIB_DIR "ext")
endif()

if (NOT HL_INC_DIR) 
    set(HL_INC_DIR "${COMMON_LIB_DIR}/include")
endif()
if (NOT HL_LIB_DIR) 
    set(HL_LIB_DIR "${COMMON_LIB_DIR}/lib")
endif()

if (NOT HDLL_DESTINATION) 
set(HDLL_DESTINATION "./installed/lib")
endif()

if (NOT VULKAN_VERSION)
    set(VULKAN_VERSION "1.3.239.0")
endif()

if (NOT VULKAN_ROOT)
    set(VULKAN_ROOT "C:/VulkanSDK/${VULKAN_VERSION}")
endif()

if (NOT SDL_ROOT)
    set(SDL_ROOT "${COMMON_LIB_DIR}/sdl2")
endif()

if (NOT EXISTS ${VULKAN_ROOT})
    message(FATAL_ERROR "Building on windows requires the Vulkan SDK. See config.cmake for pathing information.")
endif()
################# END
endif()


###### common derivatives


set(SHADERC_INC_DIR 
"${SHADERC_ROOT_DIR}/libshaderc/include"
"${SHADERC_ROOT_DIR}/libshaderc_util/include"
)

set(SPIRV_HEADERS_INC_DIR )
set(LIB_SHADERC_SRC_DIR "${SHADERC_ROOT_DIR}/libshaderc/src")
set(LIB_SHADERC_UTIL_SRC_DIR "${SHADERC_ROOT_DIR}/libshaderc_util/src")
set(SPIRV_CROSS_INC_DIR "${SPIRV_CROSS_ROOT_DIR}")
set(SPIRV_CROSS_SRC_DIR "${SPIRV_CROSS_ROOT_DIR}")
set(SPIRV_CROSS_LIB_DIR "${SPIRV_CROSS_ROOT_DIR}/build")
message ("SPIRV CROSS LIB DIR '${SPIRV_CROSS_LIB_DIR}'")
set(SPIRV_TOOLS_INC_DIR "${SPIRV_TOOLS_ROOT_DIR}/include") # ${SPIRV_TOOLS_ROOT_DIR}
#set(SPIRV_TOOLS_SRC_DIR "${SPIRV_TOOLS_ROOT_DIR}/source")
set(SPIRV_TOOLS_LIB_DIR "${SPIRV_TOOLS_ROOT_DIR}/build/source")
set(SDL_INC_DIR "${SDL_ROOT}/include")
set(SDL_LIB_DIR "${SDL_ROOT}/lib/x64")
set(VULKAN_INC_DIR "${VULKAN_ROOT}/include")
set(VULKAN_LIB_DIR "${VULKAN_ROOT}/lib")
set(SPIRV_REFLECT_ROOT_DIR "${VULKAN_ROOT}/Source/SPIRV-Reflect")
#set(SPIRV_REFLECT_INC_DIR "${SPIRV_REFLECT_ROOT_DIR}/include")
set(SPIRV_REFLECT_SRC_DIR "${SPIRV_REFLECT_ROOT_DIR}")
set(SPIRV_GLSL_INC_DIR "${GLSLANG_ROOT_DIR}/include/glslang")
set(SHADERC_LIB_DIR "${SHADERC_ROOT_DIR}/build/libshaderc")
set(SPIRV_HEADER_DIRS
    "${CMAKE_SOURCE_DIR}/ext/SPIRV-Headers/include"
    ${SPIRV_CROSS_INC_DIR}
    ${SPIRV_TOOLS_INC_DIR}
#    ${SPIRV_REFLECT_INC_DIR}
    ${SPIRV_GLSL_INC_DIR}
)

set(SPIRV_LIB_DIRS
    ${SPIRV_TOOLS_LIB_DIR}
)