#ifndef __HL_FORGE_OSX_H_
#define __HL_FORGE_OSX_H_

#include <Foundation/Foundation.h>

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
#endif