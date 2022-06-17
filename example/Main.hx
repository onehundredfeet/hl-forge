import h3d.impl.ForgeDriver;
import sdl.WindowMetal;
import sdl.Window;

class Main {
    static function allocTexture( width, height, name, format ) {

		var ftd = new forge.Native.TextureDesc();

		ftd.width = width;
		ftd.height = height;
		ftd.arraySize = 1;
		ftd.depth = 1;
		ftd.mipLevels = 1;
		ftd.sampleCount = SAMPLE_COUNT_1;
		ftd.startState = RESOURCE_STATE_COMMON;
		ftd.flags = TEXTURE_CREATION_FLAG_OWN_MEMORY_BIT;

		ftd.format = format;
        /*switch( t.format) {
			case RGBA: TinyImageFormat_R8G8B8A8_UNORM;
			case SRGB: TinyImageFormat_R8G8B8A8_SRGB;
			case RGBA16F: TinyImageFormat_R16G16B16A16_SFLOAT;
			default:throw "Unsupported texture format "+t.format;
		}
        */
		trace ('Format is ${ftd.format}');
		var flt = ftd.load(name, null);
		
		
		if (flt == null) throw "invalid texture";
		forge.Native.Globals.waitForAllResourceLoads();
		

		return { t : flt, width : width, height : height, internalFmt : ftd.format.toValue(),  bits : -1, bind : 0, bias : 0 #if multidriver, driver : this #end };
	}

    public static function main() {
        trace('Initializing SDL');
        sdl.Sdl.init();

		var sdlFlags = sdl.Window.SDL_WINDOW_SHOWN | sdl.Window.SDL_WINDOW_RESIZABLE;

        var width = 1920;
        var height = 1080;
		
		trace('Initializing forge');
        forge.Native.Globals.initialize("Haxe Forge");


        trace('Creating window');
        var window = new sdl.WindowForge("Main", width, height, sdl.Window.SDL_WINDOWPOS_CENTERED, sdl.Window.SDL_WINDOWPOS_CENTERED, sdlFlags);	

		var fw = window.getForgeSDLWindow();


		
  

		var _renderer =fw.renderer();
		trace ('creating queue');
		var _queue = _renderer.createQueue();

        var _swap_count = 3;

        var _swapCmdPools = new Array<forge.Native.CmdPool>();
        var _swapCmds = new Array<forge.Native.Cmd>();
        var _swapCompleteFences = new Array<forge.Native.Fence>();
        var _swapCompleteSemaphores = new Array<forge.Native.Semaphore>();
        var _ImageAcquiredSemaphore : forge.Native.Semaphore;

        
        trace ('Initializing swap command pools');
        for (i in 0..._swap_count) {

			var cmdPool = _renderer.createCommandPool( _queue );
			_swapCmdPools.push(cmdPool);

			var cmd = _renderer.createCommand( cmdPool );
			_swapCmds.push(cmd);

			_swapCompleteFences.push(_renderer.createFence());
			_swapCompleteSemaphores.push(_renderer.createSemaphore());
		}

        trace ('Initializing image semaphore');

		_ImageAcquiredSemaphore = _renderer.createSemaphore();
		
		// create window
		trace ('create swap ');
		fw.createSwapChain(_renderer, _queue, 1920, 1800, 1, false);
		
        trace ('Initializing loader interfacce');

		_renderer.initResourceLoaderInterface();

        trace ('allocating texture');

        var tex = allocTexture(128, 256, "junk", TinyImageFormat_R8G8B8A8_UNORM);

		forge.Native.Globals.waitForAllResourceLoads();

		var len = 128 * 256 * 4;
		var pixels = new Array<hl.UI8>();
		pixels.resize(len);

		trace('Sending bytes ${pixels} : ${len}');
		tex.t.upload(hl.Bytes.getArray(pixels), len);
		trace('Done uplaoding');


        trace('Done');
    }
}