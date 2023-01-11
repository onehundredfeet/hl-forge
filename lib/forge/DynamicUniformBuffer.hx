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

class DynamicUniformBuffer {
    var _byteCount : Int;
    var _depth : Int;
    var _index : Int;
    var _buffers : Buffer;
    var _cache : haxe.io.Bytes;
    var _dirty : Bool = true;
    var _offset : Int = 0;
    function new() {}

    public static function allocate( byteCount : Int, depth: Int, ? bytes : Bytes ) {
        var buffer = new DynamicUniformBuffer();
        buffer._byteCount = byteCount;
        buffer._depth = depth;

        buffer._cache = haxe.io.Bytes.alloc(byteCount);
        
        if( bytes != null ) {
            var bts : Bytes = buffer._cache;
            bts.blit(0, bytes, 0, byteCount);
        }

        var ld = new BufferLoadDesc();
        ld.setUniformBuffer( byteCount, buffer._cache );
        ld.setUsage(true);
        ld.setDynamic(depth);
        buffer._buffers = ld.load(null);

        return buffer;
    }
    
    public function createDescriptors(renderer : Renderer, rootsig : RootSignature, slotIndex : Int, frequency: forge.Native.DescriptorUpdateFrequency) : DescriptorSet {
        var setDesc = new forge.Native.DescriptorSetDesc();
		setDesc.pRootSignature = rootsig;
		setDesc.updateFrequency = frequency;
		setDesc.maxSets = _depth; // 2^13 = 8192
		var ds = renderer.addDescriptorSet( setDesc );
        
        renderer.fillDescriptorSet( _buffers, ds, DescriptorSlotMode.DBM_UNIFORMS, slotIndex);

        return ds;
    }

    public function push( data : hl.Bytes,  length: Int, srcOffset : Int = 0) {
        if( _offset + length > _byteCount ) {
            throw 'DynamicUniformBuffer overflow: ${_offset} + ${length} > ${_byteCount}';
        }
//        trace('Blitting ${length} from ${srcOffset} to ${_offset} with a max of ${_byteCount} into ${_cache} [${_cache.length}]]');
        var bts : Bytes = _cache;
        bts.blit( _offset, data, srcOffset, length );
        _dirty = true;
        _offset += length;
    }

    public function sync() {
        _buffers.update( _cache );
    }

    public function next() {
        _offset = 0;
        return _buffers.next();
    }


    public function currentIdx() : Int {
        return _buffers.currentIdx();
    }

}