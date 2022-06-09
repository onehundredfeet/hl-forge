genhl:
	haxe -cp generator  -lib hl-idl --macro "forge.Generator.generateCpp()"
	
genjs:
	haxe -cp generator -lib hl-idl --macro "forge.Generator.generateJs()"
