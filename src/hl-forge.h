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

Renderer *createRenderer(const char *name);
void destroyRenderer( Renderer * );
Queue* createQueue(Renderer *renderer);
SwapChain* createSwapChain( SDL_Window *window, Renderer *renderer, Queue *queue, int width, int height, int chainCount, bool hdr10);
SDL_Window *forge_sdl_get_window(void *ptr);
void forge_sdl_buffer_load_desc_set_index_buffer( BufferLoadDesc *bld, int size, void *data, bool shared);
Buffer*forge_sdl_buffer_load( BufferLoadDesc *bld, SyncToken *token);

void forge_sdl_buffer_update(Buffer *buffer, void *data);
void forge_sdl_buffer_update_region(Buffer *buffer, void *data, int toffset, int size, int soffset);

SDL_MetalView forge_create_metal_view(SDL_Window *);
#endif