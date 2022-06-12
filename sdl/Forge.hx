package sdl;

abstract Uniform(Null<Int>) {
}

abstract Program(Null<Int>) {
}

abstract Shader(Null<Int>) {
}

abstract Texture(Null<Int>) {
}

@:forward
abstract Buffer(forge.Native.Buffer) from forge.Native.Buffer to forge.Native.Buffer {
}

abstract Framebuffer(Null<Int>) {
}

abstract Renderbuffer(Null<Int>) {
}

abstract Query(Null<Int>) {
}

abstract VertexArray(Null<Int>) {
}