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
   fsSetPathForResourceDir(pSystemFileIO, RM_DEBUG, RD_LOG, "");

	//init the log
	// Initialization/Exit functions are thread unsafe
	initLog(name, eALL);
//	exitLog(void);
	return true;
}