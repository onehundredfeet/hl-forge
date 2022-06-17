#include <Foundation/Foundation.h>
#include <AppKit/AppKit.h>
void *getNSViewFromNSWindow(NSWindow*nswin) {
    NSView *view = [nswin contentView];
    return (void *)CFBridgingRetain(view);
}