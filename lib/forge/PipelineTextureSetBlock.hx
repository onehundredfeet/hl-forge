// This file is written entirely in the haxe programming language.
package forge;

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
    var _builder : DescriptorDataBuilder;
    var _samplers: Array<forge.Native.Sampler>;
	function new() {}

	public var depth(get, never):Int;

	function get_depth()
		return _depth;

	static function getQuadWordSize(bytes:Int) {
		return Std.int((bytes + QW_BYTES - 1) / QW_BYTES) * QW_BYTES;
	}

	static final QW_BYTES = 4 * 4; // 4 components of 4 bytes each, x, y, z w

	static final VERTEX_TYPE_NAME = [
		for (i in 0...GLSLTranscoder.ETextureType.TEX_TYPE_COUNT) "_" + GLSLTranscoder.getTextureArrayName(VERTEX, i, false)
	];
	static final FRAGMENT_TYPE_NAME = [
		for (i in 0...GLSLTranscoder.ETextureType.TEX_TYPE_COUNT) "_" + GLSLTranscoder.getTextureArrayName(FRAGMENT, i, false)
	];
	static final VERTEX_SAMPLER_TYPE_NAME = [
		for (i in 0...GLSLTranscoder.ETextureType.TEX_TYPE_COUNT) "_" + GLSLTranscoder.getTextureArrayName(VERTEX, i, true)
	];
	static final FRAGMENT_SAMPLER_TYPE_NAME = [
		for (i in 0...GLSLTranscoder.ETextureType.TEX_TYPE_COUNT) "_" + GLSLTranscoder.getTextureArrayName(FRAGMENT, i, true)
	];

	public static function allocate(renderer:Renderer, rootsig:RootSignature, vertex:CompiledShader, fragment:CompiledShader, samplers : Array<forge.Native.Sampler>, length:Int = 16, depth:Int = 3) {
		var buffer = new PipelineTextureSetBlock();
        buffer._samplers = samplers;

		trace('RENDER PIPELINE TEXTURE BLOCK Allocating  with ${vertex.textureTotalCount()} vertex textures, ${fragment.textureTotalCount()}  fragment textures ${length} length, ${depth} depth');
		//        buffer._vertexBytes = getQuadWordSize(vBytes);
		//      buffer._fragmentBytes = getQuadWordSize(fBytes);

		buffer._depth = depth;
		buffer._length = length;

		var setDesc = new forge.Native.DescriptorSetDesc();
		setDesc.pRootSignature = rootsig;
		setDesc.setIndex = EDescriptorSetSlot.TEXTURES;
		setDesc.maxSets = buffer._depth * buffer._length; // 2^13 = 8192
		buffer._ds = renderer.addDescriptorSet(setDesc);

		// this can be moved elsewhere to avoid repetition
		var vidx = [];
		var fidx = [];
		var vidx_smplr = [];
		var fidx_smplr = [];
        trace('RENDER TEXTURE GETTING DESCRPITOR INDEXES');

		for (i in 0...GLSLTranscoder.ETextureType.TEX_TYPE_COUNT) {
			vidx.push(rootsig.getDescriptorIndexFromName(VERTEX_TYPE_NAME[i]));
			fidx.push(rootsig.getDescriptorIndexFromName(FRAGMENT_TYPE_NAME[i]));
			vidx_smplr.push(rootsig.getDescriptorIndexFromName(VERTEX_SAMPLER_TYPE_NAME[i]));
			fidx_smplr.push(rootsig.getDescriptorIndexFromName(FRAGMENT_SAMPLER_TYPE_NAME[i]));

            trace('\tRENDER TEXTURE TYPE ${i} ${VERTEX_TYPE_NAME[i]}:${vidx[i]} ${FRAGMENT_TYPE_NAME[i]}:${fidx[i]} ${VERTEX_SAMPLER_TYPE_NAME[i]}:${vidx_smplr[i]} ${FRAGMENT_SAMPLER_TYPE_NAME[i]}:${fidx_smplr[i]}');
		}

        buffer._builder = new DescriptorDataBuilder();
        var builder = buffer._builder;

        var vtxAdded = 0;
        var fragAdded = 0;
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

		return buffer;
	}

	public function nextLayer() {
		_writeHead = 0;
		_currentDepth = (_currentDepth + 1) % _depth;
	}

	public function full():Bool {
		return _writeHead == _length;
	}

	public function beginUpdate():Bool {
		if (_writeHead == _length) {
			return false;
		}
		return true;
	}

	public function fill(renderer: Renderer, vertex:Array< Array<h3d.mat.Texture>>, fragment:Array< Array<h3d.mat.Texture>>) {
		DebugTrace.trace('RENDER Filling texture record with wh ${_writeHead} depth ${_currentDepth}');
        var slotIdx = 0;
        for (i in 0...GLSLTranscoder.ETextureType.TEX_TYPE_COUNT) {					
            DebugTrace.trace('RENDER Filling shader type ${i} has ${vertex[i].length} vertex textures and ${fragment[i].length} fragment textures');
            if (vertex[i].length > 0) {
                trace('RENDER Filling vertex type ${i} has ${vertex[i].length} textures on ${slotIdx}');
                for (j in 0...vertex[i].length) {
                    _builder.setSlotTexture(slotIdx, j, @:privateAccess vertex[i][j].t.t);
//                    _builder.setSlotSampler(slotIdx + 1, j, _samplers[i]);
                }
                slotIdx += 2;
            }
            if (fragment[i].length > 0) {
                trace('RENDER Filling fragment type ${i} has ${fragment[i].length} textures on ${slotIdx}');
                for (j in 0...fragment[i].length) {
                    _builder.setSlotTexture(slotIdx, j, @:privateAccess fragment[i][j].t.t);
//                    _builder.setSlotSampler(slotIdx + 1, j, _samplers[i]);
                }
                slotIdx += 2;
            }
        }
        var idx = _currentDepth *  _writeHead + _currentDepth;
        trace('RENDER Updating texture descriptor idx ${idx} head ${_writeHead} depth ${_currentDepth} ds ${_ds}');
        _builder.update( renderer, idx, _ds);
	}

	public function bind(cmd:forge.Native.Cmd) {
		var idx = _currentDepth * _writeHead + _currentDepth;

		DebugTrace.trace('RENDER Binding texture descriptor idx ${idx} head ${_writeHead} depth ${_currentDepth}');
		cmd.bindDescriptorSet(idx, _ds);
	}

	public function next():Bool {
		_writeHead++;

		if (_writeHead == _length) {
			return false;
		}
		return true;
	}
}
