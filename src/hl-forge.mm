#include <stdio.h>
#include "hl-forge.h"

#include <Renderer/IRenderer.h>
#include <Renderer/IShaderReflection.h>
#include <Renderer/IResourceLoader.h>
#include <OS/Logging/Log.h>
#include <OS/Interfaces/IInput.h>
#include <OS/Interfaces/IMemory.h>

bool hlForgeInitialize(const char *name) {
    //init memory allocator
	if (!initMemAlloc(name))
	{
		printf("Failed to init memory allocator\n");
		return false;
	}

   FileSystemInitDesc fsDesc = {};
   fsDesc.pAppName = name;

	//init file system
   if (!initFileSystem(&fsDesc))
	{
		printf("Failed to init file system\n");
		return false;
	}

   //set root directory for the log, must set this before we initialize the log
   fsSetPathForResourceDir(pSystemFileIO, RM_DEBUG, RD_LOG, "../../tmp");

	//init the log
	// Initialization/Exit functions are thread unsafe
	initLog(name, eALL);
//	exitLog(void);

  //work out which api we are using
   RendererApi api;
#if defined(VULKAN)
   api = RENDERER_API_VULKAN;
#elif defined(DIRECT3D12)
   api = RENDERER_API_D3D12;
#elif defined(METAL)
   api = RENDERER_API_METAL;
#else
   #error Trying to use a renderer API not supported by this demo
#endif

	// all this configuration should moved to a higher level
	
	//set directories for the selected api
	switch (api)
	{
		#if defined(DIRECT3D12)
	case RENDERER_API_D3D12:
      fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_SHADER_SOURCES, "shaders/d3d12/");
      fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_SHADER_BINARIES, "shaders/d3d12/binary/");
		break;
		#endif
	#if defined(VULKAN)
	case RENDERER_API_VULKAN:
      fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_SHADER_SOURCES, "shaders/vulkan/");
      fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_SHADER_BINARIES, "shaders/vulkan/binary/");
		break;
	#endif
	#if defined(METAL)
	case RENDERER_API_METAL:
      fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_SHADER_SOURCES, "shaders/metal/");
      fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_SHADER_BINARIES, "shaders/metal/binary/");
	break;
	#endif
	default:
      LOGF(LogLevel::eERROR, "No support for this API");
		return false;
	}

	//set texture dir
   fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_TEXTURES, "textures/");
	//set font dir
   fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_FONTS, "fonts/");
	//set GPUCfg dir
   fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_GPU_CONFIG, "gpucfg/");



	return true;
}