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
    var _buffers : Array<Buffer>;
    var _cache : haxe.io.Bytes;
    var _dirty : Bool = true;
    var _offset : Int = 0;
    function new() {}

    public static function allocate( byteCount : Int, depth: Int, ? bytes : Bytes ) {
        var buffer = new DynamicUniformBuffer();
        buffer._byteCount = byteCount;
        buffer._depth = depth;

        buffer._cache = haxe.io.Bytes.alloc(byteCount);
        buffer._buffers = new Array<Buffer>();
        
        if( bytes != null ) {
            var bts : Bytes = buffer._cache;
            bts.blit(0, bytes, 0, byteCount);
        }

        for (i in 0...depth) {
            var ld = new BufferLoadDesc();
            ld.setUniformBuffer( byteCount, buffer._cache );
            ld.setUsage(true);
            buffer._buffers.push( ld.load(null) );
        }

        return buffer;
    }
    
    public function createDescriptors(renderer : Renderer, rootsig : RootSignature, slotIndex : Int, frequency: forge.Native.DescriptorUpdateFrequency) : DescriptorSet {
        var setDesc = new forge.Native.DescriptorSetDesc();
		setDesc.pRootSignature = rootsig;
		setDesc.updateFrequency = frequency;
		setDesc.maxSets = _buffers.length; // 2^13 = 8192
		var ds = renderer.addDescriptorSet( setDesc );
        
        for( i in 0..._buffers.length ) {
            trace('Buffer length ${i} / ${_buffers.length}}');
            var builder = new forge.Native.DescriptorDataBuilder();
            var s = builder.addSlot( DescriptorSlotMode.DBM_UNIFORMS);        
            builder.setSlotBindIndex(s, slotIndex);
            builder.addSlotUniformBuffer( s, _buffers[i] );
            builder.update( renderer, i, ds);
        }
        
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
        if( _dirty ) {
            _buffers[_index].update( _cache );
            _dirty = false;
        }
    }

    public function next() : Buffer {
        _index = (_index + 1) % _depth;
        _offset = 0;
        return _buffers[_index];
    }

    public function current() : Buffer {
        return _buffers[_index];
    }
    public function currentIdx() : Int {
        return _index;
    }

}