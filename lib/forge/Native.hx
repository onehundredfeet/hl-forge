package forge;


typedef Native = haxe.macro.MacroType<[webidl.Module.build({ idlFile : "forge/forge.idl", chopPrefix : "rc", autoGC : true, nativeLib : "forge" })]>;
