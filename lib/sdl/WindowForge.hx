package sdl;

class WindowForge extends sdl.Window {

    var _forgeWindow : forge.Native.ForgeSDLWindow;

	static var _initialized = false;

	static function init_once() {
		if (!_initialized) {
			trace('Initializing...');
			_initialized = forge.Native.Globals.initialize("HaxeForge.txt");
			if (!_initialized) {
				throw "Could not initialize forge";
			}
		}
		
	}
    public function new( title : String, width : Int, height : Int, x : Int = Window.SDL_WINDOWPOS_CENTERED, y : Int = Window.SDL_WINDOWPOS_CENTERED, sdlFlags : Int = Window.SDL_WINDOW_SHOWN | Window.SDL_WINDOW_RESIZABLE) {

        // Need to choose the render that aligns with what's built in forge
        
        var backend = switch( Sys.systemName()) {
            case "Mac":Window.SDL_WINDOW_METAL;
            case "Windows": Window.SDL_WINDOW_VULKAN;
            default: 0;
        } ;

        win = sdl.Window.winCreateEx(x, y, width, height, sdlFlags |  backend);
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