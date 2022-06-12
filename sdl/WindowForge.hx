package sdl;

class WindowForge extends sdl.Window {



    public function new( title : String, width : Int, height : Int, x : Int = Window.SDL_WINDOWPOS_CENTERED, y : Int = Window.SDL_WINDOWPOS_CENTERED, sdlFlags : Int = Window.SDL_WINDOW_SHOWN | Window.SDL_WINDOW_RESIZABLE) {

        // Need to choose the render that aligns with what's built in forge
        win = sdl.Window.winCreateEx(x, y, width, height, sdlFlags | Window.SDL_WINDOW_METAL );
        if( win == null ) throw "Failed to create window";



        super(title);
    }

    public override function renderTo() {
        throw "not implemented";
	}

    public override function destroy() {
        throw "not implemented";
        super.destroy();
	}
    override function set_vsync(v) {
        throw "not implemented";
		return vsync = v;
	}

    public function getSDLWindow() {
        return _getSDLWindow(win);
    }
    @:hlNative("forge", "forge_get_sdl_window")
	static function _getSDLWindow(win : sdl.Window.WinPtr) : forge.Native.Window {
		return null;
	}

}