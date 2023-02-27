#!/bin/bash
haxe -cp generator  -lib hl-idl --macro "Generator.generateCpp(\"hl\")"