package forge;

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
import forge.DebugTrace;
import forge.EnumTools;


typedef DescriptorIndex = Null<Int>;
typedef ShaderProgram = forge.Forge.Program;
typedef ForgeShader = forge.Native.Shader;


class CompiledTextureRef {
	public function new() {}

	public var index:Int;
    public var kind:hxsl.Ast.Type;
}

class CompiledTextureArray {
	public function new() {}

//	public var textureIndex:DescriptorIndex;
//	public var samplerIndex:DescriptorIndex;
//	public var textures:Array<CompiledTextureRef>;
    public var count:Int = 0;
}

class CompiledArgBuffer {
	public function new() {}

	public var index:DescriptorIndex;
	public var descriptorSetIndex:DescriptorIndex;
	public var programDescriptorSet:forge.Native.DescriptorSet;
}


@:enum abstract ECompiledArgBuffer(Int) from Int to Int {
	var GLOBALS = 0;
	var PARAMS = 1;
	var PUSH = 2;
}

@:enum
abstract CompiledAttributeType(Int) from Int to Int {
	public var UNKNOWN = 0;
	public var BYTE = 1;
	public var FLOAT = 2;
	public var VECTOR = 3;
}

class CompiledAttribute {
	public var index:Int;
	public var type:CompiledAttributeType;
	public var sizeBytes:Int;
	public var count:Int;
	public var divisor:Int;
	public var offsetBytes:Int;
	public var name:String;
	public var strideBytes:Int;
	public var semantic:forge.Native.ShaderSemantic;

	public function new() {}
}


class CompiledShader {
	public var s:ForgeShader;
	public var vertex:Bool;
	public var constantsIndex:DescriptorIndex;
	public var globalsIndex:DescriptorIndex;
	public var globalsDescriptorSetIndex:DescriptorIndex;
	public var globalProgramDescriptorSet:forge.Native.DescriptorSet;
	public var params:DescriptorIndex;
	public var globalsLength:Int;
	public var paramsLengthFloats:Int;
	public var paramsLengthBytes:Int;
	public var bufferCount:Int;
	public var buffers:Array<DescriptorIndex>;
	public var shader:hxsl.RuntimeShader.RuntimeShaderData;
	public var name : String;
//	public var textureArrays:Array<CompiledTextureArray>;
	public var argBuffers:Array<CompiledArgBuffer>;
    public var shaderVarNames : Map<Int, String>;
	/*
		public var textures:Array<{u:DescriptorIndex, t:hxsl.Ast.Type, mode:Int}>;
		public var texturesCubes:Array<{u:DescriptorIndex, t:hxsl.Ast.Type, mode:Int}>;
		public var textureIndex:DescriptorIndex;
		public var textureCubeIndex:DescriptorIndex;
		public var samplerIndex:DescriptorIndex;
		public var samplerCubeIndex:DescriptorIndex;
		public var textureSlot:Int;
		public var samplerSlot:Int;
	 */
	public var md5:String;
	public var globalsBuffer:DynamicUniformBuffer;
	public var globalsLastUpdated:Int = -1;
/*
	public function textureCount(array:ETextureType):Int {
		if (textureArrays == null)
			return 0;
		var idx:Int = array;
		if (idx >= textureArrays.length)
			return 0;

		return textureArrays[idx].count;
	}

	public function textureTotalCount():Int {
		if (textureArrays == null)
			return 0;
		var total = 0;
		for (i in 0...textureArrays.length) {
			if (textureArrays[i] != null) {
				total += textureArrays[i].count;
			}
		}
		return total;
	}*/

    public inline function textureTotalCount():Int {
		return shader.texturesCount;
    }
	

	public function hasTextures():Bool {
        return shader.texturesCount > 0;
//		return textureArrays != null;
	}

    /*
	public function getTextureArray(array:ETextureType):CompiledTextureArray {
		if (textureArrays == null)
			return null;
		var idx:Int = array;
		if (idx >= textureArrays.length)
			return null;
		return textureArrays[idx];
	}

	public function getOrCreateTextureArray(array:ETextureType):CompiledTextureArray {
		if (textureArrays == null)
			textureArrays = new Array<CompiledTextureArray>();
		var arrayIdx = Std.int(array);
		if (textureArrays.length <= arrayIdx)
			textureArrays[arrayIdx] = new CompiledTextureArray();
		return textureArrays[arrayIdx];
	}
*/
	//	public var textureDataBuilder:forge.Native.DescriptorDataBuilder;
	//	public var samplerDataBuilder:forge.Native.DescriptorDataBuilder;

	public function new(s, vertex, shader) {
		this.s = s;
		this.vertex = vertex;
		this.shader = shader;
		this.bufferCount = shader.bufferCount;
		shader.buffers;
	}
}
