package h3d.impl;
import forge.RuntimeRenderTarget;

typedef IndexBufferExt = { b : sdl.Forge.Buffer, is32 : Bool };
typedef VertexBufferExt = { b : sdl.Forge.Buffer, stride : Int, strideBytes : Int, descriptorMap : Map<Int, forge.Native.DescriptorSet> };
typedef DepthBufferExt = { r : forge.RuntimeRenderTarget };
typedef QueryExt = { q : sdl.Forge.Query, kind : h3d.impl.Driver.QueryKind };
typedef TextureExt = { t : sdl.Forge.Texture, width : Int, height : Int, internalFmt : Int, bits : Int, bind : Int, bias : Float, rt : RuntimeRenderTarget };
typedef RenderTarget = forge.RuntimeRenderTarget;
