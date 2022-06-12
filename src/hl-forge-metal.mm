#include <Foundation/Foundation.h>
#include <AppKit/AppKit.h>
void *getNSViewFromNSWindow(void*ptr) {
    NSWindow *nswin = (NSWindow *)ptr;
    NSView *view = [nswin contentView];
    return view;
}