package sdl;



typedef Texture = forge.Native.Texture;
/*
@:forward
abstract Texture(forge.Native.Texture) from forge.Native.Buffer to forge.Native.Buffer {
}
*/

@:forward
abstract Buffer(forge.Native.Buffer) from forge.Native.Buffer to forge.Native.Buffer {
}

abstract Framebuffer(Null<Int>) {
}
@:forward
abstract Renderbuffer( forge.Native.RenderTarget ) from forge.Native.RenderTarget to forge.Native.RenderTarget {
}

abstract Query(Null<Int>) {
}

abstract VertexArray(Null<Int>) {
}