
################# COMMON
if (NOT PATH_TO_IDL)
set(PATH_TO_IDL "ext/hl-idl")
endif()


if (NOT EXISTS ${PATH_TO_IDL})
message( "This library requires library hl-idl https://github.com/onehundredfeet/hl-idl.git ")
message( FATAL_ERROR "Please use -DPATH_TO_IDL=<IDLPATH> to set the path or run `git update submodules --init --recursive`")
endif()

if (NOT FORGE_LIB_DIR)
    set(FORGE_LIB_DIR "ext/forge")
endif()

if (NOT EXISTS ${FORGE_LIB_DIR})
message( "This library requires a specific version of forge library https://github.com/onehundredfeet/The-Forge.git ")
message( FATAL_ERROR "Please run `git update submodules --init --recursive`")
endif()


set(XXHASH_ROOT_DIR "ext/xxHash")
set(FP16_INC_DIR "ext/FP16/include")

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

if (NOT HL_INCLUDE_DIR) 
    set(HL_INCLUDE_DIR "/usr/local/include")
endif()

################# WINDOWS
elseif(WIN32)

if (NOT COMMON_LIB_DIR) 
    set(COMMON_LIB_DIR "ext")
endif()

if (NOT HL_INCLUDE_DIR) 
    set(HL_INCLUDE_DIR "${COMMON_LIB_DIR}/include")
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
