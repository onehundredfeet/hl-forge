genhl:
	haxe -cp generator  -lib hl-idl --macro "mirage.Generator.generateCpp()"
	
genjs:
	haxe -cp generator -lib hl-idl --macro "mirage.Generator.generateJs()"
