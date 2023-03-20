

#include <IInput.h>
#include <ILog.h>
//#include <Renderer/IRenderer.h>
#include <IResourceLoader.h>
#include <IShaderReflection.h>
#include <IOperatingSystem.h>
#include <basis_universal/basisu_enc.h>
#include <tinyimageformat/tinyimageformat_apis.h>
#include <tinyimageformat/tinyimageformat_base.h>
#include <tinyimageformat/tinyimageformat_bits.h>
#include <tinyimageformat/tinyimageformat_query.h>
//#include <TextureContainers.h>

#include <filesystem>
#include <iostream>
#include <sstream>

#include "hl-forge-shaders.h"

#define HL_NAME(x) forge_##x
#include <hl.h>

#ifdef __APPLE__
#include "hl-forge-meta.h"
#endif
#include "hl-forge.h"


#ifdef __APPLE__
#include "hl-forge_osx.hpp"
#endif


struct BufferDispose {
    BufferDispose(Buffer *buffer, uint64_t frame) {
        this->buffer = buffer;
        this->frame = frame;
    }
    Buffer *buffer;
    uint64_t frame;
};

BufferLoadDescExt::BufferLoadDescExt() : _depth(1) {

    this->ppBuffer = nullptr;
    this->pData = nullptr;
    this->mForceReset = false;
    this->mDesc = {};
}

BufferLoadDescExt::~BufferLoadDescExt() {

}
static CpuInfo gCpu;
// This needs to be renderer relative? or window relative?
static int64 _frame = 0;
// Need to make this renderer relative [TODO] RC
static std::vector<BufferDispose> _toDisposeA;
static std::vector<BufferDispose> _toDisposeB;
static std::vector<BufferDispose> *_toDispose = nullptr;
// From SDL
#ifdef __APPLE__
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

#endif


bool hlForgeInitialize(const char *name) {
    // init memory allocator
    DEBUG_PRINT("Intializing memory\n");
    if (!initMemAlloc(name)) {
        DEBUG_PRINT("Failed to init memory allocator\n");
        return false;
    }

    FileSystemInitDesc fsDesc = {};
    fsDesc.pAppName = name;
    fsDesc.pResourceMounts[RM_CONTENT] = ".";
    fsDesc.pResourceMounts[RM_DEBUG] = ".";
    
    DEBUG_PRINT("Intializing file system\n");

    // init file system
    if (!initFileSystem(&fsDesc)) {
        DEBUG_PRINT("Failed to init file system\n");
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
            fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_SHADER_SOURCES, "shadercache/"); // it post-pends vulkan anyway
            fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_SHADER_BINARIES, "shadercache/binary/");
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

void forge_blend_state_desc_set_rt(BlendStateDesc *bs, BlendStateTargets rt, bool enabled) {
    if (enabled) {
        bs->mRenderTargetMask = (BlendStateTargets)(bs->mRenderTargetMask | rt);
    } else {
        bs->mRenderTargetMask = (BlendStateTargets)(bs->mRenderTargetMask & ~rt);
    }
}

VertexAttrib *forge_vertex_layout_get_attrib(VertexLayout *layout, int idx) {
    return &layout->mAttribs[idx];
}
void forge_vertex_layout_set_stride(VertexLayout *layout, int idx, int stride) {
    layout->mStrides[idx] = stride;
}

int forge_vertex_layout_get_stride(VertexLayout *layout, int idx) {
    return layout->mStrides[idx];
}

void forge_vertex_attrib_set_semantic(VertexAttrib *attrib, const char *name) {
    strncpy(&attrib->mSemanticName[0], name, MAX_SEMANTIC_NAME_LENGTH);
    //    attrib->mSemanticNameLength
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
    DEBUG_PRINT("SDL_Window %p\n", window);
    #ifdef __APPLE__
    DEBUG_PRINT("NSWindow %p\n", wmInfo.info.cocoa.window);
    void *view = getNSViewFromNSWindow(wmInfo.info.cocoa.window);
    DEBUG_PRINT("NSVIew %p\n", view);

    if (IsMetalAvailable(&wmInfo) != 0) {
        DEBUG_PRINT("Metal is not avaialble!!!\n");
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

    #elif defined( _WINDOWS )


    DEBUG_PRINT("Window Info %p %p %p\n", wmInfo.info.win.window, wmInfo.info.win.hdc, wmInfo.info.win.hinstance);
    auto window_flags = SDL_GetWindowFlags(window);
    if (!(window_flags & SDL_WINDOW_VULKAN)) {
        assert(false && "WTF - Vulkan isn't supported by the window");
    }
    RendererDesc rendererDesc = {};

    rendererDesc.mEnableGPUBasedValidation = true;
    rendererDesc.mD3D11Supported = false;
    _renderer = nullptr;
    initRenderer("haxe_forge", &rendererDesc, &_renderer);

    #else
        void *view = getNSViewFromNSWindow(wmInfo.info.cocoa.window);
    DEBUG_PRINT("NSVIew %p\n", view);

    if (IsMetalAvailable(&wmInfo) != 0) {
        DEBUG_PRINT("Metal is not avaialble!!!\n");
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

    rendererDesc.mEnableGPUBasedValidation = true;
    _renderer = nullptr;
    initRenderer("haxe_forge", &rendererDesc, &_renderer);

    _layer.device = _renderer->pDevice;

    ///    _layer.framebufferOnly = NO;
    _layer.framebufferOnly = YES;  // todo: optimized way
    _layer.pixelFormat = false ? MTLPixelFormatRGBA16Float : MTLPixelFormatBGRA8Unorm;
    _layer.wantsExtendedDynamicRangeContent = false ? true : false;
    _layer.drawableSize = CGSizeMake(wmInfo.info.cocoa.window.frame.size.width, wmInfo.info.cocoa.window.frame.size.height);
    
    #endif
   
}



SwapChain *ForgeSDLWindow::createSwapChain(Renderer *renderer, Queue *queue, int width, int height, int chainCount, bool hdr10) {
    //SDL_WindowData* data = (__bridge SDL_WindowData *)window->driverdata;
    //   NSView *view = data.nswindow.contentView;

    //	NSView *view = [nswin contentView];
    
#ifdef __APPLE__
    _layer.drawableSize = CGSizeMake(width, height);
#else
#endif
    SwapChainDesc swapChainDesc = {};
    #ifdef __APPLE__
    swapChainDesc.mWindowHandle.window = _view;
#elif _WINDOWS
    swapChainDesc.mWindowHandle.window = wmInfo.info.win.window;
#endif

    swapChainDesc.mPresentQueueCount = 1;
    swapChainDesc.ppPresentQueues = &queue;
    swapChainDesc.mWidth = width;
    swapChainDesc.mHeight = height;
    swapChainDesc.mImageCount = chainCount;

    printf("ForgeSDLWindow::createSwapChain - Swap chain dimensions %d %d handle %p \n", width, height, swapChainDesc.mWindowHandle.window);

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

void forge_renderer_destroySwapChain(Renderer *pRenderer, SwapChain *swapChain) {
    removeSwapChain(pRenderer, swapChain);
}

void forge_renderer_destroyRenderTarget(Renderer *pRenderer, RenderTarget *rt) {
    removeRenderTarget(pRenderer, rt);
}

static void disposeStaleBuffers( ) {
     _frame++;
    auto *last = _toDispose;
    if (_toDispose == nullptr)
        _toDispose = &_toDisposeA;
    else if (_toDispose == &_toDisposeA)
        _toDispose = &_toDisposeB;
    else
        _toDispose = &_toDisposeA;

    _toDispose->clear();

    if (last != nullptr) {
        for (auto i = 0; i < last->size(); i++) {
            auto &x = (*last)[i];
            if (x.frame + 3 < _frame) { // TODO: make this configurable [RC]
                removeResource(x.buffer);
            } else {
                _toDispose->push_back(x);
            }
        }
    }
}

void ForgeSDLWindow::present(Queue *pGraphicsQueue, SwapChain *pSwapChain, int swapchainImageIndex, Semaphore *pRenderCompleteSemaphore) {
    QueuePresentDesc presentDesc = {};
    presentDesc.mIndex = swapchainImageIndex;
    presentDesc.mWaitSemaphoreCount = 1;
    presentDesc.pSwapChain = pSwapChain;
    presentDesc.ppWaitSemaphores = &pRenderCompleteSemaphore;
    presentDesc.mSubmitDone = true;

    queuePresent(pGraphicsQueue, &presentDesc);

    disposeStaleBuffers();
   
}
SDL_Window *forge_sdl_get_window(void *ptr) {
    return static_cast<SDL_Window *>(ptr);
}


Texture *forge_texture_load(TextureLoadDesc *desc, SyncToken *token) {
    DEBUG_PRINT("Loading texture \n");
    Texture *tmp = nullptr;
    desc->ppTexture = &tmp;
    DEBUG_PRINT("adding texture resource %p %p\n", desc, token);
    addResource(desc, token);
    return tmp;
}

Texture *forge_texture_load_from_desc(TextureDesc *tdesc, const char *name, SyncToken *token) {
    DEBUG_PRINT("Creating texture %s from description %d %d\n", name, tdesc->mWidth, tdesc->mHeight);
    const char *oldName = tdesc->pName;
    tdesc->pName = name;
    //    tdesc->mDescriptors = DESCRIPTOR_TYPE_TEXTURE | DESCRIPTOR_TYPE_RW_TEXTURE;

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



void forge_render_target_bind(Cmd *cmd, RenderTarget *pRenderTarget, RenderTarget *pDepthStencilRT, LoadActionType color, LoadActionType depth) {
    // simply record the screen cleaning command
    LoadActionsDesc loadActions = {};
    loadActions.mLoadActionsColor[0] = color;
    loadActions.mLoadActionDepth = depth;

    //    DEBUG_PRINT("RENDER CLEAR c++ BIND %f %f %f %f\n", pRenderTarget->mClearValue.r, pRenderTarget->mClearValue.g, pRenderTarget->mClearValue.b, pRenderTarget->mClearValue.a );

    // This seems to trump the stuff below
    if (pDepthStencilRT != nullptr) {
        loadActions.mClearDepth.depth = pDepthStencilRT->mClearValue.depth;
        loadActions.mClearDepth.stencil = pDepthStencilRT->mClearValue.stencil;
    }
    loadActions.mClearColorValues[0].r = pRenderTarget->mClearValue.r;
    loadActions.mClearColorValues[0].g = pRenderTarget->mClearValue.g;
    loadActions.mClearColorValues[0].b = pRenderTarget->mClearValue.b;
    loadActions.mClearColorValues[0].a = pRenderTarget->mClearValue.a;

    // DEBUG_PRINT("RENDER CLEAR c++ %f %f %f %f - %f %d\n", loadActions.mClearColorValues[0].r, loadActions.mClearColorValues[0].g, loadActions.mClearColorValues[0].b, loadActions.mClearColorValues[0].a,  loadActions.mClearDepth.depth, loadActions.mClearDepth.stencil);
    cmdBindRenderTargets(cmd, 1, &pRenderTarget, pDepthStencilRT, &loadActions, NULL, NULL, -1, -1);
    cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 0.0f, 1.0f);
    cmdSetScissor(cmd, 0, 0, pRenderTarget->mWidth, pRenderTarget->mHeight);
}
void forge_render_target_set_clear_colour(RenderTarget *rt, float r, float g, float b, float a) {
    rt->mClearValue.r = r;
    rt->mClearValue.g = g;
    rt->mClearValue.b = b;
    rt->mClearValue.a = a;

    // DEBUG_PRINT("RENDER CLEAR SET %f %f %f %f\n", rt->mClearValue.r, rt->mClearValue.g, rt->mClearValue.b, rt->mClearValue.a );
}
void forge_render_target_set_clear_depth(RenderTarget *rt, float depth, int stencil) {
    // DEBUG_PRINT("RENDER CLEAR SET DEPTH %f %f %f %f\n", rt->mClearValue.r, rt->mClearValue.g, rt->mClearValue.b, rt->mClearValue.a );
    rt->mClearValue.depth = depth;
    rt->mClearValue.stencil = stencil;
    // DEBUG_PRINT("RENDER CLEAR SET DEPTH %f %f %f %f\n", rt->mClearValue.r, rt->mClearValue.g, rt->mClearValue.b, rt->mClearValue.a );
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







/*
RenderTargetBarrier barriers[] = { { pRenderTarget, RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET },
                                { pRenderTargetShadowMap, RESOURCE_STATE_SHADER_RESOURCE, RESOURCE_STATE_DEPTH_WRITE } };
cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 2, barriers);

cmdBindRenderTargets(cmd, 0, NULL, NULL, NULL, NULL, NULL, -1, -1);

RenderTargetBarrier shadowTexBarrier = { pRenderTargetShadowMap, RESOURCE_STATE_DEPTH_WRITE, RESOURCE_STATE_SHADER_RESOURCE };
cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, &shadowTexBarrier);


*/

void forge_cmd_insert_barrier(Cmd *cmd, ResourceBarrierBuilder *barrier) {
    barrier->insert(cmd);
}

void forge_cmd_wait_for_render(Cmd *cmd, RenderTarget *pRenderTarget) {
    RenderTargetBarrier barriers[] =  // wait for resource transition
        {
            {pRenderTarget, RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET},
        };
    cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriers);
}

void forge_cmd_wait_for_present(Cmd *cmd, RenderTarget *pRenderTarget) {
    RenderTargetBarrier barriers[] =  // wait for resource transition
        {
            {pRenderTarget, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_PRESENT},
        };
    cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriers);
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
    //    updateDesc.mArrayLayer
    DEBUG_PRINT("TEXTURE upading texture width %d height %d\n", tex->mWidth, tex->mHeight);
    beginUpdateResource(&updateDesc);
    memcpy(updateDesc.pMappedData, data, dataSize);
    endUpdateResource(&updateDesc, NULL);
}

void forge_texture_upload_mip(Texture *tex, int mip, void *data, int dataSize) {
    forge_texture_upload_layer_mip(tex, 0, mip, data, dataSize);
}

void forge_texture_upload_layer_mip(Texture *tex, int layer, int mip, void *data, int dataSize) {
    TextureUpdateDesc updateDesc = {tex};

    updateDesc.mMipLevel = mip;
    updateDesc.mArrayLayer = layer;

    DEBUG_PRINT("TEXTURE upading texture layer %d width %d height %d mip %d\n", layer, tex->mWidth, tex->mHeight, mip);
    beginUpdateResource(&updateDesc);
    memcpy(updateDesc.pMappedData, data, dataSize);
    endUpdateResource(&updateDesc, NULL);
}

Shader *forge_load_compute_shader_file(Renderer *pRenderer, const char *fileName) {
    ShaderLoadDesc GenerateMipShaderDesc = {};
    GenerateMipShaderDesc.mStages[0] = {fileName, NULL, 0, NULL, SHADER_STAGE_LOAD_FLAG_ENABLE_VR_MULTIVIEW};
    Shader *tmp = nullptr;
    addShader(pRenderer, &GenerateMipShaderDesc, &tmp);
    return tmp;
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
    submitDesc.mSignalSemaphoreCount = signalSemphor == nullptr ? 0 : 1;
    submitDesc.mWaitSemaphoreCount = wait == nullptr ? 0 : 1;
    submitDesc.ppCmds = &cmd;
    submitDesc.ppSignalSemaphores = signalSemphor == nullptr ? nullptr : &signalSemphor;
    submitDesc.ppWaitSemaphores = wait == nullptr ? nullptr : &wait;
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

DescriptorSet *forge_renderer_create_descriptor_set(Renderer *pRenderer, RootSignature *pRootSignature, int setIndex, uint maxSets, uint nodeIndex) {
    DescriptorSet *tmp;
    DescriptorSetDesc desc = {
        pRootSignature,
        (DescriptorUpdateFrequency)setIndex,
        maxSets,
        nodeIndex};

    addDescriptorSet(pRenderer, &desc, &tmp);
    return tmp;
}

void forge_render_target_capture(RenderTarget *rt, Buffer *pTransferBuffer, Semaphore *semaphore) {
    TextureCopyDesc copyDesc = {};
    copyDesc.pTexture = rt->pTexture;
    copyDesc.pBuffer = pTransferBuffer;
    copyDesc.pWaitSemaphore = semaphore;
    copyDesc.mTextureState = RESOURCE_STATE_RENDER_TARGET;
    copyDesc.mQueueType = QUEUE_TYPE_GRAPHICS;
    SyncToken *st = nullptr;
    copyResource(&copyDesc, st);
}

void mapRenderTarget(Renderer *pRenderer, Queue *pQueue, Cmd *pCmd, RenderTarget *pRenderTarget, ResourceState currentResourceState, void *pImageData);

int forge_render_target_capture_size(RenderTarget *pRenderTarget) {
    uint16_t byteSize = TinyImageFormat_BitSizeOfBlock(pRenderTarget->mFormat) / 8;
    uint8_t channelCount = TinyImageFormat_ChannelCount(pRenderTarget->mFormat);

    return pRenderTarget->mWidth * pRenderTarget->mHeight * byteSize;
}
bool forge_render_target_capture_2(Renderer *pRenderer, Cmd *pCmd, RenderTarget *pRenderTarget, Queue *pQueue, ResourceState renderTargetCurrentState, uint8_t *alloc, int bufferSize) {
    printf("CAPTURE alloc buffer is %p of size %d\n", alloc, bufferSize);
    // Wait for queue to finish rendering.
    waitQueueIdle(pQueue);

    // Allocate temp space
    uint16_t byteSize = TinyImageFormat_BitSizeOfBlock(pRenderTarget->mFormat) / 8;
    uint8_t channelCount = TinyImageFormat_ChannelCount(pRenderTarget->mFormat);
    if (bufferSize != pRenderTarget->mWidth * pRenderTarget->mHeight * byteSize) return false;
    //	void*    alloc = tf_malloc(pRenderTarget->mWidth * pRenderTarget->mHeight * byteSize);
    //	resetCmdPool(pRenderer, pCmdPool);

    // Generate image data buffer.
    mapRenderTarget(pRenderer, pQueue, pCmd, pRenderTarget, renderTargetCurrentState, alloc);

    // Flip the BGRA to RGBA
    const bool flipRedBlueChannel = pRenderTarget->mFormat != TinyImageFormat_R8G8B8A8_UNORM;
    if (flipRedBlueChannel) {
        int8_t *imageData = ((int8_t *)alloc);

        for (uint32_t h = 0; h < pRenderTarget->mHeight; ++h) {
            for (uint32_t w = 0; w < pRenderTarget->mWidth; ++w) {
                uint32_t pixelIndex = (h * pRenderTarget->mWidth + w) * channelCount;
                int8_t *pixel = imageData + pixelIndex;

                // Swap blue and red.
                int8_t r = pixel[0];
                pixel[0] = pixel[2];
                pixel[2] = r;
            }
        }
    }

    return true;
}
/*
Buffer *forge_create_transfer_buffer(Renderer *rp, TinyImageFormat format, int width, int height, int nodeIndex) {
    const uint32_t rowAlignment = max(1u, rp->pActiveGpuSettings->mUploadBufferTextureRowAlignment);
    const uint32_t blockSize = max(1u, TinyImageFormat_BitSizeOfBlock(format));
    const uint32_t sliceAlignment = round_up(round_up(rp->pActiveGpuSettings->mUploadBufferTextureAlignment, blockSize), rowAlignment);

    BufferLoadDesc bufferDesc = {};
    bufferDesc.mDesc.mSize = util_get_surface_size(format, width, height, 1, rowAlignment, sliceAlignment, 0, 1, 0, 1);
    bufferDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_TO_CPU;
    bufferDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
    bufferDesc.mDesc.mQueueType = QUEUE_TYPE_TRANSFER;
    bufferDesc.mDesc.mNodeIndex = nodeIndex;

    Buffer *tmp = nullptr;
    bufferDesc.ppBuffer = &tmp;

    addResource(&bufferDesc, NULL);

    return tmp;
}
*/
void BufferExt::dispose() {
    if (_toDispose == nullptr) _toDispose = &_toDisposeA;
    for (int i = 0; i < _buffers.size(); i++) {
        _toDispose->push_back(BufferDispose(_buffers[i], _frame));
    }
}

/*
HL_PRIM void HL_NAME(Cmd_bindIndexBuffer3)(pref<Cmd>* _this, pref<BufferExt>* b, int format, int offset) {
	(cmdBindIndexBuffer( _unref(_this) , _unref_ptr_safe(b), IndexType__values[format], offset));
}
*/

void BufferExt::bindAsIndex(Cmd *cmd, BufferExt *b, IndexType it, int offset) {
    cmdBindIndexBuffer(cmd, b->current(),  it, offset);
}

#ifdef __APPLE__
std::string forge_translate_glsl_native(const char *source, const char *filepath, bool fragment) {
    auto shaderKind = fragment ? HLFG_SHADER_FRAGMENT : HLFG_SHADER_VERTEX;
    auto spirvASM = compile_file_to_assembly(filepath, shaderKind, source, false);
    auto spvCode = assemble_to_spv(spirvASM);
    return getMSLFromSPV(spvCode);
}
#elif defined(WIN32)
std::string forge_translate_glsl_native(const char *source, const char *filepath, bool fragment) {
    DEBUG_PRINT("Translating to vulkan\n");
    auto shaderKind = fragment ? HLFG_SHADER_FRAGMENT : HLFG_SHADER_VERTEX;
    auto spirvASM = compile_file_to_assembly(filepath, shaderKind, source, false);
    auto spvCode = assemble_to_spv(spirvASM);
    return getVulkanFromSPV(spvCode);
}
#endif

bool isTargetFileOutOfDate(const std::string &source, const std::string &target) {
    if (!std::filesystem::exists(source)) return false;
    if (!std::filesystem::exists(target)) return true;
    auto sourceTime = std::filesystem::last_write_time(source);  // read back from the filesystem
    auto targetTime = std::filesystem::last_write_time(target);  // read back from the filesystem
    return sourceTime > targetTime;
}

#if __APPLE__
void generateNativeShader(const std::string &glslPath, const std::string &metalPath, bool fragment) {
    if (isTargetFileOutOfDate(glslPath, metalPath)) {
        auto src = getShaderSource(glslPath);

        if (src.length() == 0) {
            std::cout << "Could not read GLSL source: " << glslPath << std::endl;
            return;
        }

        auto msl = forge_translate_glsl_native(src.c_str(), glslPath.c_str(), fragment);
        if (fragment) {
            auto bufferspot = msl.find("spvDescriptorSet0 [[buffer(");
            if (bufferspot != std::string::npos) {
                DEBUG_PRINT("RENDER modifying metal shader code at %d\n", bufferspot);
                bufferspot += sizeof("spvDescriptorSet0 [[buffer(") - 1;
                msl = msl.replace(bufferspot, 1, "UPDATE_FREQ_PER_DRAW");
            }
        }

        writeShaderSource(metalPath, msl);
    }
}
#elif defined(WIN32)
void generateNativeShader(const std::string &glslPath, const std::string &vulkanPath, bool fragment) {
    DEBUG_PRINT("Generating vulkan shader %s to %s\n", glslPath.c_str(), vulkanPath.c_str());
    if (isTargetFileOutOfDate(glslPath, vulkanPath) || true) {
        auto src = getShaderSource(glslPath);
        DEBUG_PRINT("\tGetting source\n");

        if (src.length() == 0) {
            std::cout << "Could not read GLSL source: " << glslPath << std::endl;
            return;
        }

        auto vulkan = forge_translate_glsl_native(src.c_str(), glslPath.c_str(), fragment);

/*
        auto updateFreqSpot = vulkan.find("(set = 3,");

        while (updateFreqSpot != std::string::npos) {
            vulkan = vulkan.replace(updateFreqSpot + 1, 7, "UPDATE_FREQ_PER_DRAW");
            updateFreqSpot = vulkan.find("(set = 3,");
        }*/
        /*
        if (fragment) {
            auto bufferspot = msl.find("spvDescriptorSet0 [[buffer(");
            if (bufferspot != std::string::npos) {
                DEBUG_PRINT("RENDER modifying metal shader code at %d\n", bufferspot);
                bufferspot += sizeof("spvDescriptorSet0 [[buffer(") - 1;
                msl = msl.replace(bufferspot, 1, "UPDATE_FREQ_PER_DRAW");
            }
        }
        */
        DEBUG_PRINT("\twriting source to %s\n", vulkanPath.c_str());

        writeShaderSource(vulkanPath, vulkan);
    } else {
        DEBUG_PRINT("\tUp to date\n");
    }
}
#endif

Shader *forge_renderer_shader_create(Renderer *pRenderer, const char *vertFile, const char *fragFile) {

    std::string vertFilePathOriginal(vertFile);
    std::string fragFilePathOriginal(fragFile);

    #ifdef __APPLE__
    std::string vertFilePath = removeExtension(vertFilePathOriginal);
    std::string fragFilePath = removeExtension(fragFilePathOriginal);
    auto vertFilePathSpecific = vertFilePath + ".metal";
    auto fragFilePathSpecific = fragFilePath + ".metal";

    generateNativeShader(vertFilePathOriginal, vertFilePathSpecific, false);
    generateNativeShader(fragFilePathOriginal, fragFilePathSpecific, true);
     auto vertFN = getFilename(vertFilePath);
    auto fragFN = getFilename(fragFilePath);
    #elif defined(_WINDOWS)
    std::string vertFilePath = removeExtension(vertFilePathOriginal); // removes glsl
    std::string fragFilePath = removeExtension(fragFilePathOriginal);
    vertFilePath = removeExtension(vertFilePath); // removes .vert
    fragFilePath = removeExtension(fragFilePath); // removes .frag

    auto vertFilePathSpecific = vertFilePath + ".vulkan.glsl.vert";
    auto fragFilePathSpecific = fragFilePath + ".vulkan.glsl.frag";

    generateNativeShader(vertFilePathOriginal, vertFilePathSpecific, false);
    generateNativeShader(fragFilePathOriginal, fragFilePathSpecific, true);
    auto vertFN = getFilename(vertFilePathSpecific);
    auto fragFN = getFilename(fragFilePathSpecific);
    #endif

    /*
        auto vertSrc = getShaderSource(vertFile);
        auto fragSrc = getShaderSource(fragFile);

        if (vertSrc.length() == 0) {
            std::cout << "Could not read vert source: " << vertFile << std::endl;
            return nullptr;
        }

        auto vertMSL = forge_translate_glsl_metal( vertSrc.c_str(), vertFile, false );
        auto fragMSL = forge_translate_glsl_metal( fragSrc.c_str(), fragFile, true );

    //    auto vertSpirvAsm = compile_file_to_assembly(vertFile, HLFG_SHADER_VERTEX, vertSrc, false);
      //  auto fragSpirvAsm = compile_file_to_assembly(fragFile, HLFG_SHADER_FRAGMENT, fragSrc, false);




        auto vertFilePathSpirv = vertFilePath + ".spirvasm";
        auto fragFilePathSpirv = fragFilePath + ".spirvasm";

    //    writeShaderSource(vertFilePathSpirv, vertSpirvAsm);
      //  writeShaderSource(fragFilePathSpirv, fragSpirvAsm);

    //    auto vertCode = assemble_to_spv(vertSpirvAsm);
      //  auto fragCode = assemble_to_spv(fragSpirvAsm);

        //writeShaderSPV(vertFilePath + ".spv", vertCode);
        //writeShaderSPV(fragFilePath + ".spv", fragCode);

        DEBUG_PRINT("Getting MTL\n");
    //    auto vertMSL = getMSLFromSPV(vertCode);
    //    auto fragMSL = getMSLFromSPV(fragCode);




        DEBUG_PRINT("writing MTL\n");
        writeShaderSource(vertFilePathMSL, vertMSL);
        writeShaderSource(fragFilePathMSL, fragMSL);
    */

   
    //

    ShaderLoadDesc shaderDesc = {};

#ifdef __APPLE__
    shaderDesc.mStages[0] = {vertFN.c_str(), NULL, 0, "main0", SHADER_STAGE_LOAD_FLAG_ENABLE_VR_MULTIVIEW};
    shaderDesc.mStages[1] = {fragFN.c_str(), NULL, 0, "main0"};
    #elif defined(WIN32)
    shaderDesc.mStages[0] = {vertFN.c_str(), NULL, 0, "main", SHADER_STAGE_LOAD_FLAG_ENABLE_VR_MULTIVIEW};
    shaderDesc.mStages[1] = {fragFN.c_str(), NULL, 0, "main"};
    #endif
    Shader *tmp = nullptr;
    addShader(pRenderer, &shaderDesc, &tmp);

    if (tmp == nullptr) {
        printf("Couldn't compile shader %s | %s\n",vertFN.c_str(), fragFN.c_str());
        ASSERT(tmp != nullptr);
        exit(-1);
    }

    if (tmp->pReflection) {
        std::stringstream ss;

        ss << "vertex stage index: " << tmp->pReflection->mVertexStageIndex << std::endl;
        ss << "frag stage index: " << tmp->pReflection->mPixelStageIndex << std::endl;

        ss << "stage reflection count: " << tmp->pReflection->mStageReflectionCount << std::endl;
        for (auto i = 0; i < tmp->pReflection->mStageReflectionCount; ++i) {
            auto &s = tmp->pReflection->mStageReflections[i];
            ss << "\t" << i << " stage:" << s.mShaderStage << std::endl;
            ss << "\t" << i << " variable count:" << s.mVariableCount << std::endl;
            ss << "\t" << i << " resource count:" << s.mShaderResourceCount << std::endl;

            if (s.mShaderStage == SHADER_STAGE_VERT) {
                ss << "\t" << i << " vert input counts:" << s.mVertexInputsCount << std::endl;
                for (auto vi = 0; vi < s.mVertexInputsCount; ++vi) {
                    auto &v = s.pVertexInputs[vi];
                    ss << "\t\t" << vi << " name:" << v.name << std::endl;
                    ss << "\t\t" << vi << " name size:" << v.name_size << std::endl;
                    ss << "\t\t" << vi << " size:" << v.size << std::endl;
                }
            }
        }

        ss << "resource count: " << tmp->pReflection->mShaderResourceCount << std::endl;
        for (auto i = 0; i < tmp->pReflection->mShaderResourceCount; ++i) {
            auto &r = tmp->pReflection->pShaderResources[i];
            ss << "\t" << i << " name:" << r.name << std::endl;
            ss << "\t" << i << " reg:" << r.reg << std::endl;
            #if __APPLE__
            ss << "\t" << i << " argument buffer field:" << r.mIsArgumentBufferField << std::endl;
            #endif
            
            ss << "\t" << i << " used stages:" << r.used_stages << std::endl;
            ss << "\t" << i << " set:" << r.set << std::endl;
            ss << "\t" << i << " size:" << r.size << std::endl;
            ss << "\t" << i << " dim:" << r.dim << std::endl;
            #ifdef __APPLE__
            ss << "\t" << i << " alignment:" << r.alignment << std::endl;
            ss << "\t" << i << " argument descriptor index:" << r.mtlArgumentDescriptors.mArgumentIndex << std::endl;
            ss << "\t" << i << " argument array length:" << r.mtlArgumentDescriptors.mArrayLength << std::endl;
            ss << "\t" << i << " argument buffer index:" << r.mtlArgumentDescriptors.mBufferIndex << std::endl;
            #endif

            ss << "\t" << i << " type:";

            switch (r.type) {
                case DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                    ss << "Uniform buffer";
                    break;
                case DESCRIPTOR_TYPE_SAMPLER:
                    ss << "Sampler";
                    break;
                case DESCRIPTOR_TYPE_TEXTURE:
                    ss << "Texture";
                    break;
                case DESCRIPTOR_TYPE_RW_TEXTURE:
                    ss << "Rw Texture";
                    break;
                case DESCRIPTOR_TYPE_BUFFER:
                    ss << "Buffer";
                    break;
                case DESCRIPTOR_TYPE_BUFFER_RAW:
                    ss << "Buffer Raw";
                    break;
                case DESCRIPTOR_TYPE_RW_BUFFER:
                    ss << "RW Buffer";
                    break;
                case DESCRIPTOR_TYPE_RW_BUFFER_RAW:
                    ss << "RW Buffer Raw";
                    break;
                case DESCRIPTOR_TYPE_ROOT_CONSTANT:
                    ss << "Root constant";
                    break;
                    #if __APPLE__
                case DESCRIPTOR_TYPE_INDIRECT_BUFFER:
                    ss << "Indirect buffer";
                    break;
                case DESCRIPTOR_TYPE_INDIRECT_COMMAND_BUFFER:
                    ss << "Indirect command buffer";
                    break;
                case DESCRIPTOR_TYPE_VERTEX_BUFFER:
                    ss << "Vertex  buffer";
                    break;
                case DESCRIPTOR_TYPE_INDEX_BUFFER:
                    ss << "Index  buffer";
                    break;
                    #endif
                default:
                    ss << "Unknown: " << r.type;
                    break;
            }

            ss << std::endl;
        }
        ss << "var count: " << tmp->pReflection->mVariableCount << std::endl;

        for (auto i = 0; i < tmp->pReflection->mVariableCount; ++i) {
            auto &v = tmp->pReflection->pVariables[i];
            ss << "\t" << i << " var name:" << v.name << std::endl;
            ss << "\t" << i << " offset:" << v.offset << std::endl;
            ss << "\t" << i << " parent:" << v.parent_index << std::endl;
        }
        std::string str = ss.str();
        writeShaderSource(vertFilePath + ".reflect", str);
    } else {
        DEBUG_PRINT("-->No reflection\n");
    }
    return tmp;
}

void hl_compile_native_to_bin(const char *filePath, const char *outFilePath) {
    DEBUG_PRINT("MTL COMPILE SHADER\n");
    char intermediateFilePath[FS_MAX_PATH] = {};
    sprintf(intermediateFilePath, "%s.air", outFilePath);

    const char *xcrun = "/usr/bin/xcrun";
    std::vector<std::string> args;
    char tmpArgs[FS_MAX_PATH + 10]{};

    // Compile the source into a temporary .air file.
    args.push_back("-sdk");
    args.push_back("macosx");
    args.push_back("metal");
    args.push_back("-frecord-sources=flat");
    args.push_back("-c");
    args.push_back(filePath);
    args.push_back("-o");
    args.push_back(intermediateFilePath);

    // enable the 2 below for shader debugging on xcode
    // args.push_back("-MO");
    // args.push_back("-gline-tables-only");
    args.push_back("-D");
    args.push_back("MTL_SHADER=1");  // Add MTL_SHADER macro to differentiate structs in headers shared by app/shader code.

    std::vector<const char *> cArgs;
    for (std::string &arg : args) {
        cArgs.push_back(arg.c_str());
    }

    std::string params;

    for (std::string &arg : args) {
        params += " " + arg;
    }

    DEBUG_PRINT("Running %s %s\n", xcrun, params.c_str());

    if (systemRun(xcrun, &cArgs[0], cArgs.size(), NULL) == 0) {
        // Create a .metallib file from the .air file.
        args.clear();
        sprintf(tmpArgs, "");
        args.push_back("-sdk");
        args.push_back("macosx");
        args.push_back("metallib");
        args.push_back(intermediateFilePath);
        args.push_back("-o");
        sprintf(
            tmpArgs,
            ""
            "%s"
            "",
            outFilePath);
        args.push_back(tmpArgs);

        cArgs.clear();
        for (std::string &arg : args) {
            cArgs.push_back(arg.c_str());
        }

        if (systemRun(xcrun, &cArgs[0], cArgs.size(), NULL) == 0) {
            // Remove the temp .air file.
            const char *nativePath = intermediateFilePath;
            systemRun("rm", &nativePath, 1, NULL);
        } else {
            LOGF(eERROR, "Failed to assemble shader's %s .metallib file", outFilePath);
        }
    } else {
        LOGF(eERROR, "Failed to compile shader %s", filePath);
    }
}
/*
// On Metal, on the other hand, we can compile from code into a MTLLibrary, but cannot save this
// object's bytecode to disk. We instead use the xcbuild bash tool to compile the shaders.
void mtl_compileShader(	Renderer* pRenderer, const char* fileName, const char* outFile, uint32_t macroCount, ShaderMacro* pMacros, BinaryShaderStageDesc* pOut, const char*entrypoint )
{

}
*/
RootSignature *forge_renderer_createRootSignatureSimple(Renderer *pRenderer, Shader *shader) {
    RootSignature *tmp;

    RootSignatureDesc rootDesc = {&shader, 1, 0, nullptr, nullptr, 0};
    addRootSignature(pRenderer, &rootDesc, &tmp);

    for (auto i = 0; i < tmp->mDescriptorCount; i++) {
        DEBUG_PRINT("\tRENDER Descriptor [%d] is %s\n", i, tmp->pDescriptors[i].pName);
    }
    return tmp;
}

RootSignature *forge_renderer_createRootSignature(Renderer *pRenderer, RootSignatureFactory *desc) {
    return desc->create(pRenderer);
}

Texture *forge_render_target_get_texture(RenderTarget *rt) {
    return rt->pTexture;
}

void forge_renderer_fill_descriptor_set(Renderer *pRenderer, BufferExt *buf, DescriptorSet *pDS, DescriptorSlotMode mode, int slotIndex) {
        for( int i = 0; i < buf->_buffers.size(); ++i ) {
            DescriptorDataBuilder builder;
            auto s = builder.addSlot( mode );        
            builder.setSlotBindIndex(s, slotIndex);
//            _buffers.setCurrent(0);
            builder.addSlotData( s, buf->_buffers[i] );
            builder.update( pRenderer, i, pDS);
        }
    }

////

static XXH64_state_t *_state;

uint64_t StateBuilder::getSignature(int shaderID, RenderTarget *rt, RenderTarget *depth) {
    // memset(this, 0, sizeof(StateBuilder));
    if (_state == NULL) _state = XXH64_createState();

    XXH64_hash_t const seed = 0xf1ebd85245909a37;
    XXH64_reset(_state, seed);
    XXH64_update(_state, this, sizeof(StateBuilder));
    XXH64_update(_state, &shaderID, sizeof(int));
    XXH64_update(_state, &rt->mFormat, sizeof(rt->mFormat));
    XXH64_update(_state, &rt->mSampleCount, sizeof(rt->mSampleCount));
    int sq = rt->mSampleQuality;
    XXH64_update(_state, &sq, sizeof(sq));
    uint8_t hasDepth = depth != nullptr ? 1 : 0;
    XXH64_update(_state, &hasDepth, sizeof(hasDepth));
    if (depth != nullptr) {
        XXH64_update(_state, &depth->mFormat, sizeof(depth->mFormat));
    }
    XXH64_hash_t const hash = XXH64_digest(_state);

    //    *l = (int)(hash & 0x00000000ffffffff);
    //  *h = (int)((hash >> 32) & 0x00000000ffffffff);

    //    DEBUG_PRINT("Signature HASH %llu\n", hash);

    //*h = y == x ? 1 : 0;
    return hash;
}

///////
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

    /*
    Shader**           ppShaders;
            uint32_t           mShaderCount;
            uint32_t           mMaxBindlessTextures;
            const char**       ppStaticSamplerNames;
            Sampler**          ppStaticSamplers;
            uint32_t           mStaticSamplerCount;
            RootSignatureFlags mFlags;
    */
    const char **pointers = (const char **)alloca(sizeof(char *) * _names.size());

    for (int i = 0; i < _names.size(); i++) {
        pointers[i] = _names[i].c_str();
        DEBUG_PRINT("\tRENDER ROOT SIG has sampler %s\n",  pointers[i]);
    }
    RootSignatureDesc rootDesc = {
        &_shaders[0],
        (uint32_t)_shaders.size(),
        0, // bindless textures
        pointers,
        &_samplers[0],
        (uint32_t)_samplers.size(),
        ROOT_SIGNATURE_FLAG_NONE// flags
        };

    addRootSignature(pRenderer, &rootDesc, &tmp);

    for (auto i = 0; i < tmp->mDescriptorCount; i++) {
        DescriptorInfo &info = tmp->pDescriptors[i];
        DEBUG_PRINT("\tRENDER Descriptor %s [%d] isRoot %d set/freq %d handle %d dim %d size %d static %d type %d\n", info.pName, i, info.mRootDescriptor, info.mUpdateFrequency, info.mHandleIndex, info.mDim, info.mSize, info.mStaticSampler, info.mType);
    }

    return tmp;
}
void RootSignatureFactory::addShader(Shader *pShader) {
    _shaders.push_back(pShader);
}
void RootSignatureFactory::addSampler(Sampler *sampler, const char *name) {
    _samplers.push_back(sampler);
    _names.push_back(name);
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
    DEBUG_PRINT("Forge SDL Pointer is %p\n", ptr);
    return _alloc_const((SDL_Window *)ptr);
}

/*
HL_PRIM HL_CONST _ref(SDL_Window)* HL_NAME(Window_getWindow1)(void* ptr) {
        return alloc_ref_const((forge_sdl_get_window(ptr)),Window);
}
DEFINE_PRIM(_IDL, Window_getWindow1, _BYTES);
*/

DEFINE_PRIM(_BYTES, unpackSDLWindow, TWIN);

// WINDOWS CALLBACKS
void onDeviceLost() {
}


void requestShutdown() 
{ 
    #if _WINDOWS
	PostQuitMessage(0); 
    #endif
}

#ifdef _WINDOWS
void requestReset(const ResetDesc* pResetDesc)
{
	//gResetDescriptor = *pResetDesc;
        printf("!!!!!!!!!!!!!!!!!! DEVICE LOST !!!!!!!!!!!!!!!!\n");

}
#endif

void requestReload(const ReloadDesc* pReloadDesc)
{
	//gReloadDescriptor = *pReloadDesc;
        printf("!!!!!!!!!!!!!!!!!! REQUEST RELOAD !!!!!!!!!!!!!!!!\n");

}

CustomMessageProcessor sCustomProc = nullptr;
