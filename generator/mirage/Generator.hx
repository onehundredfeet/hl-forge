package mirage;

#if eval
class Generator {

	// Put any necessary includes in this string and they will be added to the generated files
	static var INCLUDE = "
#ifdef _WIN32
#pragma warning(disable:4305)
#pragma warning(disable:4244)
#pragma warning(disable:4316)
#endif

#include <mirage/mirage.h>

#include \"hl-mirage.h\"
";
	
	public static function generateCpp() {	
		var options = { idlFile : "generator/mirage.idl", nativeLib : "mirage", outputDir : "src", includeCode : INCLUDE, autoGC : true };
		webidl.Generate.generateCpp(options);
	}

}
#end