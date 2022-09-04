package forge;

@:enum abstract ShaderType(Int) {
    var VERTEX_SHADER = 1;
    var FRAGMENT_SHADER = 2;
}

abstract Program(Null<Int>) {
}

/*
typedef Shader = forge.Native.ForgeShader;
abstract Shader(Null<Int>) {
}
*/