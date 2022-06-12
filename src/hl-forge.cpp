#define HL_NAME(x) forge_##x
#include <hl.h>

#include "hl-forge.h"
#include "hl-forge-meta.h"

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

//	SDL_WindowData* data = (__bridge SDL_WindowData *)window->driverdata;
 //   NSView *view = data.nswindow.contentView;

	printf("SDL_Window %p\n", window);
	printf("NSWindow %p\n", wmInfo.info.cocoa.window);
	void *view = getNSViewFromNSWindow(wmInfo.info.cocoa.window);
	printf("NSVIew %p\n", view);

//	NSView *view = [nswin contentView];

	SwapChainDesc swapChainDesc = {};
	swapChainDesc.mWindowHandle.window = view;
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

SDL_Window *forge_sdl_get_window(void *ptr) {
	return static_cast<SDL_Window *>(ptr);
}

Buffer*forge_sdl_buffer_load( BufferLoadDesc *bld, SyncToken *token) {
	printf("Loading buffer \n");
	Buffer *tmp = nullptr;
	bld->ppBuffer = &tmp;
	printf("adding resource %p %p\n", bld, token);
	addResource( bld, token);
	return tmp;
}

void forge_sdl_buffer_load_desc_set_index_buffer( BufferLoadDesc *bld, int size, void *data, bool shared) {
		bld->mDesc.mDescriptors = DESCRIPTOR_TYPE_INDEX_BUFFER;
		if (shared) {
			bld->mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
		} else {
			bld->mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
		}
		bld->mDesc.mSize = size;
		bld->pData = data;
//		

}

void forge_sdl_buffer_load_desc_set_vertex_buffer( BufferLoadDesc *bld, int size, void *data, bool shared) {
		bld->mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
		if (shared) {
			bld->mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
		} else {
			bld->mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
		}
		bld->mDesc.mSize = size;
		bld->pData = data;
//		

}




void forge_sdl_buffer_update(Buffer *buffer, void *data) {
	BufferUpdateDesc desc = {buffer};
	beginUpdateResource(&desc);
	memcpy(desc.pMappedData, data, buffer->mSize);
	endUpdateResource(&desc, NULL);
}

void forge_sdl_buffer_update_region(Buffer *buffer, void *data, int toffset, int size, int soffset) {
	BufferUpdateDesc desc = {buffer};
	desc.mDstOffset = toffset;
	desc.mSize = size;
	beginUpdateResource(&desc);
	memcpy(desc.pMappedData, &((char *)data)[soffset], size);
	endUpdateResource(&desc, NULL);
}

RenderTarget *forge_sdl_create_render_target(Renderer *renderer, RenderTargetDesc *desc) {
	RenderTarget *pDepthBuffer;
	addRenderTarget(renderer, desc, &pDepthBuffer);
	return pDepthBuffer;
}

SDL_MetalView forge_create_metal_view(SDL_Window *win) {
	return SDL_Metal_CreateView(win);
}

template <typename T> struct pref {
	void (*finalize)( pref<T> * );
	T *value;
};
template<typename T> pref<T> *_alloc_const( const T *value ) {
	if (value == nullptr) return nullptr;
	pref<T> *r = (pref<T>*)hl_gc_alloc_noptr(sizeof(pref<T>));
	r->finalize = NULL;
	r->value = (T*)value;
	return r;
}
#define _ref(t) pref<t>

HL_PRIM  _ref(SDL_Window)*HL_NAME(forge_get_sdl_window)(void *ptr) {
	printf("Forge SDL Pointer is %p\n", ptr);
	return _alloc_const((SDL_Window *)ptr);
}

/*
HL_PRIM HL_CONST _ref(SDL_Window)* HL_NAME(Window_getWindow1)(void* ptr) {
	return alloc_ref_const((forge_sdl_get_window(ptr)),Window);
}
DEFINE_PRIM(_IDL, Window_getWindow1, _BYTES);
*/

DEFINE_PRIM(_BYTES, forge_get_sdl_window, TWIN);

