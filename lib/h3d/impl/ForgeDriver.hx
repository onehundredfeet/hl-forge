package h3d.impl;

import haxe.macro.Type.DefType;
import forge.PipelineTextureSetBlock;
import forge.GLSLTranscoder;
import forge.GLSLTranscoder.EGLSLFlavour;
import haxe.crypto.Crc32;
import haxe.Int64;
// import hl.I64;
import h3d.mat.Data.Compare;
import hxsl.GlslOut;
import sys.FileSystem;
import h3d.impl.Driver;
import h3d.mat.Pass as Pass;
import forge.Native.BlendStateTargets as BlendStateTargets;
import forge.Native.ColorMask as ColorMask;
import forge.Native.StateBuilder as StateBuilder;
import forge.Native.DescriptorSlotMode;
import forge.Native.DescriptorDataBuilder;
import forge.Forge;
import forge.DynamicUniformBuffer;
import h3d.impl.GraphicsDriver;
import forge.Native.TextureCreationFlags;
import forge.*;
import forge.CompiledShader;
import haxe.ds.Vector;

typedef ComputePipeline = {pipeline:forge.Native.Pipeline, rootconstant:Int, shader:forge.Native.Shader, rootsig:forge.Native.RootSignature};

private class CompiledMaterial {
	public function new() {}

	public var _shader:CompiledProgram;
	public var _hash:Int64;
	public var _id:Int;
	public var _rawState:StateBuilder;
	public var _pipeline:forge.Native.Pipeline;
	public var _layout:forge.Native.VertexLayout;
	public var _textureDescriptor:forge.Native.DescriptorSet;
	public var _pass:h3d.mat.Pass;
	public var _stride:Int;
}

@:enum abstract ForgeBackend(Int) {
	var Metal = 0;
	var Vulkan;
}

@:access(h3d.impl.Shader)
class ForgeDriver extends h3d.impl.Driver {
	var onContextLost:Void->Void;

	var _renderer:forge.Native.Renderer;
	var _queue:forge.Native.Queue;
	var _swapCmdPools = new Array<forge.Native.CmdPool>();
	var _swapCmds = new Array<forge.Native.Cmd>();
	var _tmpCmd:forge.Native.Cmd;
	var _tmpPool:forge.Native.CmdPool;
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
	var _defaultDepth:RuntimeRenderTarget;
	var _frameIndex = 0;
	var _frameBegun = false;
	var _currentRT:RenderTargetBlock;
	var _currentSem:forge.Native.Semaphore;
	var _currentFence:forge.Native.Fence;
	var _currentCmd:forge.Native.Cmd;
	var _vertConstantBuffer = new Array<Float>();
	var _fragConstantBuffer = new Array<Float>();
	var _currentSwapIndex = 0;
	var _renderTargets = [];

	//	var _vertexTextures = [for (i in 0...TEX_TYPE_COUNT) new Array<h3d.mat.Texture>()];
	//	var _fragmentTextures = [for (i in 0...TEX_TYPE_COUNT) new Array<h3d.mat.Texture>()];
	var _vertexTextures:Vector<h3d.mat.Texture>;
	var _fragmentTextures:Vector<h3d.mat.Texture>;

	var _swapRenderTargets = new Array<RuntimeRenderTarget>();
	var _swapRenderBlocks = new Array<RenderTargetBlock>();

	var _fragmentUniformBuffers = new Array<VertexBufferExt>();

	var _backend:ForgeBackend;

	//	var defaultDepthInst : h3d.mat.DepthBuffer;
	var count = 0;

	public function new() {
		//		window = @:privateAccess dx.Window.windows[0];
		//		Driver.setErrorHandler(onDXError);

		var x = Std.string(FileSystem.absolutePath(''));
		DebugTrace.trace('CWD Driver IS ${x}');

		_backend = switch (Sys.systemName()) {
			case "Mac": ForgeBackend.Metal;
			case "Windows": ForgeBackend.Vulkan;
			default: ForgeBackend.Vulkan;
		}
		reset();
	}

	inline function renderReady() {
		return true;
		//		return _queueResize == false && _queueAttach == false;
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

		DebugTrace.trace('-------> Specified window dims ${win.width} x ${win.height}');

		_renderer = _forgeSDLWin.renderer();
		if (_renderer == null)
			throw "Could not initialize renderer";

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

		_capturePool = _renderer.createCommandPool(_queue);
		_captureCmd = _renderer.createCommand(_capturePool);

		_tmpPool = _renderer.createCommandPool(_queue);
		_tmpCmd = _renderer.createCommand(_tmpPool);
		_currentCmd = null;

		_ImageAcquiredSemaphore = _renderer.createSemaphore();

		_renderer.initResourceLoaderInterface();

		createDefaultResources();

		attach();
	}

	var _mipGen:ComputePipeline;
	var _mipGenDescriptor:forge.Native.DescriptorSet;
	var _mipGenArray:ComputePipeline;
	var _mipGenArrayDescriptor:forge.Native.DescriptorSet;
	var _computePool:forge.Native.CmdPool;
	var _computeCmd:forge.Native.Cmd;
	var _computeFence:forge.Native.Fence;
	var _capturePool:forge.Native.CmdPool;
	var _captureCmd:forge.Native.Cmd;

	function createComputePipeline(filename:String):ComputePipeline {
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

		return {
			pipeline: pipeline,
			rootconstant: rootConst,
			shader: shader,
			rootsig: rootSig
		};
	}

	var _mipGenParams = new Array<Int>();

	static final MAX_MIP_LEVELS = 13;

	var _mipGenDB = new forge.Native.DescriptorDataBuilder();

	function createDefaultResources() {
		_mipGen = createComputePipeline("generateMips");
		_mipGenArray = createComputePipeline("generateMipsArray");

		var setDesc = new forge.Native.DescriptorSetDesc();
		setDesc.pRootSignature = _mipGen.rootsig;
		setDesc.setIndex = forge.Native.DescriptorUpdateFrequency.DESCRIPTOR_UPDATE_FREQ_PER_DRAW.toValue();
		setDesc.maxSets = MAX_MIP_LEVELS; // 2^13 = 8192
		DebugTrace.trace('RENDER Creating default descriptors for mip generation');
		_mipGenDescriptor = _renderer.addDescriptorSet(setDesc);
		DebugTrace.trace('RENDER Creating default descriptors for mip array generation');
		_mipGenArrayDescriptor = _renderer.addDescriptorSet(setDesc);
		DebugTrace.trace('RENDER Default Descriptors done');

		var s = _mipGenDB.addSlot(DBM_TEXTURES); // ?
		_mipGenDB.setSlotBindName(s, "Source");
		s = _mipGenDB.addSlot(DBM_TEXTURES);
		_mipGenDB.setSlotBindName(s, "Destination");
	}

	function attach() {
		DebugTrace.trace('Attaching...');
		if (_sc == null) {
			DebugTrace.trace('Creating swap achain with ${_width} x ${_height}');
			_sc = _forgeSDLWin.createSwapChain(_renderer, _queue, _width, _height, _swap_count, _hdr);
			if (_sc == null)
				throw "Couldn\'t create swap chain";
			DebugTrace.trace('Adding swap count ${_swap_count} RT size ${_swapRenderTargets.length}');
			for (i in 0..._swap_count) {
				var rt = _sc.getRenderTarget(i);
				if (rt == null)
					throw "Render target is null";
				var inBarrier = new forge.Native.ResourceBarrierBuilder();
				inBarrier.addRTBarrier(rt, RESOURCE_STATE_SHADER_RESOURCE, RESOURCE_STATE_RENDER_TARGET);
				var outBarrier = new forge.Native.ResourceBarrierBuilder();
				outBarrier.addRTBarrier(rt, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_SHADER_RESOURCE);
				var begin = new forge.Native.ResourceBarrierBuilder();
				begin.addRTBarrier(rt, RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET);
				var present = new forge.Native.ResourceBarrierBuilder();
				present.addRTBarrier(rt, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_PRESENT);

				var rtrt = new RuntimeRenderTarget();

				rtrt.nativeRT = rt;
				rtrt.inBarrier = null;
				rtrt.outBarrier = null;
				rtrt.begin = begin;
				rtrt.present = present;
				rtrt.captureBuffer = null;
				rtrt.swapChain = _sc;

				_swapRenderTargets.push(rtrt);

				var rtb = new RenderTargetBlock();
				rtb.colorTargets.push(rtrt);
				var dfdb = getDefaultDepthBuffer();
				rtb.depthBuffer = dfdb;
				rtb.depthTarget = @:privateAccess dfdb.b.r;
				_swapRenderBlocks.push(rtb);
				_currentRT = _swapRenderBlocks[_currentSwapIndex];
			}
		} else {
			DebugTrace.trace('Duplicate attach');
		}

		DebugTrace.trace('Done attaching swap chain');

		return true;
	}

	function detach() {
		DebugTrace.trace('Detatching...');
		if (_sc != null) {
			_queue.waitIdle();
			DebugTrace.trace('Destroying swap chain...');
			_renderer.destroySwapChain(_sc);
			_sc = null;
		}

		_currentRT = null;
		//		currentDepth = null;
		if (_defaultDepth != null) {
			_renderer.destroyRenderTarget(_defaultDepth.nativeRT);
			// _defaultDepth.dispose();
			_defaultDepth = null;
		}
		_swapRenderTargets.resize(0);
		_swapRenderBlocks.resize(0);
	}

	// second function called
	public override function allocIndexes(count:Int, is32:Bool):IndexBuffer {
		DebugTrace.trace('RENDER ALLOC INDEX allocating index buffer ${count} is32 ${is32}');
		var bits = is32 ? 2 : 1;
		var desc = new forge.Native.BufferLoadDesc();
		var placeHolder = new Array<hl.UI8>();
		placeHolder.resize(count << bits);

		desc.setIndexBuffer(count << bits, hl.Bytes.getArray(placeHolder));
		desc.setUsage(true);

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
		DebugTrace.trace('RENDER UPDATE INDEX updating index buffer | start ${startIndice} count ${indiceCount} is32 ${i.is32} buf pos ${bufPos}');
		i.b.updateRegion(hl.Bytes.getArray(buf.getNative()), startIndice << bits, indiceCount << bits, bufPos << bits);
	}

	public override function hasFeature(f:Feature) {
		//		DebugTrace.trace('Has Feature ${f}');
		// copied from DX driver
		return switch (f) {
			case StandardDerivatives: true;
			case Queries: false;
			case BottomLeftCoords: false;
			case HardwareAccelerated: true;
			case AllocDepthBuffer: true;
			case MultipleRenderTargets: false;
			case Wireframe: false;
			case SRGBTextures: true;
			case ShaderModel3: true;
			case InstancedRendering: false;
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

	var _queueResize = false;

	public override function resize(width:Int, height:Int) {
		DebugTrace.trace('Resizing ${width} ${height}');
		DebugTrace.trace('----> SC IS ${_sc}');

		_width = width;
		_height = height;
		_queueResize = true;
	}

	override function getDefaultDepthBuffer():h3d.mat.DepthBuffer {
		DebugTrace.trace('Getting default depth buffer ${_width}  ${_height}');

		if (_defaultDepth != null)
			return _defaultDepth.heapsDepthBuffer;

		var tmpDB = new h3d.mat.DepthBuffer(0, 0);
		// 0's makes for 0 allocation
		@:privateAccess {
			tmpDB.width = _width;
			tmpDB.height = _height;
			_defaultDepth = allocDepthBuffer(tmpDB).r;
		}
		return _defaultDepth.heapsDepthBuffer;
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
		DebugTrace.trace('RENDER ALLOC Allocating depth buffer ${b.width} ${b.height}');

		@:privateAccess {
			if (b.b != null) {
				DebugTrace.trace('RENDER ALLOC Allocating depth buffer ${b.width} ${b.height} ALREADY ALLOCATED');
				return b.b;
			}
		}
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
		//		depthRT.mClearValue.depth = 0.0f;
		//		depthRT.mClearValue.stencil = 0;
		// depthRT.mClearValue = depthStencilClear;
		// depthRT.mSampleCount = SAMPLE_COUNT_1;
		// depthRT.mSampleQuality = 0;

		if (b.format != null) {
			//			DebugTrace.trace('b is ${b} format is ${b != null ? b.format : null}');
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
		depthRT.flags = TEXTURE_CREATION_FLAG_ON_TILE.toValue() | TEXTURE_CREATION_FLAG_VR_MULTIVIEW.toValue();

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

		var rtrt = new RuntimeRenderTarget();
		rtrt.nativeRT = rt;
		rtrt.nativeDesc = depthRT;
		rtrt.heapsDepthBuffer = b;

		@:privateAccess {
			if (b.b == null) {
				b.b = {r: rtrt};
			}
		}

		forge.Native.Globals.waitForAllResourceLoads();
		return {r: rtrt};
	}

	//
	// Uncalled yet
	//

	public override function allocVertexes(m:ManagedBuffer):VertexBuffer {
		DebugTrace.trace('RENDER ALLOC VERTEX BUFFER llocating vertex buffer size ${m.size} stride ${m.stride}');

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
		var byteCount = m.size * m.strideBytes;
		placeHolder.resize(byteCount);

		var desc = new forge.Native.BufferLoadDesc();
		desc.setUsage(true);

		if (m.flags.has(Dynamic))
			desc.setDynamic(3);

		if (m.flags.has(UniformBuffer))
			desc.setUniformBuffer(byteCount, hl.Bytes.getArray(placeHolder));
		else
			desc.setVertexBuffer(byteCount, hl.Bytes.getArray(placeHolder));

		/*
			var bits = is32 ? 2 : 1;
			var placeHolder = new Array<hl.UI8>();
			placeHolder.resize(count << bits);

			desc.setIndexBuffer(count << bits, hl.Bytes.getArray(placeHolder), true);


			return {b: buff, is32: is32};
		 */

		var buff = desc.load(null);

		DebugTrace.trace('STRIDE FILTER - Allocating vertex buffer ${m.size} with stride ${m.stride} total bytecount  ${byteCount}');
		return {
			b: buff,
			strideBytes: m.strideBytes,
			stride: m.stride,
			descriptorMap: null
			#if multidriver
			, driver: this
			#end
		};
	}

	function getTinyTextureFormat(f:h3d.mat.Data.TextureFormat):forge.Native.TinyImageFormat {
		return switch (f) {
			case RGBA: TinyImageFormat_R8G8B8A8_UNORM;
			case SRGB: TinyImageFormat_R8G8B8A8_SRGB;
			case RGBA16F: TinyImageFormat_R16G16B16A16_SFLOAT;
			case R16F: TinyImageFormat_R16_SFLOAT;
			case R32F: TinyImageFormat_R32_SFLOAT;
			case RGBA32F: TinyImageFormat_R32G32B32A32_SFLOAT;
			default: throw "Unsupported texture format " + f;
		}
	}

	function getPixelFormat(f:forge.Native.TinyImageFormat):hxd.PixelFormat {
		return switch (f) {
			case TinyImageFormat_R8G8B8A8_UNORM: RGBA;
			case TinyImageFormat_R8G8B8A8_SNORM: RGBA;
			case TinyImageFormat_R8G8B8A8_UINT: RGBA;
			case TinyImageFormat_R8G8B8A8_SINT: RGBA;

			default: throw "Unsupported texture format " + f;
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

	function createShadowRT(width:Int, height:Int) {
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
		_renderer.createRenderTarget(shadowPassRenderTargetDesc);
		//		addRenderTarget(pRenderer, &shadowPassRenderTargetDesc, &pRenderTargetShadowMap);
	}

	public override function allocTexture(t:h3d.mat.Texture):Texture {
		DebugTrace.trace('RENDER ALLOC Allocating ${t} texture width ${t.width} height ${t.height}');

		var rt = t.flags.has(Target);
		var flt = null;
		var internalFormat:forge.Native.TinyImageFormat = switch (t.format) {
			case RGBA: TinyImageFormat_R8G8B8A8_UNORM;
			case SRGB: TinyImageFormat_R8G8B8A8_SRGB;
			case RGBA16F: TinyImageFormat_R16G16B16A16_SFLOAT;
			case RGBA32F: TinyImageFormat_R32G32B32A32_SFLOAT;
			case R32F: TinyImageFormat_R32_SFLOAT;
			case R16F:
				DebugTrace.trace('RF16 WARNING This is very likely a render target w ${t.width} h ${t.height}');
				TinyImageFormat_R16_SFLOAT;
			default: throw "Unsupported texture format " + t.format;
		};

		if (rt) {} else {
			var ftd = new forge.Native.TextureDesc();
			var mips = 1;
			if (t.flags.has(MipMapped))
				mips = t.mipLevels;

			var isCube = t.flags.has(Cube);
			var isArray = t.flags.has(IsArray);

			if (isArray != false)
				throw "Unimplemented isArray support";

			ftd.width = t.width;
			ftd.height = t.height;

			if (isArray)
				ftd.arraySize = t.layerCount;
			else
				ftd.arraySize = 1;
			//		ftd.arraySize = 1;
			ftd.depth = 1;
			ftd.mipLevels = mips;
			ftd.sampleCount = SAMPLE_COUNT_1;
			ftd.startState = RESOURCE_STATE_COMMON;
			var forceFlag:Int = isCube ? 0 : TEXTURE_CREATION_FLAG_FORCE_2D.toValue();
			ftd.flags = TEXTURE_CREATION_FLAG_OWN_MEMORY_BIT.toValue() | forceFlag;

			if (t.flags.has(Cube)) {
				ftd.descriptors = forge.Native.DescriptorType.DESCRIPTOR_TYPE_TEXTURE_CUBE.toValue();
				ftd.arraySize = 6; // MUST BE 6 to describe a cube map
			} else
				ftd.descriptors = forge.Native.DescriptorType.DESCRIPTOR_TYPE_TEXTURE.toValue();

			if (t.flags.has(MipMapped))
				ftd.descriptors = ftd.descriptors | forge.Native.DescriptorType.DESCRIPTOR_TYPE_RW_TEXTURE.toValue();
			ftd.format = internalFormat;
			//		DebugTrace.trace ('Format is ${ftd.format}');
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
			rt: null,
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
		if (!renderReady())
			return;
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

		DebugTrace.trace('RENDER STRIDE INDEX VERTEX BUFFER UPDATE updating vertex buffer start ${startVertex} vstride ${v.stride} sv ${startVertex} vc ${vertexCount} bufPos ${bufPos} buf len ${buf.length} floats');
		v.b.updateRegion(hl.Bytes.getArray(buf.getNative()), startVertex * v.strideBytes, vertexCount * v.strideBytes, 0);
	}

	public override function uploadTexturePixels(t:h3d.mat.Texture, pixels:hxd.Pixels, mipLevel:Int, layer:Int) {
		DebugTrace.trace('RENDER TEXTURE UPLOAD Uploading pixels ${pixels.width} x ${pixels.height} mip level ${mipLevel} layer/side ${layer}');
		DebugTrace.trace('----> SC IS ${_sc}');

		pixels.convert(t.format);
		pixels.setFlip(false);
		var dataLen = pixels.dataSize;

		var tt = t.t;
		var ft = tt.t;

		DebugTrace.trace('Uploading Sending bytes  ${dataLen}');
		ft.uploadLayerMip(layer, mipLevel, hl.Bytes.fromBytes(pixels.bytes), dataLen);
		DebugTrace.trace('Done Uploading');
		t.flags.set(WasCleared);
	}

	public override function present() {
		if (!renderReady())
			return;
		//		debugTrace('Presenting');
		if (_frameBegun) {
			if (_sc != null) {
				_forgeSDLWin.present(_queue, _sc, _frameIndex, _swapCompleteSemaphores[_frameIndex]);
				_frameIndex = (_frameIndex + 1) % _swap_count;
			} else {
				DebugTrace.trace('Swap chain is null???');
				// throw "Swap Chain is null";
			}
		} else {}
	}

	public override function begin(frame:Int) {
		// Check for VSYNC
		DebugTrace.trace('RENDERING BEGIN CLEAR Begin');
		if (_queueResize) {
			if (_currentFence != null) {
				_renderer.waitFence(_currentFence);
				_currentFence = null;
			}

			forge.Native.Globals.waitForAllResourceLoads();

			detach();
			attach();
			_queueResize = false;
		}

		if (_sc.isVSync() != true) {
			//			_queue.waitIdle();
			// TODO [RC] Fix vsync for vulkan
			//			_renderer.toggleVSync(_sc); // This destroys the swap chain on vulkan!
		}
		/*
			if (pSwapChain->mEnableVsync != mSettings.mVSyncEnabled)
				{
					waitQueueIdle(pGraphicsQueue);
					::toggleVSync(pRenderer, &pSwapChain);
				}

		 */

		_currentSwapIndex = _renderer.acquireNextImage(_sc, _ImageAcquiredSemaphore, null);

		_currentRT = _swapRenderBlocks[_currentSwapIndex];
		//		currentDepth = _defaultDepth;
		_currentSem = _swapCompleteSemaphores[_frameIndex];
		_currentFence = _swapCompleteFences[_frameIndex];

		// wait for
		_renderer.waitFence(_currentFence);

		// Reset cmd pool for this frame
		_renderer.resetCmdPool(_swapCmdPools[_frameIndex]);

		_currentCmd = _swapCmds[_frameIndex];

		_currentCmd.begin();
		DebugTrace.trace('Inserting current RT begin ${_currentRT} ${_currentSwapIndex}'); // Crashes here
		if (_currentRT.colorTargets[0].begin != null)
			_currentCmd.insertBarrier(_currentRT.colorTargets[0].begin);
		//		_currentCmd.renderBarrier( _currentRT.rt );
		_frameBegun = true;
		_firstDraw = true;
		_currentPipeline = null;
		_curMultiBuffer = null;
		_curBuffer = null;
		_curShader = null;
		_currentFrame = frame;
	}

	var _shaders:Map<Int, CompiledProgram>;
	var _curShader:CompiledProgram;

	/*
		function getGLSL(transcoder:forge.GLSLTranscoder, shader:hxsl.RuntimeShader.RuntimeShaderData) {
			if (shader.code == null) {
				transcoder.version = 430;
				transcoder.glES = 4.3;
				shader.code = transcoder.run(shader.data);
				//DebugTrace.trace('generated shader code ${shader.code}');
				#if !heaps_compact_mem
				shader.data.funs = null;
				#end
			}

			return shader.code;
		}
	 */
	var _programIds = 0;
	var _globalSamplers:Array<forge.Native.Sampler>;

	/*
		function convertGLSLToMetalBin(glslsource : String, file_root : String, shaderType : ShaderType)
		{
			var vertmetalsrc = forge.Native.Tools.glslToMetal(glslsource, file_root + '.glsl', shaderType == FRAGMENT_SHADER);
			var metalPath = file_root + '.metal';
			var binPath = file_root + '.metal.bin';
			forge.FileDependency.overwriteIfDifferentString( file_root + '.glsl',  glslsource);
			forge.FileDependency.overwriteIfDifferentString( file_root + '.metal',  vertmetalsrc);
			if (forge.FileDependency.isTargetOutOfDate(metalPath, binPath)) {
				forge.Native.Tools.metalToBin(metalPath, binPath);
			}

			return {glsl:glslsource, metal:vertmetalsrc, metal_bin: sys.io.File.getBytes( binPath ) };						
		}
	 */
	function initShader(p:CompiledProgram, s:CompiledShader, shader:hxsl.RuntimeShader.RuntimeShaderData, rt:hxsl.RuntimeShader,
			rootsig:forge.Native.RootSignature) {
		#if reference_is_code
		var prefix = s.vertex ? "vertex" : "fragment";
		s.globals = gl.getUniformLocation(p.p, prefix + "Globals");
		s.params = gl.getUniformLocation(p.p, prefix + "Params");
		s.textures = [];
		var index = 0;
		var curT = null;
		var mode = 0;
		var name = "";
		var t = shader.textures;
		while (t != null) {
			var tt = t.type;
			var count = 1;
			switch (tt) {
				case TChannel(_):
					tt = TSampler2D;
				case TArray(t, SConst(n)):
					tt = t;
					count = n;
				default:
			}
			if (tt != curT) {
				curT = tt;
				name = switch (tt) {
					case TSampler2D:
						mode = GL.TEXTURE_2D;
						"Textures";
					case TSamplerCube:
						mode = GL.TEXTURE_CUBE_MAP;
						"TexturesCube";
					case TSampler2DArray:
						mode = GL.TEXTURE_2D_ARRAY;
						"TexturesArray";
					default: throw "Unsupported texture type " + tt;
				}
				index = 0;
			}
			for (i in 0...count) {
				var loc = gl.getUniformLocation(p.p, prefix + name + "[" + index + "]");
				/*
					This is a texture that is used in HxSL but has been optimized out by GLSL compiler.
					While some drivers will correctly report `null` here, some others (AMD) will instead
					return a uniform but still mismatch the texture slot, leading to swapped textures or
					incorrect rendering. We also don't handle texture skipping in DirectX.

					Fix is to make sure HxSL will optimize the texture out.
					Alternate fix is to improve HxSL so he does it on its own.
				 */

				if (loc == null)
					throw "Texture " + rt.spec.instances[t.instance].shader.data.name + "." + t.name + " is missing from generated shader";
				s.textures.push({u: loc, t: curT, mode: mode});
				index++;
			}
			t = t.next;
		}
		#end

		if (shader.bufferCount > 0) {
			var x = shader.buffers;
			var i = 0;
			var buffersDescIndicies:Array<DescriptorIndex> = [];
			while (x != null) {
				var descName = '_uniformBuffer${i}';
				var descIdx = rootsig.getDescriptorIndexFromName(descName);
				if (descIdx != -1) {
					// trace('buffer called  ${x.name} has descripter ${descIdx} under name ${descName}');
				} else {
					throw('buffer shader name ${x.name} not found under ${descName}');
				}
				buffersDescIndicies.push(descIdx);
				x = x.next;
				i++;
			}

			s.buffers = buffersDescIndicies;

			/*
				s.buffers = [for( i in 0...shader.bufferCount )  ];//gl.getUniformBlockIndex(p.p,(shader.vertex?"vertex_":"")+"uniform_buffer"+i)];
				var start = 0;
				if( !s.vertex ) start = rt.vertex.bufferCount;
				for( i in 0...shader.bufferCount )
					gl.uniformBlockBinding(p.p,s.buffers[i],i + start);
			 */
		}
		// trace('Done init shader');
	}

	static inline final SIZEOF_FLOAT = 4;
	static inline final FLOATS_PER_VECT = 4;

	function createGlobalDescriptors(rootSig:forge.Native.RootSignature, set:EDescriptorSetSlot, vbuf:DynamicUniformBuffer, vidx:Int,
			fbuf:DynamicUniformBuffer, fidx:Int):forge.Native.DescriptorSet {
		DebugTrace.trace('RENDER DESCRIPTORS Creating multibuffer descriptor on set ${set} with vbuf ${vbuf}[${vidx}] fbuf ${fbuf}[${fidx}]');

		var dsfset = forge.Native.DescriptorUpdateFrequency.fromValue(set);

		if (vbuf == null && fbuf == null)
			return null;
		if (vbuf != null && fbuf != null && vbuf.depth != vbuf.depth)
			throw "Must have matching depth, i think";
		var setDesc = new forge.Native.DescriptorSetDesc();
		setDesc.pRootSignature = rootSig;
		setDesc.setIndex = EDescriptorSetSlot.GLOBALS;
		var depth = vbuf != null ? vbuf.depth : fbuf.depth; // 2^13 = 8192
		setDesc.maxSets = depth;

		DebugTrace.trace('RENDER DESCRIPTORS Adding global descriptors set ');
		var ds = _renderer.addDescriptorSet(setDesc);

		DebugTrace.trace('RENDER DESCRIPTORS Filling set ');

		for (i in 0...depth) {
			var builder = new DescriptorDataBuilder();

			if (vbuf != null) {
				var s0 = builder.addSlot(DBM_UNIFORMS);
				builder.setSlotBindIndex(s0, vidx);
				builder.addSlotUniformBuffer(s0, @:privateAccess vbuf._buffers.get(i));
			}
			if (fbuf != null) {
				var s1 = builder.addSlot(DBM_UNIFORMS);
				builder.setSlotBindIndex(s1, fidx);
				builder.addSlotUniformBuffer(s1, @:privateAccess fbuf._buffers.get(i));
			}
			DebugTrace.trace('RENDER DESCRIPTORS Updating depth ${i}');

			builder.update(_renderer, i, ds);
		}

		DebugTrace.trace('RENDER DESCRIPTORS DONE multibuffer descriptor on set ${set}');

		return ds;
	}

	public function compileProgram(shader:hxsl.RuntimeShader):CompiledProgram {
		trace('Compiling program ${shader.id}');
		var flavour = switch (_backend) {
			case Metal: EGLSLFlavour.Metal;
			case Vulkan: EGLSLFlavour.Vulkan;
		}

		DebugTrace.trace('RENDER SHADER compiling shader ${shader.id} for ${EnumTools.nameByValue(EGLSLFlavour)[flavour]}');
		var transcoder = new forge.GLSLTranscoder(flavour);

		var p = new CompiledProgram();
		p.id = _programIds++;
		DebugTrace.trace('RENDER SHADER Setting up transcoder');

		transcoder.setStageCode(VERTEX, shader.vertex.data);
		transcoder.setStageCode(FRAGMENT, shader.fragment.data);
		transcoder.version = 430;
		transcoder.glES = 4.3;
		DebugTrace.trace('RENDER SHADER building');
		transcoder.build();
		DebugTrace.trace('Getting code');
		var vert_glsl = shader.vertex.code = transcoder.getGLSL(VERTEX);
		var frag_glsl = shader.fragment.code = transcoder.getGLSL(FRAGMENT);

		// DebugTrace.trace('vert shader ${vert_glsl}');
		// DebugTrace.trace('frag shader ${frag_glsl}');

		DebugTrace.trace('getting hash');
		var vert_md5 = haxe.crypto.Md5.encode(vert_glsl);
		var frag_md5 = haxe.crypto.Md5.encode(frag_glsl);
		DebugTrace.trace('RENDER MATERIAL SHADER vert md5 ${vert_md5}');
		DebugTrace.trace('RENDER MATERIAL SHADER frag md5 ${frag_md5}');

		// DebugTrace.trace('shader cache exists ${FileSystem.exists('shadercache')}');
		// DebugTrace.trace('cwd ${FileSystem.absolutePath('')}');

		var platform = switch (Sys.systemName()) {
			case "Mac": "MACOS";
			case "Windows": "vulkan";
			default: "";
		};

		var vertpath = 'shadercache/${platform}/shader_${vert_md5}.vert';
		var fragpath = 'shadercache/${platform}/shader_${frag_md5}.frag';

		forge.FileDependency.overwriteIfDifferentString(vertpath + ".glsl", vert_glsl);
		forge.FileDependency.overwriteIfDifferentString(fragpath + ".glsl", frag_glsl);
		//		sys.io.File.saveContent(vertpath + ".glsl", vert_glsl);
		//		sys.io.File.saveContent(fragpath + ".glsl", frag_glsl);
		DebugTrace.trace('Creating shader....');
		var fgShader = _renderer.createShader(vertpath + ".glsl", fragpath + ".glsl");
		p.forgeShader = fgShader;
		p.vertex = new CompiledShader(fgShader, true, shader.vertex);
		p.fragment = new CompiledShader(fgShader, false, shader.fragment);
		p.vertex.md5 = vert_md5;
		p.fragment.md5 = frag_md5;
		DebugTrace.trace('RENDER SHADER caching remapped variable names');
		p.vertex.shaderVarNames = transcoder.getStageVarNames(VERTEX);
		p.fragment.shaderVarNames = transcoder.getStageVarNames(FRAGMENT);

		DebugTrace.trace('RENDER Shader texture count vert ${shader.vertex.texturesCount}');
		DebugTrace.trace('RENDER TEXTURE Shader texture count frag ${shader.fragment.texturesCount}');

		var rootDesc = new forge.Native.RootSignatureDesc();

		if (_globalSamplers == null) {
			_globalSamplers = [];
			for (i in 0...ETextureType.TEX_TYPE_COUNT) {
				switch (i) {
					case ETextureType.TEX_TYPE_2D:
					case ETextureType.TEX_TYPE_CUBE:
					case ETextureType.TEX_TYPE_RT:
					default:
						throw 'Invalid texture type';
				}
				var samplerDesc = new forge.Native.SamplerDesc();
				samplerDesc.mMinFilter = FILTER_LINEAR;
				samplerDesc.mMagFilter = FILTER_LINEAR;
				samplerDesc.mAddressU = ADDRESS_MODE_CLAMP_TO_EDGE;
				samplerDesc.mAddressV = ADDRESS_MODE_CLAMP_TO_EDGE;
				samplerDesc.mAddressW = ADDRESS_MODE_CLAMP_TO_EDGE;
				samplerDesc.mMipMapMode = MIPMAP_MODE_NEAREST;
				samplerDesc.mMipLodBias = 0.0;
				samplerDesc.mMaxAnisotropy = 0.0;

				_globalSamplers[i] = _renderer.createSampler(samplerDesc);
			}

			forge.Native.Globals.waitForAllResourceLoads();
		}
		DebugTrace.trace('RENDER Adding fragment shader');

		rootDesc.addShader(fgShader);

		var tt = shader.fragment.textures;
		/*
			for (i in 0...shader.fragment.texturesCount) {
				DebugTrace.trace('RENDER SAMPLER ADDING SAMPER ${tt.name}');
				rootDesc.addSampler(_bilinearClamp2DSampler, 'fragmentTextures'); // This is still suspect. [RC], need to look deeper into what this is doing
				tt = tt.next;
			}
		 */

		DebugTrace.trace('RENDER Creating root signature');

		var rootSig = _renderer.createRootSig(rootDesc);
		p.rootSig = rootSig;

		forge.Native.Globals.waitForAllResourceLoads();

		#if OLD_OLD
		DebugTrace.trace('RENDER Scanning vertex textures');

		if (shader.vertex.texturesCount > 0) {
			var tt = shader.vertex.textures;

			for (i in 0...shader.vertex.texturesCount) {
				var arrayType = switch (tt.type) {
					case TChannel(_): TEX_TYPE_RT;
					case TSampler2D: TEX_TYPE_2D;
					case TSamplerCube: TEX_TYPE_CUBE;
					default: throw 'Not a supported texture type ${tt.type}';
				}
				var arr = p.vertex.getOrCreateTextureArray(arrayType);
				arr.count++;

				tt = tt.next;
			}
		}
		DebugTrace.trace('RENDER Scanning fragment textures ${shader.fragment}');

		if (shader.fragment.texturesCount > 0) {
			var tt = shader.fragment.textures;

			for (i in 0...shader.fragment.texturesCount) {
				if (tt == null)
					throw 'Null texture ${i}';
				var arrayType = switch (tt.type) {
					case TChannel(_): TEX_TYPE_RT;
					case TSampler2D: TEX_TYPE_2D;
					case TSamplerCube: TEX_TYPE_CUBE;
					default: throw 'Not a supported texture type ${tt.type}';
				}
				var arr = p.fragment.getOrCreateTextureArray(arrayType);
				arr.count++;

				tt = tt.next;
			}
		}
		#end
		// DebugTrace.trace('RENDER TEXTURE name ${tt.name} index ${tt.index} instance ${tt.instance} pos ${tt.pos} type ${tt.type} next ${tt.next}');

		// var ds = _renderer.createDescriptorSet(rootSig, DESCRIPTOR_UPDATE_FREQ_PER_DRAW, 1, 0);
		// p.fragment.textureDS = ds;

		// var sds = _renderer.createDescriptorSet(rootSig, DESCRIPTOR_UPDATE_FREQ_PER_DRAW, 1, 0);
		// p.fragment.samplerDS = sds;

		//			p.fragment.textures = new Array<{u:DescriptorIndex, t:hxsl.Ast.Type, mode:Int}>();
		//			p.fragment.texturesCubes = new Array<{u:DescriptorIndex, t:hxsl.Ast.Type, mode:Int}>();
		#if false
		for (i in 0...shader.fragment.texturesCount) {
			var arrayType = switch (tt.type) {
				case TChannel(_): TEX_TYPE_RT;
				case TSampler2D: TEX_TYPE_2D;
				case TSamplerCube: TEX_TYPE_CUBE;
				default: throw 'Not a supported texture type ${tt.type}';
			}
			var arr = p.fragment.getOrCreateTextureArray(arrayType);
			var trec = new CompiledTextureRef();
			arr.textures.push(trec);
			/*
				switch (tt.type) {
					case TChannel(size):
						
					case TSampler2D:
						var xx = {u: p.fragment.textureIndex, t: tt.type, mode: 0};
						p.fragment.textures.push(xx);

						DebugTrace.trace('RENDER TEXTURE Added 2D texture');
					case TSamplerCube:
						var xx = {u: p.fragment.textureCubeIndex, t: tt.type, mode: 0};
						p.fragment.texturesCubes.push(xx);
						DebugTrace.trace('RENDER TEXTURE Added Cube texture');
					default:
						throw 'Not a supported texture type ${tt.type}';
				}
			 */

			tt = tt.next;
		}
		#end
		/*
			for (i in 0...TEX_TYPE_COUNT) {
				var arr = p.fragment.textureArrays[i];
				if (arr != null) {
					switch (i) {
						case TEX_TYPE_RT:
							arr.textureIndex = rootSig.getDescriptorIndexFromName("fragmentChannels");
							arr.samplerIndex = rootSig.getDescriptorIndexFromName("fragmentChannelsSmplr");
							break;
						case TEX_TYPE_2D:
							arr.textureIndex = rootSig.getDescriptorIndexFromName("fragmentTextures");
							arr.samplerIndex = rootSig.getDescriptorIndexFromName("fragmentTexturesSmplr");
							break;
						case TEX_TYPE_CUBE:
							arr.textureIndex = rootSig.getDescriptorIndexFromName("fragmentTexturesCube");
							arr.samplerIndex = rootSig.getDescriptorIndexFromName("fragmentTexturesCubeSmplr");
							break;
						default:
							throw 'Not a supported texture array type ${i}';
					}

					DebugTrace.trace('RENDER FRAGMENT TEXTURE ARRAY ${i} tdi ${arr.textureIndex} tsi ${arr.samplerIndex}');
				}
			}
		 */
		/*
			if (p.fragment.textureCount() > 0) {
				p.fragment.textureIndex = rootSig.getDescriptorIndexFromName("fragmentTextures");
				p.fragment.samplerIndex = rootSig.getDescriptorIndexFromName("fragmentTexturesSmplr");
			} else {
				p.fragment.textureIndex = -1;
				p.fragment.samplerIndex = -1;
			}
			if (p.fragment.textureCubeCount() > 0) {
				p.fragment.textureCubeIndex = rootSig.getDescriptorIndexFromName("fragmentTexturesCube");
				p.fragment.samplerCubeIndex = rootSig.getDescriptorIndexFromName("fragmentTexturesCubeSmplr");
			} else {
				p.fragment.textureCubeIndex = -1;
				p.fragment.samplerCubeIndex = -1;
			}
		 */
		// }

		DebugTrace.trace('RENDER Building packed descriptors');

		// get inputs

		p.vertex.params = rootSig.getDescriptorIndexFromName(GLSLTranscoder.getVariableBufferName(VERTEX, PARAMS));
		p.vertex.constantsIndex = rootSig.getDescriptorIndexFromName(GLSLTranscoder.getVariableBufferName(VERTEX, PARAMS));
		p.vertex.globalsIndex = rootSig.getDescriptorIndexFromName(GLSLTranscoder.getVariableBufferName(VERTEX, GLOBALS));

		trace('VERTEX DESCRIPTORS globals ${p.vertex.globalsIndex} ${p.vertex.params}');

		//		p.vertex.globalsDescriptorSetIndex = rootSig.getDescriptorIndexFromName( "spvDescriptorSetBuffer0"); // doesn't seem to resolve
		p.vertex.globalsLength = shader.vertex.globalsSize * FLOATS_PER_VECT; // vectors to floats
		p.vertex.paramsLengthFloats = shader.vertex.paramsSize * FLOATS_PER_VECT; // vectors to floats
		p.vertex.paramsLengthBytes = shader.vertex.paramsSize * 16; // vectors to bytes
		if (p.vertex.globalsLength > 0) {
			p.vertex.globalsBuffer = DynamicUniformBuffer.allocate(p.vertex.globalsLength * SIZEOF_FLOAT, _swap_count);
			DebugTrace.trace('RENDER SHADER Creating vertex globals descriptor');
		} else {
			trace('RENDER No vertex globals');
		}
		if (p.vertex.paramsLengthBytes == 0) {
			trace('RENDER EMPTY No vertex params');
		}

		p.fragment.params = rootSig.getDescriptorIndexFromName(GLSLTranscoder.getVariableBufferName(FRAGMENT, PARAMS));
		p.fragment.constantsIndex = rootSig.getDescriptorIndexFromName(GLSLTranscoder.getVariableBufferName(FRAGMENT, PARAMS));
		p.fragment.globalsLength = shader.fragment.globalsSize * 4; // vectors to floats
		p.fragment.globalsIndex = rootSig.getDescriptorIndexFromName(GLSLTranscoder.getVariableBufferName(FRAGMENT, GLOBALS));
		//		p.fragment.globalsDescriptorSetIndex = rootSig.getDescriptorIndexFromName( "spvDescriptorSetBuffer0");
		trace('FRAGMENT DESCRIPTORS globals ${p.fragment.globalsIndex} ${p.fragment.params}');

		DebugTrace.trace('RENDER SHADER DESCRIPTOR p.vertex.globalsIndex ${p.vertex.globalsIndex} p.vertex.globalsDescriptorSetIndex ${p.vertex.globalsDescriptorSetIndex}');
		DebugTrace.trace('RENDER SHADER DESCRIPTOR p.fragment.globalsIndex ${p.fragment.globalsIndex} p.fragment.globalsDescriptorSetIndex ${p.fragment.globalsDescriptorSetIndex}');

		p.fragment.paramsLengthFloats = shader.fragment.paramsSize * FLOATS_PER_VECT; // vectors to floats
		p.fragment.paramsLengthBytes = shader.fragment.paramsSize * 16; // vectors to bytes

		if (p.fragment.paramsLengthBytes == 0) {
			trace('RENDER EMPTY No fragment params');
		}
		if (p.fragment.globalsLength > 0) {
			p.fragment.globalsBuffer = DynamicUniformBuffer.allocate(p.fragment.globalsLength * SIZEOF_FLOAT,
				_swap_count); // need to use the swap chain depth, but 3 is enough for now
			DebugTrace.trace('RENDER SHADER Creating fragment globals descriptor');
			//			p.fragment.globalDescriptorSet = p.fragment.globalsBuffer.createDescriptors( _renderer, rootSig, p.fragment.globalsIndex, GLOBAL_DESCRIPTOR_SET);
		} else {
			trace('RENDER No fragment globals');
		}

		p.globalDescriptorSet = createGlobalDescriptors(rootSig, EDescriptorSetSlot.GLOBALS, p.vertex.globalsBuffer, p.vertex.globalsIndex,
			p.fragment.globalsBuffer, p.fragment.globalsIndex);

		//		p.fragment.textures
		/*
			struct spvDescriptorSetBuffer0
			{
			array<texture2d<float>, 1> fragmentTextures [[id(0)]];
			array<sampler, 1> fragmentTexturesSmplr [[id(1)]];
			};
		 */

		// fragmentTextures = 1
		// var textureDescIndex = p.fragment.textureIndex;
		// fragmentTexturesSmplr = 2
		// var textureSampIndex = p.fragment.samplerIndex;
		// spvDescriptorSetBuffer0 = -1
		DebugTrace.trace('PARAMS Indices vg ${p.vertex.constantsIndex} vp ${p.vertex.params} fg ${p.fragment.constantsIndex} fp ${p.fragment.params}');

		initShader(p, p.vertex, shader.vertex, shader, rootSig);
		initShader(p, p.fragment, shader.fragment, shader, rootSig);
		var attribNames = [];
		p.attribs = [];
		p.hasAttribIndex = [];
		var idxCount = 0;
		var curOffsetBytes = 0;

		for (v in shader.vertex.data.vars) {
			switch (v.kind) {
				case Input:
					var name = transcoder.getStageVarNames(VERTEX).get(v.id);
					if (name == null)
						name = v.name;
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
					a.semantic = switch (name) {
						case "position": SEMANTIC_POSITION;
						case "normal": SEMANTIC_NORMAL;
						case "uv": SEMANTIC_TEXCOORD0;
						case "color": SEMANTIC_COLOR;
						case "weights": SEMANTIC_UNDEFINED;
						case "indexes": SEMANTIC_UNDEFINED;
						case "time": SEMANTIC_UNDEFINED;
						case "life": SEMANTIC_UNDEFINED;
						case "init": SEMANTIC_UNDEFINED;
						case "delta": SEMANTIC_UNDEFINED;
						default: throw('unknown vertex attribute ${name}');
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

		// accumulate total size
		var strideBytes = 0;
		for (a in p.attribs) {
			strideBytes += a.sizeBytes;
		}
		for (a in p.attribs) {
			a.strideBytes = strideBytes;
		}
		p.inputs = InputNames.get(attribNames);
		p.naturalLayout = buildLayoutFromShader(p);

		// trace('Done compiling shader ${shader.id}');
		return p;
	}

	var _emptyNames = @:privateAccess new InputNames([]);

	public override function getShaderInputNames():InputNames {
		if (_curShader == null)
			_emptyNames;
		return _curShader.inputs;
	}

	public override function selectShader(shader:hxsl.RuntimeShader) {
		DebugTrace.trace('RENDER MATERIAL SHADER selectShader ${shader.id}');
		//		if (!renderReady()) return true;
		var p = _shaders.get(shader.id);
		if (_curShader != p) {
			_currentPipeline = null;
		}

		if (p == null) {
			DebugTrace.trace('RENDER MATERIAL SHADER compiling program ...${shader.id}');
			p = compileProgram(shader);
			_shaders.set(shader.id, p);
			DebugTrace.trace('RENDER MATERIAL SHADER compiled program is ${p}');
		}

		_curShader = p;

		if (_curShader != null) {
			var vertTotalLength = _curShader.vertex.paramsLengthFloats; // + _curShader.vertex.globalsLength;
			var fragTotalLength = _curShader.fragment.paramsLengthFloats; // + _curShader.fragment.globalsLength;
			// _vertConstantBuffer is in units of floats
			// total length should also be in floats, but may be in vectors
			if (_vertConstantBuffer.length < vertTotalLength)
				_vertConstantBuffer.resize(vertTotalLength);
			if (_fragConstantBuffer.length < fragTotalLength)
				_fragConstantBuffer.resize(fragTotalLength);

			DebugTrace.trace('RENDER SHADER PARAMS length v ${vertTotalLength} v ${_curShader.vertex.globalsLength} f ${_curShader.vertex.paramsLengthFloats}');
			DebugTrace.trace('RENDER SHADER PARAMS length f ${fragTotalLength} v ${_curShader.fragment.globalsLength} f ${_curShader.fragment.paramsLengthFloats}');

			if (_curShader.lastFrame != _currentFrame) {
				_curShader.lastFrame = _currentFrame;
				DebugTrace.trace('RENDER RESETTING BLOCKS for ${shader.id} on current frame ${_currentFrame}');
				_curShader.resetBlocks();
			}
		}
		DebugTrace.trace('RENDER MATERIAL SHADER all good! ${_curShader}');

		return true;
	}

	function setCurrentShader(shader:CompiledProgram) {
		_curShader = shader;
	}

	inline function crcInt(crc:Crc32, x:Int) {
		crc.byte(x & 0xff);
		crc.byte((x >> 8) & 0xff);
		crc.byte((x >> 16) & 0xff);
		crc.byte((x >> 24) & 0xff);
	}

	var _textureDescriptorMap = new Map<Int, {
		mat:CompiledMaterial,
		tex:Array<h3d.mat.Texture>,
		texCubes:Array<h3d.mat.Texture>,
		ds:forge.Native.DescriptorSet
	}>();
	var _textureDataBuilder:Array<forge.Native.DescriptorDataBuilder>;

	public override function uploadShaderBuffers(buf:h3d.shader.Buffers, which:h3d.shader.Buffers.BufferKind) {
		if (!renderReady())
			return;

		DebugTrace.trace('RENDER CALLSTACK uploadShaderBuffers ${which}');

		switch (which) {
			case Globals: // trace ('Ignoring globals'); // do nothing as it was all done by the globals
				if (_curShader.vertex.globalsLength > 0) {
					if (buf.vertex.globals == null)
						throw "Vertex Globals are expected on this shader";
					if (_curShader.vertex.globalsLength > buf.vertex.globals.length)
						throw 'vertex Globals mismatch ${_curShader.vertex.globalsLength} vs ${buf.vertex.globals.length}';

					//					var tmpBuff = hl.Bytes.getArray(_vertConstantBuffer);
					//					tmpBuff.blit(0, hl.Bytes.getArray(buf.vertex.globals.toData()), 0,  _curShader.vertex.globalsLength * 4);

					if (_curShader.vertex.globalsLastUpdated != _currentFrame && _curShader.vertex.globalsBuffer != null) {
						_curShader.vertex.globalsLastUpdated = _currentFrame;
						_curShader.vertex.globalsBuffer.next();
						_curShader.vertex.globalsBuffer.push(hl.Bytes.getArray(buf.vertex.globals.toData()), _curShader.vertex.globalsLength * 4);
						_curShader.vertex.globalsBuffer.sync();
						// _curShader.vertex.globalsCrc = 0; // interesting, copilot suggested this
					}
				}

				if (_curShader.fragment.globalsLength > 0) {
					if (buf.fragment.globals == null)
						throw "Fragment Globals are expected on this shader";
					if (_curShader.fragment.globalsLength > buf.fragment.globals.length)
						throw 'fragment Globals mismatch ${_curShader.fragment.globalsLength} vs ${buf.fragment.globals.length}';

					// var tmpBuff = hl.Bytes.getArray(_fragConstantBuffer);
					// tmpBuff.blit(0, hl.Bytes.getArray(buf.fragment.globals.toData()), 0,  _curShader.fragment.globalsLength * 4);

					if (_curShader.fragment.globalsLastUpdated != _currentFrame && _curShader.fragment.globalsBuffer != null) {
						_curShader.fragment.globalsLastUpdated = _currentFrame;
						_curShader.fragment.globalsBuffer.next();
						_curShader.fragment.globalsBuffer.push(hl.Bytes.getArray(buf.fragment.globals.toData()), _curShader.fragment.globalsLength * 4);
						_curShader.fragment.globalsBuffer.sync();
						// _curShader.fragment.globalsCrc = 0; // interesting, copilot suggested this
					}
				}

			// Update buffer
			//					gl.uniform4fv(s.globals, streamData(hl.Bytes.getArray(buf.globals.toData()), 0, s.shader.globalsSize * 16), 0, s.shader.globalsSize * 4);

			case Params: // trace ('Upload Globals & Params');
				if (_curShader.vertex.paramsLengthFloats > 0) {
					if (buf.vertex.params == null)
						throw "Vertex Params are expected on this shader";
					if (_curShader.vertex.paramsLengthFloats > buf.vertex.params.length)
						throw 'Vertex Params mismatch ${_curShader.vertex.paramsLengthFloats} vs ${buf.vertex.params.length}  in ${_curShader.vertex.md5}';

					var tmpBuff = hl.Bytes.getArray(_vertConstantBuffer);
					// var offset = _curShader.vertex.globalsLength * 4;
					var offset = 0;
					DebugTrace.trace('Vert blitting ${_curShader.vertex.paramsLengthFloats} float ie ${_curShader.vertex.paramsLengthFloats * 4} bytes to buffer of length ${buf.vertex.params.length}');

					tmpBuff.blit(offset, hl.Bytes.getArray(buf.vertex.params.toData()), 0, _curShader.vertex.paramsLengthFloats * 4);
				}

				if (_curShader.fragment.paramsLengthFloats > 0) {
					if (buf.fragment.params == null)
						throw "Fragment Params are expected on this shader";
					if (_curShader.fragment.paramsLengthFloats > buf.fragment.params.length)
						throw 'fragment params mismatch ${_curShader.fragment.paramsLengthFloats} vs ${buf.fragment.params.length}';

					var tmpBuff = hl.Bytes.getArray(_fragConstantBuffer);
					//				var offset = _curShader.fragment.globalsLength * 4;
					var offset = 0;
					DebugTrace.trace('Frag blitting ${_curShader.fragment.paramsLengthFloats} float ie ${_curShader.fragment.paramsLengthFloats * 4} bytes');
					tmpBuff.blit(offset, hl.Bytes.getArray(buf.fragment.params.toData()), 0, _curShader.fragment.paramsLengthFloats * 4);
				}
			case Textures:
				DebugTrace.trace('RENDER TEXTURES PIPELINE PROVIDED v ${buf.vertex.tex.length} f ${buf.fragment.tex.length} ');

				_vertexTextures = buf.vertex.tex;
				_fragmentTextures = buf.fragment.tex;

				#if OLD_NEW
				// Make sure that we have the minimum number of textures for the shader
				for (i in 0...TEX_TYPE_COUNT) {
					_vertexTextures[i].resize(0);
					_fragmentTextures[i].resize(0);
				}

				var vertRuntimeData = _curShader.vertex.shader;
				var fragRuntimeData = _curShader.fragment.shader;

				DebugTrace.trace('RENDER TEXTURES PIPELINE shader texture count vert ${vertRuntimeData.texturesCount} frag ${fragRuntimeData.texturesCount}');
				{
					var tt = vertRuntimeData.textures;
					for (i in 0...vertRuntimeData.texturesCount) {
						trace('RENDER TEXTURES vertex tex ${i} name ${tt.name} type ${tt.type}');
						tt = tt.next;
					}
				} {
					var tt = fragRuntimeData.textures;
					for (i in 0...fragRuntimeData.texturesCount) {
						trace('RENDER TEXTURES frag tex ${i} name ${tt.name} type ${tt.type}');
						tt = tt.next;
					}
				}
				for (i in 0...2) {
					var texturesList = i == 0 ? buf.vertex.tex : buf.fragment.tex;
					var targetTextureArrays = i == 0 ? _vertexTextures : _fragmentTextures;

					for (t in texturesList) {
						if (t.flags.has(Cube)) {
							targetTextureArrays[TEX_TYPE_CUBE].push(t);
							if (t.t == null)
								throw "Cube Texture is null";
							if (t.t.t == null)
								throw "Forge Cube texture is null";
						} else if (t.flags.has(Target)) {
							trace('RENDER TEXTURES target ${t.name}');
							targetTextureArrays[TEX_TYPE_RT].push(t);
							if (t.t.rt == null)
								throw "Render target is null";
							var rtt = t.t.rt.rt.getTexture();
							if (rtt == null) {
								throw "Render target texture is null";
							}
						} else {
							targetTextureArrays[TEX_TYPE_2D].push(t);
							if (t.t == null)
								throw "Texture 2D is null";
							if (t.t.t == null)
								throw "Forge texture 2D is null";
						}

						t.lastFrame = _currentFrame;
					}
				}
				#end
			case Buffers:
				DebugTrace.trace('RENDER BUFFERS Upload Buffers v ${buf.vertex.buffers} f ${buf.fragment.buffers}');

				// throw ('Maybe ${_curShader} has buffers?');
				if (_curShader.vertex.buffers != null) {
					//				throw ("Not supported");
					DebugTrace.trace('Vertex buffers length ${_curShader.vertex.buffers}');
				}

				_fragmentUniformBuffers.resize(0);
				if (_curShader.fragment.buffers != null) {
					if (buf.fragment.buffers == null)
						throw "Buffers not allocated";
					DebugTrace.trace('Fragment buffers length ${_curShader.fragment.buffers}');

					for (i in 0...buf.fragment.buffers.length) {
						var hbuf = @:privateAccess buf.fragment.buffers[i].buffer.vbuf;
						_fragmentUniformBuffers.push(hbuf);
					}
				}
		}
	}

	var _curTarget:h3d.mat.Texture;
	var _rightHanded = false;

	function selectMaterialBits(bits:Int) {
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

	function selectStencilBits(opBits:Int, maskBits:Int) {
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

	function convertDepthFunc(c:Compare):forge.Native.CompareMode {
		return switch (c) {
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

	function buildDepthState(d:forge.Native.DepthStateDesc, pass:h3d.mat.Pass) {
		d.depthTest = pass.depthTest != Always || pass.depthWrite;
		d.depthWrite = pass.depthWrite;
		//		d.depthTest = false;
		//		DebugTrace.trace('DEPTH depth test ${pass.depthTest} write ${pass.depthWrite}');
		d.depthFunc = convertDepthFunc(pass.depthTest);
		//		d.depthFunc = CMP_GREATER;
		//		DebugTrace.trace ('RENDERER DEPTH config ${d.depthTest} ${d.depthWrite} ${d.depthFunc}');
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

	function buildRasterState(r:forge.Native.RasterizerStateDesc, pass:h3d.mat.Pass) {
		var bits = @:privateAccess pass.bits;
		/*
			When rendering to a render target, our output will be flipped in Y to match
			output texture coordinates. We also need to flip our culling.
			The result is inverted if we are using a right handed camera.
		 */

		if ((_curTarget == null) == _rightHanded) {
			switch (pass.culling) {
				case Back:
					bits = (bits & ~Pass.culling_mask) | (2 << Pass.culling_offset);
				case Front:
					bits = (bits & ~Pass.culling_mask) | (1 << Pass.culling_offset);
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

	inline function convertBlendConstant(blendSrc:h3d.mat.Data.Blend):forge.Native.BlendConstant {
		return switch (blendSrc) {
			case One: BC_ONE;
			case Zero: BC_ZERO;
			case SrcAlpha: BC_SRC_ALPHA;
			case DstAlpha: BC_DST_ALPHA;
			case SrcColor: BC_SRC_COLOR;
			case DstColor: BC_DST_COLOR;
			case OneMinusSrcAlpha: BC_ONE_MINUS_SRC_ALPHA;
			case OneMinusSrcColor: BC_ONE_MINUS_SRC_COLOR;
			case OneMinusDstAlpha: BC_ONE_MINUS_DST_ALPHA;
			case OneMinusDstColor: BC_ONE_MINUS_DST_COLOR;
			case ConstantColor: BC_BLEND_FACTOR;
			case OneMinusConstantColor: BC_ONE_MINUS_BLEND_FACTOR;
			default: throw "Unrecognized blend";
		}
	}

	inline function convertBlendMode(blend:h3d.mat.Data.Operation):forge.Native.BlendMode {
		return switch (blend) {
			case Add: return BM_ADD;
			case Sub: return BM_SUBTRACT;
			case ReverseSub: return BM_REVERSE_SUBTRACT;
			case Min: return BM_MIN;
			case Max: return BM_MAX;
		};
	}

	inline function convertColorMask(heapsMask:Int):Int {
		// Need to do the remap [RC]
		return ColorMask.ALL;
	}

	function buildBlendState(b:forge.Native.BlendStateDesc, pass:h3d.mat.Pass) {
		DebugTrace.trace("RENDER CALLSTACK buildBlendState");

		DebugTrace.trace('BLENDING CM ${pass.colorMask} src ${pass.blendSrc} dest ${pass.blendDst} src alpha ${pass.blendAlphaSrc} op ${pass.blendOp}');
		b.setMasks(0, convertColorMask(pass.colorMask));
		b.setRenderTarget(BLEND_STATE_TARGET_ALL, true);
		b.setSrcFactors(0, convertBlendConstant(pass.blendSrc));
		b.setDstFactors(0, convertBlendConstant(pass.blendDst));
		b.setSrcAlphaFactors(0, convertBlendConstant(pass.blendAlphaSrc));
		b.setDstAlphaFactors(0, convertBlendConstant(pass.blendAlphaDst));
		b.setBlendModes(0, convertBlendMode(pass.blendOp));
		b.setBlendAlphaModes(0, convertBlendMode(pass.blendAlphaOp));
		b.independentBlend = false;

		// b.alphaToCoverage = false;
	}

	var _materialMap = new forge.Native.Map64Int();
	var _materialInternalMap = new haxe.ds.IntMap<CompiledMaterial>();
	var _matID = 0;
	var _currentPipeline:CompiledMaterial;
	var _currentState = new StateBuilder();
	var _currentPass:h3d.mat.Pass;

	function getLayoutFormat(a:CompiledAttribute):forge.Native.TinyImageFormat
		return switch (a.type) {
			case BYTE:
				DebugTrace.trace('RENDER STRIDE attribute ${a.name} has bytes with ${a.count} length');
				switch (a.count) {
					case 1: TinyImageFormat_R8_UINT;
					case 2: TinyImageFormat_R8G8_UINT;
					case 3: TinyImageFormat_R8G8B8_UINT;
					case 4: TinyImageFormat_R8G8B8A8_UINT;
					default: throw('Unsupported count ${a.count}');
				}
			case FLOAT: TinyImageFormat_R32_SFLOAT;
			case VECTOR: switch (a.count) {
					case 1: TinyImageFormat_R32_SFLOAT;
					case 2: TinyImageFormat_R32G32_SFLOAT;
					case 3: TinyImageFormat_R32G32B32_SFLOAT;
					case 4: TinyImageFormat_R32G32B32A32_SFLOAT;
					default: throw('Unsupported count ${a.count}');
				}
			case UNKNOWN: throw "type unknown";
		}

	function buildLayoutFromShader(s:CompiledProgram):forge.Native.VertexLayout {
		var vl = new forge.Native.VertexLayout();

		var attribCount = 0;

		var totalAttribs = s.attribs.length;
		var bindingCount = 0;

		for (a in s.attribs) {
			var location = attribCount++;
			vl.attribCount = attribCount;

			var layout_attr = vl.attrib(location);

			layout_attr.mFormat = getLayoutFormat(a);

			layout_attr.mBinding = bindingCount; // <- WRONG
			layout_attr.mLocation = a.index;
			layout_attr.mOffset = a.offsetBytes;
			layout_attr.mSemantic = a.semantic;
			layout_attr.mSemanticNameLength = a.name.length;
			layout_attr.setSemanticName(a.name);
			DebugTrace.trace('LAYOUT STRIDE Building layout for location ${location}/${totalAttribs} from compiled attribute ${a.offsetBytes} offset, ${a.strideBytes} stride bytes');
			vl.setStride(bindingCount, a.strideBytes);
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
	function buildHeapsLayout(s:CompiledProgram, m:ManagedBuffer):forge.Native.VertexLayout {
		var vl = new forge.Native.VertexLayout();
		vl.attribCount = 0;

		var offsetBytes = 8 * 4; // floats * sizeof(float)

		for (a in s.attribs) {
			var location = vl.attribCount++;
			var layout_attr = vl.attrib(location);

			layout_attr.mFormat = getLayoutFormat(a);

			var bytePos;
			switch (a.name) {
				case "position":
					bytePos = 0;
				case "normal":
					if (m.stride < 6)
						throw "Buffer is missing NORMAL data, set it to RAW format ?" #if track_alloc + @:privateAccess v.allocPos #end;
					bytePos = 3 * 4;
				case "uv":
					if (m.stride < 8)
						throw "Buffer is missing UV data, set it to RAW format ?" #if track_alloc + @:privateAccess v.allocPos #end;
					bytePos = 6 * 4;
				case s:
					bytePos = offsetBytes;
					offsetBytes += a.sizeBytes;
					if (offsetBytes > (m.strideBytes))
						throw "Buffer is missing '" + s + "' data, set it to RAW format ?" #if track_alloc + @:privateAccess v.allocPos #end;
			}

			layout_attr.mBinding = 0;
			layout_attr.mLocation = a.index;
			layout_attr.mOffset = bytePos; // floats to bytes
			// layout_attr.mRate = VERTEX_ATTRIB_RATE_VERTEX;
			layout_attr.mSemantic = a.semantic;
			layout_attr.mSemanticNameLength = a.name.length;
			layout_attr.setSemanticName(a.name);
			vl.setStride(location, m.strideBytes);
		}

		return vl;
	}

	function buildLayoutFromMultiBuffer(s:CompiledProgram, b:Buffer.BufferOffset):forge.Native.VertexLayout {
		var vl = buildLayoutFromShader(s);
		for (i in 0...vl.attribCount) {
			DebugTrace.trace('RENDER BUILDING LAYOUT MULTI BUFFER ${i}/${vl.attribCount} stride ${b.buffer.buffer.strideBytes} ');
			vl.setStride(i, b.buffer.buffer.strideBytes);
		}
		DebugTrace.trace('RENDER BUILDING LAYOUT MULTI BUFFER DONE ');

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
						DebugTrace.trace ('getting name for ${v.id}');
						var name =  vertTranscoder.varNames.get(v.id);
						if (name == null) name = v.name;
						DebugTrace.trace ('STRIDE Laying out ${v.name} with ${size} floats at ${offset}');

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
						DebugTrace.trace('FILTER STRIDE Setting ${v.name} to stride ${p.stride * 4}');
						vl.setstrides(location++, p.stride * 4);
						default:
				}
			}
		 */
	}

	var _hashBulder = new forge.Native.HashBuilder();
	var _pipelineSeed = Int64.make(0x41843714, 0x85913423);

	function bindPipeline() {
		if (!renderReady())
			return;
		DebugTrace.trace("RENDER CALLSTACK bindPipeline");

		if (_currentPass == null) {
			throw "can't build a pipeline without a pass";
		}
		if (_curShader == null)
			throw "Can't build a pipeline without a shader";

		// Unique hash code for pipeline
		_hashBulder.reset(_pipelineSeed);
		_hashBulder.addInt32(_curShader.id);
		_hashBulder.addInt32(_currentRT.sampleCount.toValue());
		_hashBulder.addInt32(_currentRT.sampleQuality);
		_currentState.addToHash(_hashBulder);
		var dt = _currentRT.depthTarget;
		if (dt != null) {
			_hashBulder.addInt8(1);

			if (dt.nativeRT != null)
				_hashBulder.addInt32(dt.nativeRT.format.toValue());
			else
				throw 'Native depth buffer RT is null';
		}
		for (i in 0..._currentRT.colorTargets.length) {
			_hashBulder.addInt8(i);
			if (_currentRT.colorTargets[i].nativeRT != null)
				_hashBulder.addInt32(_currentRT.colorTargets[i].nativeRT.format.toValue());
			else
				throw 'Native color target ${i} RT is null';
		}
		if (_curBuffer != null) {
			if (_curBuffer.flags.has(RawFormat)) {
				DebugTrace.trace('RENDER BUFFER BIND Raw format selected');
				_hashBulder.addInt8(0);
			} else {
				DebugTrace.trace('RENDER BUFFER BIND non Raw format selected');
				_hashBulder.addInt8(2);
				_hashBulder.addInt16(_curBuffer.buffer.strideBytes);
			}
		} else if (_curMultiBuffer != null) {
			DebugTrace.trace('RENDER BUFFER BIND multi buffer selected');
			_hashBulder.addInt8(1);
			_hashBulder.addInt16(_curMultiBuffer.buffer.buffer.strideBytes); // this is unreliable
		} else
			throw "no buffer specified to bind pipeline";

		var hv = _hashBulder.getValue();
		var cmatidx = _materialMap.get(hv);

		// Get pipeline combination
		var cmat = _materialInternalMap.get(cmatidx);
		DebugTrace.trace('RENDER PIPELINE Signature  xx.h ${hv.high} | xx.l ${hv.low} pipeid ${cmatidx}');

		/*
			var shaderFragTextureCount = _fragmentTextures.length;
			for (i in 0...TEX_TYPE_COUNT) {
				var shaderFragTextureCount = _curShader.fragment.textureCount(i); // .textures == null ? 0 : _curShader.fragment.textures.length;
				DebugTrace.trace('RENDER PIPELINE texture count ${shaderFragTextureCount} vs ${_fragmentTextures[i].length}');
			}
		 */

		if (cmat == null) {
			cmat = new CompiledMaterial();
			cmat._id = _matID++;
			cmat._hash = hv;
			cmat._shader = _curShader;
			cmat._pass = _currentPass;

			if (_curBuffer != null) {
				if (_curBuffer.flags.has(RawFormat)) {
					DebugTrace.trace('LAYOUT BINDING NATURAL layout');
					cmat._layout = _curShader.naturalLayout;
					cmat._stride = _curShader.naturalLayout.getStride(0);
				} else {
					DebugTrace.trace('LAYOUT BINDING HEAPS layout');
					cmat._layout = buildHeapsLayout(_curShader, _curBuffer.buffer);
					cmat._stride = cmat._layout.getStride(0);
				}
			} else if (_curMultiBuffer != null) {
				DebugTrace.trace('LAYOUT BINDING Multi layout');
				// there can be a stride mistmatch here
				cmat._layout = buildLayoutFromMultiBuffer(_curShader, _curMultiBuffer);
				cmat._stride = cmat._layout.getStride(0);
			} else
				throw "no buffer specified to bind pipeline";

			DebugTrace.trace('RENDER PIPELINE  Adding material for pass ${_currentPass.name} with id ${cmat._id} and hash ${cmat._hash.high}|${cmat._hash.low}');
			_materialMap.set(hv, cmat._id);
			_materialInternalMap.set(cmat._id, cmat);

			var pdesc = new forge.Native.PipelineDesc();
			var gdesc = pdesc.graphicsPipeline();
			gdesc.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
			var pDepthState = _currentRT.depthTarget != null ? _currentState.depth() : null;
			if (pDepthState != null) {
				DebugTrace.trace('RENDER PIPELINE  DEPTH PARAMS depth target ${ _currentRT.depthTarget != null} test ${pDepthState.depthTest} write ${pDepthState.depthWrite}');

			} else {
				DebugTrace.trace('RENDER PIPELINE  NO DEPTH BUFFER ');				
			}

			gdesc.pDepthState = pDepthState;
			gdesc.pBlendState = _currentState.blend();
			gdesc.depthStencilFormat = _currentRT.depthTarget != null ? _currentRT.depthTarget.nativeRT.format : TinyImageFormat_UNDEFINED;
			// gdesc.pColorFormats = &rt.mFormat;

			gdesc.pVertexLayout = cmat._layout;
			gdesc.pRootSignature = _curShader.rootSig;
			gdesc.pShaderProgram = _curShader.forgeShader;
			gdesc.pRasterizerState = _currentState.raster();
			gdesc.mVRFoveatedRendering = false;
			var rt = _currentRT;
			pdesc.setRenderTargetGlobals(rt.sampleCount, rt.sampleQuality);
			for (ct in rt.colorTargets) {
				pdesc.addGraphicsColourTarget(ct.nativeRT.format);
			}

			var p = _renderer.createPipeline(pdesc);

			cmat._pipeline = p;
		}

		if (_currentPipeline != cmat) {
			DebugTrace.trace('RENDER CALLSTACK PIPELINE changing binding to another existing pipeline ${_currentPass.name} : ${cmat._id} vs ${_currentPipeline == null ? -1 : _currentPipeline._id}');
			_currentPipeline = cmat;
			_currentCmd.bindPipeline(_currentPipeline._pipeline);
		}
	}

	static inline final FLOAT_BYTES = 4;

	function getParamBuffer() {
		var cp = _curShader;
		if (cp.paramCurBuffer < cp.paramBuffers.length) {
			var bp = cp.paramBuffers[cp.paramCurBuffer];
			if (bp.full()) {
				cp.paramCurBuffer++;
			}
		}

		if (cp.paramCurBuffer >= cp.paramBuffers.length) {
			var length = cp.paramCurBuffer == 0 ? 4 : 16;
			DebugTrace.trace('RENDER Allocating new param buffer verts ${cp.vertex.paramsLengthFloats * FLOAT_BYTES} frag ${cp.fragment.paramsLengthFloats * FLOAT_BYTES}');
			var pb = forge.PipelineParamBufferBlock.allocate(_renderer, cp.rootSig, cp.vertex.paramsLengthFloats * FLOAT_BYTES,
				cp.fragment.paramsLengthFloats * FLOAT_BYTES, PARAMS, length);
			cp.paramBuffers.push(pb);
		}

		return cp.paramBuffers[cp.paramCurBuffer];
	}

	function pushParameters() {
		DebugTrace.trace("RENDER CALLSTACK pushParameters");

		//		DebugTrace.trace ('PARAMS Pushing Vertex Constants ${_temp} floats ${total / 4} vectors: [0] = x ${x} y ${y} z ${z} w ${w}');
		#if PUSH_CONSTANTS
		if (_curShader.vertex.constantsIndex != -1) {
			DebugTrace.trace('RENDER CALLSTACK CONSTANTS pushing vertex parameters ${_curShader.vertex.constantsIndex} ${_curShader.vertex.paramsLengthFloats}');
			_currentCmd.pushConstants(_curShader.rootSig, _curShader.vertex.constantsIndex, hl.Bytes.getArray(_vertConstantBuffer));
		}
		if (_curShader.fragment.constantsIndex != -1) {
			DebugTrace.trace('RENDER CALLSTACK CONSTANTS pushing fragment parameters ${_curShader.fragment.paramsLengthFloats}');
			_currentCmd.pushConstants(_curShader.rootSig, _curShader.fragment.constantsIndex, hl.Bytes.getArray(_fragConstantBuffer));
		}
		#else
		if (_curShader.vertex.constantsIndex != -1 || _curShader.fragment.constantsIndex != -1) {
			var pb = getParamBuffer();
			pb.beginUpdate();
			{
				DebugTrace.trace('FILLING VERT PARAMS vert length ${_curShader.vertex.paramsLengthBytes}');
				var qw = Std.int(_curShader.vertex.paramsLengthFloats / 4);
				for (i in 0...qw) {
					DebugTrace.trace('\tVert qw ${i} ${_vertConstantBuffer[i * 4 + 0]},${_vertConstantBuffer[i * 4 + 1]},${_vertConstantBuffer[i * 4 + 2]},${_vertConstantBuffer[i * 4 + 3]}');
				}
			}
			{
				DebugTrace.trace('FILLING FRAG PARAMS  flength ${_curShader.fragment.paramsLengthBytes}');
				var qw = Std.int(_curShader.fragment.paramsLengthFloats / 4);
				for (i in 0...qw) {
					DebugTrace.trace('\tVert qw ${i} ${_fragConstantBuffer[i * 4 + 0]},${_fragConstantBuffer[i * 4 + 1]},${_fragConstantBuffer[i * 4 + 2]},${_fragConstantBuffer[i * 4 + 3]}');
				}
			}
			pb.fill(hl.Bytes.getArray(_vertConstantBuffer), hl.Bytes.getArray(_fragConstantBuffer));
			pb.bind(_currentCmd);
			pb.next();
		}
		#end

		if (_curShader.globalDescriptorSet != null) {
			var idx = _curShader.vertex.globalsBuffer != null ? _curShader.vertex.globalsBuffer.currentIdx() : _curShader.fragment.globalsBuffer.currentIdx();
			DebugTrace.trace('RENDER Setting global descriptor set to idx ${idx} vert buf ${_curShader.vertex.globalsBuffer} frag buf ${_curShader.fragment.globalsBuffer}');
			_currentCmd.bindDescriptorSet(idx, _curShader.globalDescriptorSet);
		}
	}

	function bindBuffers() {
		DebugTrace.trace("RENDER CALLSTACK bindBuffers");

		if (_fragmentUniformBuffers.length > 0 && _curShader.fragment.bufferCount > 0) {
			for (i in 0..._fragmentUniformBuffers.length) {
				var hbuf = _fragmentUniformBuffers[i];

				if (hbuf.descriptorMap == null)
					hbuf.descriptorMap = new Map<Int, forge.Native.DescriptorSet>();
				if (_currentPipeline == null)
					throw "No current pipeline";
				var ds = hbuf.descriptorMap.get(_currentPipeline._id);
				if (ds == null) {
					var setDesc = new forge.Native.DescriptorSetDesc();
					setDesc.pRootSignature = _curShader.rootSig;
					setDesc.setIndex = EDescriptorSetSlot.BUFFERS;
					setDesc.maxSets = 3; // TODO: make this configurable
					DebugTrace.trace('RENDER Creating descriptors for shader id ${_curShader.id}');
					ds = _renderer.addDescriptorSet(setDesc);
					var in_buf:sdl.Forge.Buffer = @:privateAccess hbuf.b;
					DebugTrace.trace('Filling descriptor set');
					_renderer.fillDescriptorSet(in_buf, ds, DBM_UNIFORMS, _curShader.fragment.buffers[i]);

					hbuf.descriptorMap[_currentPipeline._id] = ds;
				}

				if (ds == null)
					throw 'no descriptor set for buffer ${i}';
				DebugTrace.trace('RENDER CALLSTACK BINDING BUFFER ${i} to ${hbuf.b.currentIdx()}');
				_currentCmd.bindDescriptorSet(hbuf.b.currentIdx(), ds);
			}
			//			_currentCmd.bindDescriptorSet(_curShader.fragment.globalsBuffer.currentIdx(), _curShader.fragment.globalDescriptorSet);
		}
	}

	static inline final TBDT_2D = 0x1;
	static inline final TBDT_CUBE = 0x2;
	static inline final TBDT_RT = 0x4;
	static inline final TBDT_2D_AND_CUBE = TBDT_2D | TBDT_CUBE;
	static inline final TBDT_2D_IDX = 0;
	static inline final TBDT_CUBE_IDX = 1;
	static inline final TBDT_2D_AND_CUBE_IDX = 2;

	function getTextureBlock() {
		var cp = _curShader;
		if (cp.textureCurBlock < cp.textureBlocks.length) {
			var bp = cp.textureBlocks[cp.textureCurBlock];
			if (bp.full()) {
				cp.textureCurBlock++;
			}
		}

		if (cp.textureCurBlock >= cp.textureBlocks.length) {
			var length = cp.textureCurBlock == 0 ? 4 : 16;
			DebugTrace.trace('RENDER Allocating new texture block verts ${cp.vertex.paramsLengthFloats * FLOAT_BYTES} frag ${cp.fragment.paramsLengthFloats * FLOAT_BYTES}');
			var tsb = PipelineTextureSetBlock.allocate(_renderer, cp.rootSig, cp.vertex, cp.fragment, _globalSamplers, length);
			cp.textureBlocks.push(tsb);
		}

		return cp.textureBlocks[cp.textureCurBlock];
	}

	function bindTextures() {
		DebugTrace.trace("RENDER CALLSTACK bindTextures");
		if (_currentPipeline == null)
			throw "Can't bind textures on null pipeline";
		if (_curShader == null)
			throw "Can't bind textures on null shader";
		if (_curShader.fragment.hasTextures() || _curShader.vertex.hasTextures()) {
			if (_vertexTextures.length < _curShader.vertex.textureTotalCount())
				throw 'Not enough vertex textures ';
			if (_fragmentTextures.length < _curShader.fragment.textureTotalCount())
				throw 'Not enough fragment textures  have ${_fragmentTextures.length} should have ${_curShader.fragment.textureTotalCount()}';
			if (_vertexTextures.length != _curShader.vertex.textureTotalCount())
				DebugTrace.trace('RENDER WARNING shader vertex texture count ${_curShader.vertex.textureTotalCount()} doesn\'t match provided texture count ${_vertexTextures.length}');
			if (_fragmentTextures.length != _curShader.fragment.textureTotalCount())
				DebugTrace.trace('RENDER WARNING shader fragment texture count ${_curShader.fragment.textureTotalCount()} doesn\'t match provided texture count ${_vertexTextures.length}');

			var block = getTextureBlock();
			block.beginUpdate();
			block.fill(_renderer, _vertexTextures, _fragmentTextures);
			block.bind(_currentCmd);
			block.next();
		}

		#if OLD_NEW
		for (i in 0...TEX_TYPE_COUNT) {
			trace('RENDER CALLSTACK bindTextures ${i} verts ${_vertexTextures[i].length} frags ${_fragmentTextures[i].length}');
		}

		if (_curShader.fragment.hasTextures() || _curShader.vertex.hasTextures()) {
			#if !YEE_OLD_BINDER
			for (i in 0...TEX_TYPE_COUNT) {
				if (_vertexTextures[i].length < _curShader.vertex.textureCount(i))
					throw 'Not enough vertex textures for array ${i}';
				if (_fragmentTextures[i].length < _curShader.fragment.textureCount(i))
					throw 'Not enough fragment textures for array ${i} have ${_fragmentTextures[i].length} should have ${_curShader.fragment.textureCount(i)}';
				if (_vertexTextures[i].length != _curShader.vertex.textureCount(i))
					DebugTrace.trace('RENDER WARNING shader vertex texture count ${_curShader.vertex.textureCount(i)} doesn\'t match provided texture count ${_vertexTextures[i].length}');
				if (_fragmentTextures[i].length != _curShader.fragment.textureCount(i))
					DebugTrace.trace('RENDER WARNING shader fragment texture count ${_curShader.fragment.textureCount(i)} doesn\'t match provided texture count ${_vertexTextures[i].length}');
			}

			trace('RENDER UPDATING TEXTURE BLOCK'); 
			var block = getTextureBlock();
			block.beginUpdate();
			block.fill(_renderer, _vertexTextures, _fragmentTextures);
			block.bind(_currentCmd);
			block.next();

			trace('RENDER DONE UPDATING TEXTURE BLOCK'); 
			#else
			throw ('Should not be here');
			if (_curShader.fragment.hasTextures() || _curShader.vertex.hasTextures()) {
				//		if (_curShader.vertex.textures.length == 0) return;
				var fragmentSeed = 0x3917437;

				var textureMode = 0;
				for (i in 0...TEX_TYPE_COUNT) {
					if (_curShader.vertex.textureCount(i) > 0) {
						textureMode |= 1 << i;
					}
					if (_curShader.fragment.textureCount(i) > 0) {
						textureMode |= 1 << (i + TEX_TYPE_COUNT);
					}
				}

				var crc = new Crc32();

				crcInt(crc, fragmentSeed);
				crcInt(crc, textureMode);
				crcInt(crc, _currentPipeline._hash.low);
				crcInt(crc, _currentPipeline._hash.high);

				for (i in 0...TEX_TYPE_COUNT) {
					crcInt(crc, _curShader.fragment.textureCount(i));
					for (j in 0..._curShader.fragment.textureCount(i)) {
						crcInt(crc, _fragmentTextures[i][j].id);
					}
					crcInt(crc, _curShader.vertex.textureCount(i));
					for (j in 0..._curShader.vertex.textureCount(i)) {
						crcInt(crc, _vertexTextures[i][j].id);
					}
				}

				// probably want to do this inside each pipeline?
				// This is generating a new descriptor set for each unique combination of textures
				var tds = _textureDescriptorMap.get(crc.get());

				if (tds == null) {
					DebugTrace.trace('RENDER Adding texture ${crc.get()} mode ${textureMode}');
					var descriptorSets = _backend == Metal ? 2 : 1;
					var ds = _renderer.createDescriptorSet(_curShader.rootSig, EDescriptorSetSlot.TEXTURES, descriptorSets, 0);

					if (_textureDataBuilder == null) {
						DebugTrace.trace('RENDER TExture builder is null, creating one');
						_textureDataBuilder = new Array<forge.Native.DescriptorDataBuilder>();

						var cdb = new forge.Native.DescriptorDataBuilder();

						for (i in 0...TEX_TYPE_COUNT) {
							// 2D Case
							if (textureMode & (1 << i) != 0) {
								var s = cdb.addSlot(DBM_TEXTURES);
								cdb.setSlotBindName(s, VERTEX_TEX_ARRAY_SHADER_NAME[i]);
								s = cdb.addSlot(DBM_SAMPLERS);
								cdb.setSlotBindName(s, VERTEX_TEX_ARRAY_SAMPLER_SHADER_NAME[i]);
							}

							// fragments
							if (textureMode & (1 << (i + TEX_TYPE_COUNT)) != 0) {
								var s = cdb.addSlot(DBM_TEXTURES);
								cdb.setSlotBindName(s, FRAGMENT_TEX_ARRAY_SHADER_NAME[i]);
								s = cdb.addSlot(DBM_SAMPLERS);
								cdb.setSlotBindName(s, FRAGMENT_TEX_ARRAY_SAMPLER_SHADER_NAME[i]);
							}
							_textureDataBuilder.push(cdb);
						}
					} else {
						DebugTrace.trace('TExture builder is not null');

						for (i in 0...TEX_TYPE_COUNT) {
							// 2D Case
							if (textureMode & (1 << i) != 0) {
								var s = cdb.addSlot(DBM_TEXTURES);
								cdb.setSlotBindName(s, VERTEX_TEX_ARRAY_SHADER_NAME[i]);
								s = cdb.addSlot(DBM_SAMPLERS);
								cdb.setSlotBindName(s, VERTEX_TEX_ARRAY_SAMPLER_SHADER_NAME[i]);
							}

							// fragments
							if (textureMode & (1 << (i + TEX_TYPE_COUNT)) != 0) {
								var s = cdb.addSlot(DBM_TEXTURES);
								cdb.setSlotBindName(s, FRAGMENT_TEX_ARRAY_SHADER_NAME[i]);
								s = cdb.addSlot(DBM_SAMPLERS);
								cdb.setSlotBindName(s, FRAGMENT_TEX_ARRAY_SAMPLER_SHADER_NAME[i]);
							}
							_textureDataBuilder.push(cdb);
						}

						// Needs to be a better way to do this
						switch (textureMode) {
							case TBDT_2D:
								_textureDataBuilder[TBDT_2D_IDX].clearSlotData(0);
								if (_backend == Metal)
									_textureDataBuilder[TBDT_2D_IDX].clearSlotData(1);
							case TBDT_CUBE:
								_textureDataBuilder[TBDT_CUBE_IDX].clearSlotData(0);
								if (_backend == Metal)
									_textureDataBuilder[TBDT_CUBE_IDX].clearSlotData(1);
							case TBDT_2D_AND_CUBE:
								_textureDataBuilder[TBDT_2D_AND_CUBE_IDX].clearSlotData(0);
								_textureDataBuilder[TBDT_2D_AND_CUBE_IDX].clearSlotData(1);
								_textureDataBuilder[TBDT_2D_AND_CUBE_IDX].clearSlotData(2);
								_textureDataBuilder[TBDT_2D_AND_CUBE_IDX].clearSlotData(3);
							default:
								throw("Unrecognized texture mode");
						}
					}

					var tdb = _textureDataBuilder[textureModeIdx];

					DebugTrace.trace('RENDER shader fragment texture count is ${shaderFragTextureCount}');

					for (i in 0...shaderFragTextureCount) {
						var t = _fragmentTextures[i];
						if (t.t == null)
							throw "Texture is null";
						var ft = (t.t.rt != null) ? t.t.rt.rt.getTexture() : t.t.t;

						if (ft == null)
							throw "Forge texture is null";

						switch (_backend) {
							case Metal:
								tdb.addSlotTexture(0, ft);
								tdb.addSlotSampler(1, _bilinearClamp2DSampler);
							case Vulkan:
								tdb.addSlotTexture(0, ft);
								tdb.addSlotSampler(1, _bilinearClamp2DSampler);
						}
					}

					for (i in 0...shaderFragTextureCubeCount) {
						var t = _fragmentTextureCubes[i];
						if (t.t == null)
							throw "Texture is null";
						var ft = (t.t.rt != null) ? t.t.rt.rt.getTexture() : t.t.t;

						if (ft == null)
							throw "Forge texture is null";
						var slot = (textureMode & TBDT_2D != 0) ? 2 : 0;
						switch (_backend) {
							case Metal:
								tdb.addSlotTexture(slot, ft);
								tdb.addSlotSampler(slot + 1, _bilinearClamp2DSampler);
							case Vulkan:
								tdb.addSlotTexture(slot, ft);
						}
					}

					DebugTrace.trace('RENDER shader updating descriptor set');
					tdb.update(_renderer, 0, ds);

					tds = {
						mat: _currentPipeline,
						tex: _fragmentTextures.copy(),
						texCubes: _fragmentTextureCubes.copy(),
						ds: ds
					};
					_textureDescriptorMap.set(crc.get(), tds);
					DebugTrace.trace('RENDER Done setting up textures');
				} else {
					DebugTrace.trace('RENDER TEXTURE Reusing texture ${crc.get()}');
				}

				DebugTrace.trace('RENDER BINDING TEXTURE descriptor tex ${shaderFragTextureCount} texCube ${shaderFragTextureCubeCount}');
				_currentCmd.bindDescriptorSet(0, tds.ds);
				DebugTrace.trace('RENDER DONE tex');
			}
			#end
		} else {
			DebugTrace.trace('RENDER No textures specified in shader');
		}
		#end
	}

	public override function selectMaterial(pass:h3d.mat.Pass) {
		DebugTrace.trace('RENDER MATERIAL SHADER selectMaterial PASS NAME: "${pass.name}" ${[for (s in pass.getShaders()) '${s.toString()} : ${s.name}']}');

		// culling
		// stencil
		// mode
		// blending

		_currentState.reset();

		// @:privateAccess selectStencilBits(s.opBits, s.maskBits);
		buildBlendState(_currentState.blend(), pass);
		buildRasterState(_currentState.raster(), pass);
		buildDepthState(_currentState.depth(), pass);

		_currentPipeline = null;
		_currentPass = pass;
	}

	var _curBuffer:h3d.Buffer;
	var _bufferBinder = new forge.Native.BufferBinder();
	var _curMultiBuffer:Buffer.BufferOffset;

	public override function selectMultiBuffers(buffers:Buffer.BufferOffset) {
		DebugTrace.trace('RENDER CALLSTACK selectMultiBuffers');
		_curMultiBuffer = buffers;
		_curBuffer = null;
	}

	public function bindMultiBuffers() {
		DebugTrace.trace('RENDER CALLSTACK bindMultiBuffers ${_curShader.attribs.length}');
		if (_curMultiBuffer == null)
			throw "Multibuffers are null";
		var curBufferView = _curMultiBuffer;

		_bufferBinder.reset();
		var strideCountBytes = 0;
		for (a in _curShader.attribs) {
			var bb = curBufferView.buffer;
			var mb = bb.buffer;
			// trace ('RENDER selectMultiBuffers with strid ${mb.stride}');
			var isByteBuffer = mb.flags.has(ByteBuffer);
			var elementSize = isByteBuffer ? 1 : 4;
			var vb = @:privateAccess mb.vbuf;
			var b = @:privateAccess vb.b;
			// DebugTrace.trace('FILTER prebinding buffer b ${b} i ${a.index} o ${a.offset} t ${a.type} d ${a.divisor} si ${a.size} bbvc ${bb.vertices} mbstr ${mb.stride} mbsi ${mb.size} bo ${buffers.offset} - ${_curShader.inputs.names[a.index]}' );
			// _bufferBinder.add( b, buffers.buffer.buffer.stride * 4, buffers.offset * 4);

			strideCountBytes += a.sizeBytes;
			_bufferBinder.add(b, mb.strideBytes, curBufferView.offset * elementSize);
			DebugTrace.trace('RENDER STRIDE ${mb.strideBytes} OFFSET ${curBufferView.offset * elementSize} attr ${a.name} offset ${curBufferView.offset} offset bytes ${curBufferView.offset * elementSize}');
			// gl.bindBuffer(GL.ARRAY_BUFFER, @:privateAccess buffers.buffer.buffer.vbuf.b);
			// gl.vertexAttribPointer(a.index, a.size, a.type, false, buffers.buffer.buffer.stride * 4, buffers.offset * 4);
			// updateDivisor(a);
			curBufferView = curBufferView.next;
		}
		DebugTrace.trace('RENDER STRIDE shader stride ${strideCountBytes} vs pipe ${_currentPipeline._stride} ');

		// if (_currentPipeline._stride  != mb.strideBytes) throw "Shader - buffer stride mistmatch";

		// DebugTrace.trace('Binding vertex buffer');
		_currentCmd.bindVertexBuffer(_bufferBinder);
	}

	var _curIndexBuffer:IndexBuffer;
	var _firstDraw = true;

	public override function draw(ibuf:IndexBuffer, startIndex:Int, ntriangles:Int) {
		DebugTrace.trace('RENDER CALLSTACK draw INDEXED tri count ${ntriangles} index count ${ntriangles * 3} start ${startIndex} is32 ${ibuf.is32}');

		bindPipeline();
		pushParameters();
		bindTextures();
		bindBuffers();
		if (_curBuffer != null)
			bindBuffer();
		else if (_curMultiBuffer != null)
			bindMultiBuffers();
		else
			throw "No bound vertex data";

		// if (!_firstDraw) return;
		_firstDraw = false;
		if (ibuf != _curIndexBuffer) {
			_curIndexBuffer = ibuf;
		}

		// cmdBindDescriptorSet(cmd, 0, pDescriptorSetTexture);
		// cmdBindDescriptorSet(cmd, gFrameIndex * 2 + 0, pDescriptorSetUniforms);
		// cmdBindDescriptorSet(cmd, 0, pDescriptorSetShadow[1]);
		// DebugTrace.trace('Binding index buffer');
		_currentCmd.bindIndexBuffer(ibuf.b, ibuf.is32 ? INDEX_TYPE_UINT32 : INDEX_TYPE_UINT16, 0);
		// DebugTrace.trace('Drawing ${ntriangles} triangles');
		_currentCmd.drawIndexed(ntriangles * 3, startIndex, 0);

		/*
			if( ibuf.is32 )
				gl.drawElements(drawMode, ntriangles * 3, GL.UNSIGNED_INT, startIndex * 4);
			else
				gl.drawElements(drawMode, ntriangles * 3, GL.UNSIGNED_SHORT, startIndex * 2);
		 */
		DebugTrace.trace('RENDER CALLSTACK drawing complete');
	}

	public override function selectBuffer(v:Buffer) {
		_curBuffer = v;
		_curMultiBuffer = null;
		DebugTrace.trace('RENDER CALLSTACK selectBuffer raw: ${v != null ? v.flags.has(RawFormat) : false}');
	}

	public function bindBuffer() {
		if (!renderReady())
			return;
		DebugTrace.trace('RENDER CALLSTACK bindBuffer');

		if (_currentPipeline == null)
			throw "No pipeline defined";

		//		DebugTrace.trace('selecting buffer ${v.id}');

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
		// if( m.stride < _curShader.stride )
		//	throw "Buffer stride (" + m.stride + ") and shader stride (" + _curShader.stride + ") mismatch";

		// DebugTrace.trace('STRIDE SELECT BUFFER m.stride ${m.stride} vbuf ${vbuf.stride} cur shader stride ${_curShader.stride}');
		#if multidriver
		if (m.driver != this)
			throw "Invalid buffer context";
		#end
		//		gl.bindBuffer(GL.ARRAY_BUFFER, m.b);

		if (v.flags.has(RawFormat)) {
			DebugTrace.trace('SELECT RAW FORMAT');

			// Use the shader binding
			for (a in _curShader.attribs) {
				DebugTrace.trace('STRIDE selectBuffer binding ${a.index} strid ${m.stride} stridebytes ${m.strideBytes} offset ${a.offsetBytes}');

				_bufferBinder.add(b, m.strideBytes, a.offsetBytes);
				// gl.vertexAttribPointer(a.index, a.size, a.type, false, m.stride * 4, pos * 4);

				// m.stride * 4
				// pos * 4
				// _bufferBinder.add( m.b, 0, 0 );

				// updateDivisor(a);
			}
			_currentCmd.bindVertexBuffer(_bufferBinder);
		} else {
			DebugTrace.trace('SELECT NON RAW FORMAT');

			var offsetBytes = 8 * 4; // 8 floats * 4 bytes a piece
			var strideCheck = 0;
			for (i in 0..._curShader.attribs.length) {
				var a = _curShader.attribs[i];
				var posBytes;
				switch (_curShader.inputs.names[i]) {
					case "position":
						posBytes = 0;
					case "normal":
						if (m.stride < 6)
							throw "Buffer is missing NORMAL data, set it to RAW format ?" #if track_alloc + @:privateAccess v.allocPos #end;
						posBytes = 3 * 4;
					case "uv":
						if (m.stride < 8)
							throw "Buffer is missing UV data, set it to RAW format ?" #if track_alloc + @:privateAccess v.allocPos #end;
						posBytes = 6 * 4;
					case s:
						DebugTrace.trace('RENDER STRIDE WARNING unrecognized buffer ${s}');
						posBytes = offsetBytes;
						offsetBytes += a.sizeBytes;
						if (offsetBytes > m.strideBytes)
							throw "Buffer is missing '" + s + "' data, set it to RAW format ?" #if track_alloc + @:privateAccess v.allocPos #end;
				}
				// m.stride * 4
				// _bufferBinder.add( m.b, 0, pos * 4 );

				// gl.vertexAttribPointer(a.index, a.size, a.type, false, m.stride * 4, pos * 4);
				// updateDivisor(a);
				DebugTrace.trace('STRIDE BUFFER ${_curShader.inputs.names[i]} type is ${a.type} size is ${a.sizeBytes} bytes vs stride ${m.stride} elements (${m.strideBytes})');
				_bufferBinder.add(b, m.strideBytes, posBytes);
			}
			if (offsetBytes != m.strideBytes) {
				DebugTrace.trace('RENDER WARNING stride byte mistmatch ${offsetBytes} bytes vs ${m.strideBytes} bytes attrib len ${_curShader.attribs.length}');
			}
			_currentCmd.bindVertexBuffer(_bufferBinder);
			//			throw ("unsupported");
		}
	}

	public override function clear(?color:h3d.Vector, ?depth:Float, ?stencil:Int) {
		DebugTrace.trace('RENDER CALLSTACK TARGET CLEAR bind and clear ${_currentRT} and ${_currentRT.depthBuffer} ${color} ${depth} ${stencil}');
		if (!renderReady())
			return;

		if (color != null) {
			var x:h3d.Vector = color;
			DebugTrace.trace('RENDER CLEAR ${x} : ${x.r}, ${x.g}, ${x.b}, ${x.a}');
			_currentRT.colorTargets[0].nativeRT.setClearColor(x.r, x.g, x.b, x.a);
		}

		if (depth != null && _currentRT.depthTarget != null) {
			var sten = stencil != null ? stencil : 0;
			var d:Float = depth;
			var rt = _currentRT.depthTarget.nativeRT;
			if (rt != null)
				rt.setClearDepthNormalized(d, sten);
		}

		DebugTrace.trace('RENDER FORCE CLEAR BINDING');
		//
		_currentCmd.bind(_currentRT.colorTargets[0].nativeRT, _currentRT.depthTarget == null ? null : _currentRT.depthTarget.nativeRT, LOAD_ACTION_CLEAR,
			LOAD_ACTION_CLEAR);
		DebugTrace.trace('RENDER FORCE CLEAR BINDING DONE');
	}

	public override function end() {
		DebugTrace.trace('RENDER CALLSTACK TARGET end');
		_currentCmd.unbindRenderTarget();
		if (_currentRT.colorTargets[0].outBarrier != null)
			_currentCmd.insertBarrier(_currentRT.colorTargets[0].outBarrier); // needs to be full out barrier, not just read
		DebugTrace.trace('Inserting current RT present');
		_currentRT = _swapRenderBlocks[_currentSwapIndex];
		if (_currentRT.colorTargets[0].present != null)
			_currentCmd.insertBarrier(_currentRT.colorTargets[0].present);
		_currentRT = null;
		_currentCmd.end();

		forge.Native.Globals.waitForAllResourceLoads();

		_queue.submit(_currentCmd, _currentSem, _ImageAcquiredSemaphore, _currentFence);
		_currentCmd = null;
	}

	public override function uploadTextureBitmap(t:h3d.mat.Texture, bmp:hxd.BitmapData, mipLevel:Int, side:Int) {
		DebugTrace.trace('RENDER CALLSTACK uploadTextureBitmap');
		var pixels = bmp.getPixels();
		uploadTexturePixels(t, pixels, mipLevel, side);
		pixels.dispose();
	}

	public override function disposeVertexes(v:VertexBuffer) {
		DebugTrace.trace('RENDER CALLSTACK DEALLOC disposeVertexes');
		v.b.dispose();
		v.b = null;
	}

	public override function disposeIndexes(i:IndexBuffer) {
		DebugTrace.trace('RENDER CALLSTACK DEALLOC disposeIndexes');
		i.b.dispose();
		i.b = null;
	}

	public override function disposeTexture(t:h3d.mat.Texture) {
		DebugTrace.trace('RENDER CALLSTACK DEALLOC disposeTexture');

		var tt = t.t;
		if (tt == null)
			return;
		if (tt.t == null)
			return;
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
	// var currentDepth : DepthBuffer;
	// var currentDepth:h3d.mat.DepthBuffer;

	function setRenderTargetsInternal(rtrt:RuntimeRenderTarget) { // textures:Array<h3d.mat.Texture>, layer:Int, mipLevel:Int) {
		if (!renderReady())
			return;

		DebugTrace.trace('RENDER TARGET CALLSTACK setRenderTargetsInternal');

		if (_currentCmd == null) {
			_currentCmd = _tmpCmd;
			_currentCmd.begin();
		} else {
			_currentCmd.unbindRenderTarget();
			DebugTrace.trace('Inserting current RT out');

			if (_currentRT != null && _currentRT.colorTargets[0].outBarrier != null)
				_currentCmd.insertBarrier(_currentRT.colorTargets[0].outBarrier); // this should be implicit??
		}

		if (!rtrt.created) {
			if (rtrt.texture == null) {
				throw 'RENDER TARGET  no input textures';
			}
			var tex = rtrt.texture;
			if (tex.t == null)
				tex.alloc();
			rtrt.heapsDepthBuffer = tex.depthBuffer;

			DebugTrace.trace('RENDER TARGET  creating new render target on ${tex} w ${tex.width} h ${tex.height} db ${rtrt.heapsDepthBuffer}');
			var renderTargetDesc = new forge.Native.RenderTargetDesc();
			renderTargetDesc.arraySize = 1;

			//		renderTargetDesc.clearValue.depth = 1.0f;
			//		renderTargetDesc.clearValue.stencil = 0;
			renderTargetDesc.descriptors = DESCRIPTOR_TYPE_TEXTURE;
			renderTargetDesc.format = getTinyTextureFormat(tex.format);
			renderTargetDesc.startState = RESOURCE_STATE_SHADER_RESOURCE;
			renderTargetDesc.height = tex.width;
			renderTargetDesc.width = tex.height;
			renderTargetDesc.sampleCount = SAMPLE_COUNT_1;
			renderTargetDesc.sampleQuality = 0;
			//			renderTargetDesc.nativeHandle = itex.t.
			renderTargetDesc.flags = forge.Native.TextureCreationFlags.TEXTURE_CREATION_FLAG_OWN_MEMORY_BIT.toValue();
			// renderTargetDesc.name = depthBuffer ? "Depth Render Target" : "Colour render target";
			renderTargetDesc.depth = 1; // 3D texture

			if (rtrt.heapsDepthBuffer != null) {
				if (tex.depthBuffer != null && (tex.depthBuffer.width != tex.width || tex.depthBuffer.height != tex.height))
					throw "Invalid depth buffer size : does not match render target size";

				renderTargetDesc.setDepthClear(1.0, 0);
			} else {}
			var rt = _renderer.createRenderTarget(renderTargetDesc);
			if (rt == null)
				throw 'RENDER TARGET  failed to create render target';

			var inBarrier = new forge.Native.ResourceBarrierBuilder();

			inBarrier.addRTBarrier(rt, RESOURCE_STATE_SHADER_RESOURCE, RESOURCE_STATE_RENDER_TARGET);

			var outBarrier = new forge.Native.ResourceBarrierBuilder();
			outBarrier.addRTBarrier(rt, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_SHADER_RESOURCE);

			rtrt.nativeRT = rt;
			rtrt.nativeDesc = renderTargetDesc;
			rtrt.inBarrier = inBarrier;
			rtrt.outBarrier = outBarrier;
			rtrt.begin = null;
			rtrt.present = null;
			rtrt.captureBuffer = null;

			rtrt.created = true;

			tex.t.t = rt.getTexture();
			tex.t.rt = rtrt;
		} else {
			DebugTrace.trace('RENDER TARGET  setting render target to existing ${rtrt.texture} w ${rtrt.texture.width} h ${rtrt.texture.height}');
		}

		_currentRT = new RenderTargetBlock();
		_currentRT.colorTargets[0] = rtrt;
		var db = rtrt.texture.depthBuffer;
		var dbr = db != null ? @:privateAccess db.b.r : null;

		_currentRT.depthTarget = dbr;

		//		curTexture = textures[0];

		var tex = rtrt.texture;
		var currentDepth = _currentRT.depthTarget;
	
		DebugTrace.trace('RENDER TARGET  setting depth buffer target to existing ${currentDepth} ');
		DebugTrace.trace('Inserting current RT in barrier');
		if (_currentRT.colorTargets[0].inBarrier != null)
			_currentCmd.insertBarrier(_currentRT.colorTargets[0].inBarrier);

		if (_currentFrame != _currentRT.colorTargets[0].lastClearedFrame) {
			tex.flags.set(WasCleared); // once we draw to, do not clear again

			DebugTrace.trace('RENDER TARGET CLEAR set render target internal cleared');
			_currentCmd.bind(_currentRT.colorTargets[0].nativeRT, currentDepth == null ? null : currentDepth.nativeRT, LOAD_ACTION_CLEAR, LOAD_ACTION_CLEAR);
			_currentRT.colorTargets[0].lastClearedFrame = _currentFrame;
		} else {
			DebugTrace.trace('RENDER TARGET CLEAR set render target internal no clear');
			_currentCmd.bind(_currentRT.colorTargets[0].nativeRT, currentDepth == null ? null : currentDepth.nativeRT, LOAD_ACTION_LOAD, LOAD_ACTION_LOAD);
		}

		if (_currentPass != null) {
			DebugTrace.trace('RENDER SELECTING DEFAULT MATERIAL');

			selectMaterial(_currentPass);
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
					DebugTrace.trace('RENDER TARGET REMINDER to clear render target [RC]');
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

	//	var _tmpTextures = new Array<h3d.mat.Texture>();

	function setDefaultRenderTarget() {
		DebugTrace.trace('RENDER CALLSTACK TARGET CLEAR setDefaultRenderTarget ');
		if (!renderReady())
			return;

		_currentCmd.unbindRenderTarget();
		DebugTrace.trace('Inserting current RT out');
		if (_currentRT.colorTargets[0].outBarrier != null)
			_currentCmd.insertBarrier(_currentRT.colorTargets[0].outBarrier);
		//		_currentRT = _sc.getRenderTarget(_currentSwapIndex);
		_currentRT = _swapRenderBlocks[_currentSwapIndex];

		if (_currentRT.colorTargets[0].inBarrier != null)
			_currentCmd.insertBarrier(_currentRT.colorTargets[0].inBarrier);
		_currentCmd.bind(_currentRT.colorTargets[0].nativeRT, _currentRT.depthTarget == null ? null : _currentRT.depthTarget.nativeRT, LOAD_ACTION_LOAD,
			LOAD_ACTION_LOAD);

		if (_currentPass != null) {
			selectMaterial(_currentPass);
		}
		
		// currentTargetResources[0] = null;

		/*
			currentTargetResources[0] = null;
			Driver.omSetRenderTargets(1, currentTargets, currentDepth.view);
			viewport[2] = outputWidth;
			viewport[3] = outputHeight;
			viewport[5] = 1.;
			Driver.rsSetViewports(1, viewport);
		 */

		//		DebugTrace.trace("RENDER REMINDER return to default RT [RC]");
	}

	public override function setRenderTarget(tex:Null<h3d.mat.Texture>, layer = 0, mipLevel = 0) {
		if (tex == null) {
			DebugTrace.trace('RENDER setRenderTarget setting default');
			setDefaultRenderTarget();
		} else {
			DebugTrace.trace('RENDER CALLSTACK TARGET setRenderTarget ${tex.name} ${tex.t}  layer ${layer} mip ${mipLevel} db ${tex != null ? tex.depthBuffer : null}');
			if (tex.t.rt == null) {
				var cfg = new RuntimeRenderTarget();
				cfg.texture = tex;
				cfg.layer = layer;
				cfg.mip = mipLevel;
				setRenderTargetsInternal(cfg);
			} else {
				setRenderTargetsInternal(tex.t.rt);
			}
		}
		_currentPipeline = null;
	}

	public override function setRenderTargets(textures:Array<h3d.mat.Texture>) {
		DebugTrace.trace('RENDER CALLSTACK TARGET setRenderTargets');

		throw "not implemented";

		var cfg = new RuntimeRenderTarget();
		cfg.texture = textures[0];
		cfg.layer = 0;
		cfg.mip = 0;

		setRenderTargetsInternal(cfg);
	}

	var _captureBuffer:haxe.io.Bytes;

	function captureSubRenderBuffer(rt:RenderTarget, pixels:hxd.Pixels, x:Int, y:Int, w:Int, h:Int) {
		if (rt == null)
			throw "Can't capture null buffer";

		_renderer.resetCmdPool(_capturePool);

		var size = rt.nativeRT.captureSize();
		DebugTrace.trace('CAPTURE Capturing buffer of size ${size} from w ${w} h ${h} at x ${x} y ${y} of format ${pixels.format}');
		if (_captureBuffer == null || size > _captureBuffer.length) {
			_captureBuffer = haxe.io.Bytes.alloc(size);
		}
		var outBuffer = @:privateAccess pixels.bytes.b;
		var outLen = pixels.bytes.length;
		var tmpBuffer = hl.Bytes.fromBytes(_captureBuffer);

		DebugTrace.trace('CAPTURE TMP BUFFER IS ${tmpBuffer} : ${StringTools.hex(tmpBuffer.address().high)}${StringTools.hex(tmpBuffer.address().low)}');
		if (!_renderer.captureAsBytes(_captureCmd, rt.nativeRT, _queue, forge.Native.ResourceState.RESOURCE_STATE_PRESENT, hl.Bytes.fromBytes(_captureBuffer),
			size)) {
			throw "Capture Failed";
		}
		var rt_w = rt.nativeRT.width;

		DebugTrace.trace('CAPTURE blitting rows of length ${w}  into an image of ${rt_w} at x ${x} y ${y}');

		if (outLen < w * h * 4)
			throw 'output buffer isn\'t big enough ${outLen} vs ${w * h * 4}';

		for (i in 0...h) {
			outBuffer.blit(i * w * 4, tmpBuffer, ((i + y) * rt_w + x) * 4, w * 4);
		}

		/*
			gl.readPixels(x, y, pixels.width, pixels.height, getChannels(curTarget.t), curTarget.t.pixelFmt, buffer);
			var error = gl.getError();
			if( error != 0 ) throw "Failed to capture pixels (error "+error+")";
			@:privateAccess pixels.innerFormat = rt.rt.format;
		 */
	}

	public override function capturePixels(tex:h3d.mat.Texture, layer:Int, mipLevel:Int, ?region:h2d.col.IBounds):hxd.Pixels {
		if (mipLevel != 0)
			throw "Capturing mip levels is unsupported";
		if (layer != 0)
			throw "Capturing layers is unsupported";

		DebugTrace.trace('RENDER TEXTURE CAPTURE TARGET t ${tex} l ${layer} m ${mipLevel} r ${region}');
		var pixels:hxd.Pixels;
		var x:Int, y:Int, w:Int, h:Int;
		if (region != null) {
			if (region.xMax > tex.width)
				region.xMax = tex.width;
			if (region.yMax > tex.height)
				region.yMax = tex.height;
			if (region.xMin < 0)
				region.xMin = 0;
			if (region.yMin < 0)
				region.yMin = 0;
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
		if (w == 0)
			w = 1;
		if (h == 0)
			h = 1;
		pixels = hxd.Pixels.alloc(w, h, tex.format);

		if (tex.t.rt == null) {
			throw "Capturing pixels from non-render target is not supported";
		} else {
			var rt = tex.t.rt;
			@:privateAccess var b = pixels.bytes.b;
			for (i in 0...pixels.bytes.length) {
				b[i] = 255;
			}

			captureSubRenderBuffer(rt, pixels, x, y, w, h);

			// rt.rt.capture( null, )
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

		return pixels;
	}

	public override function uploadVertexBytes(v:VertexBuffer, startVertex:Int, vertexCount:Int, buf:haxe.io.Bytes, bufPos:Int) {
		//		DebugTrace.trace('RENDER STRIDE UPLOAD  uploadVertexBytes start ${startVertex} vstride ${v.stride} vstridebytes ${v.strideBytes} sv ${startVertex} vc ${vertexCount} bufPos ${bufPos} buf len ${buf.length} elements');
		v.b.updateRegion(buf, startVertex * v.strideBytes, vertexCount * v.strideBytes, 0);
		//		DebugTrace.trace('Done');
	}

	public override function uploadIndexBytes(i:IndexBuffer, startIndice:Int, indiceCount:Int, buf:haxe.io.Bytes, bufPos:Int) {
		var bits = i.is32 ? 2 : 1;
		i.b.updateRegion(buf, startIndice << bits, indiceCount << bits, bufPos << bits);
	}

	public override function generateMipMaps(source:h3d.mat.Texture) {
		var mipLevels = 0;
		var mipSizeX = 1 << Std.int(Math.ceil(Math.log(source.width) / Math.log(2)));
		var mipSizeY = 1 << Std.int(Math.ceil(Math.log(source.height) / Math.log(2)));

		for (i in 0...MAX_MIP_LEVELS) {
			if (mipSizeX >> i < 1) {
				mipLevels = i;
				break;
			}
			if (mipSizeY >> i < 1) {
				mipLevels = i;
				break;
			}
		}
		if (mipLevels <= 1)
			return;

		_renderer.resetCmdPool(_computePool);

		var cmd = _computeCmd;

		cmd.begin();

		_mipGenParams.resize(2);

		for (i in 1...mipLevels) {
			_mipGenDB.clearSlotData(0);
			_mipGenDB.clearSlotData(1);

			_mipGenDB.addSlotTexture(0, source.t.t);
			_mipGenDB.setSlotUAVMipSlice(0, i - 1);
			_mipGenDB.addSlotTexture(1, source.t.t);
			_mipGenDB.setSlotUAVMipSlice(1, i);

			trace('Updating MIP Gen');
			_mipGenDB.update(_renderer, i - 1, _mipGenArrayDescriptor); // this is computation index not mip index
		}

		forge.Native.Globals.waitForAllResourceLoads();

		cmd.bindPipeline(_mipGenArray.pipeline);

		for (i in 1...mipLevels) {
			cmd.bindDescriptorSet(i - 1, _mipGenArrayDescriptor); // this is computation index not mip index

			mipSizeX >>= 1;
			mipSizeY >>= 1;
			_mipGenParams[0] = mipSizeX;
			_mipGenParams[1] = mipSizeY;

			// var mipSize = { mipSizeX, mipSizeY };

			cmd.pushConstants(_mipGen.rootsig, _mipGen.rootconstant, hl.Bytes.getArray(_mipGenParams));
			/*
				textureBarriers[0] = { pSSSR_DepthHierarchy, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_UNORDERED_ACCESS };
				cmd.resourceBarrier(cmd, 0, NULL, 1, textureBarriers, 0, NULL);
			 */
			var groupCountX = Std.int(mipSizeX / 16);
			var groupCountY = Std.int(mipSizeY / 16);
			if (groupCountX == 0)
				groupCountX = 1;
			if (groupCountY == 0)
				groupCountY = 1;
			cmd.dispatch(groupCountX, groupCountY, source.layerCount); // 2D texture, not a texture array?
		}

		cmd.end();

		// slowest possible route, but oh well
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
		switch (r) {
			case CameraHandness:
				_rightHanded = value != 0;
			default:
				throw "Not implemented";
		}
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
			DebugTrace.trace('Forge Driver Debug ${d}');
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
