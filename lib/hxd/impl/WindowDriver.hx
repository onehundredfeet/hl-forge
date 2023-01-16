package hxd.impl;
import hxd.impl.MouseMode;
import hxd.Window.DisplaySetting;
import hxd.Window.Monitor;

@:forward
#if heaps_forge
abstract WindowHandle(sdl.WindowForge) from sdl.WindowForge to sdl.WindowForge{
#else
abstract WindowHandle(sdl.Window) {
#end
	public function getCurrentMonitor() {
		return this.currentMonitor;
	}

	public function setCursorPosition(mode : MouseMode, x: Int, y : Int) {
        if (mode == MouseMode.Absolute) {
            this.warpMouse(x, y);
        }
    }

    
	public function setMouseMode(mode : MouseMode) {
    }


	public function getMouseClip() {
        return this.grab;
    }

	public function setMouseClip(clip:Bool) : Bool{
        this.grab = clip;
        return clip;
    }


}

typedef DisplayMode = sdl.Window.DisplayMode;
typedef Event = sdl.Event;

class WindowDriver {
	public static function createWindow(heapsWindow:hxd.Window, title:String, width:Int, height:Int, fixed:Bool = false):WindowHandle {
		var sdlFlags = if (!fixed) sdl.Window.SDL_WINDOW_SHOWN | sdl.Window.SDL_WINDOW_RESIZABLE else sdl.Window.SDL_WINDOW_SHOWN;
		#if heaps_forge
		var window:WindowHandle = new sdl.WindowForge(title, width, height, sdl.Window.SDL_WINDOWPOS_CENTERED, sdl.Window.SDL_WINDOWPOS_CENTERED, sdlFlags);
		#else
		var window:WindowHandle = new sdl.WindowGL(title, width, height, sdl.Window.SDL_WINDOWPOS_CENTERED, sdl.Window.SDL_WINDOWPOS_CENTERED, sdlFlags);
		#end
		@:privateAccess heapsWindow.window = window;
		@:privateAccess heapsWindow.windowWidth = window.width;
		@:privateAccess heapsWindow.windowHeight = window.height;

		return window;
	}

	public static function getMonitors() {
		return sdl.Sdl.getDisplays();
	}

    public static function getCurrentDisplaySetting(?mid : Int, mname : String, registry : Bool) : DisplaySetting {
		return sdl.Sdl.getCurrentDisplayMode(mid == null ? 0 : mid, true);
	}

    public static function getDisplaySettings(mid : Int,  mname : String) : Array<DisplaySetting> {
        return sdl.Sdl.getDisplayModes(mid );
    }

}
