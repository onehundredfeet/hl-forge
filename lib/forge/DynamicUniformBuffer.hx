// This file is written entirely in the haxe programming language.
package forge;

import hl.Bytes;
import forge.Native.BufferLoadDesc;
import forge.Native.Buffer;

class DynamicUniformBuffer {
    var _byteCount : Int;
    var _depth : Int;
    var _index : Int;
    var _buffers : Array<Buffer>;
    var _cache : Array<hl.UI8>;
    var _dirty : Bool = true;
    function new() {}

    public static function allocate( byteCount : Int, depth: Int, ? bytes : Bytes ) {
        var buffer = new DynamicUniformBuffer();
        buffer._byteCount = byteCount;
        buffer._depth = depth;

        buffer._cache = new Array<hl.UI8>();
        buffer._cache.resize(byteCount);

        if( bytes != null ) {
            Bytes.getArray(buffer._cache).blit(0, bytes, 0, byteCount);
        }

        for (i in 0...depth) {
            var ld = new BufferLoadDesc();
            ld.setUnformBuffer( byteCount, Bytes.getArray(buffer._cache), true );
            buffer._buffers.push( ld.load(null) );
        }

        return buffer;
    }
    
    public function copy( data : hl.Bytes,  length: Int, offset : Int = 0) {
        if( offset + length > _byteCount ) {
            throw "DynamicUniformBuffer overflow";
        }
        Bytes.getArray(_cache).blit( offset, data, 0, offset );
        _dirty = true;
    }

    public function sync() {
        if( _dirty ) {
            _buffers[_index].update( Bytes.getArray(_cache) );
            _dirty = false;
        }
    }

    public function next() : Buffer {
        _index = (_index + 1) % _depth;
        return _buffers[_index];
    }

    public function current() : Buffer {
        return _buffers[_index];
    }

}