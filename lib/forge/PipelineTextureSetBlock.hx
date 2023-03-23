// This file is written entirely in the haxe programming language.
package forge;

import haxe.ds.Vector;
import forge.GLSLTranscoder.EDescriptorSetSlot;
import hl.Bytes;
import forge.Native.BufferLoadDesc;
import forge.Native.Buffer;
import forge.Native.RootSignature;
import forge.Native.DescriptorSetDesc;
import forge.Native.DescriptorSet;
import forge.Native.Renderer;
import forge.Native.DescriptorSlotMode;
import forge.Native.DescriptorDataBuilder;

class PipelineTextureSetBlock {
	var _depth:Int;
	var _index:Int;
	var _dirty:Bool = true;
	var _offset:Int = 0;
	var _length:Int;
	var _writeHead:Int = 0;
	var _currentDepth:Int;
	var _ds:forge.Native.DescriptorSet;
	var _builder:DescriptorDataBuilder;
	var _samplers:Array<forge.Native.Sampler>;

	function new() {}

	public var depth(get, never):Int;

	function get_depth()
		return _depth;

	static function getQuadWordSize(bytes:Int) {
		return Std.int((bytes + QW_BYTES - 1) / QW_BYTES) * QW_BYTES;
	}

	static final QW_BYTES = 4 * 4; // 4 components of 4 bytes each, x, y, z w

	static final VERTEX_TYPE_NAME = [
		for (i in 0...GLSLTranscoder.ETextureType.TEX_TYPE_COUNT)
			"_" + GLSLTranscoder.getTextureArrayName(VERTEX, i, false)
	];
	static final FRAGMENT_TYPE_NAME = [
		for (i in 0...GLSLTranscoder.ETextureType.TEX_TYPE_COUNT)
			"_" + GLSLTranscoder.getTextureArrayName(FRAGMENT, i, false)
	];
	static final VERTEX_SAMPLER_TYPE_NAME = [
		for (i in 0...GLSLTranscoder.ETextureType.TEX_TYPE_COUNT)
			"_" + GLSLTranscoder.getTextureArrayName(VERTEX, i, true)
	];
	static final FRAGMENT_SAMPLER_TYPE_NAME = [
		for (i in 0...GLSLTranscoder.ETextureType.TEX_TYPE_COUNT)
			"_" + GLSLTranscoder.getTextureArrayName(FRAGMENT, i, true)
	];

	var _vcs:CompiledShader;
	var _fcs:CompiledShader;

	public static function allocate(renderer:Renderer, rootsig:RootSignature, vertex:CompiledShader, fragment:CompiledShader,
			samplers:Array<forge.Native.Sampler>, length:Int = 16, depth:Int = 3) {
		var buffer = new PipelineTextureSetBlock();
		buffer._samplers = samplers;
		buffer._vcs = vertex;
		buffer._fcs = fragment;

		//        buffer._vertexBytes = getQuadWordSize(vBytes);
		//      buffer._fragmentBytes = getQuadWordSize(fBytes);

		buffer._depth = depth;
		buffer._length = length;

		var setDesc = new forge.Native.DescriptorSetDesc();
		setDesc.pRootSignature = rootsig;
		setDesc.setIndex = EDescriptorSetSlot.TEXTURES;
		setDesc.maxSets = buffer._depth * buffer._length; // 2^13 = 8192
		DebugTrace.trace('FRENDER PIPELINE TEXTURE BLOCK Allocating  with ${vertex.textureTotalCount()} vertex textures, ${fragment.textureTotalCount()}  fragment textures ${length} length, ${depth} depth');
		buffer._ds = renderer.addDescriptorSet(setDesc);
		DebugTrace.trace('FRENDER PIPELINE TEXTURE BLOCK Done');

		// this can be moved elsewhere to avoid repetition
		var vidx = [];
		var fidx = [];
		var vidx_smplr = [];
		var fidx_smplr = [];
		DebugTrace.trace('RENDER TEXTURE GETTING DESCRPITOR INDEXES');

		var funiques_id = [];
		var funiques_param = [];

		for (i in 0...GLSLTranscoder.ETextureType.TEX_TYPE_COUNT) {
			vidx.push(rootsig.getDescriptorIndexFromName(VERTEX_TYPE_NAME[i]));
			fidx.push(rootsig.getDescriptorIndexFromName(FRAGMENT_TYPE_NAME[i]));
			vidx_smplr.push(rootsig.getDescriptorIndexFromName(VERTEX_SAMPLER_TYPE_NAME[i]));
			fidx_smplr.push(rootsig.getDescriptorIndexFromName(FRAGMENT_SAMPLER_TYPE_NAME[i]));

			DebugTrace.trace('\tRENDER TEXTURE TYPE ${i} ${VERTEX_TYPE_NAME[i]}:${vidx[i]} ${FRAGMENT_TYPE_NAME[i]}:${fidx[i]} ${VERTEX_SAMPLER_TYPE_NAME[i]}:${vidx_smplr[i]} ${FRAGMENT_SAMPLER_TYPE_NAME[i]}:${fidx_smplr[i]}');
		}

		buffer._builder = new DescriptorDataBuilder();
		var builder = buffer._builder;

		var tt = fragment.shader.textures;

		for (i in 0...fragment.shader.texturesCount) {
			var found = false;
			var ts = builder.addSlot(DBM_TEXTURES);
			var ss = builder.addSlot(DBM_SAMPLERS);

			for (j in 0...GLSLTranscoder.ETextureType.TEX_TYPE_COUNT) {
				if (tt.name == FRAGMENT_TYPE_NAME[j]) {
					var id = rootsig.getDescriptorIndexFromName(FRAGMENT_TYPE_NAME[j]);
					DebugTrace.trace('RENDER PACKED TEXTURE id ${i} ${tt.name} ${tt.type} ${id}');
					found = true;
					if (id == -1)
						throw 'RENDER PACKED TEXTURE NOT FOUND ${tt.name}';
					break;
				}
			}

			if (!found) {
				//            DebugTrace.trace('\tRENDER TEXTURE FRAGMENT ${i} ${tt.name} ${tt.type}');
				switch (tt.type) {
					case TTexture2D, TSampler2D, TChannel(_):
						var tname = (tt.pos == -1) ? tt.name : fragment.shaderVarNames.get(tt.pos);
						var id = rootsig.getDescriptorIndexFromName(tname);
						var ids = rootsig.getDescriptorIndexFromName(tname + "Smplr");
						DebugTrace.trace('\tRENDER UNIQUE TEXTURE id ${i} name ${tt.name} ppos ${tt.pos} look name ${tname} type ${tt.type} shader id ${id}');
						funiques_id.push(id);
						funiques_param.push(tt);
						found = true;
						if (id == -1) {
							DebugTrace.trace('ALLOCATE RENDER UNIQUE TEXTURE NOT FOUND ${tt.name}');
							throw 'ALLOCATE RENDER UNIQUE TEXTURE NOT FOUND ${tt.name}';
						}
						builder.addSlotTexture(ts, null);
						builder.setSlotBindIndex(ts, id);

						builder.addSlotSampler(ss, samplers[i]);
						builder.setSlotBindIndex(ss, ids);
					case TArray(_):
					default:
				}
			} else {
				throw "Packed path not supported ATM";
			}

			if (!found)
				throw 'Unhandled fragment texture ${tt.name}';

			tt = tt.next;
		}
		/*
			// Set up builder archetype
			for (i in 0...GLSLTranscoder.ETextureType.TEX_TYPE_COUNT) {					
				if (vidx[i] >= 0) {
					var ts = builder.addSlot(DBM_TEXTURES);
					builder.setSlotBindIndex(ts, vidx[i]);
					for (j in 0...vertex.textureCount(i)) {
						builder.addSlotTexture(ts, null);
					}

					var tss = builder.addSlot(DBM_SAMPLERS);
					builder.setSlotBindIndex(tss, vidx_smplr[i]);
					for (j in 0...vertex.textureCount(i)) {
						builder.addSlotSampler(tss, samplers[i]);
					}
				} else if (vertex.textureCount(i) > 0) {
					throw ('Unfound vertex texture type ${i} should have ${vertex.textureCount(i)} textures');
				}

				if (fidx[i] >= 0) {
					var ts = builder.addSlot(DBM_TEXTURES);
					builder.setSlotBindIndex(ts, fidx[i]);
					for (j in 0...fragment.textureCount(i)) {
						builder.addSlotTexture(ts, null);
					}

					var tss = builder.addSlot(DBM_SAMPLERS);
					builder.setSlotBindIndex(tss, fidx_smplr[i]);
					for (j in 0...fragment.textureCount(i)) {
						builder.addSlotSampler(tss, samplers[i]);
					}
				} else if (vertex.textureCount(i) > 0) {
					throw ('Unfound fragment texture type ${i} should have ${fragment.textureCount(i)} textures');
				}
			}
		 */

		return buffer;
	}

	var _filled = -1;
	var _filling = -1;

	var _bound = false;

	public function nextLayer() {
		DebugTrace.trace('RENDER TEXTURE SET BLOCK ADVANCING LAYER  cd ${_currentDepth} d ${_depth} head ${_writeHead} filled ${_filled} bound ${_bound} filling ${_filling}');
		_writeHead = 0;
		_filled = -1;
		_bound = false;
		_filling = -1;

		_currentDepth = (_currentDepth + 1) % _depth;
	}

	public function full():Bool {
		return _writeHead == _length;
	}

	public function beginUpdate():Bool {
		if (_writeHead == _length) {
			return false;
		}
		if (_filled >= _writeHead)
			throw "Already filled this texture descriptor";

		_filling = _writeHead;
		_bound = false;

		return true;
	}

	public function fill(renderer:Renderer, vertex:Vector<h3d.mat.Texture>, fragmentTex:Vector<h3d.mat.Texture>) {
		var tt = _fcs.shader.textures;

		if (_filling != _writeHead)
			throw "filling the rong descriptor";
		if (_bound)
			throw "has already been bound";
		if (_filled == _writeHead)
			throw 'Descriptor ${_ds} [${_writeHead}] has already been filled';

		DebugTrace.trace('RENDER fill texture decriptor _filling ${_filling} filled ${_filled}bound  ${_bound}');

		var slotIdx = 0;
		for (i in 0..._fcs.shader.texturesCount) {
			var found = false;

			for (j in 0...GLSLTranscoder.ETextureType.TEX_TYPE_COUNT) {
				if (tt.name == FRAGMENT_TYPE_NAME[j]) {
					found = true;
					break;
				}
			}

			if (!found) {
				//            DebugTrace.trace('\tRENDER TEXTURE FRAGMENT ${i} ${tt.name} ${tt.type}');
				switch (tt.type) {
					case TTexture2D, TSampler2D, TChannel(_):
						var t = @:privateAccess fragmentTex[i].t;
						if (t == null)
							throw "Texture is null";
						var ft = (t.rt != null) ? t.rt.nativeRT.getTexture() : t.t;

						if (@:privateAccess ft == null) {
							throw 'FILL RENDER UNIQUE TEXTURE NOT FOUND ${tt.name} -> fragmentTex[i] ${fragmentTex[i].name} |  rt ${t.rt} | tt ${t.t}';
						}
						_builder.setSlotTexture(slotIdx, 0, ft);
						found = true;
					case TArray(_):
					default:
				}
			} else {
				throw "Packed path not supported ATM";
			}

			if (!found)
				throw 'Unhandled fragment texture ${tt.name}';

			slotIdx += 2;
			tt = tt.next;
		}

		// actual update
		var idx = _depth * _writeHead + _currentDepth;
		DebugTrace.trace('RENDER Updating texture descriptor idx ${idx} head ${_writeHead} depth ${_currentDepth} ds ${_ds}');
		_builder.update(renderer, idx, _ds);
		_filled = _writeHead;
	}

	#if old_new
	public function fill(renderer:Renderer, vertex:Array<Array<h3d.mat.Texture>>, fragment:Array<Array<h3d.mat.Texture>>) {
		DebugTrace.trace('RENDER Filling texture record with wh ${_writeHead} depth ${_currentDepth}');
		var slotIdx = 0;
		for (i in 0...GLSLTranscoder.ETextureType.TEX_TYPE_COUNT) {
			DebugTrace.trace('RENDER Filling shader type ${i} has ${vertex[i].length} vertex textures and ${fragment[i].length} fragment textures');
			if (vertex[i].length > 0) {
				DebugTrace.trace('RENDER Filling vertex type ${i} has ${vertex[i].length} textures on ${slotIdx}');
				for (j in 0...vertex[i].length) {
					_builder.setSlotTexture(slotIdx, j, @:privateAccess vertex[i][j].t.t);
					//                    _builder.setSlotSampler(slotIdx + 1, j, _samplers[i]);
				}
				slotIdx += 2;
			}
			if (fragment[i].length > 0) {
				DebugTrace.trace('RENDER Filling fragment type ${i} has ${fragment[i].length} textures on ${slotIdx}');
				for (j in 0...fragment[i].length) {
					_builder.setSlotTexture(slotIdx, j, @:privateAccess fragment[i][j].t.t);
					//                    _builder.setSlotSampler(slotIdx + 1, j, _samplers[i]);
				}
				slotIdx += 2;
			}
		}
		var idx = _depth * _writeHead + _currentDepth;
		DebugTrace.trace('RENDER Updating texture descriptor idx ${idx} head ${_writeHead} depth ${_currentDepth} ds ${_ds}');
		_builder.update(renderer, idx, _ds);
	}
	#end

	public function bind(cmd:forge.Native.Cmd) {
		if (_bound)
			throw "duplicate binding";
		var idx = _depth * _writeHead + _currentDepth;

		DebugTrace.trace('RENDER Binding texture descriptor idx ${idx} head ${_writeHead} depth ${_currentDepth}');
		cmd.bindDescriptorSet(idx, _ds);
		_bound = true;
	}

	public function next():Bool {
		if (!_bound || _filled != _writeHead)
			throw "Not filled in or bound";
		_writeHead++;
		if (_writeHead == _length) {
			return false;
		}
		return true;
	}
}
