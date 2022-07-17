package h3d.impl;

import haxe.crypto.Crc32;
import haxe.Int64;
import hl.I64;
import h3d.mat.Data.Compare;
import hxsl.GlslOut;
import sys.FileSystem;
import h3d.impl.Driver;
import h3d.mat.Pass as Pass;
import forge.Native.BlendStateTargets as BlendStateTargets;
import forge.Native.ColorMask as ColorMask;
import forge.Native.StateBuilder as StateBuilder;

private typedef DescriptorIndex = Null<Int>;
private typedef Program = forge.Forge.Program;
private typedef ForgeShader = forge.Native.Shader;


private class CompiledShader {
	public var s:ForgeShader;
	public var vertex:Bool;
	public var constantsIndex:DescriptorIndex;
	public var params:DescriptorIndex;
	public var globalsLength:Int;
	public var paramsLength:Int;
	public var textures:Array<{u:DescriptorIndex, t:hxsl.Ast.Type, mode:Int}>;
	public var texturesCubes:Array<{u:DescriptorIndex, t:hxsl.Ast.Type, mode:Int}>;
	public var buffers:Array<Int>;
	public var shader:hxsl.RuntimeShader.RuntimeShaderData;
	public var textureIndex:DescriptorIndex;
	public var textureCubeIndex:DescriptorIndex;
	public var samplerIndex:DescriptorIndex;
	public var samplerCubeIndex:DescriptorIndex;
	public var textureSlot: Int;
	public var samplerSlot: Int;
	public var md5 : String;

	public function textureCount() : Int {
		return textures != null ? textures.length : 0;
	}
	public function textureCubeCount() : Int {
		return texturesCubes != null ? texturesCubes.length : 0;
	}

//	public var textureDataBuilder:forge.Native.DescriptorDataBuilder;
//	public var samplerDataBuilder:forge.Native.DescriptorDataBuilder;

	public function new(s, vertex, shader) {
		this.s = s;
		this.vertex = vertex;
		this.shader = shader;
	}
}
typedef ComputePipeline = {pipeline: forge.Native.Pipeline, rootconstant: Int, shader: forge.Native.Shader, rootsig: forge.Native.RootSignature};
	
@:enum
abstract CompiledAttributeType(Int) from Int to Int {
	public var UNKNOWN = 0;
	public var BYTE = 1;
	public var FLOAT = 2;
	public var VECTOR = 3;
}

private class CompiledAttribute {
	public var index:Int;
	public var type:CompiledAttributeType;
	public var sizeBytes:Int;
	public var count:Int;
	public var divisor:Int;
	public var offsetBytes:Int;
	public var name:String;
	public var stride:Int;
	public var semantic:forge.Native.ShaderSemantic;

	public function new() {}
}

private class CompiledProgram {
	public var id: Int;
	public var p:Program;
	public var rootSig:forge.Native.RootSignature;
	public var forgeShader:forge.Native.Shader;
	public var vertex:CompiledShader;
	public var fragment:CompiledShader;
	public var inputs:InputNames;
	public var naturalLayout: forge.Native.VertexLayout;
	public var attribs:Array<CompiledAttribute>;
	public var hasAttribIndex:Array<Bool>;

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
	public var _textureDescriptor : forge.Native.DescriptorSet;
	public var _pass : h3d.mat.Pass;
	public var _stride : Int;
}

inline function debugTrace(s : String) {
	trace("DEBUG " + s);
}

@:access(h3d.impl.Shader)
class ForgeDriver extends h3d.impl.Driver {
	var onContextLost:Void->Void;

	var _renderer:forge.Native.Renderer;
	var _queue:forge.Native.Queue;
	var _swapCmdPools = new Array<forge.Native.CmdPool>();
	var _swapCmds = new Array<forge.Native.Cmd>();
	var _tmpCmd : forge.Native.Cmd;
	var _tmpPool : forge.Native.CmdPool;
	var _swapCompleteFences = new Array<forge.Native.Fence>();
	var _swapCompleteSemaphores = new Array<forge.Native.Semaphore>();
	var _ImageAcquiredSemaphore:forge.Native.Semaphore;

	var _width:Int;
	var _height:Int;
	var _swap_count = 3;
	var _hdr = false;
	var _maxCompressedTexturesSupport = 3; // arbitrary atm
	var _currentFrame = 0;
	var _sc:forge.Native.SwapChain;
	var _defaultDepth : h3d.mat.DepthBuffer;
	var _frameIndex = 0;
	var _frameBegun = false;
	var _currentRT:RenderTarget;
	var _currentSem:forge.Native.Semaphore;
	var _currentFence:forge.Native.Fence;
	var _currentCmd:forge.Native.Cmd;
	var _vertConstantBuffer = new Array<Float>();
	var _fragConstantBuffer = new Array<Float>();
	var _currentSwapIndex = 0;

	var _vertexTextures = new Array<h3d.mat.Texture>();
	var _fragmentTextures = new Array<h3d.mat.Texture>();
	var _fragmentTextureCubes = new Array<h3d.mat.Texture>();
	var _swapRenderTargets = new Array<RenderTarget>();

//	var defaultDepthInst : h3d.mat.DepthBuffer;

	var count = 0;

	public function new() {
		//		window = @:privateAccess dx.Window.windows[0];
		//		Driver.setErrorHandler(onDXError);

		debugTrace('CWD Driver IS ${FileSystem.absolutePath('')}');

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

		_computePool = _renderer.createCommandPool(_queue);
		_computeCmd = _renderer.createCommand(_computePool);
		_computeFence = _renderer.createFence();

		_tmpPool = _renderer.createCommandPool(_queue);
		_tmpCmd = _renderer.createCommand(_tmpPool);
		_currentCmd = null;

		_ImageAcquiredSemaphore = _renderer.createSemaphore();

		_renderer.initResourceLoaderInterface();

		createDefaultResources();

		attach();
	}
	
	var _mipGen : ComputePipeline;
	var _mipGenDescriptor : forge.Native.DescriptorSet;
	var _mipGenArray : ComputePipeline;
	var _mipGenArrayDescriptor : forge.Native.DescriptorSet;
	var _computePool : forge.Native.CmdPool;
	var _computeCmd : forge.Native.Cmd;
	var _computeFence : forge.Native.Fence;
	
	function createComputePipeline(filename : String) : ComputePipeline {
		var shader = _renderer.loadComputeShader(filename + ".comp");

		var rootDesc = new forge.Native.RootSignatureDesc();
		rootDesc.addShader(shader);
		var rootSig = _renderer.createRootSig(rootDesc);
		var rootConst = rootSig.getDescriptorIndexFromName("RootConstant");

		var pipelineSettings = new forge.Native.PipelineDesc();
		var computePipeline = pipelineSettings.computePipeline();
		computePipeline.rootSignature = rootSig;
		computePipeline.shaderProgram = shader;
		var pipeline = _renderer.createPipeline(pipelineSettings);

		return {pipeline: pipeline, rootconstant: rootConst, shader: shader, rootsig: rootSig};
	}

	var _mipGenParams = new Array<Int>();
	
	static final MAX_MIP_LEVELS = 13;
	var _mipGenDB = new forge.Native.DescriptorDataBuilder();
	function createDefaultResources() {
		_mipGen = createComputePipeline("generateMips");
		_mipGenArray = createComputePipeline("generateMipsArray");

		var setDesc = new forge.Native.DescriptorSetDesc();
		setDesc.pRootSignature = _mipGen.rootsig;
		setDesc.updateFrequency = DESCRIPTOR_UPDATE_FREQ_PER_DRAW;
		setDesc.maxSets = MAX_MIP_LEVELS; // 2^13 = 8192
		_mipGenDescriptor = _renderer.addDescriptorSet( setDesc );
		_mipGenArrayDescriptor = _renderer.addDescriptorSet( setDesc );
		
		_mipGenDB.addSlot("Source", DBM_TEXTURES); //?
		_mipGenDB.addSlot("Destination", DBM_TEXTURES);
	}

	function addDepthBuffer() {
		return true;
	}



	function attach() {
		if (_sc == null) {
			_sc = _forgeSDLWin.createSwapChain(_renderer, _queue, _width, _height, _swap_count, _hdr);
			for(i in 0..._swap_count) {
				var rt = _sc.getRenderTarget(i);
				var inBarrier = new forge.Native.ResourceBarrierBuilder();
				inBarrier.addRTBarrier(rt, RESOURCE_STATE_SHADER_RESOURCE, RESOURCE_STATE_RENDER_TARGET);
				var outBarrier = new forge.Native.ResourceBarrierBuilder();
				outBarrier.addRTBarrier(rt, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_SHADER_RESOURCE);
				var begin = new forge.Native.ResourceBarrierBuilder();
				begin.addRTBarrier(rt, RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET);
				var present = new forge.Native.ResourceBarrierBuilder();
				present.addRTBarrier(rt, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_PRESENT);
	
				_swapRenderTargets.push({rt:rt,inBarrier:inBarrier, outBarrier:outBarrier, begin:begin, present:present, captureBuffer : null });
			}
			if (_sc == null) {
				throw "Swapchain is null";
			}
		} else {
			debugTrace('Duplicate attach');
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
		debugTrace('RENDER ALLOC INDEX allocating index buffer ${count} is32 ${is32}');
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
		var x = buf.getNative();
		var idx = 0;

		var bits = i.is32 ? 2 : 1;
		debugTrace('RENDER UPDATE INDEX updating index buffer | start ${startIndice} count ${indiceCount} is32 ${i.is32} buf pos ${bufPos}');
		i.b.updateRegion(hl.Bytes.getArray(buf.getNative()), startIndice << bits, indiceCount << bits, bufPos << bits);
	}

	public override function hasFeature(f:Feature) {
//		debugTrace('Has Feature ${f}');
		// copied from DX driver
		return switch (f) {
			case StandardDerivatives: true;
			case Queries: false;
			case BottomLeftCoords:false;
			case HardwareAccelerated: true;
			case AllocDepthBuffer: true;
			case MultipleRenderTargets:false;
			case Wireframe:false;
			case SRGBTextures:true;
			case ShaderModel3:true;
			case InstancedRendering:false;
			default:
				false;
		};
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
		debugTrace('Resizing ${width} ${height}');
		debugTrace('----> SC IS ${_sc}');

		_width = width;
		_height = height;
		detach();
		attach();
	}


	override function getDefaultDepthBuffer():h3d.mat.DepthBuffer {
		debugTrace('Getting default depth buffer ${_width}  ${_height}');

		if (_defaultDepth != null)
			return _defaultDepth;

		// 0's makes for 0 allocation
		_defaultDepth = new h3d.mat.DepthBuffer(0, 0);
		@:privateAccess {
			_defaultDepth.width = _width;
			_defaultDepth.height = _height;
			_defaultDepth.b = allocDepthBuffer(_defaultDepth);
		}
		return _defaultDepth;
	}

	/*
	if( extraDepthInst == null ) @:privateAccess {
			extraDepthInst = new h3d.mat.DepthBuffer(0, 0);
			extraDepthInst.width = outputWidth;
			extraDepthInst.height = outputHeight;
			extraDepthInst.b = allocDepthBuffer(extraDepthInst);
		}
		return extraDepthInst;
	*/
	public override function allocDepthBuffer(b:h3d.mat.DepthBuffer):DepthBuffer {
		debugTrace('RENDER ALLOC Allocating depth buffer ${b.width} ${b.height}');

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
			//			debugTrace('b is ${b} format is ${b != null ? b.format : null}');
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

		forge.Native.Globals.waitForAllResourceLoads();
		return {r: rt};
	}

	//
	// Uncalled yet
	//

	public override function allocVertexes(m:ManagedBuffer):VertexBuffer {
		debugTrace('RENDER ALLOC VERTEX BUFFER llocating vertex buffer size ${m.size} stride ${m.stride}');

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

		debugTrace ('STRIDE FILTER - Allocating vertex buffer ${m.size} with stride ${m.stride} total bytecount  ${byteCount}');
		return {b: buff, stride: m.stride #if multidriver, driver: this #end};
	}

	function getTinyTextureFormat(f:h3d.mat.Data.TextureFormat) : forge.Native.TinyImageFormat{
		return switch (f) {
			case RGBA: TinyImageFormat_R8G8B8A8_UNORM;
			case SRGB: TinyImageFormat_R8G8B8A8_SRGB;
			case RGBA16F: TinyImageFormat_R16G16B16A16_SFLOAT;
			case R16F:  TinyImageFormat_R16_SFLOAT;
			case R32F:TinyImageFormat_R32_SFLOAT;
			case RGBA32F: TinyImageFormat_R32G32B32A32_SFLOAT;
			default: throw "Unsupported texture format " + f;
		}
	}

	function getPixelFormat(f : forge.Native.TinyImageFormat) : hxd.PixelFormat{
		return switch(f) {
			case TinyImageFormat_R8G8B8A8_UNORM:RGBA;
			case TinyImageFormat_R8G8B8A8_SNORM:RGBA;
			case TinyImageFormat_R8G8B8A8_UINT:RGBA;
			case TinyImageFormat_R8G8B8A8_SINT:RGBA;
			
			default:throw "Unsupported texture format " + f;
		};
		/*
		enum PixelFormat {
			ARGB;
			BGRA;
			RGBA;
			RGBA16F;
			RGBA32F;
			R8;
			R16F;
			R32F;
			RG8;
			RG16F;
			RG32F;
			RGB8;
			RGB16F;
			RGB32F;
			SRGB;
			SRGB_ALPHA;
			RGB10A2;
			RG11B10UF; // unsigned float
			R16U;
			RGB16U;
			RGBA16U;
			S3TC( v : Int );
		}
		*/
	}
	function createShadowRT(width : Int, height : Int) {
		var shadowPassRenderTargetDesc = new forge.Native.RenderTargetDesc();
		shadowPassRenderTargetDesc.arraySize = 1;
//		shadowPassRenderTargetDesc.clearValue.depth = 1.0f;
//		shadowPassRenderTargetDesc.clearValue.stencil = 0;
		shadowPassRenderTargetDesc.depth = 1;
//		shadowPassRenderTargetDesc.descriptors = DESCRIPTOR_TYPE_TEXTURE;
		shadowPassRenderTargetDesc.format = TinyImageFormat_D32_SFLOAT;
		shadowPassRenderTargetDesc.startState = RESOURCE_STATE_SHADER_RESOURCE;
		shadowPassRenderTargetDesc.height = height;
		shadowPassRenderTargetDesc.width = width;
		shadowPassRenderTargetDesc.sampleCount = SAMPLE_COUNT_1;
		shadowPassRenderTargetDesc.sampleQuality = 0;
//		shadowPassRenderTargetDesc.flags = forge.Native.TEXTURE_CREATION_FLAG_OWN_MEMORY_BIT.toValue();
//		shadowPassRenderTargetDesc.name = "Shadow Map Render Target";
		_renderer.createRenderTarget( shadowPassRenderTargetDesc );
//		addRenderTarget(pRenderer, &shadowPassRenderTargetDesc, &pRenderTargetShadowMap);
	}
	public override function allocTexture(t:h3d.mat.Texture):Texture {
		debugTrace('RENDER ALLOC Allocating ${t} texture width ${t.width} height ${t.height}');
		
		var rt = t.flags.has(Target);
		var flt = null;
		var internalFormat : forge.Native.TinyImageFormat = switch (t.format) {
			case RGBA: TinyImageFormat_R8G8B8A8_UNORM;
			case SRGB: TinyImageFormat_R8G8B8A8_SRGB;
			case RGBA16F: TinyImageFormat_R16G16B16A16_SFLOAT;
			case RGBA32F:TinyImageFormat_R32G32B32A32_SFLOAT;
			case R32F:TinyImageFormat_R32_SFLOAT;
			case R16F: debugTrace('WARNING This is very likely a render target w ${t.width} h ${t.height}'); TinyImageFormat_R16_SFLOAT;
			default: throw "Unsupported texture format " + t.format;
		};

		if (rt) {

		} else {
			var ftd = new forge.Native.TextureDesc();
			var mips = 1;
			if( t.flags.has(MipMapped) )
				mips = t.mipLevels;

			var isCube = t.flags.has(Cube);
			var isArray = t.flags.has(IsArray);

			if (isArray != false) throw "Unimplemented isArray support";

			ftd.width = t.width;
			ftd.height = t.height;
			
			if( isArray )
				ftd.arraySize = t.layerCount;
			else 
				ftd.arraySize = 1;
	//		ftd.arraySize = 1;
			ftd.depth = 1;
			ftd.mipLevels = mips;
			ftd.sampleCount = SAMPLE_COUNT_1;
			ftd.startState = RESOURCE_STATE_COMMON;
			ftd.flags = TEXTURE_CREATION_FLAG_OWN_MEMORY_BIT;

			if (t.flags.has(Cube))  {
				ftd.descriptors = forge.Native.DescriptorType.DESCRIPTOR_TYPE_TEXTURE_CUBE.toValue();
				ftd.arraySize = 6; // MUST BE 6 to describe a cube map
			}
			else 
				ftd.descriptors = forge.Native.DescriptorType.DESCRIPTOR_TYPE_TEXTURE.toValue();

			if( t.flags.has(MipMapped) ) ftd.descriptors = ftd.descriptors | forge.Native.DescriptorType.DESCRIPTOR_TYPE_RW_TEXTURE.toValue();
			ftd.format = internalFormat;
			//		trace ('Format is ${ftd.format}');
			flt = ftd.load(t.name, null);

			if (flt == null)
				throw "invalid texture";
			forge.Native.Globals.waitForAllResourceLoads();
		}

		/*
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
		*/
		var tt:Texture = {
			t: flt,
			width: t.width,
			height: t.height,
			internalFmt: internalFormat.toValue(),
			bits: -1,
			bind: 0,
			bias: 0,
			rt : null,
			#if multidriver
			, driver: this
			#end
		};
		t.flags.unset(WasCleared);
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

		 var stride:Int = v.stride;
		 debugTrace('RENDER STRIDE INDEX VERTEX BUFFER UPDATE updating vertex buffer start ${startVertex} vstride ${v.stride} sv ${startVertex} vc ${vertexCount} bufPos ${bufPos} buf len ${buf.length} floats');
		 v.b.updateRegion(hl.Bytes.getArray(buf.getNative()), startVertex * stride * 4, vertexCount * stride * 4, bufPos * 4 * 0);
	}

	public override function uploadTexturePixels(t:h3d.mat.Texture, pixels:hxd.Pixels, mipLevel:Int, layer:Int) {
		debugTrace('RENDER TEXTURE UPLOAD Uploading pixels ${pixels.width} x ${pixels.height} mip level ${mipLevel} layer/side ${layer}');
		debugTrace('----> SC IS ${_sc}');

		pixels.convert(t.format);
		pixels.setFlip(false);
		var dataLen = pixels.dataSize;

		var tt = t.t;
		var ft = tt.t;

		debugTrace('Uploading Sending bytes  ${dataLen}');
		ft.uploadLayerMip(layer, mipLevel, hl.Bytes.fromBytes(pixels.bytes), dataLen);
		debugTrace('Done Uploading');
		t.flags.set(WasCleared);
	}

	

	public override function present() {
//		debugTrace('Presenting');
		if (_frameBegun) {
			if (_sc != null) {
				_forgeSDLWin.present(_queue, _sc, _frameIndex, _swapCompleteSemaphores[_frameIndex]);
				_frameIndex = (_frameIndex + 1) % _swap_count;
			} else {
				debugTrace('Swap chain is null???');
				// throw "Swap Chain is null";
			}
		} else {}
	}


	public override function begin(frame:Int) {
		// Check for VSYNC
		debugTrace('RENDERING BEGIN CLEAR Begin');
		if (_sc.isVSync() != true) {
			_queue.waitIdle();
			_renderer.toggleVSync(_sc);
		}
		/*
			if (pSwapChain->mEnableVsync != mSettings.mVSyncEnabled)
				{
					waitQueueIdle(pGraphicsQueue);
					::toggleVSync(pRenderer, &pSwapChain);
				}

		 */

		_currentSwapIndex = _renderer.acquireNextImage(_sc, _ImageAcquiredSemaphore, null);

		
		_currentRT = _swapRenderTargets[_currentSwapIndex];
		_currentDepth = _defaultDepth;
		_currentSem = _swapCompleteSemaphores[_frameIndex];
		_currentFence = _swapCompleteFences[_frameIndex];

		_renderer.waitFence(_currentFence);

		// Reset cmd pool for this frame
		_renderer.resetCmdPool(_swapCmdPools[_frameIndex]);

		_currentCmd = _swapCmds[_frameIndex];

		_currentCmd.begin();
		_currentCmd.insertBarrier( _currentRT.begin );
//		_currentCmd.renderBarrier( _currentRT.rt );
		_frameBegun = true;
		_firstDraw = true;
		_currentPipeline = null;
		_curMultiBuffer = null;
		_curBuffer = null;
		_curShader = null;
	}

	var _shaders:Map<Int, CompiledProgram>;
	var _curShader:CompiledProgram;

	function getGLSL(transcoder:forge.GLSLTranscoder, shader:hxsl.RuntimeShader.RuntimeShaderData) {
		if (shader.code == null) {
			transcoder.version = 430;
			transcoder.glES = 4.3;
			shader.code = transcoder.run(shader.data);
			//debugTrace('generated shader code ${shader.code}');
			#if !heaps_compact_mem
			shader.data.funs = null;
			#end
		}

		return shader.code;
	}

	var _programIds = 0;
	var _bilinearClamp2DSampler : forge.Native.Sampler;

	function compileProgram(shader:hxsl.RuntimeShader):CompiledProgram {
		var vertTranscoder = new forge.GLSLTranscoder();
		var fragTranscoder = new forge.GLSLTranscoder();

		var p = new CompiledProgram();
		p.id = _programIds++;

		var vert_glsl = getGLSL(vertTranscoder, shader.vertex);
		var frag_glsl = getGLSL(fragTranscoder, shader.fragment);

		//debugTrace('vert shader ${vert_glsl}');
		//debugTrace('frag shader ${frag_glsl}');

		var vert_md5 = haxe.crypto.Md5.encode(vert_glsl);
		var frag_md5 = haxe.crypto.Md5.encode(frag_glsl);
		//debugTrace('vert md5 ${vert_md5}');
		//debugTrace('frag md5 ${frag_md5}');

		//debugTrace('shader cache exists ${FileSystem.exists('shadercache')}');
		//debugTrace('cwd ${FileSystem.absolutePath('')}');

		var vertpath = 'shadercache/shader_${vert_md5}.vert';
		var fragpath = 'shadercache/shader_${frag_md5}.frag';
		sys.io.File.saveContent(vertpath + ".glsl", vert_glsl);
		sys.io.File.saveContent(fragpath + ".glsl", frag_glsl);
		var fgShader = _renderer.createShader(vertpath + ".glsl", fragpath + ".glsl");
		p.forgeShader = fgShader;
		p.vertex = new CompiledShader(fgShader, true, shader.vertex);
		p.fragment = new CompiledShader(fgShader, false, shader.fragment);
		p.vertex.md5 = vert_md5;
		p.fragment.md5 = frag_md5;

		debugTrace('RENDER Shader texture count vert ${shader.vertex.texturesCount}');
		debugTrace('RENDER TEXTURE Shader texture count frag ${shader.fragment.texturesCount}');

		var rootDesc = new forge.Native.RootSignatureDesc();

		rootDesc.addShader(fgShader);
		
		var rootSig = _renderer.createRootSig(rootDesc);
		p.rootSig = rootSig;
		
		forge.Native.Globals.waitForAllResourceLoads();

		if (shader.fragment.texturesCount > 0 ) {
			if (_bilinearClamp2DSampler == null) {
				var samplerDesc = new forge.Native.SamplerDesc();
				samplerDesc.mMinFilter = FILTER_LINEAR;
				samplerDesc.mMagFilter = FILTER_LINEAR;
				samplerDesc.mAddressU = ADDRESS_MODE_CLAMP_TO_EDGE;
				samplerDesc.mAddressV = ADDRESS_MODE_CLAMP_TO_EDGE;
				samplerDesc.mAddressW = ADDRESS_MODE_CLAMP_TO_EDGE;	
				samplerDesc.mMipMapMode = MIPMAP_MODE_NEAREST;
				samplerDesc.mMipLodBias = 0.0;
				samplerDesc.mMaxAnisotropy = 0.0;
				

				_bilinearClamp2DSampler = _renderer.createSampler(samplerDesc);
			}
			forge.Native.Globals.waitForAllResourceLoads();

			var tt = shader.fragment.textures;
			
			debugTrace('RENDER TEXTURE name ${tt.name} index ${tt.index} instance ${tt.instance} pos ${tt.pos} type ${tt.type} next ${tt.next}');

			

			//var ds = _renderer.createDescriptorSet(rootSig, DESCRIPTOR_UPDATE_FREQ_PER_DRAW, 1, 0);
			//p.fragment.textureDS = ds;

			
			
			//var sds = _renderer.createDescriptorSet(rootSig, DESCRIPTOR_UPDATE_FREQ_PER_DRAW, 1, 0);
			//p.fragment.samplerDS = sds;

			p.fragment.textures = new Array<{u:DescriptorIndex, t:hxsl.Ast.Type, mode:Int}>();
			p.fragment.texturesCubes = new Array<{u:DescriptorIndex, t:hxsl.Ast.Type, mode:Int}>();
			for (i in 0...shader.fragment.texturesCount) {
				
				switch(tt.type) {
					case TSampler2D, TChannel(_):
						var xx = {u:p.fragment.textureIndex, t:tt.type, mode:0};
						p.fragment.textures.push(xx);

						debugTrace('RENDER TEXTURE Added 2D texture');
					case TSamplerCube:
						var xx = {u:p.fragment.textureCubeIndex, t:tt.type, mode:0};
						p.fragment.texturesCubes.push(xx);
						debugTrace('RENDER TEXTURE Added Cube texture');
					default: throw 'Not a supported texture type ${tt.type}';
				}
				
				tt = tt.next;
			}

			if (p.fragment.textureCount() > 0) {
				p.fragment.textureIndex = rootSig.getDescriptorIndexFromName( "fragmentTextures");
				p.fragment.samplerIndex = rootSig.getDescriptorIndexFromName( "fragmentTexturesSmplr");
			} else {
				p.fragment.textureIndex = -1;
				p.fragment.samplerIndex = -1;
			}
			if (p.fragment.textureCubeCount() > 0) {
				p.fragment.textureCubeIndex = rootSig.getDescriptorIndexFromName( "fragmentTexturesCube");					
				p.fragment.samplerCubeIndex = rootSig.getDescriptorIndexFromName( "fragmentTexturesCubeSmplr");
			}else {
				p.fragment.textureCubeIndex = -1;
				p.fragment.samplerCubeIndex = -1;
			}


			//rootDesc.addSampler( _bilinearClamp2DSampler, "fragmentTexturesSmplr" );
		}


		// get inputs
		p.vertex.params = rootSig.getDescriptorIndexFromName( "_vertrootconstants");
		p.vertex.constantsIndex = rootSig.getDescriptorIndexFromName( "_vertrootconstants");
		p.vertex.globalsLength = shader.vertex.globalsSize * 4; // vectors to floats
		p.vertex.paramsLength = shader.vertex.paramsSize * 4; // vectors to floats
		p.fragment.params = rootSig.getDescriptorIndexFromName( "_fragrootconstants");
		p.fragment.constantsIndex = rootSig.getDescriptorIndexFromName( "_fragrootconstants");
		p.fragment.globalsLength = shader.fragment.globalsSize * 4; // vectors to floats
		p.fragment.paramsLength = shader.fragment.paramsSize * 4; // vectors to floats
//		p.fragment.textures
/*
struct spvDescriptorSetBuffer0
{
    array<texture2d<float>, 1> fragmentTextures [[id(0)]];
    array<sampler, 1> fragmentTexturesSmplr [[id(1)]];
};
*/
		
		// fragmentTextures = 1
		//var textureDescIndex = p.fragment.textureIndex;
		// fragmentTexturesSmplr = 2
		//var textureSampIndex = p.fragment.samplerIndex;
		// spvDescriptorSetBuffer0 = -1
		debugTrace('RENDER TEXTURE tdi ${p.fragment.textureIndex} tsi ${p.fragment.samplerIndex} tcdi ${p.fragment.textureCubeIndex} tcsi ${p.fragment.samplerCubeIndex}'); 
		debugTrace('PARAMS Indices vg ${p.vertex.constantsIndex} vp ${p.vertex.params} fg ${p.fragment.constantsIndex} fp ${p.fragment.params}');

		var attribNames = [];
		p.attribs = [];
		p.hasAttribIndex = [];
		var idxCount = 0;
		var curOffsetBytes = 0;

		for (v in shader.vertex.data.vars) {
			switch (v.kind) {
				case Input:
					
				
					var name =  vertTranscoder.varNames.get(v.id);
					if (name == null) name = v.name;
					var a = new CompiledAttribute();
					a.type = switch (v.type) {
						case TVec(n, _): CompiledAttributeType.VECTOR;
						case TFloat: CompiledAttributeType.FLOAT;
						case TBytes(n): CompiledAttributeType.BYTE;
						default: throw "assert " + v.type;
					};
					a.sizeBytes = switch (v.type) {
						case TVec(n, _): n * 4;
						case TFloat: 4;
						case TBytes(n): n;
						default: throw "assert " + v.type;
					};
					a.count = switch (v.type) {
						case TVec(n, _): n;
						case TFloat: 1;
						case TBytes(n): n;
						default: throw "assert " + v.type;
					};
					a.name = name;
					a.index = idxCount++;
					a.divisor = 0;
					a.offsetBytes = curOffsetBytes;
					curOffsetBytes += a.sizeBytes;
					a.semantic = switch(name) {
						case "position": SEMANTIC_POSITION;
						case "normal":  SEMANTIC_NORMAL;
						case "uv": SEMANTIC_TEXCOORD0;
						case "color":SEMANTIC_COLOR;
						case "weights":SEMANTIC_UNDEFINED;
						case "indexes":SEMANTIC_UNDEFINED;
						case "time":SEMANTIC_UNDEFINED;
						case "life":SEMANTIC_UNDEFINED;
						case "init":SEMANTIC_UNDEFINED;
						case "delta":SEMANTIC_UNDEFINED;
						default: throw ('unknown vertex attribute ${name}');
					}
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
				default:
			}
		}

		p.inputs = InputNames.get(attribNames);
		p.naturalLayout = buildLayoutFromShader(p);

		return p;
	}

	public override function getShaderInputNames():InputNames {
		return _curShader.inputs;
	}

	public override function selectShader(shader:hxsl.RuntimeShader) {
		debugTrace('RENDER CALLSTACK selectShader ${shader.id}');

		var p = _shaders.get(shader.id);
		if (_curShader!= p) {
			_currentPipeline = null;
		}

		if (p == null) {
			p = compileProgram(shader);
			_shaders.set( shader.id, p);
		}

		_curShader = p;

		if (_curShader != null) {
			var vertTotalLength = _curShader.vertex.globalsLength + _curShader.vertex.paramsLength;
			var fragTotalLength = _curShader.fragment.globalsLength + _curShader.fragment.paramsLength;
			//_vertConstantBuffer is in units of floats
			//total length should also be in floats, but may be in vectors
			if (_vertConstantBuffer.length < vertTotalLength) _vertConstantBuffer.resize(vertTotalLength);
			if (_fragConstantBuffer.length < fragTotalLength) _fragConstantBuffer.resize(fragTotalLength);

			debugTrace('RENDER SHADER PARAMS length v ${vertTotalLength} v ${_curShader.vertex.globalsLength} f ${_curShader.vertex.paramsLength}');
			debugTrace('RENDER SHADER PARAMS length f ${fragTotalLength} v ${_curShader.fragment.globalsLength} f ${_curShader.fragment.paramsLength}');


			_vertexTextures.resize( _curShader.vertex.textureCount() );
			_fragmentTextures.resize( _curShader.fragment.textureCount() );
			_fragmentTextureCubes.resize( _curShader.fragment.textureCubeCount() );
		}
	

		return true;
	}
	
	function setCurrentShader( shader : CompiledProgram ) {
		_curShader = shader;
	}

	inline function crcInt( crc : Crc32, x : Int ) {
		crc.byte(x & 0xff);
		crc.byte((x >> 8) & 0xff);
		crc.byte((x >> 16) & 0xff);
		crc.byte((x >> 24) & 0xff);
	}
	var _textureDescriptorMap = new Map<Int, {mat:CompiledMaterial, tex: Array<h3d.mat.Texture>, texCubes: Array<h3d.mat.Texture>, ds : forge.Native.DescriptorSet}>();
	var _textureDataBuilder :  Array<forge.Native.DescriptorDataBuilder>;



	public override function uploadShaderBuffers(buf:h3d.shader.Buffers, which:h3d.shader.Buffers.BufferKind) {
		debugTrace('RENDER CALLSTACK uploadShaderBuffers ${which}');

		switch (which) {
			case Globals:// trace ('Ignoring globals'); // do nothing as it was all done by the globals
				if( _curShader.vertex.globalsLength > 0) {
					if (buf.vertex.globals == null) throw "Vertex Globals are expected on this shader";
					if (_curShader.vertex.globalsLength > buf.vertex.globals.length) throw 'vertex Globals mismatch ${_curShader.vertex.globalsLength} vs ${buf.vertex.globals.length}';
					
					var tmpBuff = hl.Bytes.getArray(_vertConstantBuffer);
					tmpBuff.blit(0, hl.Bytes.getArray(buf.vertex.globals.toData()), 0,  _curShader.vertex.globalsLength * 4);
				}

				if( _curShader.fragment.globalsLength > 0) {

					if (buf.fragment.globals == null) throw "Fragment Globals are expected on this shader";
					if (_curShader.fragment.globalsLength > buf.fragment.globals.length) throw 'fragment Globals mismatch ${_curShader.fragment.globalsLength} vs ${buf.fragment.globals.length}';
					
					var tmpBuff = hl.Bytes.getArray(_fragConstantBuffer);
					tmpBuff.blit(0, hl.Bytes.getArray(buf.fragment.globals.toData()), 0,  _curShader.fragment.globalsLength * 4);
				}

				// Update buffer
//					gl.uniform4fv(s.globals, streamData(hl.Bytes.getArray(buf.globals.toData()), 0, s.shader.globalsSize * 16), 0, s.shader.globalsSize * 4);
			
			case Params:  //trace ('Upload Globals & Params'); 
			if( _curShader.vertex.paramsLength > 0) {
				if (buf.vertex.params == null) throw "Vertex Params are expected on this shader";
				if (_curShader.vertex.paramsLength > buf.vertex.params.length) throw 'Vertex Params mismatch ${_curShader.vertex.paramsLength} vs ${buf.vertex.params.length}  in ${_curShader.vertex.md5}';
				
				var tmpBuff = hl.Bytes.getArray(_vertConstantBuffer);
				var offset = _curShader.vertex.globalsLength * 4;
				tmpBuff.blit(offset, hl.Bytes.getArray(buf.vertex.params.toData()), 0,  _curShader.vertex.paramsLength * 4);
			}

			if( _curShader.fragment.paramsLength > 0) {

				if (buf.fragment.params == null) throw "Fragment Params are expected on this shader";
				if (_curShader.fragment.paramsLength > buf.fragment.params.length) throw 'fragment params mismatch ${_curShader.fragment.paramsLength} vs ${buf.vertex.params.length}';
				
				var tmpBuff = hl.Bytes.getArray(_fragConstantBuffer);
				var offset = _curShader.fragment.globalsLength * 4;
				tmpBuff.blit(offset, hl.Bytes.getArray(buf.fragment.params.toData()), 0,  _curShader.fragment.paramsLength * 4);
			}
			case Textures:  
				debugTrace ('RENDER TEXTURES PIPELINE PROVIDED v ${buf.vertex.tex.length} f ${buf.fragment.tex.length} ');
				

				if (buf.vertex.tex.length < _vertexTextures.length) throw "Not enough vertex textures";

				var bufTexCount = 0;
				var bufTexCubeCount = 0;

				for(t in buf.fragment.tex) {
					if (t.flags.has(Cube)) bufTexCubeCount++; else bufTexCount++;
				}
				if (bufTexCount < _fragmentTextures.length) throw "Not enough fragment textures";
				if (bufTexCubeCount < _fragmentTextureCubes.length) throw "Not enough fragment texture cubes";

				for (i in 0..._vertexTextures.length) {
					_vertexTextures[i] = buf.vertex.tex[i];
				}
				var source2DIdx = 0;
				for (i in 0..._fragmentTextures.length) {
					while (buf.fragment.tex[source2DIdx].flags.has(Cube)) {
						source2DIdx++;
					}
					_fragmentTextures[i] = buf.fragment.tex[source2DIdx++];
					var t = _fragmentTextures[i];
					
					t.lastFrame = _currentFrame;

					if (t.t.rt != null) {
						var rtt = t.t.rt.rt.getTexture();
						if (rtt == null) {
							throw "Render target texture is null";
						}
					} else {
						if (t.t == null) throw "Texture is null";
						if (t.t.t == null) throw "Forge texture is null";	
					}
				}
				var sourceCubeIdx = 0;
				for (i in 0..._fragmentTextureCubes.length) {
					while (!buf.fragment.tex[sourceCubeIdx].flags.has(Cube)) {
						sourceCubeIdx++;
					}
					_fragmentTextureCubes[i] = buf.fragment.tex[sourceCubeIdx++];
					var t = _fragmentTextureCubes[i];
					
					t.lastFrame = _currentFrame;

					if (t.t.rt != null) {
						var rtt = t.t.rt.rt.getTexture();
						if (rtt == null) {
							throw "Render target texture is null";
						}
					} else {
						if (t.t == null) throw "Texture is null";
						if (t.t.t == null) throw "Forge texture is null";	
					}
				}
				

			case Buffers:  
				debugTrace ('RENDER BUFFERS Upload Buffers v ${buf.vertex.buffers} f ${buf.fragment.buffers}'); 

			if( _curShader.vertex.buffers != null ) {
				throw ("Not supported");
				//debugTrace('Vertex buffers length ${ _curShader.vertex.buffers}');
			}
			if (_curShader.fragment.buffers != null) {
				throw ("Not supported");
				//debugTrace('Fragment buffers length ${ _curShader.fragment.buffers}');
			}

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
		if (_currentDepth == null) {
			d.depthTest = false;
			d.stencilTest = false;
		}
		else {
			d.depthTest = pass.depthTest != Always || pass.depthWrite;
			d.depthWrite = pass.depthWrite;
	//		d.depthTest = false;
	//		debugTrace('DEPTH depth test ${pass.depthTest} write ${pass.depthWrite}');
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
		r.fillMode = pass.wireframe ? FILL_MODE_WIREFRAME : FILL_MODE_SOLID;
		r.frontFace = FRONT_FACE_CCW;
		r.multiSample = false;
		r.scissor = false;
		r.depthClampEnable = false;
	}

	inline function convertBlendConstant(blendSrc : h3d.mat.Data.Blend) : forge.Native.BlendConstant{
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

	inline function convertBlendMode(blend : h3d.mat.Data.Operation ) : forge.Native.BlendMode {
		return switch(blend) {
			case Add: return BM_ADD;
			case Sub: return BM_SUBTRACT;
			case ReverseSub: return BM_REVERSE_SUBTRACT;
			case Min: return BM_MIN;
			case Max: return BM_MAX;
		};
	}
	inline function convertColorMask( heapsMask : Int ) : Int{
		//Need to do the remap [RC]
		return ColorMask.ALL;
	}

	function buildBlendState(b : forge.Native.BlendStateDesc, pass:h3d.mat.Pass) {
		debugTrace("RENDER CALLSTACK buildBlendState");

		debugTrace('BLENDING CM ${pass.colorMask} src ${pass.blendSrc} dest ${pass.blendDst} src alpha ${pass.blendAlphaSrc} op ${pass.blendOp}');
		b.setMasks(0, convertColorMask(pass.colorMask));
		b.setRenderTarget( BLEND_STATE_TARGET_ALL, true );
		b.setSrcFactors(0, convertBlendConstant(pass.blendSrc) );
		b.setDstFactors(0, convertBlendConstant(pass.blendDst) );
		b.setSrcAlphaFactors(0, convertBlendConstant(pass.blendAlphaSrc));
		b.setDstAlphaFactors(0, convertBlendConstant(pass.blendAlphaDst));
		b.setBlendModes(0, convertBlendMode(pass.blendOp));
		b.setBlendAlphaModes(0, convertBlendMode(pass.blendAlphaOp));
		b.independentBlend = false;

		//b.alphaToCoverage = false;
	}


	var _materialMap = new forge.Native.Map64Int();
	var _materialInternalMap = new haxe.ds.IntMap<CompiledMaterial>();
	var _matID = 0;
	var _currentPipeline : CompiledMaterial;
	var _currentState = new StateBuilder();
	var _currentPass : h3d.mat.Pass;

	function getLayoutFormat(a:CompiledAttribute) : forge.Native.TinyImageFormat
		return switch(a.type) {
			case BYTE:
				debugTrace('RENDER STRIDE attribute ${a.name} has bytes with ${a.count} length');
				switch(a.count) {
					case 1:TinyImageFormat_R8_SINT;
					case 2:TinyImageFormat_R8G8_SINT;
					case 3:TinyImageFormat_R8G8B8_SINT;
					case 4:TinyImageFormat_R8G8B8A8_SINT;
					default: throw ('Unsupported count ${a.count}');
				}
			case FLOAT: TinyImageFormat_R32_SFLOAT;
			case VECTOR:switch(a.count) {
				case 1:TinyImageFormat_R32_SFLOAT;
				case 2:TinyImageFormat_R32G32_SFLOAT;
				case 3:TinyImageFormat_R32G32B32_SFLOAT;
				case 4:TinyImageFormat_R32G32B32A32_SFLOAT;
				default: throw ('Unsupported count ${a.count}');
			}
			case UNKNOWN: throw "type unknown";
		}
	
	function buildLayoutFromShader(s:CompiledProgram) : forge.Native.VertexLayout {
		var vl = new forge.Native.VertexLayout();
		vl.attribCount = 0;


		for (a in s.attribs) {
			var location = vl.attribCount++;
			var layout_attr = vl.attrib(location);

			layout_attr.mFormat = getLayoutFormat(a);
			

			layout_attr.mBinding = 0;
			layout_attr.mLocation = a.index; 
			layout_attr.mOffset = a.offsetBytes;
			layout_attr.mSemantic = a.semantic;
			layout_attr.mSemanticNameLength = a.name.length;
			layout_attr.setSemanticName( a.name );
			vl.setstrides(location, a.stride * 4);
		}

		return vl;
	}


/*
var offset = 8;
			for( i in 0...curShader.attribs.length ) {
				var a = curShader.attribs[i];
				var pos;
				switch( curShader.inputs.names[i] ) {
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
				gl.vertexAttribPointer(a.index, a.size, a.type, false, m.stride * 4, pos * 4);
				updateDivisor(a);
			}
				
	*/
	function buildHeapsLayout(s:CompiledProgram, m:ManagedBuffer) : forge.Native.VertexLayout {
		var vl = new forge.Native.VertexLayout();
		vl.attribCount = 0;

		var offsetBytes = 8 * 4; // floats * sizeof(float)

		for (a in s.attribs) {
			var location = vl.attribCount++;
			var layout_attr = vl.attrib(location);


			layout_attr.mFormat = getLayoutFormat(a);


			var bytePos;
			switch( a.name ) {
				case "position":
					bytePos = 0;
				case "normal":
					if( m.stride < 6 ) throw "Buffer is missing NORMAL data, set it to RAW format ?" #if track_alloc + @:privateAccess v.allocPos #end;
					bytePos = 3 * 4;
				case "uv":
					if( m.stride < 8 ) throw "Buffer is missing UV data, set it to RAW format ?" #if track_alloc + @:privateAccess v.allocPos #end;
					bytePos = 6 * 4;
				case s:
					bytePos = offsetBytes;
					offsetBytes += a.sizeBytes;
					if( offsetBytes > (m.stride * 4)) throw "Buffer is missing '"+s+"' data, set it to RAW format ?" #if track_alloc + @:privateAccess v.allocPos #end;

			}

			layout_attr.mBinding = 0;
			layout_attr.mLocation = a.index; 
			layout_attr.mOffset = bytePos; // floats to bytes
			//layout_attr.mRate = VERTEX_ATTRIB_RATE_VERTEX;
			layout_attr.mSemantic = a.semantic;
			layout_attr.mSemanticNameLength = a.name.length;
			layout_attr.setSemanticName( a.name );
			vl.setstrides(location, m.stride * 4);
		}

		return vl;
	}

	function buildLayoutFromMultiBuffer(s :CompiledProgram, b : Buffer.BufferOffset ) : forge.Native.VertexLayout {
		var vl = buildLayoutFromShader(s);
		for (i in 0...vl.attribCount) {
			vl.setstrides(i, b.buffer.buffer.stride * 4);
		}

		return vl;
		/*
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
					var name =  vertTranscoder.varNames.get(v.id);
					if (name == null) name = v.name;
					trace ('STRIDE Laying out ${v.name} with ${size} floats at ${offset}');

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
					debugTrace('FILTER STRIDE Setting ${v.name} to stride ${p.stride * 4}');
					vl.setstrides(location++, p.stride * 4);
					default:
			}
		}
		*/
	}

	var _hashBulder = new forge.Native.HashBuilder();
	var _pipelineSeed = Int64.make( 0x41843714, 0x85913423);

	function bindPipeline() {
		debugTrace("RENDER CALLSTACK bindPipeline");

		if (_currentPass == null) {throw "can't build a pipeline without a pass";}
		if (_curShader == null) throw "Can't build a pipeline without a shader";

		// Unique hash code for pipeline
		_hashBulder.reset(_pipelineSeed);
		_hashBulder.addInt32( _curShader.id );
		_hashBulder.addInt32( _currentRT.rt.format.toValue() );
		_hashBulder.addInt32( _currentRT.rt.sampleCount.toValue() );
		_hashBulder.addInt32( _currentRT.rt.sampleQuality );
		var db = _currentDepth != null ? @:privateAccess _currentDepth.b.r : null;
		if (db != null) {
			_hashBulder.addInt8(1);
			_hashBulder.addInt32(db.format.toValue());
		}

		if (_curBuffer != null) {
			if (_curBuffer.flags.has(RawFormat)) {
				_hashBulder.addInt8(0);
			} else {
				_hashBulder.addInt8(2);
				_hashBulder.addInt16( _curBuffer.buffer.stride );
			}
		} else if (_curMultiBuffer != null) {
			_hashBulder.addInt8(1);
			_hashBulder.addInt16(_curMultiBuffer.buffer.buffer.stride);
		} else
			throw "no buffer specified to bind pipeline";


		var hv = _hashBulder.getValue();
		var cmatidx = _materialMap.get(hv);

		// Get pipeline combination
		var cmat = _materialInternalMap.get(cmatidx);
		debugTrace('RENDER PIPELINE Signature  xx.h ${hv.high} | xx.l ${hv.low} pipeid ${cmatidx}');
		var shaderFragTextureCount = _curShader.fragment.textureCount();//.textures == null ? 0 : _curShader.fragment.textures.length;
		var shaderFragTextureCubeCount = _curShader.fragment.textureCubeCount();//.textures == null ? 0 : _curShader.fragment.textures.length;
		debugTrace('RENDER PIPELINE texture count ${shaderFragTextureCount} vs ${_fragmentTextures.length} 2D | ${shaderFragTextureCubeCount} Cubes');

		if (cmat == null) {
			cmat = new CompiledMaterial();
			cmat._id = _matID++;
			cmat._hash = hv;
			cmat._shader = _curShader;
			cmat._pass = _currentPass;

			if (_curBuffer != null) {
				if (_curBuffer.flags.has(RawFormat)) {
					cmat._layout = _curShader.naturalLayout;
					cmat._stride = _curShader.naturalLayout.getstrides(0);
				} else {
					cmat._layout = buildHeapsLayout( _curShader, _curBuffer.buffer );
					cmat._stride = cmat._layout.getstrides(0);
				}
			} else if (_curMultiBuffer != null) {
				// there can be a stride mistmatch here
				cmat._layout = buildLayoutFromMultiBuffer( _curShader, _curMultiBuffer);
				cmat._stride = cmat._layout.getstrides(0);
			} else
				throw "no buffer specified to bind pipeline";


			debugTrace('RENDER PIPELINE  Adding material for pass ${_currentPass.name} with id ${cmat._id} and hash ${cmat._hash.high}|${cmat._hash.low}');
			_materialMap.set( hv, cmat._id );
			_materialInternalMap.set(cmat._id, cmat);

			var pdesc = new forge.Native.PipelineDesc();
			var gdesc = pdesc.graphicsPipeline();
			gdesc.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
			gdesc.pDepthState = _currentDepth != null ? _currentState.depth() : null;
			gdesc.pBlendState = _currentState.blend();
			gdesc.depthStencilFormat =_currentDepth != null ? @:privateAccess _currentDepth.b.r.format : TinyImageFormat_UNDEFINED;
			//gdesc.pColorFormats = &rt.mFormat;
			
			gdesc.pVertexLayout = cmat._layout;
			gdesc.pRootSignature = _curShader.rootSig;
			gdesc.pShaderProgram = _curShader.forgeShader;
			gdesc.pRasterizerState = _currentState.raster();
			gdesc.mVRFoveatedRendering = false;
			var rt = _currentRT;
			pdesc.setRenderTargetGlobals(rt.rt.sampleCount, rt.rt.sampleQuality);
			pdesc.addGraphicsRenderTarget( rt.rt.format );
			var p = _renderer.createPipeline( pdesc );

			cmat._pipeline = p;
		}

		if (_currentPipeline != cmat) {
			_currentPipeline = cmat;
			debugTrace("RENDER CALLSTACK PIPELINE changing binding to another existing pipeline " + _currentPass.name);
			_currentCmd.bindPipeline( _currentPipeline._pipeline );			
		}
	}

	function pushParameters() {
		debugTrace("RENDER CALLSTACK pushParameters");

//		trace ('PARAMS Pushing Vertex Constants ${_temp} floats ${total / 4} vectors: [0] = x ${x} y ${y} z ${z} w ${w}');
		if (_curShader.vertex.constantsIndex != -1) {
			debugTrace('RENDER CALLSTACK CONSTANTS pushing vertex constants ${_curShader.vertex.globalsLength + _curShader.vertex.paramsLength}');
			_currentCmd.pushConstants( _curShader.rootSig, _curShader.vertex.constantsIndex, hl.Bytes.getArray(_vertConstantBuffer)  );
		}
		if (_curShader.fragment.constantsIndex != -1) {
			debugTrace('RENDER CALLSTACK CONSTANTS pushing fragment constants ${_curShader.fragment.globalsLength + _curShader.fragment.paramsLength}');
			_currentCmd.pushConstants( _curShader.rootSig, _curShader.fragment.constantsIndex, hl.Bytes.getArray(_fragConstantBuffer) );
		}
	}


	static inline final TBDT_2D = 0x1;
	static inline final  TBDT_CUBE = 0x2;
	static inline final  TBDT_2D_AND_CUBE = TBDT_2D | TBDT_CUBE;
	static inline final TBDT_2D_IDX = 0;
	static inline final  TBDT_CUBE_IDX = 1;
	static inline final  TBDT_2D_AND_CUBE_IDX = 2;

	function bindTextures() {
		debugTrace("RENDER CALLSTACK bindTextures");
		if (_currentPipeline == null) throw "Can't bind textures on null pipeline";
		if (_curShader == null) throw "Can't bind textures on null shader";
		
//		if (s.textures == null) {
//			debugTrace("WARNING: No texture array on compiled shader, may be a missing feature, also could just have no textures");
//			return;
//		}

		var shaderFragTextureCount = _curShader.fragment.textureCount();
		var shaderFragTextureCubeCount = _curShader.fragment.textureCubeCount();


		if (shaderFragTextureCount > _fragmentTextures.length) {
			throw('Not enough textures for shader, provided  ${_fragmentTextures.length} but needs ${shaderFragTextureCount}');
		}
		if (shaderFragTextureCubeCount > _fragmentTextureCubes.length) {
			throw('Not enough textures for shader, provided  ${_fragmentTextures.length} but needs ${shaderFragTextureCount}');
		}


		if (shaderFragTextureCount != _fragmentTextures.length) {
			debugTrace('RENDER WARNING shader texture count ${shaderFragTextureCount} doesn\'t match provided texture count ${_fragmentTextures.length}');
		}
		if (shaderFragTextureCubeCount != _fragmentTextureCubes.length) {
			debugTrace('RENDER WARNING shader texture cube count ${shaderFragTextureCount} doesn\'t match provided texture cube count ${_fragmentTextures.length}');
		}
		if (shaderFragTextureCount > 0 || shaderFragTextureCubeCount > 0) {
	//		if (_curShader.vertex.textures.length == 0) return;
			var fragmentSeed = 0x3917437;

			var textureMode = 0;
			if (shaderFragTextureCount > 0) textureMode |= TBDT_2D;
			if (shaderFragTextureCubeCount > 0) textureMode |= TBDT_CUBE;
			var textureModeIdx = switch(textureMode) {
				case TBDT_2D: TBDT_2D_IDX;
				case TBDT_CUBE: TBDT_CUBE_IDX;
				case TBDT_2D_AND_CUBE: TBDT_2D_AND_CUBE_IDX;
				default:throw("Unrecognized texture mode");				
			}

			var crc = new Crc32();

			crcInt(crc, fragmentSeed);
			crcInt(crc, _currentPipeline._hash.low);
			crcInt(crc, _currentPipeline._hash.high);

			crcInt(crc, shaderFragTextureCount);
			for (i in 0...shaderFragTextureCount) {
				crcInt(crc, _fragmentTextures[i].id);
			}
			crcInt(crc, shaderFragTextureCubeCount);
			for (i in 0...shaderFragTextureCubeCount) {
				crcInt(crc, _fragmentTextureCubes[i].id);
			}
			
			var tds = _textureDescriptorMap.get(crc.get());

			if (tds == null) {
				debugTrace('RENDER Adding texture ${crc.get()}');
				var ds = _renderer.createDescriptorSet(_curShader.rootSig, DESCRIPTOR_UPDATE_FREQ_PER_DRAW, 2, 0);
				
				if (_textureDataBuilder == null ) {
					_textureDataBuilder = new Array<forge.Native.DescriptorDataBuilder>();
	
					for (i in 0...3) {
						var textureMode = switch(i) {
							case TBDT_2D_IDX: TBDT_2D;
							case TBDT_CUBE_IDX: TBDT_CUBE;
							case TBDT_2D_AND_CUBE_IDX: TBDT_2D_AND_CUBE;
							default:throw("Unrecognized texture mode");
						}
						//2D Case
						var cdb = new forge.Native.DescriptorDataBuilder();

						if (textureMode & TBDT_2D != 0) {
							cdb.addSlot("fragmentTextures", DBM_TEXTURES);
							cdb.addSlot("fragmentTexturesSmplr", DBM_SAMPLERS);
						}

						if (textureMode & TBDT_CUBE != 0) {
							cdb.addSlot("fragmentTexturesCube", DBM_TEXTURES);
							cdb.addSlot("fragmentTexturesCubeSmplr", DBM_SAMPLERS);
						}
						_textureDataBuilder.push(cdb);
					}
				} else {
					//Needs to be a better way to do this
					switch(textureMode) {
						case TBDT_2D: 
							_textureDataBuilder[TBDT_2D_IDX].clearSlotData(0);
							_textureDataBuilder[TBDT_2D_IDX].clearSlotData(1);
						case TBDT_CUBE:
							_textureDataBuilder[TBDT_CUBE_IDX].clearSlotData(0);
							_textureDataBuilder[TBDT_CUBE_IDX].clearSlotData(1);
						case TBDT_2D_AND_CUBE:
							_textureDataBuilder[TBDT_2D_AND_CUBE_IDX].clearSlotData(0);
							_textureDataBuilder[TBDT_2D_AND_CUBE_IDX].clearSlotData(1);
							_textureDataBuilder[TBDT_2D_AND_CUBE_IDX].clearSlotData(2);
							_textureDataBuilder[TBDT_2D_AND_CUBE_IDX].clearSlotData(3);
						default:throw("Unrecognized texture mode");				
					}

				}

				var tdb = _textureDataBuilder[textureModeIdx];

				for (i in 0...shaderFragTextureCount) {
					var t = _fragmentTextures[i];
					if (t.t == null) throw "Texture is null";
					var ft = (t.t.rt != null) ? t.t.rt.rt.getTexture() :  t.t.t;
					
					if (ft == null) throw "Forge texture is null";

					tdb.addSlotTexture(0,ft);
					tdb.addSlotSampler(1,_bilinearClamp2DSampler);
				}

				for (i in 0...shaderFragTextureCubeCount) {
					var t = _fragmentTextureCubes[i];
					if (t.t == null) throw "Texture is null";
					var ft = (t.t.rt != null) ? t.t.rt.rt.getTexture() :  t.t.t;
					
					if (ft == null) throw "Forge texture is null";
					var slot =  (textureMode & TBDT_2D != 0) ? 2 : 0;
					tdb.addSlotTexture(slot,ft);
					tdb.addSlotSampler(slot+1,_bilinearClamp2DSampler);
				}


				tdb.update(_renderer, 0, ds);

				tds = {mat:_currentPipeline, tex:_fragmentTextures.copy(), texCubes: _fragmentTextureCubes.copy(), ds:ds};
				_textureDescriptorMap.set(crc.get(), tds);
			} else {
				debugTrace('RENDER TEXTURE Reusing texture ${crc.get()}');
			}

			debugTrace('RENDER BINDING TEXTURE descriptor tex ${shaderFragTextureCount} texCube ${shaderFragTextureCubeCount}');
			_currentCmd.bindDescriptorSet(0, tds.ds);
		} else {
			debugTrace('RENDER No textures specified in shader');
		}
		#if false
		for (i in 0...buf.tex.length) {
			var t = buf.tex[i];
			
			s.textureDataBuilder.addSlotTexture(0, t.t.t);
			if (_bilinearClamp2DSampler == null) throw "sampler is null";
			s.samplerDataBuilder.addSlotSampler(0,_bilinearClamp2DSampler);


			var pt = s.textures[i];
			if( t == null || t.isDisposed() ) {
				throw "missing texture path unsupported";
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


				trace ('Bindnig texture ${t.id} to ${idx}');
				#if multidriver
				if( t.t.driver != this )
					throw "Invalid texture context";
				#end

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
			/*
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
			}*/

		
		}

		//debugTrace('RENDER updating texture desc');

		s.textureDataBuilder.update(_renderer);
		s.samplerDataBuilder.update(_renderer);
		s.textureDataBuilder.bind(_currentCmd);
		s.samplerDataBuilder.bind(_currentCmd);
		#end
	}

	public override function selectMaterial(pass:h3d.mat.Pass) {
		debugTrace('RENDER CALLSTACK selectMaterial ${pass.name}');

		// culling
		// stencil
		// mode
		// blending

		
		_currentState.reset();

		//@:privateAccess selectStencilBits(s.opBits, s.maskBits);
		buildBlendState(_currentState.blend(), pass);
		buildRasterState(_currentState.raster(), pass);
		buildDepthState(_currentState.depth(), pass );

		_currentPipeline = null;
		_currentPass = pass;
	}

	var _curBuffer:h3d.Buffer;
	var _bufferBinder = new forge.Native.BufferBinder();

	var _curMultiBuffer : Buffer.BufferOffset;

	public override function selectMultiBuffers(buffers:Buffer.BufferOffset) {
		debugTrace('RENDER CALLSTACK selectMultiBuffers');
		_curMultiBuffer = buffers;
		_curBuffer = null;
	}

	public function bindMultiBuffers() {
		debugTrace('RENDER CALLSTACK bindMultiBuffers ${_curShader.attribs.length}');
		if (_curMultiBuffer == null) throw "Multibuffers are null";
		var buffers = _curMultiBuffer;
		var mb = buffers.buffer.buffer;
		//trace ('RENDER selectMultiBuffers with strid ${mb.stride}');
		_bufferBinder.reset();
		var strideCountBytes = 0;
		for (a in _curShader.attribs) {
			var bb = buffers.buffer;
			var vb = @:privateAccess mb.vbuf;
			var b = @:privateAccess vb.b;
			//debugTrace('FILTER prebinding buffer b ${b} i ${a.index} o ${a.offset} t ${a.type} d ${a.divisor} si ${a.size} bbvc ${bb.vertices} mbstr ${mb.stride} mbsi ${mb.size} bo ${buffers.offset} - ${_curShader.inputs.names[a.index]}' );
			//_bufferBinder.add( b, buffers.buffer.buffer.stride * 4, buffers.offset * 4);

			strideCountBytes += a.sizeBytes;
			_bufferBinder.add( b, mb.stride * 4, buffers.offset * 4);
			
			// gl.bindBuffer(GL.ARRAY_BUFFER, @:privateAccess buffers.buffer.buffer.vbuf.b);
			// gl.vertexAttribPointer(a.index, a.size, a.type, false, buffers.buffer.buffer.stride * 4, buffers.offset * 4);
			// updateDivisor(a);
			buffers = buffers.next;
		}
		debugTrace ('RENDER STRIDE shader stride ${strideCountBytes} vs pipe ${_currentPipeline._stride} vs buffer ${mb.stride * 4}');

		if (_currentPipeline._stride  != mb.stride * 4) throw "Shader - buffer stride mistmatch";

		//debugTrace('Binding vertex buffer');
		_currentCmd.bindVertexBuffer(_bufferBinder);


	}


	var _curIndexBuffer:IndexBuffer;
	var _firstDraw = true;
	public override function draw(ibuf:IndexBuffer, startIndex:Int, ntriangles:Int) {
		debugTrace('RENDER CALLSTACK draw INDEXED tri count ${ntriangles} index count ${ntriangles * 3} start ${startIndex} is32 ${ibuf.is32}');


		bindPipeline();
		pushParameters();
		bindTextures();
		if (_curBuffer != null) bindBuffer();
		else if (_curMultiBuffer != null) bindMultiBuffers();
		else
			throw "No bound vertex data";

		//if (!_firstDraw) return;
		_firstDraw = false;
		if (ibuf != _curIndexBuffer) {
			_curIndexBuffer = ibuf;
		}

		//cmdBindDescriptorSet(cmd, 0, pDescriptorSetTexture);
		//cmdBindDescriptorSet(cmd, gFrameIndex * 2 + 0, pDescriptorSetUniforms);
		//cmdBindDescriptorSet(cmd, 0, pDescriptorSetShadow[1]);
		//debugTrace('Binding index buffer');
		_currentCmd.bindIndexBuffer(ibuf.b, ibuf.is32 ? INDEX_TYPE_UINT32 : INDEX_TYPE_UINT16 , 0);
		//debugTrace('Drawing ${ntriangles} triangles');
		_currentCmd.drawIndexed( ntriangles * 3, startIndex, 0);

		/*
			if( ibuf.is32 )
				gl.drawElements(drawMode, ntriangles * 3, GL.UNSIGNED_INT, startIndex * 4);
			else
				gl.drawElements(drawMode, ntriangles * 3, GL.UNSIGNED_SHORT, startIndex * 2);
		 */
		 debugTrace('RENDER CALLSTACK drawing complete');
	}

	public override function selectBuffer(v:Buffer) {
		_curBuffer = v;
		_curMultiBuffer = null;
		debugTrace('RENDER CALLSTACK selectBuffer raw: ${v != null ? v.flags.has(RawFormat) : false}');
	}

	public  function bindBuffer() {
		debugTrace('RENDER CALLSTACK bindBuffer');

		if (_currentPipeline == null) throw "No pipeline defined";

//		debugTrace('selecting buffer ${v.id}');

/*
		if( v == _curBuffer )
			return;
		if( _curBuffer != null && v.buffer == _curBuffer.buffer && v.buffer.flags.has(RawFormat) == _curBuffer.flags.has(RawFormat) ) {
			_curBuffer = v;
			return;
		}
*/
		var v = _curBuffer;

		_bufferBinder.reset();

		var m = @:privateAccess v.buffer;
		var vbuf = @:privateAccess v.buffer.vbuf;
		var b = @:privateAccess vbuf.b;
		//if( m.stride < _curShader.stride )
		//	throw "Buffer stride (" + m.stride + ") and shader stride (" + _curShader.stride + ") mismatch";

		//debugTrace('STRIDE SELECT BUFFER m.stride ${m.stride} vbuf ${vbuf.stride} cur shader stride ${_curShader.stride}');
		#if multidriver
		if( m.driver != this )
			throw "Invalid buffer context";
		#end
//		gl.bindBuffer(GL.ARRAY_BUFFER, m.b);


		if( v.flags.has(RawFormat) ) {
			// Use the shader binding
			for( a in _curShader.attribs ) {
				debugTrace('STRIDE selectBuffer binding ${a.index} strid ${m.stride} offset ${a.offsetBytes}');
			
				_bufferBinder.add( b, m.stride * 4, a.offsetBytes);
				//gl.vertexAttribPointer(a.index, a.size, a.type, false, m.stride * 4, pos * 4);

				// m.stride * 4
				//pos * 4
				//_bufferBinder.add( m.b, 0, 0 );

				//updateDivisor(a);
			}
			_currentCmd.bindVertexBuffer(_bufferBinder);
		} else {

			var offsetBytes = 8 * 4; // 8 floats * 4 bytes a piece
			var strideCheck = 0;
			for( i in 0..._curShader.attribs.length ) {
				var a = _curShader.attribs[i];
				var posBytes;
				switch( _curShader.inputs.names[i] ) {
				case "position":
					posBytes = 0; 
				case "normal":
					if( m.stride < 6 ) throw "Buffer is missing NORMAL data, set it to RAW format ?" #if track_alloc + @:privateAccess v.allocPos #end;
					posBytes = 3 * 4;
				case "uv":
					if( m.stride < 8 ) throw "Buffer is missing UV data, set it to RAW format ?" #if track_alloc + @:privateAccess v.allocPos #end;
					posBytes = 6 * 4;
				case s:
					debugTrace('RENDER STRIDE WARNING unrecognized buffer ${s}');
					posBytes = offsetBytes;
					offsetBytes += a.sizeBytes;
					if( offsetBytes > m.stride * 4 ) throw "Buffer is missing '"+s+"' data, set it to RAW format ?" #if track_alloc + @:privateAccess v.allocPos #end;
				}
				//m.stride * 4
				//_bufferBinder.add( m.b, 0, pos * 4 );

				//gl.vertexAttribPointer(a.index, a.size, a.type, false, m.stride * 4, pos * 4);
				//updateDivisor(a);
				debugTrace('STRIDE BUFFER ${_curShader.inputs.names[i]} type is ${a.type } size is ${a.sizeBytes} bytes vs stride ${m.stride} floats (${m.stride * 4} bytes)');
				_bufferBinder.add( b, m.stride * 4, posBytes);
			}
			if (offsetBytes != m.stride * 4) {
				debugTrace('RENDER WARNING stride byte mistmatch ${offsetBytes} vs ${m.stride * 4} attrib len ${_curShader.attribs.length}');
			}
			_currentCmd.bindVertexBuffer(_bufferBinder);
//			throw ("unsupported");

		}
	}

	public override function clear(?color:h3d.Vector, ?depth:Float, ?stencil:Int) {
		debugTrace('RENDER CALLSTACK TARGET CLEAR bind and clear ${_currentRT} and ${_currentDepth} ${color} ${depth} ${stencil}');

		if (color != null) {
			var x : h3d.Vector = color;
			debugTrace('RENDER CLEAR ${x} : ${x.r}, ${x.g}, ${x.b}, ${x.a}');
			_currentRT.rt.setClearColor( x.r, x.g, x.b, x.a);
		}

		if (depth != null && _currentDepth != null) {
			var sten = stencil != null ? stencil : 0;
			var d : Float = depth;
			var rt = @:privateAccess _currentDepth.b.r;
			if (rt != null) rt.setClearDepthNormalized( d, sten );
		}


		
		debugTrace('RENDER CLEAR BINDING');
		// 

		_currentCmd.bind(_currentRT.rt,_currentDepth != null ? @:privateAccess _currentDepth.b.r : null, LOAD_ACTION_CLEAR, LOAD_ACTION_CLEAR);
		debugTrace('RENDER CLEAR BINDING DONE');
		
	}
	
	public override function end() {
		debugTrace('RENDER CALLSTACK TARGET end');
		_currentCmd.unbindRenderTarget();
		_currentCmd.insertBarrier( _currentRT.present );
		_currentRT = null;
		_currentDepth = null;
		_currentCmd.end();
		_queue.submit(_currentCmd, _currentSem, _ImageAcquiredSemaphore, _currentFence);
		_currentCmd = null;
	}
	
	public override function uploadTextureBitmap(t:h3d.mat.Texture, bmp:hxd.BitmapData, mipLevel:Int, side:Int) {
		debugTrace('RENDER CALLSTACK uploadTextureBitmap');
		var pixels = bmp.getPixels();
		uploadTexturePixels(t, pixels, mipLevel, side);
		pixels.dispose();
	}

	public override function disposeVertexes(v:VertexBuffer) {
		debugTrace('RENDER CALLSTACK DEALLOC disposeVertexes');
		v.b.dispose();
		v.b = null;
	}

	public override function disposeIndexes(i:IndexBuffer) {
		debugTrace('RENDER CALLSTACK DEALLOC disposeIndexes');
		i.b.dispose();
		i.b = null;
	}
	public override function disposeTexture(t:h3d.mat.Texture) {
		debugTrace('RENDER CALLSTACK DEALLOC disposeTexture');

		var tt = t.t;
		if( tt == null ) return;
		if (tt.t == null) return;
		tt.t.dispose();
		t.t = null;
	}


	/*
	var tmpTextures = new Array<h3d.mat.Texture>();
	override function setRenderTarget(tex:Null<h3d.mat.Texture>, layer = 0, mipLevel = 0) {
		if( tex == null ) {
			curTexture = null;
			currentDepth = defaultDepth;
			currentTargets[0] = defaultTarget;
			currentTargetResources[0] = null;
			targetsCount = 1;
			Driver.omSetRenderTargets(1, currentTargets, currentDepth.view);
			viewport[2] = outputWidth;
			viewport[3] = outputHeight;
			viewport[5] = 1.;
			Driver.rsSetViewports(1, viewport);
			return;
		}
		tmpTextures[0] = tex;
		_setRenderTargets(tmpTextures, layer, mipLevel);
	}
	*/
	
	//var _currentDepth : DepthBuffer;
	var _currentDepth : h3d.mat.DepthBuffer;

	function setRenderTargetsInternal( textures:Array<h3d.mat.Texture>, layer : Int, mipLevel : Int) {
		debugTrace('RENDER TARGET CALLSTACK setRenderTargetsInternal');

		if (_currentCmd == null) {
			_currentCmd = _tmpCmd;
			_currentCmd.begin();
		} else {
			_currentCmd.unbindRenderTarget();
			_currentCmd.insertBarrier(_currentRT.outBarrier);	
		}


		if (textures.length > 1) throw "Multiple render targets are unsupported";

		var tex = textures[0];
		
		if( tex.t == null ) {
			tex.alloc();
		}

		var itex = tex.t;
		if (itex.rt == null) {
			debugTrace('RENDER TARGET  creating new render target on ${tex} w ${tex.width} h ${tex.height}');
			var renderTargetDesc = new forge.Native.RenderTargetDesc();
			renderTargetDesc.arraySize = 1;
	//		renderTargetDesc.clearValue.depth = 1.0f;
	//		renderTargetDesc.clearValue.stencil = 0;
			renderTargetDesc.depth = 1;
			renderTargetDesc.descriptors = DESCRIPTOR_TYPE_TEXTURE;
			renderTargetDesc.format = getTinyTextureFormat(tex.format);
			renderTargetDesc.startState = RESOURCE_STATE_SHADER_RESOURCE;
			renderTargetDesc.height = tex.width;
			renderTargetDesc.width = tex.height;
			renderTargetDesc.sampleCount = SAMPLE_COUNT_1;
			renderTargetDesc.sampleQuality = 0;
//			renderTargetDesc.nativeHandle = itex.t.
			renderTargetDesc.flags = forge.Native.TextureCreationFlags.TEXTURE_CREATION_FLAG_OWN_MEMORY_BIT.toValue();
	//		renderTargetDesc.name = "Shadow Map Render Target";

			var rt =  _renderer.createRenderTarget( renderTargetDesc );
			var inBarrier = new forge.Native.ResourceBarrierBuilder();
			inBarrier.addRTBarrier(rt, RESOURCE_STATE_SHADER_RESOURCE, RESOURCE_STATE_RENDER_TARGET );
			var outBarrier = new forge.Native.ResourceBarrierBuilder();
			outBarrier.addRTBarrier(rt, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_SHADER_RESOURCE);

			itex.rt = {rt: rt, inBarrier: inBarrier, outBarrier: outBarrier, begin:null, present:null , captureBuffer: null};
		} else {
			debugTrace('RENDER TARGET  setting render target to existing ${tex} w ${tex.width} h ${tex.height}');
		}

		_currentRT = itex.rt;

//		curTexture = textures[0];

		if( tex.depthBuffer != null && (tex.depthBuffer.width != tex.width || tex.depthBuffer.height != tex.height) )
			throw "Invalid depth buffer size : does not match render target size";

		_currentDepth = @:privateAccess (tex.depthBuffer == null ? null : tex.depthBuffer);
		debugTrace('RENDER TARGET  setting depth buffer target to existing ${_currentDepth} ${_currentDepth != null ? _currentDepth.width : null} ${_currentDepth != null ? _currentDepth.height : null}');
		_currentCmd.insertBarrier(_currentRT.inBarrier);
		
		if( !tex.flags.has(WasCleared) ) {
			tex.flags.set(WasCleared); // once we draw to, do not clear again

			debugTrace('RENDER TARGET CLEAR set render target internal cleared');
			_currentCmd.bind( _currentRT.rt, _currentDepth != null ? @:privateAccess _currentDepth.b.r : null, LOAD_ACTION_CLEAR, LOAD_ACTION_CLEAR );
		} else {
			debugTrace('RENDER TARGET CLEAR set render target internal no clear');
			_currentCmd.bind(_currentRT.rt, _currentDepth != null ? @:privateAccess _currentDepth.b.r : null, LOAD_ACTION_LOAD, LOAD_ACTION_LOAD);
		}

		if (_currentPass != null) {
			selectMaterial( _currentPass);
		}
//		_currentRT = 

/*
		for( i in 0...textures.length ) {
			var tex = textures[i];

			

			//tex.t
//			if( tex.t.rt == null )
//				throw "Can't render to texture which is not allocated with Target flag";

			// prevent garbage
			if( !tex.flags.has(WasCleared) ) {
				tex.flags.set(WasCleared);
				debugTrace('RENDER TARGET REMINDER to clear render target [RC]');
//				Driver.clearColor(rt, 0, 0, 0, 0);
			}

		}
*/

		/*
		Driver.omSetRenderTargets(textures.length, currentTargets, currentDepth == null ? null : currentDepth.view);
		targetsCount = textures.length;

		var w = tex.width >> mipLevel; if( w == 0 ) w = 1;
		var h = tex.height >> mipLevel; if( h == 0 ) h = 1;
		viewport[2] = w;
		viewport[3] = h;
		viewport[5] = 1.;
		Driver.rsSetViewports(1, viewport);
		*/

		/*
		if( hasDeviceError )
			return;
		var tex = textures[0];
		curTexture = textures[0];
		if( tex.depthBuffer != null && (tex.depthBuffer.width != tex.width || tex.depthBuffer.height != tex.height) )
			throw "Invalid depth buffer size : does not match render target size";
		currentDepth = @:privateAccess (tex.depthBuffer == null ? null : tex.depthBuffer.b);
		for( i in 0...textures.length ) {
			var tex = textures[i];
			if( tex.t == null ) {
				tex.alloc();
				if( hasDeviceError ) return;
			}
			if( tex.t.rt == null )
				throw "Can't render to texture which is not allocated with Target flag";
			var index = mipLevel * tex.layerCount + layer;
			var rt = tex.t.rt[index];
			if( rt == null ) {
				var arr = tex.flags.has(Cube) || tex.flags.has(IsArray);
				var v = new dx.Driver.RenderTargetDesc(getTextureFormat(tex), arr ? Texture2DArray : Texture2D);
				v.mipMap = mipLevel;
				v.firstSlice = layer;
				v.sliceCount = 1;
				rt = Driver.createRenderTargetView(tex.t.res, v);
				tex.t.rt[index] = rt;
			}
			tex.lastFrame = frame;
			currentTargets[i] = rt;
			currentTargetResources[i] = tex.t.view;
			unbind(tex.t.view);
			// prevent garbage
			if( !tex.flags.has(WasCleared) ) {
				tex.flags.set(WasCleared);
				Driver.clearColor(rt, 0, 0, 0, 0);
			}
		}
		Driver.omSetRenderTargets(textures.length, currentTargets, currentDepth == null ? null : currentDepth.view);
		targetsCount = textures.length;

		var w = tex.width >> mipLevel; if( w == 0 ) w = 1;
		var h = tex.height >> mipLevel; if( h == 0 ) h = 1;
		viewport[2] = w;
		viewport[3] = h;
		viewport[5] = 1.;
		Driver.rsSetViewports(1, viewport);
		*/
	}

	var _tmpTextures = new Array<h3d.mat.Texture>();
	var _targetsCount = 1;
	var _curTexture : h3d.mat.Texture;
	var _currentTargets = new Array<forge.Native.RenderTarget>();
	function setDefaultRenderTarget() {
		debugTrace('RENDER CALLSTACK TARGET CLEAR setDefaultRenderTarget');
		
		_currentCmd.unbindRenderTarget();
		_currentCmd.insertBarrier( _currentRT.outBarrier );
		_currentDepth = _defaultDepth;
		_targetsCount = 1;
		_curTexture = null;
//		_currentRT = _sc.getRenderTarget(_currentSwapIndex);
		_currentRT = _swapRenderTargets[_currentSwapIndex];
		_currentTargets[0] = _currentRT.rt;

		_currentCmd.insertBarrier( _currentRT.inBarrier );
		_currentCmd.bind(_currentRT.rt, _currentDepth != null ? @:privateAccess _currentDepth.b.r : null, LOAD_ACTION_LOAD, LOAD_ACTION_LOAD);

		if (_currentPass != null) {
			selectMaterial( _currentPass);
		}


		//currentTargetResources[0] = null;

		/*
		currentTargetResources[0] = null;
		Driver.omSetRenderTargets(1, currentTargets, currentDepth.view);
		viewport[2] = outputWidth;
		viewport[3] = outputHeight;
		viewport[5] = 1.;
		Driver.rsSetViewports(1, viewport);
		*/

//		debugTrace("RENDER REMINDER return to default RT [RC]");
	}

	public override function setRenderTarget(tex:Null<h3d.mat.Texture>, layer = 0, mipLevel = 0) {
		debugTrace('RENDER CALLSTACK TARGET setRenderTarget  ${tex} layer ${layer} mip ${mipLevel} db ${tex != null ? tex.depthBuffer : null}');

		if( tex == null ) {
			setDefaultRenderTarget();
		} else {
			_tmpTextures[0] = tex;
			setRenderTargetsInternal(_tmpTextures, layer, mipLevel);	
		}
		_currentPipeline = null;
	}

	public override function setRenderTargets(textures:Array<h3d.mat.Texture>) {
		debugTrace('RENDER CALLSTACK TARGET setRenderTargets');

		setRenderTargetsInternal(textures, 0, 0);
	}



	function captureSubRenderBuffer( rt: RenderTarget, pixels : hxd.Pixels, x : Int, y : Int ) {
		if( rt == null ) throw "Can't capture null buffer";

		if (rt.captureBuffer == null) {
			rt.captureBuffer = _renderer.createTransferBuffer( rt.rt.format,rt.rt.width, rt.rt.height, 0 );
		}

		rt.rt.capture( rt.captureBuffer, null);
//		gl.getError(); // always discard
		var buffer = @:privateAccess pixels.bytes.b;
		/*
		gl.readPixels(x, y, pixels.width, pixels.height, getChannels(curTarget.t), curTarget.t.pixelFmt, buffer);
		var error = gl.getError();
		if( error != 0 ) throw "Failed to capture pixels (error "+error+")";
		@:privateAccess pixels.innerFormat = rt.rt.format;
		*/
	}

	public override function capturePixels(tex:h3d.mat.Texture, layer:Int, mipLevel:Int, ?region:h2d.col.IBounds):hxd.Pixels {
		var pixels : hxd.Pixels;
		var x : Int, y : Int, w : Int, h : Int;
		if (region != null) {
			if (region.xMax > tex.width) region.xMax = tex.width;
			if (region.yMax > tex.height) region.yMax = tex.height;
			if (region.xMin < 0) region.xMin = 0;
			if (region.yMin < 0) region.yMin = 0;
			w = region.width;
			h = region.height;
			x = region.xMin;
			y = region.yMin;
		} else {
			w = tex.width;
			h = tex.height;
			x = 0;
			y = 0;
		}

		w >>= mipLevel;
		h >>= mipLevel;
		if( w == 0 ) w = 1;
		if( h == 0 ) h = 1;
		pixels = hxd.Pixels.alloc(w, h, tex.format);

		if (tex.t.rt == null) {
			throw "Capturing pixels from non-render target is not supported";
		} else {
			var rt = tex.t.rt;
			throw "Capturing pixels from a render target is not supported";

			//rt.rt.capture( null, )
//			rt.
			// Capture render target
		}

//		tex.t.rt.rt.capture()
		/*
		var old = curTarget;
		var oldCount = numTargets;
		var oldLayer = curTargetLayer;
		var oldMip = curTargetMip;
		
		if( oldCount > 1 ) {
			numTargets = 1;
			for( i in 1...oldCount )
				if( curTargets[i] == tex )
					gl.framebufferTexture2D(GL.FRAMEBUFFER, GL.COLOR_ATTACHMENT0+i,GL.TEXTURE_2D,null,0);
		}
		setRenderTarget(tex, layer, mipLevel);
		captureSubRenderBuffer(pixels, x, y);
		setRenderTarget(old, oldLayer, oldMip);
		if( oldCount > 1 ) {
			for( i in 1...oldCount )
				if( curTargets[i] == tex )
					gl.framebufferTexture2D(GL.FRAMEBUFFER, GL.COLOR_ATTACHMENT0+i,GL.TEXTURE_2D,tex.t.t,0);
			setDrawBuffers(oldCount);
			numTargets = oldCount;
		}
		*/

		throw "Capture pixels is not supported";

		return pixels;
	}

	public override function uploadVertexBytes(v:VertexBuffer, startVertex:Int, vertexCount:Int, buf:haxe.io.Bytes, bufPos:Int) {
		debugTrace('RENDER STRIDE UPLOAD  uploadVertexBytes start ${startVertex} vstride ${v.stride} sv ${startVertex} vc ${vertexCount} bufPos ${bufPos} buf len ${buf.length} floats');
		var stride:Int = v.stride;
		v.b.updateRegion(buf, startVertex * stride * 4, vertexCount * stride * 4, bufPos * 4 * 0);
	}

	public override function uploadIndexBytes(i:IndexBuffer, startIndice:Int, indiceCount:Int, buf:haxe.io.Bytes, bufPos:Int) {
		var bits = i.is32 ? 2 : 1;
		i.b.updateRegion(buf, startIndice << bits, indiceCount << bits, bufPos << bits);
	}
	
	public override function generateMipMaps(source:h3d.mat.Texture) {
		var mipLevels = 0;
		var mipSizeX = 1 << Std.int(Math.ceil(Math.log(source.width) / Math.log(2)));
		var mipSizeY = 1 << Std.int(Math.ceil(Math.log(source.height) / Math.log(2)));

		for (i in 0...MAX_MIP_LEVELS)
		{
			if (mipSizeX >> i < 1) { mipLevels = i; break; }
			if (mipSizeY >> i < 1) { mipLevels = i; break; }
		}
		if (mipLevels <= 1) return;

		_renderer.resetCmdPool(_computePool);

		var cmd = _computeCmd;
		
		cmd.begin();
		
		_mipGenParams.resize( 2 );
	


		for (i in 1...mipLevels) {
			_mipGenDB.clearSlotData(0);
			_mipGenDB.clearSlotData(1);
	
			_mipGenDB.addSlotTexture(0, source.t.t);
			_mipGenDB.setSlotUAVMipSlice(0, i - 1);	
			_mipGenDB.addSlotTexture(1, source.t.t);
			_mipGenDB.setSlotUAVMipSlice(1, i);
	
			_mipGenDB.update(_renderer, i - 1, _mipGenArrayDescriptor); // this is computation index not mip index
		}
		
		forge.Native.Globals.waitForAllResourceLoads();

		cmd.bindPipeline(_mipGenArray.pipeline);

		for (i in 1...mipLevels)
		{
			cmd.bindDescriptorSet( i - 1, _mipGenArrayDescriptor);// this is computation index not mip index

			mipSizeX >>= 1;
			mipSizeY >>= 1;
			_mipGenParams[0] = mipSizeX;
			_mipGenParams[1] = mipSizeY;
			
			//var mipSize = { mipSizeX, mipSizeY };

			cmd.pushConstants(_mipGen.rootsig, _mipGen.rootconstant, hl.Bytes.getArray(_mipGenParams));
			/*
			textureBarriers[0] = { pSSSR_DepthHierarchy, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_UNORDERED_ACCESS };
			cmd.resourceBarrier(cmd, 0, NULL, 1, textureBarriers, 0, NULL);
			*/
			var groupCountX  = Std.int(mipSizeX / 16);
			var groupCountY =  Std.int(mipSizeY / 16);
			if (groupCountX == 0)
				groupCountX = 1;
			if (groupCountY == 0)
				groupCountY = 1;
			cmd.dispatch(groupCountX, groupCountY, source.layerCount); // 2D texture, not a texture array?
		}

		cmd.end();

		//slowest possible route, but oh well
		_queue.submit(cmd, null, null, _computeFence);
		_renderer.waitFence(_computeFence);
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



	public override function disposeDepthBuffer(b:h3d.mat.DepthBuffer) {
		throw "Not implemented";
	}



	var _debug = false;

	public override function setDebug(d:Bool) {
		if (d)
			debugTrace('Forge Driver Debug ${d}');
		_debug = d;
	}

	public override function allocInstanceBuffer(b:h3d.impl.InstanceBuffer, bytes:haxe.io.Bytes) {
		throw "Not implemented";
	}






	public override function disposeInstanceBuffer(b:h3d.impl.InstanceBuffer) {
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
		// [RC] Copy texture
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
