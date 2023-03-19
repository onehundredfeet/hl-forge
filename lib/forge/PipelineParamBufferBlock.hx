// This file is written entirely in the haxe programming language.
package forge;

import hl.Bytes;
import forge.Native.BufferLoadDesc;
import forge.Native.Buffer;
import forge.Native.RootSignature;
import forge.Native.DescriptorSetDesc;
import forge.Native.DescriptorSet;
import forge.Native.Renderer;
import forge.Native.DescriptorSlotMode;

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
    var _ds : forge.Native.DescriptorSet;
    function new() {}
    public var depth(get,never) : Int;
    function get_depth() return _depth; 
    
    static function getQuadWordSize( bytes : Int ) {
        return Std.int((bytes + QW_BYTES - 1 ) / QW_BYTES) * QW_BYTES;

    }
    static final QW_BYTES = 4 * 4;
    public static function allocate( renderer : Renderer, rootsig : RootSignature, vBytes : Int, fBytes : Int, set : Int, length : Int = 16, depth: Int = 3, scratchCount : Int = 1) {
        var buffer = new PipelineParamBufferBlock();
        buffer._vertexBytes = getQuadWordSize(vBytes);
        buffer._fragmentBytes = getQuadWordSize(fBytes);
        buffer._totalBytes = buffer._vertexBytes + buffer._fragmentBytes;

        var elementSizeBytes = buffer._totalBytes;
        buffer._depth = depth;
        buffer._length = length;

        buffer._scratch = haxe.io.Bytes.alloc(elementSizeBytes * scratchCount);
        buffer._scratch.fill(0, elementSizeBytes * scratchCount, 0);

        buffer._vbuffers = [];
        
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

        return buffer;
    }
    
    public function reset() {
        _writeHead = -1;
    }

    public function full() : Bool {
        return _writeHead == _length;
    }

    public function beginUpdate() : Int {
        _writeHead ++;
        if (_writeHead == _length) {
            return -1;
        }
        return _writeHead;
    }

    public function fill( vdata : haxe.io.Bytes,  fdata : haxe.io.Bytes ) {
        if (vdata != null)
            _vbuffers[_writeHead].update(vdata);
        if (fdata != null)
            _fbuffers[_writeHead].update(fdata);
    }

}