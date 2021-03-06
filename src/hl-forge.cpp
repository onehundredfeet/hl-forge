
#include <iostream>
#include <sstream>


#include "hl-forge-shaders.h"

#include <Renderer/IRenderer.h>
#include <OS/Interfaces/IInput.h>
#include <OS/Logging/Log.h>
#include <Renderer/IResourceLoader.h>
#include <Renderer/IShaderReflection.h>
#include <basis_universal/basisu_enc.h>
#include <OS/Core/TextureContainers.h>

#include <tinyimageformat/tinyimageformat_base.h>
#include <tinyimageformat/tinyimageformat_apis.h>
#include <tinyimageformat/tinyimageformat_query.h>
#include <tinyimageformat/tinyimageformat_bits.h>

#define HL_NAME(x) forge_##x
#include <hl.h>

#include "hl-forge-meta.h"
#include "hl-forge.h"

//#define DEBUG_PRINT(...) printf(__VA_ARGS__)
#define DEBUG_PRINT(...)

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
    DEBUG_PRINT("Intializing memory\n");
    if (!initMemAlloc(name)) {
        DEBUG_PRINT("Failed to init memory allocator\n");
        return false;
    }

    FileSystemInitDesc fsDesc = {};
    fsDesc.pAppName = name;
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

void forge_blend_state_desc_set_rt( BlendStateDesc *bs, BlendStateTargets rt, bool enabled) {
    if (enabled) {
        bs->mRenderTargetMask = (BlendStateTargets)(bs->mRenderTargetMask | rt);
    } else {
        bs->mRenderTargetMask = (BlendStateTargets)(bs->mRenderTargetMask & ~rt);
    }
}

VertexAttrib *forge_vertex_layout_get_attrib( VertexLayout *layout, int idx) {
    return &layout->mAttribs[idx];
}

void forge_vertex_attrib_set_semantic( VertexAttrib *attrib, const char *name ) {
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
    DEBUG_PRINT("Loading buffer \n");
    Buffer *tmp = nullptr;
    bld->ppBuffer = &tmp;
    DEBUG_PRINT("adding resource %p %p\n", bld, token);
    addResource(bld, token);
    return tmp;
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


void forge_render_target_bind(Cmd *cmd, RenderTarget *pRenderTarget, RenderTarget *pDepthStencilRT, LoadActionType color, LoadActionType depth ) {
    // simply record the screen cleaning command
    LoadActionsDesc loadActions = {};
    loadActions.mLoadActionsColor[0] = color;
    loadActions.mLoadActionDepth = depth;

//    DEBUG_PRINT("RENDER CLEAR c++ BIND %f %f %f %f\n", pRenderTarget->mClearValue.r, pRenderTarget->mClearValue.g, pRenderTarget->mClearValue.b, pRenderTarget->mClearValue.a );


     // This seems to trump the stuff below
     if (pDepthStencilRT != nullptr) {
        loadActions.mClearDepth.depth = pDepthStencilRT->mClearValue.depth ;
        loadActions.mClearDepth.stencil = pDepthStencilRT->mClearValue.stencil;
    }
    loadActions.mClearColorValues[0].r = pRenderTarget->mClearValue.r;
    loadActions.mClearColorValues[0].g = pRenderTarget->mClearValue.g;
    loadActions.mClearColorValues[0].b = pRenderTarget->mClearValue.b;
    loadActions.mClearColorValues[0].a = pRenderTarget->mClearValue.a;


    //DEBUG_PRINT("RENDER CLEAR c++ %f %f %f %f - %f %d\n", loadActions.mClearColorValues[0].r, loadActions.mClearColorValues[0].g, loadActions.mClearColorValues[0].b, loadActions.mClearColorValues[0].a,  loadActions.mClearDepth.depth, loadActions.mClearDepth.stencil);
    cmdBindRenderTargets(cmd, 1, &pRenderTarget, pDepthStencilRT, &loadActions, NULL, NULL, -1, -1);
    cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 0.0f, 1.0f);
    cmdSetScissor(cmd, 0, 0, pRenderTarget->mWidth, pRenderTarget->mHeight);
}
void forge_render_target_set_clear_colour( RenderTarget *rt, float r, float g, float b,float a) {
    rt->mClearValue.r = r;
    rt->mClearValue.g = g;
    rt->mClearValue.b = b;
    rt->mClearValue.a = a;

    //DEBUG_PRINT("RENDER CLEAR SET %f %f %f %f\n", rt->mClearValue.r, rt->mClearValue.g, rt->mClearValue.b, rt->mClearValue.a );
}
void forge_render_target_set_clear_depth( RenderTarget *rt, float depth, int stencil) {
    //DEBUG_PRINT("RENDER CLEAR SET DEPTH %f %f %f %f\n", rt->mClearValue.r, rt->mClearValue.g, rt->mClearValue.b, rt->mClearValue.a );
    rt->mClearValue.depth = depth;
    rt->mClearValue.stencil = stencil;
    //DEBUG_PRINT("RENDER CLEAR SET DEPTH %f %f %f %f\n", rt->mClearValue.r, rt->mClearValue.g, rt->mClearValue.b, rt->mClearValue.a );
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
    RenderTargetBarrier barriers[] =    // wait for resource transition
    {
        { pRenderTarget, RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET },
    };
    cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriers);
}

void forge_cmd_wait_for_present(Cmd *cmd, RenderTarget *pRenderTarget) {
    RenderTargetBarrier barriers[] =    // wait for resource transition
    {
        { pRenderTarget, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_PRESENT },
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
    updateDesc.mArrayLayer
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



Shader *forge_load_compute_shader_file(Renderer *pRenderer, const char *fileName ) {
    ShaderLoadDesc GenerateMipShaderDesc = {};
    GenerateMipShaderDesc.mStages[0] = { fileName, NULL, 0, NULL, SHADER_STAGE_LOAD_FLAG_ENABLE_VR_MULTIVIEW };
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

DescriptorSet *forge_renderer_create_descriptor_set( Renderer *pRenderer, RootSignature* pRootSignature, DescriptorUpdateFrequency updateFrequency, uint maxSets, uint nodeIndex) {
    DescriptorSet *tmp;
    DescriptorSetDesc desc = {
        pRootSignature, 
        updateFrequency,
        maxSets,
        nodeIndex};

    addDescriptorSet( pRenderer, &desc, &tmp );
    return tmp;
}

void forge_render_target_capture(RenderTarget*rt, Buffer *pTransferBuffer, Semaphore *semaphore) {	
    TextureCopyDesc copyDesc = {};
    copyDesc.pTexture = rt->pTexture;
    copyDesc.pBuffer = pTransferBuffer;
    copyDesc.pWaitSemaphore = semaphore;
    copyDesc.mTextureState = RESOURCE_STATE_RENDER_TARGET;
    copyDesc.mQueueType = QUEUE_TYPE_GRAPHICS;
    SyncToken *st = nullptr;
    copyResource(&copyDesc, st);
}

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

    DEBUG_PRINT("Getting MTL\n");
    auto vertMSL = getMSLFromSPV(vertCode);
    auto fragMSL = getMSLFromSPV(fragCode);

    auto vertFilePathMSL = vertFilePath + ".metal";
    auto fragFilePathMSL = fragFilePath + ".metal";

    auto bufferspot = fragMSL.find("spvDescriptorSet0 [[buffer(");
    if (bufferspot != std::string::npos) {
        DEBUG_PRINT("RENDER modifying metal shader code at %d\n", bufferspot);
        bufferspot += sizeof("spvDescriptorSet0 [[buffer(") - 1;
        fragMSL = fragMSL.replace(bufferspot, 1, "UPDATE_FREQ_PER_DRAW");
    }

    DEBUG_PRINT("writing MTL\n");
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

	if (tmp->pReflection) {
		std::stringstream ss;

		ss << "vertex stage index: " << tmp->pReflection->mVertexStageIndex << std::endl;
		ss << "frag stage index: " << tmp->pReflection->mPixelStageIndex << std::endl;

		ss << "stage reflection count: " << tmp->pReflection->mStageReflectionCount << std::endl;
		for (auto i = 0; i < tmp->pReflection->mStageReflectionCount;++i) {
            auto &s = tmp->pReflection->mStageReflections[i];
            ss << "\t" << i << " stage:" << s.mShaderStage << std::endl;
            ss << "\t" << i << " variable count:" << s.mVariableCount << std::endl;
            ss << "\t" << i << " resource count:" << s.mShaderResourceCount << std::endl;

            if (s.mShaderStage == SHADER_STAGE_VERT) {
            ss << "\t" << i << " vert input counts:" << s.mVertexInputsCount << std::endl;
             for (auto vi = 0; vi < s.mVertexInputsCount;++vi) {
                auto &v = s.pVertexInputs[vi];
                ss << "\t\t" << vi << " name:" << v.name << std::endl;
                ss << "\t\t" << vi << " name size:" << v.name_size << std::endl;
                ss << "\t\t" << vi << " size:" << v.size << std::endl;
            }

            }

           
        }

		ss << "resource count: " << tmp->pReflection->mShaderResourceCount << std::endl; 
		for (auto i = 0; i < tmp->pReflection->mShaderResourceCount;++i) {
			auto &r = tmp->pReflection->pShaderResources[i];
			ss << "\t" << i << " name:" << r.name << std::endl;
			ss << "\t" << i << " reg:" << r.reg << std::endl;
			ss << "\t" << i << " argument buffer field:" << r.mIsArgumentBufferField << std::endl;
			ss << "\t" << i << " used stages:" << r.used_stages << std::endl;
			ss << "\t" << i << " set:" << r.set << std::endl;
			ss << "\t" << i << " size:" << r.size << std::endl;
			ss << "\t" << i << " dim:" << r.dim << std::endl;
			ss << "\t" << i << " alignment:" << r.alignment << std::endl;
			ss << "\t" << i << " argument descriptor index:" << r.mtlArgumentDescriptors.mArgumentIndex << std::endl;
			ss << "\t" << i << " argument array length:" << r.mtlArgumentDescriptors.mArrayLength << std::endl;
			ss << "\t" << i << " argument buffer index:" << r.mtlArgumentDescriptors.mBufferIndex << std::endl;
			
			ss << "\t" << i << " type:";

			switch(r.type) {
				case DESCRIPTOR_TYPE_UNIFORM_BUFFER: ss << "Uniform buffer"; break;
				case DESCRIPTOR_TYPE_SAMPLER:ss << "Sampler"; break;
				case DESCRIPTOR_TYPE_TEXTURE:ss << "Texture"; break;
				case DESCRIPTOR_TYPE_RW_TEXTURE:ss << "Rw Texture"; break;
				case DESCRIPTOR_TYPE_BUFFER: ss << "Buffer"; break;
				case DESCRIPTOR_TYPE_BUFFER_RAW: ss << "Buffer Raw"; break;
				case DESCRIPTOR_TYPE_RW_BUFFER:ss << "RW Buffer"; break;
				case DESCRIPTOR_TYPE_RW_BUFFER_RAW:ss << "RW Buffer Raw"; break;
				case DESCRIPTOR_TYPE_ROOT_CONSTANT:ss << "Root constant"; break;
				case DESCRIPTOR_TYPE_ARGUMENT_BUFFER:ss << "Argument buffer"; break;
				case DESCRIPTOR_TYPE_INDIRECT_COMMAND_BUFFER:ss << "Indirect command buffer"; break;
				case DESCRIPTOR_TYPE_RENDER_PIPELINE_STATE:ss << "Render pipeline buffer"; break;
				default: ss << "Unknown: " << r.type; break;
			}

			ss << std::endl;
			
		}		
		ss << "var count: " << tmp->pReflection->mVariableCount << std::endl;

		for (auto i = 0; i < tmp->pReflection->mVariableCount;++i) {
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

RootSignature *forge_renderer_createRootSignatureSimple(Renderer *pRenderer, Shader *shader) {
    RootSignature *tmp;

    RootSignatureDesc rootDesc = {&shader, 1, 0, nullptr, nullptr, 0};
    addRootSignature(pRenderer, &rootDesc, &tmp);


    for (auto i = 0; i < tmp->mDescriptorCount; i++) {
        DEBUG_PRINT("\tRENDER Descriptor %d is %s\n", i, tmp->pDescriptors[i].pName);
    }
    return tmp;
}

RootSignature *forge_renderer_createRootSignature(Renderer *pRenderer, RootSignatureFactory *desc) {
    return desc->create(pRenderer );
}

Texture *forge_render_target_get_texture( RenderTarget *rt) {
    return rt->pTexture;
}

////

static XXH64_state_t *_state;

uint64_t StateBuilder::getSignature(int shaderID, RenderTarget *rt, RenderTarget *depth) {
    //memset(this, 0, sizeof(StateBuilder));
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
    const char **pointers = (const char **)alloca( sizeof(char *) * _names.size());

    for (int i = 0; i < _names.size(); i++) {
        pointers[i] = _names[i].c_str();
    }
    RootSignatureDesc rootDesc = {
        &_shaders[0], 
        (uint32_t)_shaders.size(),
        0,
        pointers,
        &_samplers[0],
        (uint32_t)_samplers.size()
         };
    
	
    addRootSignature(pRenderer, &rootDesc, &tmp);

    for (auto i = 0; i < tmp->mDescriptorCount; i++) {
        DescriptorInfo &info = tmp->pDescriptors[i];
        DEBUG_PRINT("\tRENDER Descriptor %d is %s hi %d dim %d size %d static %d type %d\n", i, info.pName, info.mHandleIndex, info.mDim, info.mSize, info.mStaticSampler, info.mType);
    }

	return tmp;
}
void RootSignatureFactory::addShader( Shader *pShader ) {
	_shaders.push_back(pShader);
}
void RootSignatureFactory::addSampler( Sampler * sampler, const char *name ) {
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
