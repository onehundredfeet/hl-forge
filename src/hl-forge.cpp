
#include <iostream>
#include "hl-forge-shaders.h"

#include <Renderer/IRenderer.h>
#include <OS/Interfaces/IInput.h>
#include <OS/Interfaces/IMemory.h>
#include <OS/Logging/Log.h>
#include <Renderer/IResourceLoader.h>
#include <Renderer/IShaderReflection.h>



#define HL_NAME(x) forge_##x
#include <hl.h>

#include "hl-forge-meta.h"
#include "hl-forge.h"

static CpuInfo gCpu;

// From SDL
static int IsMetalAvailable(const SDL_SysWMinfo *syswm) {
    if (syswm->subsystem != SDL_SYSWM_COCOA && syswm->subsystem != SDL_SYSWM_UIKIT) {
        return SDL_SetError("Metal render target only supports Cocoa and UIKit video targets at the moment.");
    }

    // this checks a weak symbol.
#if (defined(__MACOSX__) && (MAC_OS_X_VERSION_MIN_REQUIRED < 101100))
    if (MTLCreateSystemDefaultDevice == NULL) {  // probably on 10.10 or lower.
        return SDL_SetError("Metal framework not available on this system");
    }
#endif

    return 0;
}

static SDL_MetalView GetWindowView(SDL_Window *window) {
    SDL_SysWMinfo info;

    SDL_VERSION(&info.version);
    if (SDL_GetWindowWMInfo(window, &info)) {
#ifdef __MACOSX__
        if (info.subsystem == SDL_SYSWM_COCOA) {
            NSView *view = info.info.cocoa.window.contentView;
            if (view.subviews.count > 0) {
                view = view.subviews[0];
                if (view.tag == SDL_METALVIEW_TAG) {
                    return (SDL_MetalView)CFBridgingRetain(view);
                }
            }
        }
#else
        if (info.subsystem == SDL_SYSWM_UIKIT) {
            UIView *view = info.info.uikit.window.rootViewController.view;
            if (view.tag == SDL_METALVIEW_TAG) {
                return (SDL_MetalView)CFBridgingRetain(view);
            }
        }
#endif
    }
    return nil;
}

bool hlForgeInitialize(const char *name) {
    // init memory allocator
    printf("Intializing memory\n");
    if (!initMemAlloc(name)) {
        printf("Failed to init memory allocator\n");
        return false;
    }

    FileSystemInitDesc fsDesc = {};
    fsDesc.pAppName = name;
    printf("Intializing file system\n");

    // init file system
    if (!initFileSystem(&fsDesc)) {
        printf("Failed to init file system\n");
        return false;
    }

    // set root directory for the log, must set this before we initialize the log
    fsSetPathForResourceDir(pSystemFileIO, RM_DEBUG, RD_LOG, "log");

    // init the log
    //  Initialization/Exit functions are thread unsafe
    initLog(name, eALL);
    //	exitLog(void);

    // work out which api we are using
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

    // set directories for the selected api
    switch (api) {
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
            fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_SHADER_SOURCES, "shadercache/");
            fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_SHADER_BINARIES, "shadercache/binary/");
            break;
#endif
        default:
            LOGF(LogLevel::eERROR, "No support for this API");
            return false;
    }

    // set texture dir
    fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_TEXTURES, "textures/");
    // set font dir
    fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_FONTS, "fonts/");
    // set GPUCfg dir
    fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_GPU_CONFIG, "gpucfg/");

    return true;
}

void forge_init_loader(Renderer *renderer) {
    ResourceLoaderDesc desc = {8ull << 20, 2, true};
    initResourceLoaderInterface(renderer, &desc);
}

Queue *createQueue(Renderer *renderer) {
    // create graphics queue
    QueueDesc queueDesc = {};
    queueDesc.mType = QUEUE_TYPE_GRAPHICS;
    queueDesc.mFlag = QUEUE_FLAG_NONE;  // use QUEUE_FLAG_INIT_MICROPROFILE to enable profiling;
    Queue *pGraphicsQueue = NULL;
    addQueue(renderer, &queueDesc, &pGraphicsQueue);
    return pGraphicsQueue;
}
void destroyRenderer(Renderer *) {
}

ForgeSDLWindow::ForgeSDLWindow(SDL_Window *window) {
    this->window = window;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(window, &wmInfo);
    printf("SDL_Window %p\n", window);
    printf("NSWindow %p\n", wmInfo.info.cocoa.window);
    void *view = getNSViewFromNSWindow(wmInfo.info.cocoa.window);
    printf("NSVIew %p\n", view);

    if (IsMetalAvailable(&wmInfo) != 0) {
        printf("Metal is not avaialble!!!\n");
    }

    auto window_flags = SDL_GetWindowFlags(window);
    if (!(window_flags & SDL_WINDOW_METAL)) {
        assert(false && "WTF - Metal isn't supported by the window");
    }

    _view = GetWindowView(window);
    if (_view == nil) {
        _view = SDL_Metal_CreateView(window);
    }

#ifdef __MACOSX__
    _layer = (CAMetalLayer *)[(__bridge NSView *)_view layer];
#else
    _layer = (CAMetalLayer *)[(__bridge UIView *)_view layer];
#endif

    RendererDesc rendererDesc = {};
    rendererDesc.mEnableGPUBasedValidation = true;
    _renderer = nullptr;
    initRenderer("haxe_forge", &rendererDesc, &_renderer);

    _layer.device = _renderer->pDevice;

    ///    _layer.framebufferOnly = NO;
    _layer.framebufferOnly = YES;  // todo: optimized way
    _layer.pixelFormat = false ? MTLPixelFormatRGBA16Float : MTLPixelFormatBGRA8Unorm;
    _layer.wantsExtendedDynamicRangeContent = false ? true : false;
    _layer.drawableSize = CGSizeMake(wmInfo.info.cocoa.window.frame.size.width, wmInfo.info.cocoa.window.frame.size.height);
}

SwapChain *ForgeSDLWindow::createSwapChain(Renderer *renderer, Queue *queue, int width, int height, int chainCount, bool hdr10) {
    //	SDL_WindowData* data = (__bridge SDL_WindowData *)window->driverdata;
    //   NSView *view = data.nswindow.contentView;

    //	NSView *view = [nswin contentView];

    SwapChainDesc swapChainDesc = {};
    swapChainDesc.mWindowHandle.window = _view;
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

void ForgeSDLWindow::present(Queue *pGraphicsQueue, SwapChain *pSwapChain, int swapchainImageIndex, Semaphore *pRenderCompleteSemaphore) {
    QueuePresentDesc presentDesc = {};
    presentDesc.mIndex = swapchainImageIndex;
    presentDesc.mWaitSemaphoreCount = 1;
    presentDesc.pSwapChain = pSwapChain;
    presentDesc.ppWaitSemaphores = &pRenderCompleteSemaphore;
    presentDesc.mSubmitDone = true;

    queuePresent(pGraphicsQueue, &presentDesc);
}
SDL_Window *forge_sdl_get_window(void *ptr) {
    return static_cast<SDL_Window *>(ptr);
}

Buffer *forge_sdl_buffer_load(BufferLoadDesc *bld, SyncToken *token) {
    printf("Loading buffer \n");
    Buffer *tmp = nullptr;
    bld->ppBuffer = &tmp;
    printf("adding resource %p %p\n", bld, token);
    addResource(bld, token);
    return tmp;
}

Texture *forge_texture_load(TextureLoadDesc *desc, SyncToken *token) {
    printf("Loading texture \n");
    Texture *tmp = nullptr;
    desc->ppTexture = &tmp;
    printf("adding texture resource %p %p\n", desc, token);
    addResource(desc, token);
    return tmp;
}

Texture *forge_texture_load_from_desc(TextureDesc *tdesc, const char *name, SyncToken *token) {
    printf("Creating texture %s from description %d %d\n", name, tdesc->mWidth, tdesc->mHeight);
    const char *oldName = tdesc->pName;
    tdesc->pName = name;
    tdesc->mDescriptors = DESCRIPTOR_TYPE_TEXTURE | DESCRIPTOR_TYPE_RW_TEXTURE;

    TextureLoadDesc defaultLoadDesc = {};
    defaultLoadDesc.pDesc = tdesc;
    Texture *tmp = nullptr;
    defaultLoadDesc.ppTexture = &tmp;
    addResource(&defaultLoadDesc, token);

    tdesc->pName = oldName;
    return tmp;
}

void forge_texture_set_file_name(TextureLoadDesc *desc, const char *path) {
}

void forge_sdl_buffer_load_desc_set_index_buffer(BufferLoadDesc *bld, int size, void *data, bool shared) {
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

void forge_render_target_clear(Cmd *cmd, RenderTarget *pRenderTarget, RenderTarget *pDepthStencilRT) {
    // simply record the screen cleaning command
    LoadActionsDesc loadActions = {};
    loadActions.mLoadActionsColor[0] = LOAD_ACTION_CLEAR;
    loadActions.mLoadActionDepth = LOAD_ACTION_CLEAR;
    loadActions.mClearDepth.depth = 0.0f;
    loadActions.mClearDepth.stencil = 0;
    cmdBindRenderTargets(cmd, 1, &pRenderTarget, pDepthStencilRT, &loadActions, NULL, NULL, -1, -1);
    cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 0.0f, 1.0f);
    cmdSetScissor(cmd, 0, 0, pRenderTarget->mWidth, pRenderTarget->mHeight);
}

void forge_cmd_unbind(Cmd *cmd) {
	cmdBindRenderTargets(cmd, 0, NULL, NULL, NULL, NULL, NULL, -1, -1);
}

void forge_renderer_wait_fence(Renderer *pRenderer, Fence *pFence) {
    FenceStatus fenceStatus;
    getFenceStatus(pRenderer, pFence, &fenceStatus);
    if (fenceStatus == FENCE_STATUS_INCOMPLETE)
        waitForFences(pRenderer, 1, &pFence);
}

RenderTarget *forge_swap_chain_get_render_target(SwapChain *pSwapChain, int swapchainImageIndex) {
    return pSwapChain->ppRenderTargets[swapchainImageIndex];
}
void forge_sdl_buffer_load_desc_set_vertex_buffer(BufferLoadDesc *bld, int size, void *data, bool shared) {
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
    RenderTarget *pRT;
    addRenderTarget(renderer, desc, &pRT);
    return pRT;
}

SDL_MetalView forge_create_metal_view(SDL_Window *win) {
    return SDL_Metal_CreateView(win);
}

void forge_sdl_texture_upload(Texture *tex, void *data, int dataSize) {
    TextureUpdateDesc updateDesc = {tex};
    beginUpdateResource(&updateDesc);
    memcpy(updateDesc.pMappedData, data, dataSize);
    endUpdateResource(&updateDesc, NULL);
}

CmdPool *forge_sdl_renderer_create_cmd_pool(Renderer *pRenderer, Queue *pGraphicsQueue) {
    CmdPool *tmp;
    CmdPoolDesc cmdPoolDesc = {};
    cmdPoolDesc.pQueue = pGraphicsQueue;
    addCmdPool(pRenderer, &cmdPoolDesc, &tmp);

    return tmp;
}
Cmd *forge_sdl_renderer_create_cmd(Renderer *pRenderer, CmdPool *pPool) {
    Cmd *pCmd;
    CmdDesc cmdDesc = {};
    cmdDesc.pPool = pPool;
    addCmd(pRenderer, &cmdDesc, &pCmd);
    return pCmd;
}
Fence *forge_sdl_renderer_create_fence(Renderer *pRenderer) {
    Fence *tmp;
    addFence(pRenderer, &tmp);
    return tmp;
}
Semaphore *forge_sdl_renderer_create_semaphore(Renderer *pRenderer) {
    Semaphore *tmp;
    addSemaphore(pRenderer, &tmp);
    return tmp;
}

void forge_queue_submit_cmd(Queue *queue, Cmd *cmd, Semaphore *signalSemphor, Semaphore *wait, Fence *signalFence) {
    QueueSubmitDesc submitDesc = {};
    submitDesc.mCmdCount = 1;
    submitDesc.mSignalSemaphoreCount = 1;
    submitDesc.mWaitSemaphoreCount = 1;
    submitDesc.ppCmds = &cmd;
    submitDesc.ppSignalSemaphores = &signalSemphor;
    submitDesc.ppWaitSemaphores = &wait;
    submitDesc.pSignalFence = signalFence;
    queueSubmit(queue, &submitDesc);
}

std::string removeExtension(const std::string &path) {
    size_t lastindex = path.find_last_of(".");
    return path.substr(0, lastindex);
}

std::string getFilename(const std::string &path) {
    size_t lastindex = path.find_last_of("/");
    if (lastindex < 0) return path;

    return path.substr(lastindex + 1);
}

Shader *forge_renderer_shader_create(Renderer *pRenderer, const char *vertFile, const char *fragFile) {
    auto vertSrc = getShaderSource(vertFile);
    auto fragSrc = getShaderSource(fragFile);

    if (vertSrc.length() == 0) {
        std::cout << "Could not read vert source: " << vertFile << std::endl;
        return nullptr;
    }

    auto vertSpirvAsm = compile_file_to_assembly(vertFile, HLFG_SHADER_VERTEX, vertSrc, false);
    auto fragSpirvAsm = compile_file_to_assembly(fragFile, HLFG_SHADER_FRAGMENT, fragSrc, false);

    std::string vertFilePathOriginal(vertFile);
    std::string fragFilePathOriginal(fragFile);

    std::string vertFilePath = removeExtension(vertFilePathOriginal);
    std::string fragFilePath = removeExtension(fragFilePathOriginal);

    auto vertFilePathSpirv = vertFilePath + ".spirvasm";
    auto fragFilePathSpirv = fragFilePath + ".spirvasm";

    writeShaderSource(vertFilePathSpirv, vertSpirvAsm);
    writeShaderSource(fragFilePathSpirv, fragSpirvAsm);

    auto vertCode = assemble_to_spv(vertSpirvAsm);
    auto fragCode = assemble_to_spv(fragSpirvAsm);

    writeShaderSPV(vertFilePath + ".spv", vertCode);
    writeShaderSPV(fragFilePath + ".spv", fragCode);

    printf("Getting MTL\n");
    auto vertMSL = getMSLFromSPV(vertCode);
    auto fragMSL = getMSLFromSPV(fragCode);

    auto vertFilePathMSL = vertFilePath + ".metal";
    auto fragFilePathMSL = fragFilePath + ".metal";

    printf("writing MTL\n");
    writeShaderSource(vertFilePathMSL, vertMSL);
    writeShaderSource(fragFilePathMSL, fragMSL);

    auto vertFN = getFilename(vertFilePath);
    auto fragFN = getFilename(fragFilePath);
    //

    ShaderLoadDesc shaderDesc = {};

    shaderDesc.mStages[0] = {vertFN.c_str(), NULL, 0, "main0", SHADER_STAGE_LOAD_FLAG_ENABLE_VR_MULTIVIEW};
    shaderDesc.mStages[1] = {fragFN.c_str(), NULL, 0, "main0"};
    Shader *tmp = nullptr;
    addShader(pRenderer, &shaderDesc, &tmp);
    return tmp;
}

RootSignature *forge_renderer_createRootSignatureSimple(Renderer *pRenderer, Shader *shader) {
    RootSignature *tmp;

    RootSignatureDesc rootDesc = {&shader, 1, 0, nullptr, nullptr, 0};
    addRootSignature(pRenderer, &rootDesc, &tmp);
    return tmp;
}

RootSignature *forge_renderer_createRootSignature(Renderer *pRenderer, RootSignatureFactory *desc) {
    return desc->create(pRenderer );
}

RootSignatureFactory::RootSignatureFactory() {
	
}
RootSignatureFactory::~RootSignatureFactory() {

}

/*
Shader**           ppShaders;
	uint32_t           mShaderCount;
	uint32_t           mMaxBindlessTextures;
	const char**       ppStaticSamplerNames;
	Sampler**          ppStaticSamplers;
	uint32_t           mStaticSamplerCount;
	RootSignatureFlags mFlags;
*/
RootSignature *RootSignatureFactory::create(Renderer *pRenderer) {
	RootSignature *tmp;

    RootSignatureDesc rootDesc = {&_shaders[0], (uint32_t)_shaders.size() };
    
	
    addRootSignature(pRenderer, &rootDesc, &tmp);
	return tmp;
}
void RootSignatureFactory::addShader( Shader *pShader ) {
	_shaders.push_back(pShader);
}
void RootSignatureFactory::addSampler( Sampler * sampler ) {
	_samplers.push_back(sampler);
}
///// boilerplate

template <typename T>
struct pref {
    void (*finalize)(pref<T> *);
    T *value;
};
template <typename T>
pref<T> *_alloc_const(const T *value) {
    if (value == nullptr) return nullptr;
    pref<T> *r = (pref<T> *)hl_gc_alloc_noptr(sizeof(pref<T>));
    r->finalize = NULL;
    r->value = (T *)value;
    return r;
}
#define _ref(t) pref<t>

HL_PRIM _ref(SDL_Window) * HL_NAME(unpackSDLWindow)(void *ptr) {
    printf("Forge SDL Pointer is %p\n", ptr);
    return _alloc_const((SDL_Window *)ptr);
}

/*
HL_PRIM HL_CONST _ref(SDL_Window)* HL_NAME(Window_getWindow1)(void* ptr) {
        return alloc_ref_const((forge_sdl_get_window(ptr)),Window);
}
DEFINE_PRIM(_IDL, Window_getWindow1, _BYTES);
*/

DEFINE_PRIM(_BYTES, unpackSDLWindow, TWIN);
