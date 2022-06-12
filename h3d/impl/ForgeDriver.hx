package h3d.impl;

import h3d.impl.Driver;

@:access(h3d.impl.Shader)
class ForgeDriver extends h3d.impl.Driver {
	var onContextLost:Void->Void;

	var _renderer:forge.Native.Renderer;
	var _queue:forge.Native.Queue;

	var _width:Int;
	var _height:Int;
	var _swap_count = 3;
	var _hdr = false;
	var _maxCompressedTexturesSupport = 3; // arbitrary atm

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
	public override function init(onCreate:Bool->Void, forceSoftware = false) {
		// copied from DX driver
		onContextLost = onCreate.bind(true);
		haxe.Timer.delay(onCreate.bind(false), 1); // seems arbitrary

		if (forceSoftware)
			throw "Software mode not supported";

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

	function attach() {
		var win = cast(@:privateAccess hxd.Window.getInstance().window, sdl.WindowForge);
		var fw = win.getSDLWindow();

		if (fw == null)
			return false;

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
	public override function allocIndexes(count:Int, is32:Bool):IndexBuffer {
		var bits = is32 ? 2 : 1;
		var desc = new forge.Native.BufferLoadDesc();
		var placeHolder = new Array<hl.UI8>();
		placeHolder.resize(count << bits);

		desc.setIndexbuffer(count << bits, hl.Bytes.getArray(placeHolder), true);
		var buff = desc.load(null);

		return {b: buff, is32: is32};
	}

	function reset() {}

	public override function uploadIndexBuffer(i:IndexBuffer, startIndice:Int, indiceCount:Int, buf:hxd.IndexBuffer, bufPos:Int) {
		var bits = i.is32 ? 2 : 1;
		i.b.updateRegion(hl.Bytes.getArray(buf.getNative()), startIndice << bits, indiceCount << bits, bufPos << bits);
	}

	public override function hasFeature(f:Feature) {
		trace('Has Feature ${f}');
		// copied from DX driver
		return switch (f) {
			case Queries, BottomLeftCoords:
				false;
			case HardwareAccelerated: true;
			default:
				true;
		};

		return false;
	}

	public override function isSupportedFormat(fmt:h3d.mat.Data.TextureFormat) {
		return switch (fmt) {
			case RGBA: true;
			case RGBA16F, RGBA32F: hasFeature(FloatTextures);
			case SRGB, SRGB_ALPHA: hasFeature(SRGBTextures);
			case R8, RG8, RGB8, R16F, RG16F, RGB16F, R32F, RG32F, RGB32F, RG11B10UF, RGB10A2: true;
			case S3TC(n): n <= _maxCompressedTexturesSupport;
			default: false;
		}
	}

	public override function resize(width:Int, height:Int) {
		trace('Resizing ${width} ${height}');
		_width = width;
		_height = height;
		detach();
		attach();
	}

	var defaultDepth:h3d.mat.DepthBuffer;

	override function getDefaultDepthBuffer():h3d.mat.DepthBuffer {
		trace('Getting default depth buffer ${_width}  ${_height}');
		if (defaultDepth != null)
			return defaultDepth;
		defaultDepth = new h3d.mat.DepthBuffer(0, 0);
		@:privateAccess {
			defaultDepth.width = _width;
			defaultDepth.height = _height;
			defaultDepth.b = allocDepthBuffer(defaultDepth);
		}
		return defaultDepth;
	}

	public override function allocDepthBuffer(b:h3d.mat.DepthBuffer):DepthBuffer {
		var depthRT = new forge.Native.RenderTargetDesc();
		depthRT.arraySize = 1;
		// depthRT.clearValue.depth = 0.0f;
		// depthRT.clearValue.stencil = 0;
		depthRT.depth = 1;
		depthRT.width = b.width;
		depthRT.height = b.height;
		depthRT.sampleQuality = 0;
		depthRT.sampleCount = SAMPLE_COUNT_1;
		depthRT.startState = RESOURCE_STATE_DEPTH_WRITE;

		if (b.format != null) {
			trace('b is ${b} format is ${b != null ? b.format : null}');
			switch (b.format) {
				case Depth16:
					depthRT.format = TinyImageFormat_D32_SFLOAT;
				case Depth24:
					depthRT.format = TinyImageFormat_D32_SFLOAT;
				case Depth24Stencil8:
					depthRT.format = TinyImageFormat_D32_SFLOAT;
				default:
					throw "Unsupported depth format " + b.format;
			}
		} else {
			depthRT.format = TinyImageFormat_D32_SFLOAT;
		}
		depthRT.flags = forge.Native.TextureCreationFlags.TEXTURE_CREATION_FLAG_ON_TILE.toValue() | forge.Native.TextureCreationFlags.TEXTURE_CREATION_FLAG_VR_MULTIVIEW.toValue();

		/*
			var r = gl.createRenderbuffer();
			if( b.format == null )
				@:privateAccess b.format = #if js (glES >= 3 ? Depth24Stencil8 : Depth16) #else Depth24Stencil8 #end;
			var format = switch( b.format ) {
			case Depth16: GL.DEPTH_COMPONENT16;
			case Depth24 #if js if( glES >= 3 ) #end: GL.DEPTH_COMPONENT24;
			case Depth24Stencil8: GL.DEPTH_STENCIL;
			default:
				throw "Unsupported depth format "+b.format;
			}
			gl.bindRenderbuffer(GL.RENDERBUFFER, r);
			gl.renderbufferStorage(GL.RENDERBUFFER, format, b.width, b.height);
			gl.bindRenderbuffer(GL.RENDERBUFFER, null);
			return { r : r #if multidriver, driver : this #end };
		 */
		/*
		 */
		var rt = _renderer.createRenderTarget(depthRT);
		return {r: rt};
	}

	//
	// Uncalled yet
	//

	public override function allocVertexes(m:ManagedBuffer):VertexBuffer {
		/*
		discardError();
		var b = gl.createBuffer();
		gl.bindBuffer(GL.ARRAY_BUFFER, b);
		if( m.size * m.stride == 0 ) throw "assert";
		gl.bufferDataSize(GL.ARRAY_BUFFER, m.size * m.stride * 4, m.flags.has(Dynamic) ? GL.DYNAMIC_DRAW : GL.STATIC_DRAW);
		var outOfMem = outOfMemoryCheck && gl.getError() == GL.OUT_OF_MEMORY;
		gl.bindBuffer(GL.ARRAY_BUFFER, null);
		if( outOfMem ) {
			gl.deleteBuffer(b);
			return null;
		}
		return { b : b, stride : m.stride #if multidriver, driver : this #end };
		*/
		if( m.size * m.stride == 0 ) throw "zero size managed buffer";
		var placeHolder = new Array<hl.UI8>();
		var byteCount = m.size * m.stride * 4;
		placeHolder.resize(byteCount);

		var desc = new forge.Native.BufferLoadDesc();
		desc.setIndexbuffer(byteCount, hl.Bytes.getArray(placeHolder), true);

		/*
		var bits = is32 ? 2 : 1;
		var placeHolder = new Array<hl.UI8>();
		placeHolder.resize(count << bits);

		desc.setIndexbuffer(count << bits, hl.Bytes.getArray(placeHolder), true);
		

		return {b: buff, is32: is32};
*/
		var buff = desc.load(null);

		return { b : buff, stride : m.stride #if multidriver, driver : this #end };
	}

	public override function uploadVertexBuffer(v:VertexBuffer, startVertex:Int, vertexCount:Int, buf:hxd.FloatBuffer, bufPos:Int) {
		var stride : Int = v.stride;
		/*
		gl.bindBuffer(GL.ARRAY_BUFFER, v.b);
		var data = #if hl hl.Bytes.getArray(buf.getNative()) #else buf.getNative() #end;
		gl.bufferSubData(GL.ARRAY_BUFFER, startVertex * stride * 4, streamData(data,bufPos * 4,vertexCount * stride * 4), bufPos * 4 * STREAM_POS, vertexCount * stride * 4);
		gl.bindBuffer(GL.ARRAY_BUFFER, null);
*/

//		v.b.updateRegion(hl.Bytes.getArray(buf.getNative()), startVertex * stride * 4, indiceCount << bits, bufPos << bits);


		throw "Not implemented";
	}


	public override function setRenderFlag(r:RenderFlag, value:Int) {
		throw "Not implemented";
	}

	public override function isDisposed() {
		// throw "Not implemented";
		//		return gl.isContextLost(); // currently just returns false?

		return _renderer != null;
	}

	public override function dispose() {
		throw "Not implemented";
	}

	public override function begin(frame:Int) {
		throw "Not implemented";
	}

	public override function generateMipMaps(texture:h3d.mat.Texture) {
		throw "Mipmaps auto generation is not supported on this platform";
	}

	public override function getNativeShaderCode(shader:hxsl.RuntimeShader):String {
		throw "Not implemented";
		return null;
	}

	override function logImpl(str:String) {
		throw "Not implemented";
	}

	public override function clear(?color:h3d.Vector, ?depth:Float, ?stencil:Int) {
		throw "Not implemented";
	}

	public override function captureRenderBuffer(pixels:hxd.Pixels) {
		throw "Not implemented";
	}

	public override function capturePixels(tex:h3d.mat.Texture, layer:Int, mipLevel:Int, ?region:h2d.col.IBounds):hxd.Pixels {
		throw "Can't capture pixels on this platform";
		return null;
	}

	public override function getDriverName(details:Bool) {
		throw "Not implemented";
		return "Not available";
	}


	public override function selectShader(shader:hxsl.RuntimeShader) {
		throw "Not implemented";
		return false;
	}

	public override function selectMaterial(pass:h3d.mat.Pass) {
		throw "Not implemented";
	}

	public override function uploadShaderBuffers(buffers:h3d.shader.Buffers, which:h3d.shader.Buffers.BufferKind) {
		throw "Not implemented";
	}

	public override function getShaderInputNames():InputNames {
		throw "Not implemented";
		return null;
	}

	public override function selectBuffer(buffer:Buffer) {
		throw "Not implemented";
	}

	public override function selectMultiBuffers(buffers:Buffer.BufferOffset) {
		throw "Not implemented";
	}

	public override function draw(ibuf:IndexBuffer, startIndex:Int, ntriangles:Int) {
		throw "Not implemented";
	}

	public override function drawInstanced(ibuf:IndexBuffer, commands:h3d.impl.InstanceBuffer) {
		throw "Not implemented";
	}

	public override function setRenderZone(x:Int, y:Int, width:Int, height:Int) {
		throw "Not implemented";
	}

	public override function setRenderTarget(tex:Null<h3d.mat.Texture>, layer = 0, mipLevel = 0) {
		throw "Not implemented";
	}

	public override function setRenderTargets(textures:Array<h3d.mat.Texture>) {
		throw "Not implemented";
	}

	public override function disposeDepthBuffer(b:h3d.mat.DepthBuffer) {
		throw "Not implemented";
	}

	public override function present() {
		throw "Not implemented";
	}

	public override function end() {
		throw "Not implemented";
	}

	var _debug = false;

	public override function setDebug(d:Bool) {
		if (d)
			trace('Forge Driver Debug ${d}');
		_debug = d;
	}

	public override function allocTexture(t:h3d.mat.Texture):Texture {
		throw "Not implemented";
		return null;
	}

	public override function allocInstanceBuffer(b:h3d.impl.InstanceBuffer, bytes:haxe.io.Bytes) {
		throw "Not implemented";
	}

	public override function disposeTexture(t:h3d.mat.Texture) {
		throw "Not implemented";
	}

	public override function disposeIndexes(i:IndexBuffer) {
		throw "Not implemented";
	}

	public override function disposeVertexes(v:VertexBuffer) {
		throw "Not implemented";
	}

	public override function disposeInstanceBuffer(b:h3d.impl.InstanceBuffer) {
		throw "Not implemented";
	}

	public override function uploadIndexBytes(i:IndexBuffer, startIndice:Int, indiceCount:Int, buf:haxe.io.Bytes, bufPos:Int) {
		throw "Not implemented";
	}


	public override function uploadVertexBytes(v:VertexBuffer, startVertex:Int, vertexCount:Int, buf:haxe.io.Bytes, bufPos:Int) {
		throw "Not implemented";
	}

	public override function uploadTextureBitmap(t:h3d.mat.Texture, bmp:hxd.BitmapData, mipLevel:Int, side:Int) {
		throw "Not implemented";
	}

	public override function uploadTexturePixels(t:h3d.mat.Texture, pixels:hxd.Pixels, mipLevel:Int, side:Int) {
		throw "Not implemented";
	}

	public override function readVertexBytes(v:VertexBuffer, startVertex:Int, vertexCount:Int, buf:haxe.io.Bytes, bufPos:Int) {
		throw "Driver does not allow to read vertex bytes";
	}

	public override function readIndexBytes(v:IndexBuffer, startVertex:Int, vertexCount:Int, buf:haxe.io.Bytes, bufPos:Int) {
		throw "Driver does not allow to read index bytes";
	}

	/**
		Returns true if we could copy the texture, false otherwise (not supported by driver or mismatch in size/format)
	**/
	public override function copyTexture(from:h3d.mat.Texture, to:h3d.mat.Texture) {
		throw "Not implemented";
		return false;
	}

	// --- QUERY API

	public override function allocQuery(queryKind:QueryKind):Query {
		throw "Not implemented";
		return null;
	}

	public override function deleteQuery(q:Query) {
		throw "Not implemented";
	}

	public override function beginQuery(q:Query) {
		throw "Not implemented";
	}

	public override function endQuery(q:Query) {
		throw "Not implemented";
	}

	public override function queryResultAvailable(q:Query) {
		throw "Not implemented";
		return true;
	}

	public override function queryResult(q:Query) {
		throw "Not implemented";
		return 0.;
	}
}
