package h3d.impl;

typedef IndexBufferExt = { b : sdl.Forge.Buffer, is32 : Bool };
typedef VertexBufferExt = { b : sdl.Forge.Buffer, stride : Int, strideBytes : Int, descriptorMap : Map<Int, forge.Native.DescriptorSet> };
typedef DepthBufferExt = { r : sdl.Forge.Renderbuffer };
typedef QueryExt = { q : sdl.Forge.Query, kind : h3d.impl.Driver.QueryKind };
typedef RenderTarget = { rt : forge.Native.RenderTarget, inBarrier : forge.Native.ResourceBarrierBuilder, outBarrier: forge.Native.ResourceBarrierBuilder, begin: forge.Native.ResourceBarrierBuilder,  present: forge.Native.ResourceBarrierBuilder, captureBuffer : forge.Native.Buffer };
typedef TextureExt = { t : sdl.Forge.Texture, width : Int, height : Int, internalFmt : Int, bits : Int, bind : Int, bias : Float, rt : RenderTarget };
