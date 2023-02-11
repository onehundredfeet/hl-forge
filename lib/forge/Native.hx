package forge;

typedef Native = haxe.macro.MacroType<[
	idl.Module.build({
		idlFile: "forge/forge.idl",
		target: #if hl "hl" #elseif (java || jvm) "jvm" #else "error" #end,
		packageName: "forge",
		autoGC: true,
		nativeLib: "forge"
	})
]>;
