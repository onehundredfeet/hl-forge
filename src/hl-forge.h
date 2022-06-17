#ifndef __HL_FORGE_H_
#define __HL_FORGE_H_
#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#ifndef SDL_MAJOR_VERSION
#	error "SDL2 SDK not found in hl/include/sdl/"
#endif

#define TWIN _ABSTRACT(sdl_window)

void heuristicTest(void (*fn)());
void heuristicTest2(float (*fn)(int));

bool hlForgeInitialize(const char *name);

#include <Renderer/IRenderer.h>
#include <Renderer/IResourceLoader.h>

class ForgeSDLWindow {
    public:
        ForgeSDLWindow(SDL_Window *window);
        SwapChain* createSwapChain(Renderer *renderer, Queue *queue, int width, int height, int chainCount, bool hdr10);
        SDL_Window *window;
        SDL_SysWMinfo wmInfo;
        Renderer *renderer() { return _renderer;}
        Renderer *_renderer;
        SDL_MetalView _view;
        CAMetalLayer *_layer;
        void present(Queue *pGraphicsQueue, SwapChain *pSwapChain, int swapchainImageIndex, Semaphore * pRenderCompleteSemaphore);
};

Renderer *createRenderer(const char *name);
void destroyRenderer( Renderer * );
Queue* createQueue(Renderer *renderer);
SDL_Window *forge_sdl_get_window(void *ptr);
void forge_sdl_buffer_load_desc_set_index_buffer( BufferLoadDesc *bld, int size, void *data, bool shared);
void forge_sdl_buffer_load_desc_set_vertex_buffer( BufferLoadDesc *bld, int size, void *data, bool shared);

Buffer*forge_sdl_buffer_load( BufferLoadDesc *bld, SyncToken *token);
Texture*forge_texture_load(TextureLoadDesc *desc, SyncToken *token);
Texture *forge_texture_load_from_desc(TextureDesc *tdesc, const char *name, SyncToken *token );
void forge_sdl_texture_upload(Texture *, void *data, int dataSize);

void forge_texture_set_file_name(TextureLoadDesc *desc, const char *path);
void forge_render_target_clear(Cmd *cmd, RenderTarget *mainRT, RenderTarget *depthStencil);
void forge_sdl_buffer_update(Buffer *buffer, void *data);
void forge_sdl_buffer_update_region(Buffer *buffer, void *data, int toffset, int size, int soffset);
RenderTarget *forge_sdl_create_render_target(Renderer *, RenderTargetDesc *);
CmdPool * forge_sdl_renderer_create_cmd_pool( Renderer *, Queue * );
Cmd * forge_sdl_renderer_create_cmd( Renderer *, CmdPool * );
Fence * forge_sdl_renderer_create_fence( Renderer * );
Semaphore * forge_sdl_renderer_create_semaphore( Renderer * );
SDL_MetalView forge_create_metal_view(SDL_Window *);
void forge_init_loader( Renderer * );
void forge_renderer_wait_fence( Renderer *, Fence *);
RenderTarget *forge_swap_chain_get_render_target(SwapChain *, int );
void forge_queue_submit_cmd(Queue *queue, Cmd *cmd, Semaphore *signalSemphor, Semaphore *wait, Fence *signalFence);
#endif