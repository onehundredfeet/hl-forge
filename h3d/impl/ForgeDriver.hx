package h3d.impl;

import haxe.Int64;
import hl.I64;
import h3d.mat.Data.Compare;
import hxsl.GlslOut;
import sys.FileSystem;
import h3d.impl.Driver;
import h3d.mat.Pass as Pass;
import forge.Native.BlendStateTargets as BlendStateTargets;
import forge.Native.ColorsMask as ColorsMask;
import forge.Native.StateBuilder as StateBuilder;

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
	public var id: Int;
	public var p:Program;
	public var rootSig:forge.Native.RootSignature;
	public var forgeShader:forge.Native.Shader;
	public var vertex:CompiledShader;
	public var fragment:CompiledShader;
	public var stride:Int;
	public var inputs:InputNames;
	public var attribs:Array<CompiledAttribute>;
	public var hasAttribIndex:Array<Bool>;
	public var layout: forge.Native.VertexLayout;

	public function new() {}
}

private class CompiledMaterial {
	public function new() {}
	public var _shader : CompiledProgram;
	public var _hash : Int64;
	public var _id : Int;
	public var _rawState : StateBuilder;
	public var _pipeline : forge.Native.Pipeline;
	public var _layout: forge.Native.VertexLayout;
	public var _state : StateBuilder;
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
		trace('updating index buffer');
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
		trace('RENDER Allocating depth buffer ${b.width} ${b.height}');

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
		RenderTargetDesc depthRT = {};
		depthRT.mArraySize = 1;
		depthRT.mClearValue.depth = 0.0f;
		depthRT.mClearValue.stencil = 0;
		depthRT.mDepth = 1;
		depthRT.mFormat = TinyImageFormat_D32_SFLOAT;
		depthRT.mStartState = RESOURCE_STATE_DEPTH_WRITE;
		depthRT.mHeight = mSettings.mHeight;
		depthRT.mSampleCount = SAMPLE_COUNT_1;
		depthRT.mSampleQuality = 0;
		depthRT.mWidth = mSettings.mWidth;
		depthRT.mFlags = TEXTURE_CREATION_FLAG_ON_TILE | TEXTURE_CREATION_FLAG_VR_MULTIVIEW;

		*/
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
		
		//		desc.setVertexbuffer

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
		desc.setVertexbuffer(byteCount, hl.Bytes.getArray(placeHolder), true);
		/*
			var bits = is32 ? 2 : 1;
			var placeHolder = new Array<hl.UI8>();
			placeHolder.resize(count << bits);

			desc.setIndexbuffer(count << bits, hl.Bytes.getArray(placeHolder), true);


			return {b: buff, is32: is32};
		 */

		var buff = desc.load(null);

		trace ('FILTER - Allocating vertex buffer ${m.size} with stride ${m.stride} total bytecount  ${byteCount}');
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

	static inline var STREAM_POS = #if hl 0 #else 1 #end;

	/*
	inline function streamData(data, pos:Int, length:Int) {
		#if hl
		var needed = streamPos + length;
		var total = (needed + 7) & ~7; // align on 8 bytes
		var alen = total - streamPos;
		if( total > streamLen ) expandStream(total);
		streamBytes.blit(streamPos, data, pos, length);
		data = streamBytes.offset(streamPos);
		streamPos += alen;
		#end
		return data;
	}
	*/
	public override function uploadVertexBuffer(v:VertexBuffer, startVertex:Int, vertexCount:Int, buf:hxd.FloatBuffer, bufPos:Int) {
		var stride:Int = v.stride;
		/*
		void glBufferSubData( 
			GLenum target, 
			GLintptr offset, 
			GLsizeiptr size, 
			const void * data);

			gl.bindBuffer(GL.ARRAY_BUFFER, v.b);
var data = #if hl hl.Bytes.getArray(buf.getNative()) #else buf.getNative() #end;
gl.bufferSubData(GL.ARRAY_BUFFER, 
	startVertex * stride * 4,  // effectively 0
	streamData(data,bufPos * 4,vertexCount * stride * 4), 
	bufPos * 4 * 0, 
	vertexCount * stride * 4);
			gl.bindBuffer(GL.ARRAY_BUFFER, null);
		 */

		 trace('updating vertex buffer vstride ${v.stride} sv ${startVertex} vc ${vertexCount} bufPos ${bufPos} buf len ${buf.length}');
		v.b.updateRegion(hl.Bytes.getArray(buf.getNative()), startVertex * stride * 4, vertexCount * stride * 4, bufPos * 4 * 0);
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
		_firstDraw = true;
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

	var _programIds = 0;
	function compileProgram(shader:hxsl.RuntimeShader):CompiledProgram {
		var transcoder = new forge.GLSLTranscoder();

		var p = new CompiledProgram();
		p.id = _programIds++;

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
		var vl = new forge.Native.VertexLayout();
		vl.attribCount = 0;

		p.layout = vl;

		var offset = 0;

		for (v in shader.vertex.data.vars) {
			switch (v.kind) {
				case Input:
					var size = switch (v.type) {
						case TVec(n, _): n;
						case TFloat: 1;
						default: throw "assert " + v.type;
					}

					
					var location = vl.attribCount++;
					var layout_attr = vl.attrib(location);
					trace ('getting name for ${v.id}');
					var name =  transcoder.varNames.get(v.id);
					if (name == null) name = v.name;
					trace ('Laying out ${v.name} with ${size} floats at ${offset}');

					switch(name) {
						case "position":layout_attr.mSemantic = SEMANTIC_POSITION;
						case "normal": layout_attr.mSemantic = SEMANTIC_NORMAL;
						case "uv": layout_attr.mSemantic = SEMANTIC_TEXCOORD0;
						case "color": layout_attr.mSemantic = SEMANTIC_COLOR;
						default: throw ('unknown vertex attribute ${name}');
					}
					switch(size) {
						case 1:layout_attr.mFormat = TinyImageFormat_R32_SFLOAT;
						case 2:layout_attr.mFormat = TinyImageFormat_R32G32_SFLOAT;
						case 3:layout_attr.mFormat = TinyImageFormat_R32G32B32_SFLOAT;
						case 4:layout_attr.mFormat = TinyImageFormat_R32G32B32A32_SFLOAT;
						default: throw ('Unsupported size ${size}');
					}
					
					layout_attr.mBinding = 0;
					layout_attr.mLocation = location;
					layout_attr.mOffset =  offset; // n elements * 4 bytes (sizeof(float))
					offset += size * 4;
					/*
					//layout and pipeline for sphere draw
					VertexLayout vertexLayout = {};
					vertexLayout.mAttribCount = 2;
					vertexLayout.mAttribs[0].mSemantic = SEMANTIC_POSITION;
					vertexLayout.mAttribs[0].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
					vertexLayout.mAttribs[0].mBinding = 0;
					vertexLayout.mAttribs[0].mLocation = 0;
					vertexLayout.mAttribs[0].mOffset = 0;
					vertexLayout.mAttribs[1].mSemantic = SEMANTIC_NORMAL;
					vertexLayout.mAttribs[1].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
					vertexLayout.mAttribs[1].mBinding = 0;
					vertexLayout.mAttribs[1].mLocation = 1;
					vertexLayout.mAttribs[1].mOffset = 3 * sizeof(float);
					*/
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

		var location = 0;

		for (v in shader.vertex.data.vars) {
			switch (v.kind) {
				case Input:
					trace('FILTER STRIDE Setting ${v.name} to stride ${p.stride * 4}');
					vl.setstrides(location++, 32);
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
		trace ('RENDER uploadShaderBuffers');
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
				if (total > 0) {					
					trace ('Pushing Vertex Constants ${total}');
					_currentCmd.pushConstants( _curShader.rootSig, _curShader.vertex.globalsIndex, tmpBuff  );
				}
				// Update buffer
//					gl.uniform4fv(s.globals, streamData(hl.Bytes.getArray(buf.globals.toData()), 0, s.shader.globalsSize * 16), 0, s.shader.globalsSize * 4);
			}
			if( buf.fragment.globals != null || buf.fragment.params != null ) {
				//compute total in floats
				var total = (buf.fragment.globals != null ? buf.fragment.globals.length : 0) + (buf.fragment.params != null ? buf.fragment.params.length : 0);
				if (_tmpConstantBuffer.length < total) _tmpConstantBuffer.resize(total);
				
				// Update buffer
				if (total > 0) {		
					trace ('RENDER Pushing Fragment Constants ${total}');
					_currentCmd.pushConstants( _curShader.rootSig, _curShader.fragment.globalsIndex, hl.Bytes.getArray(_tmpConstantBuffer) );
				}
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

	var _curTarget : h3d.mat.Texture;
	var _rightHanded = false;

	function selectMaterialBits( bits : Int ) {
		/*
		var diff = bits ^ curMatBits;
		if( curMatBits < 0 ) diff = -1;
		if( diff == 0 )
			return;

		var wireframe = bits & Pass.wireframe_mask != 0;
		#if hlsdl
		if ( wireframe ) {
			gl.polygonMode(GL.FRONT_AND_BACK, GL.LINE);
			// Force set to cull = None
			bits = (bits & ~Pass.culling_mask);
			diff |= Pass.culling_mask;
		} else {
			gl.polygonMode(GL.FRONT_AND_BACK, GL.FILL);
		}
		#else
		// Not entirely accurate wireframe, but the best possible on WebGL.
		drawMode = wireframe ? GL.LINE_STRIP : GL.TRIANGLES;
		#end

		if( diff & Pass.culling_mask != 0 ) {
			var cull = Pass.getCulling(bits);
			if( cull == 0 )
				gl.disable(GL.CULL_FACE);
			else {
				if( curMatBits < 0 || Pass.getCulling(curMatBits) == 0 )
					gl.enable(GL.CULL_FACE);
				gl.cullFace(FACES[cull]);
			}
		}
		if( diff & (Pass.blendSrc_mask | Pass.blendDst_mask | Pass.blendAlphaSrc_mask | Pass.blendAlphaDst_mask) != 0 ) {
			var csrc = Pass.getBlendSrc(bits);
			var cdst = Pass.getBlendDst(bits);
			var asrc = Pass.getBlendAlphaSrc(bits);
			var adst = Pass.getBlendAlphaDst(bits);
			if( csrc == asrc && cdst == adst ) {
				if( csrc == 0 && cdst == 1 )
					gl.disable(GL.BLEND);
				else {
					if( curMatBits < 0 || (Pass.getBlendSrc(curMatBits) == 0 && Pass.getBlendDst(curMatBits) == 1) ) gl.enable(GL.BLEND);
					gl.blendFunc(BLEND[csrc], BLEND[cdst]);
				}
			} else {
				if( curMatBits < 0 || (Pass.getBlendSrc(curMatBits) == 0 && Pass.getBlendDst(curMatBits) == 1) ) gl.enable(GL.BLEND);
				gl.blendFuncSeparate(BLEND[csrc], BLEND[cdst], BLEND[asrc], BLEND[adst]);
			}
		}
		if( diff & (Pass.blendOp_mask | Pass.blendAlphaOp_mask) != 0 ) {
			var cop = Pass.getBlendOp(bits);
			var aop = Pass.getBlendAlphaOp(bits);
			if( cop == aop ) {
				gl.blendEquation(OP[cop]);
			}
			else
				gl.blendEquationSeparate(OP[cop], OP[aop]);
		}
		if( diff & Pass.depthWrite_mask != 0 )
			gl.depthMask(Pass.getDepthWrite(bits) != 0);
		if( diff & Pass.depthTest_mask != 0 ) {
			var cmp = Pass.getDepthTest(bits);
			if( cmp == 0 )
				gl.disable(GL.DEPTH_TEST);
			else {
				if( curMatBits < 0 || Pass.getDepthTest(curMatBits) == 0 ) gl.enable(GL.DEPTH_TEST);
				gl.depthFunc(COMPARE[cmp]);
			}
		}
		curMatBits = bits;
		*/
	}
	
	function selectStencilBits( opBits : Int, maskBits : Int ) {
		/*
		var diffOp = opBits ^ curStOpBits;
		var diffMask = maskBits ^ curStMaskBits;

		if ( (diffOp | diffMask) == 0 ) return;

		if( diffOp & (Stencil.frontSTfail_mask | Stencil.frontDPfail_mask | Stencil.frontPass_mask) != 0 ) {
			gl.stencilOpSeparate(
				FACES[Type.enumIndex(Front)],
				STENCIL_OP[Stencil.getFrontSTfail(opBits)],
				STENCIL_OP[Stencil.getFrontDPfail(opBits)],
				STENCIL_OP[Stencil.getFrontPass(opBits)]);
		}

		if( diffOp & (Stencil.backSTfail_mask | Stencil.backDPfail_mask | Stencil.backPass_mask) != 0 ) {
			gl.stencilOpSeparate(
				FACES[Type.enumIndex(Back)],
				STENCIL_OP[Stencil.getBackSTfail(opBits)],
				STENCIL_OP[Stencil.getBackDPfail(opBits)],
				STENCIL_OP[Stencil.getBackPass(opBits)]);
		}

		if( (diffOp & Stencil.frontTest_mask) | (diffMask & (Stencil.reference_mask | Stencil.readMask_mask)) != 0 ) {
			gl.stencilFuncSeparate(
				FACES[Type.enumIndex(Front)],
				COMPARE[Stencil.getFrontTest(opBits)],
				Stencil.getReference(maskBits),
				Stencil.getReadMask(maskBits));
		}

		if( (diffOp & Stencil.backTest_mask) | (diffMask & (Stencil.reference_mask | Stencil.readMask_mask)) != 0 ) {
			gl.stencilFuncSeparate(
				FACES[Type.enumIndex(Back)],
				COMPARE[Stencil.getBackTest(opBits)],
				Stencil.getReference(maskBits),
				Stencil.getReadMask(maskBits));
		}

		if( diffMask & Stencil.writeMask_mask != 0 ) {
			var w = Stencil.getWriteMask(maskBits);
			gl.stencilMaskSeparate(FACES[Type.enumIndex(Front)], w);
			gl.stencilMaskSeparate(FACES[Type.enumIndex(Back)], w);
		}

		curStOpBits = opBits;
		curStMaskBits = maskBits;
		*/
	}

	function convertDepthFunc(c: Compare) : forge.Native.CompareMode {
		return switch(c) {
				case Always: CMP_ALWAYS;
				case Never: CMP_NEVER;
				case Equal: CMP_EQUAL;
				case NotEqual: CMP_NOTEQUAL;
				case Greater: CMP_GREATER;
				case GreaterEqual: CMP_GREATER;
				case Less: CMP_LESS;
				case LessEqual: CMP_LEQUAL;
		}
	}
	function buildDepthState(  d : forge.Native.DepthStateDesc, pass:h3d.mat.Pass ) {		
		d.depthTest = pass.depthTest != Always || pass.depthWrite;
		d.depthWrite = pass.depthWrite;
//		d.depthTest = false;
		d.depthFunc = convertDepthFunc(pass.depthTest);
//		d.depthFunc = CMP_GREATER;
//		trace ('RENDERER DEPTH config ${d.depthTest} ${d.depthWrite} ${d.depthFunc}');
		d.stencilTest = pass.stencil != null;
		d.stencilReadMask = pass.colorMask;
		d.stencilWriteMask = 0; // TODO
		d.stencilFrontFunc = CMP_NEVER;
		d.stencilFrontFail = STENCIL_OP_KEEP;
		d.depthFrontFail = STENCIL_OP_KEEP;
		d.stencilFrontPass = STENCIL_OP_KEEP;
		d.stencilBackFunc = CMP_NEVER;
		d.stencilBackFail = STENCIL_OP_KEEP;
		d.depthBackFail = STENCIL_OP_KEEP;
		d.stencilBackPass = STENCIL_OP_KEEP;
	}

	function buildRasterState( r : forge.Native.RasterizerStateDesc, pass:h3d.mat.Pass) {
		var bits = @:privateAccess pass.bits;
		/*
			When rendering to a render target, our output will be flipped in Y to match
			output texture coordinates. We also need to flip our culling.
			The result is inverted if we are using a right handed camera.
		*/
		
		if( (_curTarget == null) == _rightHanded ) {
			switch( pass.culling ) {
			case Back: bits = (bits & ~Pass.culling_mask) | (2 << Pass.culling_offset);
			case Front: bits = (bits & ~Pass.culling_mask) | (1 << Pass.culling_offset);
			default:
			}
		}

		r.cullMode = CULL_MODE_NONE;
		r.depthBias = 0;
		r.slopeScaledDepthBias = 0.;
		r.fillMode = FILL_MODE_SOLID;
		r.frontFace = FRONT_FACE_CCW;
		r.multiSample = false;
		r.scissor = false;
		r.depthClampEnable = false;
	}

	function convertBlendConstant(blendSrc : h3d.mat.Data.Blend) : forge.Native.BlendConstant{
		return switch(blendSrc) {
			case One: BC_ONE;
			case Zero: BC_ZERO;
			case SrcAlpha: BC_SRC_ALPHA;
			case DstAlpha: BC_DST_ALPHA;
			case SrcColor: BC_SRC_COLOR;
			case DstColor: BC_DST_COLOR;
			case OneMinusSrcAlpha:BC_ONE_MINUS_SRC_ALPHA;
			case OneMinusSrcColor:BC_ONE_MINUS_SRC_COLOR;
			case OneMinusDstAlpha:BC_ONE_MINUS_DST_ALPHA;
			case OneMinusDstColor:BC_ONE_MINUS_DST_COLOR;
			case ConstantColor:BC_BLEND_FACTOR;
			case OneMinusConstantColor:BC_ONE_MINUS_BLEND_FACTOR;
			default : throw "Unrecognized blend";
		}
	}

	function convertBlendMode(blend : h3d.mat.Data.Operation ) : forge.Native.BlendMode {
		return switch(blend) {
			case Add: return BM_ADD;
			case Sub: return BM_SUBTRACT;
			case ReverseSub: return BM_REVERSE_SUBTRACT;
			case Min: return BM_MIN;
			case Max: return BM_MAX;
		};
	}
	function buildBlendState(b : forge.Native.BlendStateDesc, pass:h3d.mat.Pass) {
		b.setMasks(0, pass.colorMask); // probably want a convienience function for this
		b.setRenderTarget( BLEND_STATE_TARGET_0, true );
		b.setSrcFactors(0, convertBlendConstant(pass.blendSrc) );
		b.setDstFactors(0, convertBlendConstant(pass.blendDst) );
		b.setSrcAlphaFactors(0, convertBlendConstant(pass.blendAlphaSrc));
		b.setDstAlphaFactors(0, convertBlendConstant(pass.blendAlphaDst));
		b.setBlendModes(0, convertBlendMode(pass.blendOp));
		b.setBlendAlphaModes(0, convertBlendMode(pass.blendAlphaOp));

		b.alphaToCoverage = false;
		b.independentBlend = false;
	}


	var _materialMap = new forge.Native.Map64Int();
	var _materialInternalMap = new haxe.ds.IntMap<CompiledMaterial>();
	var _matID = 0;
	var _currentMaterial : CompiledMaterial;

	public override function selectMaterial(pass:h3d.mat.Pass) {
		trace ('RENDER selectMaterial');

		// culling
		// stencil
		// mode
		// blending

		var stateBuilder = new StateBuilder();

		//@:privateAccess selectStencilBits(s.opBits, s.maskBits);
		buildBlendState(stateBuilder.blend(), pass);
		buildRasterState(stateBuilder.raster(), pass);
		buildDepthState(stateBuilder.depth(), pass );

		//Layout = shader x buffers

		var xx = stateBuilder.getSignature(_curShader.id);

		var cmatidx = _materialMap.get(xx);
		var cmat = _materialInternalMap.get(cmatidx);

		if (cmat == null) {
			trace('Signature  xx.h ${xx.high} | xx.l ${xx.low}');
			cmat = new CompiledMaterial();
			cmat._id = _matID++;
			cmat._hash = xx;
			cmat._shader = _curShader;
			trace('Adding material for pass ${pass.name} with id ${cmat._id} and hash ${cmat._hash}');
			_materialMap.set( xx, cmat._id );
			_materialInternalMap.set(cmat._id, cmat);

			var pdesc = new forge.Native.PipelineDesc();
			var gdesc = pdesc.graphicsPipeline();
			gdesc.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
			gdesc.pDepthState = stateBuilder.depth();
			gdesc.depthStencilFormat = @:privateAccess defaultDepth.b.r.format;
			//gdesc.pColorFormats = &rt.mFormat;
			gdesc.pVertexLayout = _curShader.layout;
			gdesc.pRootSignature = _curShader.rootSig;
			gdesc.pShaderProgram = _curShader.forgeShader;
			gdesc.pRasterizerState = stateBuilder.raster();
			gdesc.mVRFoveatedRendering = false;
			pdesc.addGraphicsRenderTarget( _sc.getRenderTarget(0 ) );
			var p = _renderer.createPipeline( pdesc );

			cmat._pipeline = p;
			cmat._state = stateBuilder;

		}

		_currentMaterial = cmat;
		trace('Binding pipeline');
		_currentCmd.bindPipeline( _currentMaterial._pipeline );

	}

	var _curBuffer:h3d.Buffer;
	var _bufferBinder = new forge.Native.BufferBinder();

	var _curMultiBuffer : Buffer.BufferOffset;

	public override function selectMultiBuffers(buffers:Buffer.BufferOffset) {
		_curMultiBuffer = buffers;
		_curBuffer = null;
		var mb = buffers.buffer.buffer;
		trace ('RENDER selectMultiBuffers with strid ${mb.stride}');
		_bufferBinder.reset();
		for (a in _curShader.attribs) {
			var bb = buffers.buffer;
			var vb = @:privateAccess mb.vbuf;
			var b = @:privateAccess vb.b;

			
			trace('FILTER prebinding buffer b ${b} i ${a.index} o ${a.offset} t ${a.type} d ${a.divisor} si ${a.size} bbvc ${bb.vertices} mbstr ${mb.stride} mbsi ${mb.size} bo ${buffers.offset} - ${_curShader.inputs.names[a.index]}' );

			//
			// 
//			_bufferBinder.add( b, buffers.buffer.buffer.stride * 4, buffers.offset * 4);
			_bufferBinder.add( b, mb.stride * 4, buffers.offset * 4);
			
			// gl.bindBuffer(GL.ARRAY_BUFFER, @:privateAccess buffers.buffer.buffer.vbuf.b);
			// gl.vertexAttribPointer(a.index, a.size, a.type, false, buffers.buffer.buffer.stride * 4, buffers.offset * 4);
			// updateDivisor(a);
			buffers = buffers.next;
		}
		trace('Binding vertex buffer');
		_currentCmd.bindVertexBuffer(_bufferBinder);


	}

	var _curIndexBuffer:IndexBuffer;
	var _firstDraw = true;
	public override function draw(ibuf:IndexBuffer, startIndex:Int, ntriangles:Int) {
		trace ('RENDER draw multi - ${_curMultiBuffer != null}');

		//if (!_firstDraw) return;
		_firstDraw = false;
		if (ibuf != _curIndexBuffer) {
			_curIndexBuffer = ibuf;
		}

		//cmdBindDescriptorSet(cmd, 0, pDescriptorSetTexture);
		//cmdBindDescriptorSet(cmd, gFrameIndex * 2 + 0, pDescriptorSetUniforms);
		//cmdBindDescriptorSet(cmd, 0, pDescriptorSetShadow[1]);
		trace('Binding index buffer');
		_currentCmd.bindIndexBuffer(ibuf.b, ibuf.is32 ? INDEX_TYPE_UINT32 : INDEX_TYPE_UINT16 , 0);
		trace('Drawing ${ntriangles} triangles');
		_currentCmd.drawIndexed( ntriangles * 3, 0, 0);

		/*
			if( ibuf.is32 )
				gl.drawElements(drawMode, ntriangles * 3, GL.UNSIGNED_INT, startIndex * 4);
			else
				gl.drawElements(drawMode, ntriangles * 3, GL.UNSIGNED_SHORT, startIndex * 2);
		 */

	}

	public override function selectBuffer(v:Buffer) {
		trace('selecting buffer ${v.id}');

		if( v == _curBuffer )
			return;
		if( _curBuffer != null && v.buffer == _curBuffer.buffer && v.buffer.flags.has(RawFormat) == _curBuffer.flags.has(RawFormat) ) {
			_curBuffer = v;
			return;
		}

		if( _curShader == null )
			throw "No shader selected";
		_curBuffer = v;

		_bufferBinder.reset();

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

				// m.stride * 4
				//pos * 4
				//_bufferBinder.add( m.b, 0, 0 );

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
				//m.stride * 4
				//_bufferBinder.add( m.b, 0, pos * 4 );

				//gl.vertexAttribPointer(a.index, a.size, a.type, false, m.stride * 4, pos * 4);
				//updateDivisor(a);
			}
		}
	}

	public override function clear(?color:h3d.Vector, ?depth:Float, ?stencil:Int) {
		// 
		_currentCmd.clear(_currentRT,@:privateAccess defaultDepth.b.r);
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
