package h3d.impl;

import h3d.impl.Driver;

@:access(h3d.impl.Shader)
class ForgeDriver extends h3d.impl.Driver {
	var onContextLost:Void->Void;

	var _renderer:forge.Native.Renderer;
	var _queue:forge.Native.Queue;
	var _swapCmdPools = new Array<forge.Native.CmdPool>();
	var _swapCmds = new Array<forge.Native.Cmd>();
	var _swapCompleteFences = new Array<forge.Native.Fence>();
	var _swapCompleteSemaphores = new Array<forge.Native.Semaphore>();
	var _ImageAcquiredSemaphore:forge.Native.Semaphore;

	var _width:Int;
	var _height:Int;
	var _swap_count = 3;
	var _hdr = false;
	var _maxCompressedTexturesSupport = 3; // arbitrary atm

	public function new() {
		//		window = @:privateAccess dx.Window.windows[0];
		//		Driver.setErrorHandler(onDXError);

		reset();
	}

	var _forgeSDLWin:forge.Native.ForgeSDLWindow;

	// This is the first function called after the constructor
	public override function init(onCreate:Bool->Void, forceSoftware = false) {
		var win = cast(@:privateAccess hxd.Window.getInstance().window, sdl.WindowForge);
		_forgeSDLWin = win.getForgeSDLWindow();

		// copied from DX driver
		onContextLost = onCreate.bind(true);
		haxe.Timer.delay(onCreate.bind(false), 1); // seems arbitrary

		if (forceSoftware)
			throw "Software mode not supported";

		var win = @:privateAccess hxd.Window.getInstance();
		_width = win.width;
		_height = win.height;

		_renderer = _forgeSDLWin.renderer();
		_queue = _renderer.createQueue();

		for (i in 0..._swap_count) {
			var cmdPool = _renderer.createCommandPool(_queue);
			_swapCmdPools.push(cmdPool);

			var cmd = _renderer.createCommand(cmdPool);
			_swapCmds.push(cmd);

			_swapCompleteFences.push(_renderer.createFence());
			_swapCompleteSemaphores.push(_renderer.createSemaphore());
		}

		_ImageAcquiredSemaphore = _renderer.createSemaphore();

		_renderer.initResourceLoaderInterface();
		attach();
	}

	function addSwapChain() {
		return true;
	}

	function addDepthBuffer() {
		return true;
	}

	var _sc:forge.Native.SwapChain;

	function attach() {
		if (_sc == null) {
			_sc = _forgeSDLWin.createSwapChain(_renderer, _queue, _width, _height, _swap_count, _hdr);
			if (_sc == null) {
				throw "Swapchain is null";
			}
		} else {
			trace('Duplicate attach');
		}

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
		trace('----> SC IS ${_sc}');

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
			//			trace('b is ${b} format is ${b != null ? b.format : null}');
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
		if (m.size * m.stride == 0)
			throw "zero size managed buffer";
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

		return {b: buff, stride: m.stride #if multidriver, driver: this #end};
	}

	public override function allocTexture(t:h3d.mat.Texture):Texture {
		var ftd = new forge.Native.TextureDesc();

		ftd.width = t.width;
		ftd.height = t.height;
		ftd.arraySize = 1;
		ftd.depth = 1;
		ftd.mipLevels = 1;
		ftd.sampleCount = SAMPLE_COUNT_1;
		ftd.startState = RESOURCE_STATE_COMMON;
		ftd.flags = TEXTURE_CREATION_FLAG_OWN_MEMORY_BIT;

		ftd.format = switch (t.format) {
			case RGBA: TinyImageFormat_R8G8B8A8_UNORM;
			case SRGB: TinyImageFormat_R8G8B8A8_SRGB;
			case RGBA16F: TinyImageFormat_R16G16B16A16_SFLOAT;
			default: throw "Unsupported texture format " + t.format;
		}
		//		trace ('Format is ${ftd.format}');
		var flt = ftd.load(t.name, null);

		if (flt == null)
			throw "invalid texture";
		forge.Native.Globals.waitForAllResourceLoads();

		var tt:Texture = {
			t: flt,
			width: t.width,
			height: t.height,
			internalFmt: ftd.format.toValue(),
			bits: -1,
			bind: 0,
			bias: 0
			#if multidriver
			, driver: this
			#end
		};

		/*
			var tt = gl.createTexture();
			var bind = getBindType(t);
			var tt : Texture = { t : tt, width : t.width, height : t.height, internalFmt : GL.RGBA, pixelFmt : GL.UNSIGNED_BYTE, bits : -1, bind : bind, bias : 0 #if multidriver, driver : this #end };
			switch( t.format ) {
			case RGBA:
				// default
			case RGBA32F if( hasFeature(FloatTextures) ):
				tt.internalFmt = GL.RGBA32F;
				tt.pixelFmt = GL.FLOAT;
			case RGBA16F if( hasFeature(FloatTextures) ):
				tt.pixelFmt = GL.HALF_FLOAT;
				tt.internalFmt = GL.RGBA16F;
			case BGRA:
				tt.internalFmt = GL.RGBA8;
			case SRGB:
				tt.internalFmt = GL.SRGB8;
			#if !js
			case SRGB_ALPHA:
				tt.internalFmt = GL.SRGB8_ALPHA;
			#end
			case RGB8:
				tt.internalFmt = GL.RGB;
			case R8:
				tt.internalFmt = GL.R8;
			case RG8:
				tt.internalFmt = GL.RG8;
			case R16F:
				tt.internalFmt = GL.R16F;
				tt.pixelFmt = GL.HALF_FLOAT;
			case RG16F:
				tt.internalFmt = GL.RG16F;
				tt.pixelFmt = GL.HALF_FLOAT;
			case R32F:
				tt.internalFmt = GL.R32F;
				tt.pixelFmt = GL.FLOAT;
			case RG32F:
				tt.internalFmt = GL.RG32F;
				tt.pixelFmt = GL.FLOAT;
			case RGB16F:
				tt.internalFmt = GL.RGB16F;
				tt.pixelFmt = GL.HALF_FLOAT;
			case RGB32F:
				tt.internalFmt = GL.RGB32F;
				tt.pixelFmt = GL.FLOAT;
			case RGB10A2:
				tt.internalFmt = GL.RGB10_A2;
				tt.pixelFmt = GL.UNSIGNED_INT_2_10_10_10_REV;
			case RG11B10UF:
				tt.internalFmt = GL.R11F_G11F_B10F;
				tt.pixelFmt = GL.UNSIGNED_INT_10F_11F_11F_REV;
			case S3TC(n) if( n <= maxCompressedTexturesSupport ):
				if( t.width&3 != 0 || t.height&3 != 0 )
					throw "Compressed texture "+t+" has size "+t.width+"x"+t.height+" - must be a multiple of 4";
				switch( n ) {
				case 1: tt.internalFmt = 0x83F1; // COMPRESSED_RGBA_S3TC_DXT1_EXT
				case 2:	tt.internalFmt = 0x83F2; // COMPRESSED_RGBA_S3TC_DXT3_EXT
				case 3: tt.internalFmt = 0x83F3; // COMPRESSED_RGBA_S3TC_DXT5_EXT
				default: throw "Unsupported texture format "+t.format;
				}
			default:
				throw "Unsupported texture format "+t.format;
			}
			t.lastFrame = frame;
			t.flags.unset(WasCleared);
			gl.bindTexture(bind, tt.t);
			var outOfMem = false;

			inline function checkError() {
				if( !outOfMemoryCheck ) return false;
				var err = gl.getError();
				if( err == GL.OUT_OF_MEMORY ) {
					outOfMem = true;
					return true;
				}
				if( err != 0 ) throw "Failed to alloc texture "+t.format+"(error "+err+")";
				return false;
			}

			#if (js || (hlsdl >= version("1.12.0")))
			gl.texParameteri(bind, GL.TEXTURE_BASE_LEVEL, 0);
			gl.texParameteri(bind, GL.TEXTURE_MAX_LEVEL, t.mipLevels-1);
			#end
			for(mip in 0...t.mipLevels) {
				var w = hxd.Math.imax(1, tt.width >> mip);
				var h = hxd.Math.imax(1, tt.height >> mip);
				if( t.flags.has(Cube) ) {
					for( i in 0...6 ) {
						gl.texImage2D(CUBE_FACES[i], mip, tt.internalFmt, w, h, 0, getChannels(tt), tt.pixelFmt, null);
						if( checkError() ) break;
					}
				} else if( t.flags.has(IsArray) ) {
					gl.texImage3D(bind, mip, tt.internalFmt, w, h, t.layerCount, 0, getChannels(tt), tt.pixelFmt, null);
					checkError();
				} else {
					#if js
					if( !t.format.match(S3TC(_)) )
					#end
					gl.texImage2D(bind, mip, tt.internalFmt, w, h, 0, getChannels(tt), tt.pixelFmt, null);
					checkError();
				}
			}
			restoreBind();

			if( outOfMem ) {
				gl.deleteTexture(tt.t);
				return null;
			}

			return tt;
		 */
		/*
			var desc = new forge.Native.TextureLoadDesc();
			desc.creationFlag = 
		 */
		return tt;
	}

	public override function uploadVertexBuffer(v:VertexBuffer, startVertex:Int, vertexCount:Int, buf:hxd.FloatBuffer, bufPos:Int) {
		var stride:Int = v.stride;
		/*
			gl.bindBuffer(GL.ARRAY_BUFFER, v.b);
			var data = #if hl hl.Bytes.getArray(buf.getNative()) #else buf.getNative() #end;
			gl.bufferSubData(GL.ARRAY_BUFFER, startVertex * stride * 4, streamData(data,bufPos * 4,vertexCount * stride * 4), bufPos * 4 * STREAM_POS, vertexCount * stride * 4);
			gl.bindBuffer(GL.ARRAY_BUFFER, null);
		 */

		v.b.updateRegion(hl.Bytes.getArray(buf.getNative()), startVertex * stride * 4, vertexCount * stride * 4, bufPos * 4);
	}

	public override function uploadTexturePixels(t:h3d.mat.Texture, pixels:hxd.Pixels, mipLevel:Int, side:Int) {
		trace('Uploading pixels ${pixels.width} x ${pixels.height}');
		trace('----> SC IS ${_sc}');

		pixels.convert(t.format);
		pixels.setFlip(false);
		var dataLen = pixels.dataSize;

		var tt = t.t;
		var ft = tt.t;

		//		trace('Sending bytes ${pixels.bytes} : ${dataLen}');
		ft.upload(hl.Bytes.fromBytes(pixels.bytes), dataLen);
		//		trace('Done uplaoding');
		t.flags.set(WasCleared);
	}

	var _frameIndex = 0;
	var _frameBegun = false;

	public override function present() {
		if (_frameBegun) {
			if (_sc != null) {
				_forgeSDLWin.present(_queue, _sc, _frameIndex, _swapCompleteSemaphores[_frameIndex]);
				_frameIndex = (_frameIndex + 1) % _swap_count;
			} else {
				trace('Swap chain is null???');
				// throw "Swap Chain is null";
			}
		} else {}
	}

	/*
		var cubic = t.flags.has(Cube);
		var face = cubic ? CUBE_FACES[side] : t.flags.has(IsArray) ? GL.TEXTURE_2D_ARRAY : GL.TEXTURE_2D;
		var bind = getBindType(t);
		gl.bindTexture(bind, t.t.t);
		pixels.convert(t.format);
		pixels.setFlip(false);
		var dataLen = pixels.dataSize;
		#if hl
		var stream = streamData(pixels.bytes.getData(),pixels.offset,dataLen);
		if( t.format.match(S3TC(_)) ) {
			if( t.flags.has(IsArray) )
				#if (hlsdl >= version("1.12.0"))
				gl.compressedTexSubImage3D(face, mipLevel, 0, 0, side, pixels.width, pixels.height, 1, t.t.internalFmt, dataLen, stream);
				#else throw "TextureArray support requires hlsdl 1.12+"; #end
			else
				gl.compressedTexImage2D(face, mipLevel, t.t.internalFmt, pixels.width, pixels.height, 0, dataLen, stream);
		} else {
			if( t.flags.has(IsArray) )
				#if (hlsdl >= version("1.12.0"))
				gl.texSubImage3D(face, mipLevel, 0, 0, side, pixels.width, pixels.height, 1, getChannels(t.t), t.t.pixelFmt, stream);
				#else throw "TextureArray support requires hlsdl 1.12+"; #end
			else
				gl.texImage2D(face, mipLevel, t.t.internalFmt, pixels.width, pixels.height, 0, getChannels(t.t), t.t.pixelFmt, stream);
		}
		#elseif js
		#if hxnodejs
		if( (pixels:Dynamic).bytes.b.hxBytes != null ) {
			// if the pixels are a nodejs buffer, their might be GC'ed while upload !
			// might be some problem with Node/WebGL relation
			// let's clone the pixels in order to have a fresh JS bytes buffer
			pixels = pixels.clone();
		}
		#end
		var buffer : ArrayBufferView = switch( t.format ) {
		case RGBA32F, R32F, RG32F, RGB32F: new Float32Array(@:privateAccess pixels.bytes.b.buffer, pixels.offset, dataLen>>2);
		case RGBA16F, R16F, RG16F, RGB16F: new Uint16Array(@:privateAccess pixels.bytes.b.buffer, pixels.offset, dataLen>>1);
		case RGB10A2, RG11B10UF: new Uint32Array(@:privateAccess pixels.bytes.b.buffer, pixels.offset, dataLen>>2);
		default: new Uint8Array(@:privateAccess pixels.bytes.b.buffer, pixels.offset, dataLen);
		}
		if( t.format.match(S3TC(_)) ) {
			if( t.flags.has(IsArray) )
				gl.compressedTexSubImage3D(face, mipLevel, 0, 0, side, pixels.width, pixels.height, 1, t.t.internalFmt, buffer);
			else
				gl.compressedTexImage2D(face, mipLevel, t.t.internalFmt, pixels.width, pixels.height, 0, buffer);
		} else {
			if( t.flags.has(IsArray) )
				gl.texSubImage3D(face, mipLevel, 0, 0, side, pixels.width, pixels.height, 1, getChannels(t.t), t.t.pixelFmt, buffer);
			else
				gl.texImage2D(face, mipLevel, t.t.internalFmt, pixels.width, pixels.height, 0, getChannels(t.t), t.t.pixelFmt, buffer);
		}
		#else
		throw "Not implemented";
		#end
		t.flags.set(WasCleared);
		restoreBind();
	 */
	public override function setRenderFlag(r:RenderFlag, value:Int) {
		throw "Not implemented";
	}

	public override function isDisposed() {
		return _renderer == null;
	}

	public override function dispose() {
		throw "Not implemented";
	}

	var _currentRT : forge.Native.RenderTarget;
	var _currentSem : forge.Native.Semaphore;
	var _currentFence : forge.Native.Fence;
	var _currentCmd : forge.Native.Cmd;
	
	public override function begin(frame:Int) {

		// Check for VSYNC
		/*
		if (pSwapChain->mEnableVsync != mSettings.mVSyncEnabled)
			{
				waitQueueIdle(pGraphicsQueue);
				::toggleVSync(pRenderer, &pSwapChain);
			}

		*/

		var swapIndex = _renderer.acquireNextImage(_sc, _ImageAcquiredSemaphore, null);

		_currentRT = _sc.getRenderTarget(swapIndex);
		_currentSem = _swapCompleteSemaphores[_frameIndex];
		_currentFence = _swapCompleteFences[_frameIndex];

		_renderer.waitFence( _currentFence );

		// Reset cmd pool for this frame
		_renderer.resetCmdPool( _swapCmdPools[_frameIndex] );

		_currentCmd = _swapCmds[_frameIndex];

		_currentCmd.begin();

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
		_currentCmd.clear(_currentRT, null);
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

	public override function end() {

		_queue.submit(_currentCmd, _currentSem, _ImageAcquiredSemaphore, _currentFence);

		throw "Not implemented";


	}

	var _debug = false;

	public override function setDebug(d:Bool) {
		if (d)
			trace('Forge Driver Debug ${d}');
		_debug = d;
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
