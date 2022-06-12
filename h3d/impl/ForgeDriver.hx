package h3d.impl;

import  h3d.impl.Driver;

@:access(h3d.impl.Shader)
class ForgeDriver extends h3d.impl.Driver{
    var onContextLost : Void -> Void;

	var _renderer : forge.Native.Renderer;
	var _queue : forge.Native.Queue;

	var _width : Int;
	var _height : Int;
	var _swap_count = 3;
	var _hdr = false;

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


    public function new() {
//		window = @:privateAccess dx.Window.windows[0];
//		Driver.setErrorHandler(onDXError);

		init_once();



		reset();
    }

    // This is the first function called after the constructor
    public override function init( onCreate : Bool -> Void, forceSoftware = false ) {
        // copied from DX driver
		onContextLost = onCreate.bind(true);
		haxe.Timer.delay(onCreate.bind(false), 1); // seems arbitrary

        if (forceSoftware) throw "Software mode not supported";

		var win = @:privateAccess hxd.Window.getInstance();
		_width = win.width;
		_height = win.height;

		_renderer = forge.Native.Renderer.create("haxe_metal");
		_queue = _renderer.createQueue();



		attach();
	}

	function addSwapChain() {
		return true;
	}
	
	function addDepthBuffer() {
		return true;
	}

	function attach()
	{
		var win = cast(@:privateAccess hxd.Window.getInstance().window, sdl.WindowForge);
		var fw = win.getSDLWindow();

		if (fw == null) return false;

		var sc = fw.createSwapChain(_renderer, _queue, _width, _height, _swap_count, _hdr);
		if (!addSwapChain())
			return false;

		if (!addDepthBuffer())
			return false;


		return true;
	}

	function detach() {
//		waitQueueIdle(pGraphicsQueue);

//		removeSwapChain(pRenderer, pSwapChain);
//		removeRenderTarget(pRenderer, pDepthBuffer);
	}

    // second function called
    public override function allocIndexes( count : Int, is32 : Bool ) : IndexBuffer {

		var bits = is32 ? 2 : 1;
		var desc = new forge.Native.BufferLoadDesc();
		var placeHolder = new Array<hl.UI8>();
		placeHolder.resize(count << bits);

		desc.setIndexbuffer( count << bits, hl.Bytes.getArray(placeHolder), true);
		var buff = desc.load(null);

        return {b : buff, is32 : is32};
	}

    function reset() {

    }

	public override function uploadIndexBuffer( i : IndexBuffer, startIndice : Int, indiceCount : Int, buf : hxd.IndexBuffer, bufPos : Int ) {
		var bits = i.is32 ? 2 : 1;
		i.b.updateRegion(hl.Bytes.getArray(buf.getNative()), startIndice << bits, indiceCount << bits, bufPos << bits );
	}

    public override function hasFeature( f : Feature ) {
		trace('Has Feature ${f}');
        // copied from DX driver
        return switch(f) {
            case Queries, BottomLeftCoords:
                false;
			case HardwareAccelerated:true;
            default:
                true;
            };

		return false;
	}

	public override function setRenderFlag( r : RenderFlag, value : Int ) {
        throw "Not implemented";

	}

	public override function isSupportedFormat( fmt : h3d.mat.Data.TextureFormat ) {
        throw "Not implemented";
		return false;
	}

	public override function isDisposed() {
        //throw "Not implemented";
//		return gl.isContextLost(); // currently just returns false?

		return _renderer != null;
	}

	public override function dispose() {
        throw "Not implemented";
	}

	public override function begin( frame : Int ) {
        throw "Not implemented";
	}


	public override function generateMipMaps( texture : h3d.mat.Texture ) {
		throw "Mipmaps auto generation is not supported on this platform";
	}

	public override function getNativeShaderCode( shader : hxsl.RuntimeShader ) : String {
        throw "Not implemented";
		return null;
	}

	override function logImpl( str : String ) {
        throw "Not implemented";
	}

	public override function clear( ?color : h3d.Vector, ?depth : Float, ?stencil : Int ) {
        throw "Not implemented";
	}

	public override function captureRenderBuffer( pixels : hxd.Pixels ) {
        throw "Not implemented";
	}

	public override function capturePixels( tex : h3d.mat.Texture, layer : Int, mipLevel : Int, ?region : h2d.col.IBounds ) : hxd.Pixels {
		throw "Can't capture pixels on this platform";
		return null;
	}

	public override function getDriverName( details : Bool ) {
        throw "Not implemented";
		return "Not available";
	}


	public override function resize( width : Int, height : Int ) {
		trace('Resizing ${width} ${height}');
		_width = width;
		_height = height;
		detach();
		attach();
	}

	public override function selectShader( shader : hxsl.RuntimeShader ) {
        throw "Not implemented";
		return false;
	}

	public override function selectMaterial( pass : h3d.mat.Pass ) {
        throw "Not implemented";
	}

	public override function uploadShaderBuffers( buffers : h3d.shader.Buffers, which : h3d.shader.Buffers.BufferKind ) {
        throw "Not implemented";
	}

	public override function getShaderInputNames() : InputNames {
        throw "Not implemented";
		return null;
	}

	public override function selectBuffer( buffer : Buffer ) {
        throw "Not implemented";
	}

	public override function selectMultiBuffers( buffers : Buffer.BufferOffset ) {
        throw "Not implemented";
	}

	public override function draw( ibuf : IndexBuffer, startIndex : Int, ntriangles : Int ) {
        throw "Not implemented";
	}

	public override function drawInstanced( ibuf : IndexBuffer, commands : h3d.impl.InstanceBuffer ) {
        throw "Not implemented";
	}

	public override function setRenderZone( x : Int, y : Int, width : Int, height : Int ) {
        throw "Not implemented";
	}

	public override function setRenderTarget( tex : Null<h3d.mat.Texture>, layer = 0, mipLevel = 0 ) {
        throw "Not implemented";
	}

	public override function setRenderTargets( textures : Array<h3d.mat.Texture> ) {
        throw "Not implemented";
	}

	public override function allocDepthBuffer( b : h3d.mat.DepthBuffer ) : DepthBuffer {
        throw "Not implemented";
		return null;
	}

	public override function disposeDepthBuffer( b : h3d.mat.DepthBuffer ) {
        throw "Not implemented";
	}

	public override function getDefaultDepthBuffer() : h3d.mat.DepthBuffer {
        throw "Not implemented";
		return null;
	}

	public override function present() {
        throw "Not implemented";
	}

	public override function end() {
        throw "Not implemented";
	}

	var _debug = false;
	public override function setDebug( d : Bool ) {
        if (d) trace('Forge Driver Debug ${d}');
		_debug = d;
	}

	public override function allocTexture( t : h3d.mat.Texture ) : Texture {
        throw "Not implemented";
		return null;
	}



	public override function allocVertexes( m : ManagedBuffer ) : VertexBuffer {
        throw "Not implemented";
		return null;
	}

	public override function allocInstanceBuffer( b : h3d.impl.InstanceBuffer, bytes : haxe.io.Bytes ) {
        throw "Not implemented";
	}

	public override function disposeTexture( t : h3d.mat.Texture ) {
        throw "Not implemented";
	}

	public override function disposeIndexes( i : IndexBuffer ) {
        throw "Not implemented";
	}

	public override function disposeVertexes( v : VertexBuffer ) {
        throw "Not implemented";
	}

	public override function disposeInstanceBuffer( b : h3d.impl.InstanceBuffer ) {
        throw "Not implemented";
	}



	public override function uploadIndexBytes( i : IndexBuffer, startIndice : Int, indiceCount : Int, buf : haxe.io.Bytes , bufPos : Int ) {
        throw "Not implemented";
	}

	public override function uploadVertexBuffer( v : VertexBuffer, startVertex : Int, vertexCount : Int, buf : hxd.FloatBuffer, bufPos : Int ) {
        throw "Not implemented";
	}

	public override function uploadVertexBytes( v : VertexBuffer, startVertex : Int, vertexCount : Int, buf : haxe.io.Bytes, bufPos : Int ) {
        throw "Not implemented";
	}

	public override function uploadTextureBitmap( t : h3d.mat.Texture, bmp : hxd.BitmapData, mipLevel : Int, side : Int ) {
        throw "Not implemented";
	}

	public override function uploadTexturePixels( t : h3d.mat.Texture, pixels : hxd.Pixels, mipLevel : Int, side : Int ) {
        throw "Not implemented";
	}

	public override function readVertexBytes( v : VertexBuffer, startVertex : Int, vertexCount : Int, buf : haxe.io.Bytes, bufPos : Int ) {
		throw "Driver does not allow to read vertex bytes";
	}

	public override function readIndexBytes( v : IndexBuffer, startVertex : Int, vertexCount : Int, buf : haxe.io.Bytes, bufPos : Int ) {
		throw "Driver does not allow to read index bytes";
	}

	/**
		Returns true if we could copy the texture, false otherwise (not supported by driver or mismatch in size/format)
	**/
	public override function copyTexture( from : h3d.mat.Texture, to : h3d.mat.Texture ) {
        throw "Not implemented";
		return false;
	}

	// --- QUERY API

	public override function allocQuery( queryKind : QueryKind ) : Query {
        throw "Not implemented";
		return null;
	}

	public override function deleteQuery( q : Query ) {
        throw "Not implemented";
	}

	public override function beginQuery( q : Query ) {
        throw "Not implemented";
	}

	public override function endQuery( q : Query ) {
        throw "Not implemented";
	}

	public override function queryResultAvailable( q : Query ) {
        throw "Not implemented";
		return true;
	}

	public override function queryResult( q : Query ) {
        throw "Not implemented";
		return 0.;
	}
}