#ifndef __HL_FORGE_H_
#define __HL_FORGE_H_
#pragma once

void heuristicTest(void (*fn)());
void heuristicTest2(float (*fn)(int));

bool hlForgeInitialize(const char *name);

#include <Renderer/IRenderer.h>

Renderer *createRenderer(const char *name);
void destroyRenderer( Renderer * );

#endif