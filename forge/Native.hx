package forge;


typedef Native = haxe.macro.MacroType<[webidl.Module.build({ idlFile : "generator/forge.idl", chopPrefix : "rc", autoGC : true, nativeLib : "forge" })]>;
