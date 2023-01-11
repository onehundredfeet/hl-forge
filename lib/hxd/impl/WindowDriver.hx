package hxd.impl;

typedef WindowHandle = sdl.Window;
typedef DisplayMode = sdl.Window.DisplayMode;

class WindowDriver {
    public static function createWindow(heapsWindow : hxd.Window, title:String, width:Int, height:Int, fixed:Bool = false) : WindowHandle{
        var sdlFlags = if (!fixed) sdl.Window.SDL_WINDOW_SHOWN | sdl.Window.SDL_WINDOW_RESIZABLE else sdl.Window.SDL_WINDOW_SHOWN;
        #if heaps_forge
        var window = new sdl.WindowForge(title, width, height, sdl.Window.SDL_WINDOWPOS_CENTERED, sdl.Window.SDL_WINDOWPOS_CENTERED, sdlFlags);	
        #else
        var window = new sdl.WindowGL(title, width, height, sdl.Window.SDL_WINDOWPOS_CENTERED, sdl.Window.SDL_WINDOWPOS_CENTERED, sdlFlags);
        #end
        @:privateAccess heapsWindow.window = window;
        @:privateAccess heapsWindow.windowWidth = window.width;
        @:privateAccess heapsWindow.windowHeight = window.height;

        return window;
    }
}