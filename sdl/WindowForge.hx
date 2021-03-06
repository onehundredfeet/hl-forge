package sdl;

class WindowForge extends sdl.Window {

    var _forgeWindow : forge.Native.ForgeSDLWindow;

	static var _initialized = false;

	static function init_once() {
		if (!_initialized) {
			trace('Initializing...');
			_initialized = forge.Native.Globals.initialize("Haxe Forge");
			if (!_initialized) {
				throw "Could not initialize forge";
			}
		}
		
	}
    public function new( title : String, width : Int, height : Int, x : Int = Window.SDL_WINDOWPOS_CENTERED, y : Int = Window.SDL_WINDOWPOS_CENTERED, sdlFlags : Int = Window.SDL_WINDOW_SHOWN | Window.SDL_WINDOW_RESIZABLE) {

        // Need to choose the render that aligns with what's built in forge
        win = sdl.Window.winCreateEx(x, y, width, height, sdlFlags | Window.SDL_WINDOW_METAL );
        if( win == null ) throw "Failed to create window";

        init_once();
        
        _forgeWindow = new forge.Native.ForgeSDLWindow(_getSDLWindow(win));


        super(title);
    }

    public function getForgeSDLWindow() {
        return _forgeWindow;
    }

    public override function renderTo() {
        throw "not implemented render To";
	}

    public override function destroy() {
        throw "not implemented destroy";
        super.destroy();
	}
    override function set_vsync(v) {
//        throw "not implemented vsync";
		return vsync = v;
	}

    public function getSDLWindow() {
        return _getSDLWindow(win);
    }
    @:hlNative("forge", "unpackSDLWindow")
	static function _getSDLWindow(win : sdl.Window.WinPtr) : forge.Native.SDLWindow {
		return null;
	}

}