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

class PipelineParamBufferBlock {
    var _vertexBytes : Int;
    var _fragmentBytes : Int;
    var _totalBytes : Int;
    
    var _depth : Int;
    var _index : Int;
    var _vbuffers : Array<Buffer>;
    var _fbuffers : Array<Buffer>;
    var _scratch : haxe.io.Bytes;
    var _dirty : Bool = true;
    var _offset : Int = 0;
    var _length : Int;
    var _writeHead : Int = 0;
    var _currentDepth : Int;
    var _ds : forge.Native.DescriptorSet;
    var _set : EDescriptorSetSlot;
    

    function new() {}
    public var depth(get,never) : Int;
    function get_depth() return _depth; 
    
    static function getQuadWordSize( bytes : Int ) {
        return Std.int((bytes + QW_BYTES - 1 ) / QW_BYTES) * QW_BYTES;

    }
    static final QW_BYTES = 4 * 4;
    public static function allocate( renderer : Renderer, rootsig : RootSignature, vBytes : Int, fBytes : Int, set : EDescriptorSetSlot, length : Int = 16, depth: Int = 3, scratchCount : Int = 1) {
        var buffer = new PipelineParamBufferBlock();
        buffer._vertexBytes = getQuadWordSize(vBytes);
        buffer._fragmentBytes = getQuadWordSize(fBytes);
        buffer._totalBytes = buffer._vertexBytes + buffer._fragmentBytes;
        buffer._set = set;

        var elementSizeBytes = buffer._totalBytes;
        buffer._depth = depth;
        buffer._length = length;

        buffer._scratch = haxe.io.Bytes.alloc(elementSizeBytes * scratchCount);
        buffer._scratch.fill(0, elementSizeBytes * scratchCount, 0);

        buffer._vbuffers = [];
        buffer._fbuffers = [];
        
        var vd = new BufferLoadDesc();
        vd.setUniformBuffer( buffer._vertexBytes, buffer._scratch ); // all zeros
        vd.setUsage(true);
        vd.setDynamic(depth);

        var fd = new BufferLoadDesc();
        fd.setUniformBuffer( buffer._fragmentBytes, buffer._scratch );
        fd.setUsage(true);
        fd.setDynamic(depth);

        for (i in 0...length) {
            buffer._vbuffers.push(vd.load(null));
            buffer._fbuffers.push(fd.load(null));
        }

        var setDesc = new forge.Native.DescriptorSetDesc();
		setDesc.pRootSignature = rootsig;
		setDesc.setIndex = set;
		setDesc.maxSets = buffer._depth * buffer._length; // 2^13 = 8192
		buffer._ds = renderer.addDescriptorSet( setDesc );   

        var vidx = rootsig.getDescriptorIndexFromName( "_" + GLSLTranscoder.getVariableBufferName(VERTEX, set));
        var fidx = rootsig.getDescriptorIndexFromName( "_" + GLSLTranscoder.getVariableBufferName(FRAGMENT, set));

        for (l in 0...length) {
            for (d in 0...depth) {
                var builder = new DescriptorDataBuilder();
                var ds_idx = l * depth + d;
                if (vBytes > 0 && vidx >= 0) {
                    var s0 = builder.addSlot( DBM_UNIFORMS );     
                    builder.setSlotBindIndex(s0, vidx);
                    builder.addSlotUniformBuffer( s0, @:privateAccess buffer._vbuffers[l].get(d) );
                }
                if (fBytes > 0 && fidx >= 0) {
                    var s1 = builder.addSlot( DBM_UNIFORMS );     
                    builder.setSlotBindIndex(s1, fidx);
                    builder.addSlotUniformBuffer( s1, @:privateAccess buffer._vbuffers[l].get(d) );	
                }
                DebugTrace.trace( 'RENDER DESCRIPTORS PipelineParamBufferBlock Creating with ${l} length, ${d} depth ${ds_idx} index');
    
                builder.update( renderer, ds_idx, buffer._ds);
            }
        }
       
        return buffer;
    }
    
    public function nextLayer() {
        _writeHead = 0;
        _currentDepth = (_currentDepth + 1) % _depth;
    }

    public function full() : Bool {
        return _writeHead == _length;
    }

    public function beginUpdate() : Bool {
        if (_writeHead == _length) {
            return false;
        }
        return true;
    }

    public function fill( vdata : hl.Bytes,  fdata : hl.Bytes ) {
        DebugTrace.trace('RENDER Filling ${_writeHead} depth ${_currentDepth} ${_vertexBytes} ${_fragmentBytes}');
        if (vdata != null) {
            _vbuffers[_writeHead].setCurrent(_currentDepth);
            _vbuffers[_writeHead].update(vdata);
        }
        if (fdata != null) {
            _fbuffers[_writeHead].setCurrent(_currentDepth);
            _fbuffers[_writeHead].update(fdata);
        }
    }

    public function bind(cmd: forge.Native.Cmd) {
        var idx = _currentDepth * _length + _writeHead;
        DebugTrace.trace('RENDER Binding descriptor set ${_set} to idx ${idx} head ${_writeHead} depth ${_currentDepth}');
        cmd.bindDescriptorSet(idx, _ds);
    }
    public function next() : Bool {
        _writeHead++;

        if (_writeHead == _length) {
            return false;
        }
        return true;
    }
}