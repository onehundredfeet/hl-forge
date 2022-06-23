package h3d.impl;

import hxsl.GlslOut;
import sys.FileSystem;
import h3d.impl.Driver;

private typedef DescriptorIndex = Null<Int>;
private typedef Program = forge.Forge.Program;
private typedef ForgeShader = forge.Native.Shader;

private class CompiledShader {
	public var s:ForgeShader;
	public var vertex:Bool;
	public var globalsIndex:DescriptorIndex;
	public var params:DescriptorIndex;
	public var textures:Array<{u:DescriptorIndex, t:hxsl.Ast.Type, mode:Int}>;
	public var buffers:Array<Int>;
	public var shader:hxsl.RuntimeShader.RuntimeShaderData;

	public function new(s, vertex, shader) {
		this.s = s;
		this.vertex = vertex;
		this.shader = shader;
	}
}

private class CompiledAttribute {
	public var index:Int;
	public var type:Int;
	public var size:Int;
	public var offset:Int;
	public var divisor:Int;

	public function new() {}
}

private class CompiledProgram {
	public var p:Program;
	public var rootSig:forge.Native.RootSignature;
	public var forgeShader:forge.Native.Shader;
	public var vertex:CompiledShader;
	public var fragment:CompiledShader;
	public var stride:Int;
	public var inputs:InputNames;
	public var attribs:Array<CompiledAttribute>;
	public var hasAttribIndex:Array<Bool>;

	public function new() {}
}

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
	var _currentFrame = 0;
	var _boundTextures : Array<Texture> = [];

	public function new() {
		//		window = @:privateAccess dx.Window.windows[0];
		//		Driver.setErrorHandler(onDXError);

		trace('CWD Driver IS ${FileSystem.absolutePath('')}');

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

	function reset() {
		_shaders = new Map();
	}

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
		trace('Presenting');
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

	var _currentRT:forge.Native.RenderTarget;
	var _currentSem:forge.Native.Semaphore;
	var _currentFence:forge.Native.Fence;
	var _currentCmd:forge.Native.Cmd;

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

		_renderer.waitFence(_currentFence);

		// Reset cmd pool for this frame
		_renderer.resetCmdPool(_swapCmdPools[_frameIndex]);

		_currentCmd = _swapCmds[_frameIndex];

		_currentCmd.begin();

		_frameBegun = true;
	}

	var _shaders:Map<Int, CompiledProgram>;

	var _curShader:CompiledProgram;

	function getGLSL(transcoder:forge.GLSLTranscoder, shader:hxsl.RuntimeShader.RuntimeShaderData) {
		if (shader.code == null) {
			transcoder.version = 430;
			transcoder.glES = 4.3;
			shader.code = transcoder.run(shader.data);
			//trace('generated shader code ${shader.code}');
			#if !heaps_compact_mem
			shader.data.funs = null;
			#end
		}

		return shader.code;
	}

	function compileProgram(shader:hxsl.RuntimeShader):CompiledProgram {
		var transcoder = new forge.GLSLTranscoder();

		var p = new CompiledProgram();


		var vert_glsl = getGLSL(transcoder, shader.vertex);
		var frag_glsl = getGLSL(transcoder, shader.fragment);

		//trace('vert shader ${vert_glsl}');
		//trace('frag shader ${frag_glsl}');

		var vert_md5 = haxe.crypto.Md5.encode(vert_glsl);
		var frag_md5 = haxe.crypto.Md5.encode(frag_glsl);
		//trace('vert md5 ${vert_md5}');
		//trace('frag md5 ${frag_md5}');

		//trace('shader cache exists ${FileSystem.exists('shadercache')}');
		//trace('cwd ${FileSystem.absolutePath('')}');

		var vertpath = 'shadercache/shader_${vert_md5}.vert';
		var fragpath = 'shadercache/shader_${frag_md5}.frag';
		sys.io.File.saveContent(vertpath + ".glsl", vert_glsl);
		sys.io.File.saveContent(fragpath + ".glsl", frag_glsl);
		var fgShader = _renderer.createShader(vertpath + ".glsl", fragpath + ".glsl");
		p.forgeShader = fgShader;
		p.vertex = new CompiledShader(fgShader, true, shader.vertex);
		p.fragment = new CompiledShader(fgShader, false, shader.fragment);


		var rootDesc = new forge.Native.RootSignatureDesc();
		rootDesc.addShader(fgShader);
		var rootSig = _renderer.createRootSig(rootDesc);
		p.rootSig = rootSig;


		// get inputs
		p.vertex.params = rootSig.getDescriptorIndexFromName( "_vertrootconstants");
		p.vertex.globalsIndex = rootSig.getDescriptorIndexFromName( "_vertrootconstants");
		p.fragment.params = rootSig.getDescriptorIndexFromName( "_fragrootconstants");
		p.fragment.globalsIndex = rootSig.getDescriptorIndexFromName( "_fragrootconstants");
		trace('Indices vg ${p.vertex.globalsIndex} vp ${p.vertex.params} fg ${p.fragment.globalsIndex} fp ${p.fragment.params}');

		var attribNames = [];
		p.attribs = [];
		p.hasAttribIndex = [];
		p.stride = 0;
		var idxCount = 0;
		for (v in shader.vertex.data.vars) {
			switch (v.kind) {
				case Input:
					var size = switch (v.type) {
						case TVec(n, _): n;
						case TFloat: 1;
						default: throw "assert " + v.type;
					}

					/*
						var t = GL.FLOAT;

						var index = gl.getAttribLocation(p.p, transcoder.varNames.exists(v.id) ? transcoder.varNames.get(v.id) : v.name);
						if (index < 0) {
							p.stride += size;
							continue;
						}
					 */

					var a = new CompiledAttribute();
					a.type = 0;
					a.size = size;
					a.index = idxCount++;
					a.offset = p.stride;
					a.divisor = 0;
					if (v.qualifiers != null) {
						for (q in v.qualifiers)
							switch (q) {
								case PerInstance(n): a.divisor = n;
								default:
							}
					}
					p.attribs.push(a);
					p.hasAttribIndex[a.index] = true;
					attribNames.push(v.name);
					p.stride += size;
				default:
			}
		}
		p.inputs = InputNames.get(attribNames);

		return p;
	}

	public override function getShaderInputNames():InputNames {
		return _curShader.inputs;
	}

	public override function selectShader(shader:hxsl.RuntimeShader) {
		var p = _shaders.get(shader.id);
		if (p == null) {
			p = compileProgram(shader);
			_shaders.set( shader.id, p);
		}

		_curShader = p;

		return true;
	}
	
	function setCurrentShader( shader : CompiledProgram ) {
		_curShader = shader;
	}

	function configureTextures( s : CompiledShader, buf : h3d.shader.Buffers.ShaderBuffers) {
		if (s == null) return;
		if (buf == null) return;
		if (s.textures == null) {
			trace("WARNING: No texture array on compiled shader, may be a missing feature, also could just have no textures");
			return;
		}
		for (i in 0...s.textures.length) {
			var t = buf.tex[i];
			var pt = s.textures[i];
			if( t == null || t.isDisposed() ) {
				switch( pt.t ) {
				case TSampler2D:
					var color = h3d.mat.Defaults.loadingTextureColor;
					t = h3d.mat.Texture.fromColor(color, (color >>> 24) / 255);
				case TSamplerCube:
					t = h3d.mat.Texture.defaultCubeTexture();
				default:
					throw "Missing texture";
				}
			}
			if( t != null && t.t == null && t.realloc != null ) {
				var ts = _curShader;
				t.alloc();
				t.realloc();	// why re-alloc after allocing?
				if( _curShader != ts ) {
					// realloc triggered a shader change !
					// we need to reset the original shader and reupload everything
					throw("Shader reallocation!!! Seems like bad architecture.");

					return;
				}
			}
			t.lastFrame = _currentFrame;

			if( pt.u == null ) continue;

			var idx = s.vertex ? i : _curShader.vertex.textures.length + i;

			if( _boundTextures[idx] != t.t ) {
				_boundTextures[idx] = t.t;

				#if multidriver
				if( t.t.driver != this )
					throw "Invalid texture context";
				#end
				throw "Not implemented";

				/*
				var mode = getBindType(t);
				if( mode != pt.mode )
					throw "Texture format mismatch: "+t+" should be "+pt.t;
				gl.activeTexture(GL.TEXTURE0 + idx);
				gl.uniform1i(pt.u, idx);
				gl.bindTexture(mode, t.t.t);
				lastActiveIndex = idx;
				*/
			}
			var mip = Type.enumIndex(t.mipMap);
			var filter = Type.enumIndex(t.filter);
			var wrap = Type.enumIndex(t.wrap);
			var bits = mip | (filter << 3) | (wrap << 6);
			if( bits != t.t.bits ) {
				t.t.bits = bits;
				throw "Not implemented";
				/*
				var flags = TFILTERS[mip][filter];
				var mode = pt.mode;
				gl.texParameteri(mode, GL.TEXTURE_MAG_FILTER, flags[0]);
				gl.texParameteri(mode, GL.TEXTURE_MIN_FILTER, flags[1]);
				var w = TWRAP[wrap];
				gl.texParameteri(mode, GL.TEXTURE_WRAP_S, w);
				gl.texParameteri(mode, GL.TEXTURE_WRAP_T, w);
				*/
			}
		}
	}
	var _tmpConstantBuffer = new Array<Float>();
	public override function uploadShaderBuffers(buf:h3d.shader.Buffers, which:h3d.shader.Buffers.BufferKind) {
		if (_curShader == null)
			throw "No current shader to upload buffers to";

		switch (which) {
			case Globals: trace ('Ignoring globals'); // do nothing as it was all done by the globals
			case Params:  trace ('Upload Globals & Params'); 
			if( buf.vertex.globals != null || buf.vertex.params != null) {
				var max = (buf.vertex.globals != null ? buf.vertex.globals.length : 0) + (buf.vertex.params != null ? buf.vertex.params.length : 0);
				if (_tmpConstantBuffer.length < max) _tmpConstantBuffer.resize(max);
				var total = 0;
				var tmpBuff = hl.Bytes.getArray(_tmpConstantBuffer);

				if (buf.vertex.globals != null && buf.vertex.globals.length > 0) {
					var l = buf.vertex.globals.length * 4;
					tmpBuff.blit(total, hl.Bytes.getArray(buf.vertex.globals.toData()), 0, buf.vertex.globals.length * 4);
					total += buf.vertex.globals.length * 4;
				}

				if(buf.vertex.params != null && buf.vertex.params.length > 0) {
					tmpBuff.blit(total, hl.Bytes.getArray(buf.vertex.params.toData()), 0, buf.vertex.params.length * 4);
					total +=  buf.vertex.params.length;
				}
				trace ('Total is ${total}');
				if (total > 0) {					
					_currentCmd.pushConstants( _curShader.rootSig, _curShader.vertex.globalsIndex, tmpBuff  );
				}
				// Update buffer
//					gl.uniform4fv(s.globals, streamData(hl.Bytes.getArray(buf.globals.toData()), 0, s.shader.globalsSize * 16), 0, s.shader.globalsSize * 4);
			}
			if( buf.fragment.globals != null || buf.fragment.params != null ) {
				//compute total in floats
				var total = (buf.fragment.globals != null ? buf.fragment.globals.length : 0) + (buf.fragment.params != null ? buf.fragment.params.length : 0);
				if (_tmpConstantBuffer.length < total) _tmpConstantBuffer.resize(total);
				trace ('Total is ${total}');
				// Update buffer
				_currentCmd.pushConstants( _curShader.rootSig, _curShader.fragment.globalsIndex, hl.Bytes.getArray(_tmpConstantBuffer) );

//					gl.uniform4fv(s.globals, streamData(hl.Bytes.getArray(buf.globals.toData()), 0, s.shader.globalsSize * 16), 0, s.shader.globalsSize * 4);
			}
			case Textures:  
				trace ('Upload Textures');
				configureTextures(_curShader.vertex, buf.vertex);
				configureTextures(_curShader.fragment, buf.fragment);

			case Buffers:  trace ('Upload Buffers'); 

			if( _curShader.vertex.buffers != null ) {
				trace('Vertex buffers length ${ _curShader.vertex.buffers}');
			}
			if (_curShader.fragment.buffers != null) {
				trace('Fragment buffers length ${ _curShader.fragment.buffers}');
			}
			/*
				var start = 0;
				if( !s.vertex && curShader.vertex.buffers != null )
					start = curShader.vertex.buffers.length;
				for( i in 0...s.buffers.length )
					gl.bindBufferBase(GL.UNIFORM_BUFFER, i + start, @:privateAccess buf.buffers[i].buffer.vbuf.b);
			}
			*/

		}


	}


	public override function selectMaterial(pass:h3d.mat.Pass) {
		// culling
		// stencil
		// mode
		// blending
		throw "Not implemented";

	}

	var _curBuffer:h3d.Buffer;

	public override function selectMultiBuffers(buffers:Buffer.BufferOffset) {
		for (a in _curShader.attribs) {
			// gl.bindBuffer(GL.ARRAY_BUFFER, @:privateAccess buffers.buffer.buffer.vbuf.b);
			// gl.vertexAttribPointer(a.index, a.size, a.type, false, buffers.buffer.buffer.stride * 4, buffers.offset * 4);
			// updateDivisor(a);
			buffers = buffers.next;
		}
		_curBuffer = null;
		throw "Not implemented";

	}

	var _curIndexBuffer:IndexBuffer;

	public override function draw(ibuf:IndexBuffer, startIndex:Int, ntriangles:Int) {
		if (ibuf != _curIndexBuffer) {
			_curIndexBuffer = ibuf;
			//			gl.bindBuffer(GL.ELEMENT_ARRAY_BUFFER, ibuf.b);
		}

		/*
			if( ibuf.is32 )
				gl.drawElements(drawMode, ntriangles * 3, GL.UNSIGNED_INT, startIndex * 4);
			else
				gl.drawElements(drawMode, ntriangles * 3, GL.UNSIGNED_SHORT, startIndex * 2);
		 */
		 throw "Not implemented";

	}

	public override function selectBuffer(v:Buffer) {
		trace('selecting buffer');
		if( v == _curBuffer )
			return;
		if( _curBuffer != null && v.buffer == _curBuffer.buffer && v.buffer.flags.has(RawFormat) == _curBuffer.flags.has(RawFormat) ) {
			_curBuffer = v;
			return;
		}

		if( _curShader == null )
			throw "No shader selected";
		_curBuffer = v;

		var m = @:privateAccess v.buffer.vbuf;
		if( m.stride < _curShader.stride )
			throw "Buffer stride (" + m.stride + ") and shader stride (" + _curShader.stride + ") mismatch";

		#if multidriver
		if( m.driver != this )
			throw "Invalid buffer context";
		#end
//		gl.bindBuffer(GL.ARRAY_BUFFER, m.b);

		if( v.flags.has(RawFormat) ) {
			for( a in _curShader.attribs ) {
				var pos = a.offset;
				//gl.vertexAttribPointer(a.index, a.size, a.type, false, m.stride * 4, pos * 4);
				//updateDivisor(a);
			}
		} else {
			var offset = 8;
			for( i in 0..._curShader.attribs.length ) {
				var a = _curShader.attribs[i];
				var pos;
				switch( _curShader.inputs.names[i] ) {
				case "position":
					pos = 0;
				case "normal":
					if( m.stride < 6 ) throw "Buffer is missing NORMAL data, set it to RAW format ?" #if track_alloc + @:privateAccess v.allocPos #end;
					pos = 3;
				case "uv":
					if( m.stride < 8 ) throw "Buffer is missing UV data, set it to RAW format ?" #if track_alloc + @:privateAccess v.allocPos #end;
					pos = 6;
				case s:
					pos = offset;
					offset += a.size;
					if( offset > m.stride ) throw "Buffer is missing '"+s+"' data, set it to RAW format ?" #if track_alloc + @:privateAccess v.allocPos #end;
				}
				//gl.vertexAttribPointer(a.index, a.size, a.type, false, m.stride * 4, pos * 4);
				//updateDivisor(a);
			}
		}
		throw "Not implemented";

	}

	public override function clear(?color:h3d.Vector, ?depth:Float, ?stencil:Int) {
		_currentCmd.clear(_currentRT, null);
	}
	public override function end() {
		_currentCmd.unbindRenderTarget();
		_currentCmd.end();
		_queue.submit(_currentCmd, _currentSem, _ImageAcquiredSemaphore, _currentFence);
	}
	
	
	/*
		function uploadBuffer( buffer : h3d.shader.Buffers, s : CompiledShader, buf : h3d.shader.Buffers.ShaderBuffers, which : h3d.shader.Buffers.BufferKind ) {
			switch( which ) {
			case Globals:
				if( s.globals != null ) {
					gl.uniform4fv(s.globals, streamData(hl.Bytes.getArray(buf.globals.toData()), 0, s.shader.globalsSize * 16), 0, s.shader.globalsSize * 4);
				}
			case Params:
				if( s.params != null ) {
					gl.uniform4fv(s.params, streamData(hl.Bytes.getArray(buf.params.toData()), 0, s.shader.paramsSize * 16), 0, s.shader.paramsSize * 4);
				}
			case Buffers:
				
			
		}

	 */
	/*
		function compileShader(shader : hxsl.RuntimeShader.RuntimeShaderData ) {

			var type = shader.vertex ? GL.VERTEX_SHADER : GL.FRAGMENT_SHADER;
			
			var s = gl.createShader(type);
			gl.shaderSource(s, shader.code);
			gl.compileShader(s);
			var log = gl.getShaderInfoLog(s);
			if ( gl.getShaderParameter(s, GL.COMPILE_STATUS) != cast 1 ) {
				var log = gl.getShaderInfoLog(s);
				var lid = Std.parseInt(log.substr(9));
				var line = lid == null ? null : shader.code.split("\n")[lid - 1];
				if( line == null ) line = "" else line = "(" + StringTools.trim(line) + ")";
				var codeLines = shader.code.split("\n");
				for( i in 0...codeLines.length )
					codeLines[i] = (i+1) + "\t" + codeLines[i];
				throw "An error occurred compiling the shaders: " + log + line+"\n\n"+codeLines.join("\n");
			}
			  
		}
	 */
	/*


			

			p.p = gl.createProgram();
			#if ((hlsdl || usegl) && !hlmesa)
			if( glES == null ) {
				var outCount = 0;
				for( v in shader.fragment.data.vars )
					switch( v.kind ) {
					case Output:
						gl.bindFragDataLocation(p.p, outCount++, glout.varNames.exists(v.id) ? glout.varNames.get(v.id) : v.name);
					default:
					}
			}
			#end
			gl.attachShader(p.p, p.vertex.s);
			gl.attachShader(p.p, p.fragment.s);
			var log = null;
			try {
				gl.linkProgram(p.p);
				if( gl.getProgramParameter(p.p, GL.LINK_STATUS) != cast 1 )
					log = gl.getProgramInfoLog(p.p);
			} catch( e : Dynamic ) {
				throw "Shader linkage error: "+Std.string(e)+" ("+getDriverName(false)+")";
			}
			gl.deleteShader(p.vertex.s);
			gl.deleteShader(p.fragment.s);
			if( log != null ) {
				#if js
				gl.deleteProgram(p.p);
				#end
				#if hlsdl
					//Tentative patch on some driver that report an higher shader version that it's allowed to use.
				if( log == "" && shaderVersion > 130 && firstShader ) {
					shaderVersion -= 10;
					return selectShader(shader);
				}
				#end
				throw "Program linkage failure: "+log+"\nVertex=\n"+shader.vertex.code+"\n\nFragment=\n"+shader.fragment.code;
			}
			firstShader = false;
			initShader(p, p.vertex, shader.vertex, shader);
			initShader(p, p.fragment, shader.fragment, shader);
			var attribNames = [];
			p.attribs = [];
			p.hasAttribIndex = [];
			p.stride = 0;
			for( v in shader.vertex.data.vars )
				switch( v.kind ) {
				case Input:
					var t = GL.FLOAT;
					var size = switch( v.type ) {
					case TVec(n, _): n;
					case TBytes(n): t = GL.BYTE; n;
					case TFloat: 1;
					default: throw "assert " + v.type;
					}
					var index = gl.getAttribLocation(p.p, glout.varNames.exists(v.id) ? glout.varNames.get(v.id) : v.name);
					if( index < 0 ) {
						p.stride += size;
						continue;
					}
					var a = new CompiledAttribute();
					a.type = t;
					a.size = size;
					a.index = index;
					a.offset = p.stride;
					a.divisor = 0;
					if( v.qualifiers != null ) {
						for( q in v.qualifiers )
							switch( q ) {
							case PerInstance(n): a.divisor = n;
							default:
							}
					}
					p.attribs.push(a);
					p.hasAttribIndex[a.index] = true;
					attribNames.push(v.name);
					p.stride += size;
				default:
				}
			p.inputs = InputNames.get(attribNames);
			programs.set(shader.id, p);
		}
		if( curShader == p ) return false;
		setProgram(p);
		return true;
	 */
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
