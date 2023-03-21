package forge;

import forge.CompiledShader;

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


class CompiledProgram {
	public var id:Int;
	public var p:ShaderProgram;
	public var rootSig:forge.Native.RootSignature;
	public var forgeShader:forge.Native.Shader;
	public var vertex:CompiledShader;
	public var fragment:CompiledShader;
	public var globalDescriptorSet:forge.Native.DescriptorSet;
	public var inputs:InputNames;
	public var naturalLayout:forge.Native.VertexLayout;
	public var attribs:Array<CompiledAttribute>;
	public var hasAttribIndex:Array<Bool>;
	public var paramBuffers = new Array<forge.PipelineParamBufferBlock>();
	public var paramCurBuffer = 0;
    public var textureBlocks = new Array<forge.PipelineTextureSetBlock>();
    public var textureCurBlock = 0;

	public var lastFrame = -1;

	public function new() {}

	public function resetBlocks() {
		paramCurBuffer = 0;
		for (p in paramBuffers) {
			p.nextLayer();
		}
        textureCurBlock = 0;
        for (t in textureBlocks) {
			t.nextLayer();
		}
	}
}
