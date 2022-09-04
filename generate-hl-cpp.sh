#!/bin/bash
haxe -cp generator  -lib hl-idl --macro "forge.Generator.generateCpp()"