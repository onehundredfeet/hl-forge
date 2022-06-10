#ifndef __HL_FORGE_H_
#define __HL_FORGE_H_
#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#ifndef SDL_MAJOR_VERSION
#	error "SDL2 SDK not found in hl/include/sdl/"
#endif

void heuristicTest(void (*fn)());
void heuristicTest2(float (*fn)(int));

bool hlForgeInitialize(const char *name);

#include <Renderer/IRenderer.h>

Renderer *createRenderer(const char *name);
void destroyRenderer( Renderer * );
Queue* createQueue(Renderer *renderer);
SwapChain* createSwapChain(SDL_Window *window, Renderer *renderer, Queue *queue, int width, int height, int chainCount, bool hdr10);
#endif