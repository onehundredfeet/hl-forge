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

Renderer *createRenderer(const char *name) {
	RendererDesc rendererDesc = {};
	Renderer *renderer = nullptr;
	initRenderer(name, &rendererDesc, &renderer);

	if (renderer != nullptr) {
		//init resource loader interface
		initResourceLoaderInterface(renderer);
	}

	return renderer;
}

Queue* createQueue(Renderer *renderer) {
		//create graphics queue
	QueueDesc queueDesc = {};
	queueDesc.mType = QUEUE_TYPE_GRAPHICS;
	queueDesc.mFlag = QUEUE_FLAG_NONE;//use QUEUE_FLAG_INIT_MICROPROFILE to enable profiling;
	Queue*   pGraphicsQueue = NULL;
	addQueue(renderer, &queueDesc, &pGraphicsQueue);
	return pGraphicsQueue;
}
void destroyRenderer( Renderer *) {

}

SwapChain *createSwapChain(SDL_Window *window, Renderer *renderer, Queue *queue, int width, int height, int chainCount, bool hdr10) {

	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(window, &wmInfo);


	SwapChainDesc swapChainDesc = {};
	swapChainDesc.mWindowHandle.window = wmInfo.info.cocoa.window;
	swapChainDesc.mPresentQueueCount = 1;
	swapChainDesc.ppPresentQueues = &queue;
	swapChainDesc.mWidth = width;
	swapChainDesc.mHeight = height;
	swapChainDesc.mImageCount = chainCount;

	if (hdr10)
		swapChainDesc.mColorFormat = TinyImageFormat_R10G10B10A2_UNORM;
	else
		swapChainDesc.mColorFormat = getRecommendedSwapchainFormat(false, true);

	swapChainDesc.mColorClearValue = {{1, 1, 1, 1}};
	swapChainDesc.mEnableVsync = false;
	SwapChain *pSwapChain = nullptr;
	addSwapChain(renderer, &swapChainDesc, &pSwapChain);
	return pSwapChain;
}