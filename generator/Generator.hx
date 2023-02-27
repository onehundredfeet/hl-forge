package ;

#if eval
class Generator {
	static var HL_INCLUDE = "
#ifdef _WIN32
#pragma warning(disable:4305)
#pragma warning(disable:4244)
#pragma warning(disable:4316)
#endif


#include \"hl-forge.h\"
";
	// Put any necessary includes in this string and they will be added to the generated files
	static var JVM_INCLUDE = "
#ifdef _WIN32
#pragma warning(disable:4305)
#pragma warning(disable:4244)
#pragma warning(disable:4316)
#endif


#include \"hl-forge.h\"
";
	
static var options = {
	idlFile: "lib/forge/forge.idl",
	target: null,
	packageName: "forge",
	nativeLib: "forge",
	outputDir: "src",
	includeCode: null,
	autoGC: true
};

	public static function generateCpp(target = idl.Options.Target.TargetHL) {	
		options.target = target;
		options.includeCode = switch (target) {
			case idl.Options.Target.TargetHL: HL_INCLUDE;
			case idl.Options.Target.TargetJVM: JVM_INCLUDE;
			default: "";
		};
		idl.generator.Generate.generateCpp(options);
	}

}
#end