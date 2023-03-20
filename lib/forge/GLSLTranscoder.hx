package forge;

import format.abc.Data.ABCData;
import hxsl.Ast;
import hxsl.Printer;

enum abstract EGLSLFlavour(Int) {
	var OpenGL = 0;
	var Vulkan;
	var Metal;
	var Auto;
}

// Mediocre abstraction until this is under control
@:enum abstract EDescriptorSetSlot(Int) to Int {
	var GLOBALS = 0;
	var PARAMS;
	var TEXTURES;
	var BUFFERS;
	var NONE;
}

@:enum abstract EShaderStage(Int) to Int {
	var VERTEX = 0;
	var FRAGMENT;
	var COMPUTE;
	var NONE;
}

class GLSLTranscoder {
	static final KWD_LIST = [
		"input", "output", "discard", #if js "sample", #end "dvec2", "dvec3", "dvec4", "hvec2", "hvec3", "hvec4", "fvec2", "fvec3", "fvec4", "int", "float",
		"bool", "long", "short", "double", "half", "fixed", "unsigned", "superp", "lowp", "mediump", "highp", "precision", "invariant", "discard", "struct",
		"asm", "union", "template", "this", "packed", "goto", "sizeof", "namespace", "noline", "volatile", "external", "flat", "input", "output", "out",
		"attribute", "const", "uniform", "varying", "inout", "void",
	];
	static final STAGE_SHORT_NAME = ["Vert", "Frag", "Comp"];
	static final SET_SHORT_NAME = ["Globals", "Params", "Textures", "Buffers", ""];
	static final LAYOUT_PUSH_CONSTANT = "push_constant";

	static var KWDS = [for (k in KWD_LIST) k => true];
	static var GLOBALS = {
		var gl = [];
		inline function set(g:hxsl.Ast.TGlobal, str:String) {
			gl[g.getIndex()] = str;
		}
		for (g in hxsl.Ast.TGlobal.createAll()) {
			var n = "" + g;
			n = n.charAt(0).toLowerCase() + n.substr(1);
			set(g, n);
		}
		set(ToInt, "int");
		set(ToFloat, "float");
		set(ToBool, "bool");
		set(LReflect, "reflect");
		set(Mat3x4, "_mat3x4");
		set(VertexID, "gl_VertexID");
		set(InstanceID, "gl_InstanceID");
		set(IVec2, "ivec2");
		set(IVec3, "ivec3");
		set(IVec4, "ivec4");
		set(BVec2, "bvec2");
		set(BVec3, "bvec3");
		set(BVec4, "bvec4");
		set(FragCoord, "gl_FragCoord");
		set(FrontFacing, "gl_FrontFacing");
		for (g in gl)
			KWDS.set(g, true);
		gl;
	};
	static var MAT34 = "struct _mat3x4 { vec4 a; vec4 b; vec4 c; };";
	static var MUL_MAT34 = "vec3 m3x4mult( vec3 v, _mat3x4 m) { vec4 ve = vec4(v,1.0); return vec3(dot(m.a,ve),dot(m.b,ve),dot(m.c,ve)); }";

	var buf:StringBuf;
	var exprIds = 0;
	var exprValues:Array<String>;
	var locals:Map<Int, TVar>;
	var decls:Array<String>;
	var allNames:Map<String, Int>;
	var outIndexes:Map<Int, Int>;
	var varBuffer:Map<String, String>;
	var _bindingCounts = new Array<Int>();
	var _bufferNameLookup:Map<String, String>;
	var isES(get, never):Bool;
	var isES2(get, never):Bool;
	var uniformBuffer:Int = 0;
	var outIndex:Int = 0;
	var _stageVarNames = new Array<Map<Int, String>>();

	public var glES:Null<Float>;
	public var version:Null<Int>;

	/*
		Intel HD driver fix:
			single element arrays are interpreted as not arrays, creating mismatch when
			handling uniforms/textures. The fix changes decl[1] into decl[2] with one unused element.

		Should not be enabled on AMD driver as it will create mismatch wrt uniforms binding
		when there are some unused textures in shader output.
	 */
	var intelDriverFix:Bool;
	var _flavour:EGLSLFlavour;
	var _currentStage = EShaderStage.NONE;
	var _shaderData = new Array<ShaderData>();
	var _glslCode = new Array<String>();

	public function new(flavour:EGLSLFlavour = EGLSLFlavour.Auto) {
		allNames = new Map();
		if (flavour == EGLSLFlavour.Auto) {
			_flavour = switch (Sys.systemName()) {
				case "Mac": EGLSLFlavour.Metal;
				case "Windows": EGLSLFlavour.Vulkan;
				default: EGLSLFlavour.Vulkan;
			}
		}
		_flavour = flavour;
		_stageVarNames[VERTEX] = new Map<Int, String>();
		_stageVarNames[FRAGMENT] = new Map<Int, String>();
	}

	public function getStageVarNames(stage:EShaderStage):Map<Int, String> {
		return _stageVarNames[stage];
	}

	public function setStageCode(stage:EShaderStage, s:ShaderData) {
		_shaderData[stage] = s;
	}

	inline function get_isES()
		return glES != null;

	inline function get_isES2()
		return glES != null && glES <= 2;

	inline function add(v:Dynamic) {
		buf.add(v);
	}

	static inline function debugTrace(s:String) {
		// trace(s);
	}

	inline function ident(v:TVar) {
		add(varName(v));
	}

	static var _defaultFlavour = switch (Sys.systemName()) {
			case "Mac": EGLSLFlavour.Metal;
			case "Windows": EGLSLFlavour.Vulkan;
			default: EGLSLFlavour.Vulkan;
		};

	static final PUSH_CONSTANT_TAG = "_rootconstant";

	function isSamplerType( t: Type ) {
		return switch(t) {
			case TSampler2D, TSamplerCube, TChannel(_): true;
			case TArray(t, size): isSamplerType(t);
			default: false;
		}
	}


	public static function getVariableBufferName(stage:EShaderStage, set:EDescriptorSetSlot, flavour = EGLSLFlavour.Auto):String {
		if (flavour == EGLSLFlavour.Auto)
			flavour = _defaultFlavour;

		#if PUSH_CONSTANTS
		if (set == EDescriptorSetSlot.PARAMS && false) {
			return '${STAGE_SHORT_NAME[stage]}${SET_SHORT_NAME[set]}${PUSH_CONSTANT_TAG}';
		}
		#end
		// (flavour == EGLSLFlavour.Metal || stage == VERTEX) &&
		return '${STAGE_SHORT_NAME[stage]}${SET_SHORT_NAME[set]}';
	}
	
	var texPostFix = "";
	function getLookupName(set:EDescriptorSetSlot, name:String):String {
		if (set == TEXTURES)
			return name + texPostFix;
		return "_" + getVariableBufferName(_currentStage, set, _flavour) + "." + name;
	}


	function identLookup(v:TVar) {
		var n = switch (v.kind) {
			case VarKind.Global: getLookupName(EDescriptorSetSlot.GLOBALS, varName(v));
			case VarKind.Param:
				if (isSamplerType(v.type)) getLookupName(EDescriptorSetSlot.TEXTURES, varName(v));
				else switch (v.type) {
					case TBuffer(t, size):
						if (_bufferNameLookup == null)
							throw "Buffer names are null";
						_bufferNameLookup.get(v.name) + "." + varName(v);
					default:
						getLookupName(EDescriptorSetSlot.PARAMS, varName(v));
				}
			default: varName(v); // locals etc
		}

		add(n);
	}

	function decl(s:String) {
		for (d in decls)
			if (d == s)
				return;
		if (s.charCodeAt(0) == '#'.code)
			decls.unshift(s);
		else
			decls.push(s);
	}

	function addType(t:Type) {
		switch (t) {
			case TVoid:
				add("void");
			case TInt:
				add("int");
			case TBytes(n):
				if (n == 1)
					add("uint");
				else
					add("uvec" + n);
			case TBool:
				add("bool");
			case TFloat:
				add("float");
			case TString:
				add("string");
			case TVec(size, k):
				switch (k) {
					case VFloat:
					case VInt: add("i");
					case VBool: add("u");
				}
				add("vec");
				add(size);
			case TMat2:
				add("mat2");
			case TMat3:
				add("mat3");
			case TMat4:
				add("mat4");
			case TMat3x4:
				decl(MAT34);
				add("_mat3x4");
			case TSampler2D:
				add("sampler");
			case TSampler2DArray:
				add("sampler2DArray");
				if (isES)
					decl("precision lowp sampler2DArray;");
			case TSamplerCube:
				add("samplerCube");
			case TStruct(vl):
				add("struct { ");
				for (v in vl) {
					addVar(v);
					add(";");
				}
				add(" }");
			case TFun(_):
				add("function");
			case TArray(t, size):
				addType(t);
				add("[");
				switch (size) {
					case SVar(v):
						ident(v);
					case SConst(1) if (intelDriverFix):
						add(2);
					case SConst(v):
						add(v);
				}
				add("]");
			case TBuffer(_):
				throw "assert";
			case TChannel(n):
				add("sampler2D");
			case TTexture2D:
				add("texture2D");
		}
	}

	function addVar(v:TVar) {
		switch (v.type) {
			case TArray(t, size):
				var old = v.type;
				v.type = t;
				addVar(v);
				v.type = old;
				add("[");
				switch (size) {
					case SVar(v): ident(v);
					case SConst(1) if (intelDriverFix): add(2);
					case SConst(n): add(n);
					default: throw 'Unrecognized size ${size}';
				}
				add("]");
			//			trace('Added size ${size} for ${v}');
			case TBuffer(t, size):
				add(STAGE_SHORT_NAME[_currentStage] + "_UniformBuffer" + (uniformBuffer));
				add(" { ");
				v.type = TArray(t, size);
				addVar(v);
				v.type = TBuffer(t, size);
				add("; } _uniformBuffer" + (uniformBuffer));
				if (_bufferNameLookup == null)
					_bufferNameLookup = new haxe.ds.StringMap();
				_bufferNameLookup.set(v.name, "_uniformBuffer" + (uniformBuffer));
				uniformBuffer++;
			default:
				addType(v.type);
				add(" ");
				ident(v);
		}
	}

	function addValue(e:TExpr, tabs:String) {
		switch (e.e) {
			case TBlock(el):
				var name = "val" + (exprIds++);
				var tmp = buf;
				buf = new StringBuf();
				addType(e.t);
				add(" ");
				add(name);
				add("(void)");
				var el2 = el.copy();
				var last = el2[el2.length - 1];
				el2[el2.length - 1] = {e: TReturn(last), t: e.t, p: last.p};
				var e2 = {
					t: TVoid,
					e: TBlock(el2),
					p: e.p,
				};
				addExpr(e2, "");
				exprValues.push(buf.toString());
				buf = tmp;
				add(name);
				add("()");
			case TIf(econd, eif, eelse):
				add("( ");
				addValue(econd, tabs);
				add(" ) ? ");
				addValue(eif, tabs);
				add(" : ");
				addValue(eelse, tabs);
			case TMeta(_, _, e):
				addValue(e, tabs);
			default:
				addExpr(e, tabs);
		}
	}

	function addBlock(e:TExpr, tabs:String) {
		addExpr(e, tabs);
	}

	function getFunName(g:TGlobal, args:Array<TExpr>, rt:Type) {
		switch (g) {
			case Mat3x4:
				decl(MAT34);
				if (args.length == 1) {
					decl("_mat3x4 mat_to_34( mat4 m ) { return _mat3x4(m[0],m[1],m[2]); }");
					return "mat_to_34";
				}
			case DFdx, DFdy, Fwidth:
				decl("#extension GL_OES_standard_derivatives:enable");
			case Pack:
				decl("vec4 pack( float v ) { vec4 color = fract(v * vec4(1, 255, 255.*255., 255.*255.*255.)); return color - color.yzww * vec4(1. / 255., 1. / 255., 1. / 255., 0.); }");
			case Unpack:
				decl("float unpack( vec4 color ) { return dot(color,vec4(1., 1. / 255., 1. / (255. * 255.), 1. / (255. * 255. * 255.))); }");
			case PackNormal:
				decl("vec4 packNormal( vec3 v ) { return vec4((v + vec3(1.)) * vec3(0.5),1.); }");
			case UnpackNormal:
				decl("vec3 unpackNormal( vec4 v ) { return normalize((v.xyz - vec3(0.5)) * vec3(2.)); }");
			case Texture:
				switch (args[0].t) {
					case TSampler2D, TSampler2DArray, TChannel(_) if (isES2):
						return "texture2D";
					case TSamplerCube if (isES2):
						return "textureCube";
					default:
				}
			case TextureLod:
				switch (args[0].t) {
					case TSampler2D, TSampler2DArray, TChannel(_) if (isES2):
						decl("#extension GL_EXT_shader_texture_lod : enable");
						return "texture2DLodEXT";
					case TSamplerCube if (isES2):
						decl("#extension GL_EXT_shader_texture_lod : enable");
						return "textureCubeLodEXT";
					default:
				}
			case Texel:
				// if ( isES2 )
				// 	decl("vec4 _texelFetch(sampler2d tex, ivec2 pos, int lod) ...")
				// 	return "_texelFetch";
				// else
				return "texelFetch";
			case TextureSize:
				switch (args[0].t) {
					case TSampler2D, TChannel(_):
						decl("vec2 _textureSize(sampler2D sampler, int lod) { return vec2(textureSize(sampler, lod)); }");
					case TSamplerCube:
						decl("vec2 _textureSize(samplerCube sampler, int lod) { return vec2(textureSize(sampler, lod)); }");
					case TSampler2DArray:
						decl("vec3 _textureSize(sampler2DArray sampler, int lod) { return vec3(textureSize(sampler, lod)); }");
					default:
				}
				return "_textureSize";
			case Mod if (rt == TInt && isES):
				decl("int _imod( int x, int y ) { return int(mod(float(x),float(y))); }");
				return "_imod";
			case Mat3 if (args[0].t == TMat3x4):
				decl(MAT34);
				decl("mat3 _mat3( _mat3x4 v ) { return mat3(v.a.xyz,v.b.xyz,v.c.xyz); }");
				return "_mat3";
			case ScreenToUv:
				decl("vec2 screenToUv( vec2 v ) { return v * vec2(0.5,-0.5) + vec2(0.5,0.5); }");
			case UvToScreen:
				decl("vec2 uvToScreen( vec2 v ) { return v * vec2(2.,-2.) + vec2(-1., 1.); }");
			default:
		}
		return GLOBALS[g.getIndex()];
	}

	function addExpr(e:TExpr, tabs:String) {
		switch (e.e) {
			case TConst(c):
				switch (c) {
					case CInt(v): add(v);
					case CFloat(f):
						var str = "" + f;
						add(str);
						if (str.indexOf(".") == -1 && str.indexOf("e") == -1) add(".");
					case CString(v): add('"' + v + '"');
					case CNull: add("null");
					case CBool(b): add(b);
				}
			case TVar(v):
				identLookup(v);
			case TGlobal(g):
				add(GLOBALS[g.getIndex()]);
			case TParenthesis(e):
				add("(");
				addValue(e, tabs);
				add(")");
			case TBlock(el):
				add("{\n");
				var t2 = tabs + "\t";
				for (e in el) {
					add(t2);
					addExpr(e, t2);
					newLine(e);
				}
				add(tabs);
				add("}");
			case TBinop(op, e1, e2):
				switch ([op, e1.t, e2.t]) {
					case [OpAssignOp(OpMult) | OpMult, TVec(3, VFloat), TMat3x4]:
						decl(MAT34);
						decl(MUL_MAT34);
						if (op.match(OpAssignOp(_))) {
							addValue(e1, tabs);
							add(" = ");
						}
						add("m3x4mult(");
						addValue(e1, tabs);
						add(",");
						addValue(e2, tabs);
						add(")");
					case [OpAssignOp(OpMod) | OpMod, _, _] if (e.t != TInt):
						if (op.match(OpAssignOp(_))) {
							addValue(e1, tabs);
							add(" = ");
						}
						addExpr({e: TCall({e: TGlobal(Mod), t: TFun([]), p: e.p}, [e1, e2]), t: e.t, p: e.p}, tabs);
					case [OpUShr, _, _]:
						decl("int _ushr( int i, int j ) { return int(uint(i) >> uint(j)); }");
						add("_ushr(");
						addValue(e1, tabs);
						add(",");
						addValue(e2, tabs);
						add(")");
					case [OpEq | OpNotEq | OpLt | OpGt | OpLte | OpGte, TVec(n, _), TVec(_)]:
						add("vec" + n + "(");
						add(switch (op) {
							case OpEq: "equal";
							case OpNotEq: "notEqual";
							case OpLt: "lessThan";
							case OpGt: "greaterThan";
							case OpLte: "lessThanEqual";
							case OpGte: "greaterThanEqual";
							default: throw "assert";
						});
						add("(");
						addValue(e1, tabs);
						add(",");
						addValue(e2, tabs);
						add("))");
					default:
						addValue(e1, tabs);
						add(" ");
						add(Printer.opStr(op));
						add(" ");
						addValue(e2, tabs);
				}
			case TUnop(op, e1):
				add(switch (op) {
					case OpNot: "!";
					case OpNeg: "-";
					case OpIncrement: "++";
					case OpDecrement: "--";
					case OpNegBits: "~";
					default: throw "assert"; // OpSpread for Haxe4.2+
				});
				addValue(e1, tabs);
			case TVarDecl(v, init):
				locals.set(v.id, v);
				if (init != null) {
					ident(v);
					add(" = ");
					addValue(init, tabs);
				} else {
					add("/*var*/");
				}
			case TCall({e: TGlobal(Saturate)}, [e]):
				add("clamp(");
				addValue(e, tabs);
				add(", 0., 1.)");
			case TCall({e: TGlobal(g = Texel)}, args):
				add(getFunName(g, args, e.t));
				add("(");
				addValue(args[0], tabs); // sampler
				add(", ");
				addValue(args[1], tabs); // uv
				if (args.length != 2) {
					// with LOD argument
					add(", ");
					addValue(args[2], tabs);
					add(")");
				} else {
					add(", 0)");
				}
			case TCall({e: TGlobal(g = TextureSize)}, args):
				add(getFunName(g, args, e.t));
				add("(");
				addValue(args[0], tabs);
				if (args.length != 1) {
					// with LOD argument
					add(", ");
					addValue(args[1], tabs);
					add(")");
				} else {
					add(", 0)");
				}
			case TCall({e: TGlobal(g = Texture)}, args):
				add("texture(");
				
				add("sampler2D(");
					addValue(args[0], tabs);
					add(",");
					texPostFix = "Smplr"; // SUPER hacky
					addValue(args[0], tabs);
					texPostFix = "";
				add(")");
				for (i in 1...args.length){
					add(",");
					addValue(args[i], tabs);
				}
				add(")");
			case TCall(v, args):
				switch (v.e) {
					case TGlobal(g):
						add(getFunName(g, args, e.t));
					default:
						addValue(v, tabs);
				}
				add("(");
				var first = true;
				for (e in args) {
					if (first)
						first = false
					else
						add(", ");
					addValue(e, tabs);
				}
				add(")");
			case TSwiz(e, regs):
				switch (e.t) {
					case TFloat:
						for (r in regs)
							if (r != X)
								throw "assert";
						switch (regs.length) {
							case 1:
								addValue(e, tabs);
							case 2:
								decl("vec2 _vec2( float v ) { return vec2(v,v); }");
								add("_vec2(");
								addValue(e, tabs);
								add(")");
							case 3:
								decl("vec3 _vec3( float v ) { return vec3(v,v,v); }");
								add("_vec3(");
								addValue(e, tabs);
								add(")");
							case 4:
								decl("vec4 _vec4( float v ) { return vec4(v,v,v,v); }");
								add("_vec4(");
								addValue(e, tabs);
								add(")");
							default:
								throw "assert";
						}
					default:
						addValue(e, tabs);
						add(".");
						for (r in regs)
							add(switch (r) {
								case X: "x";
								case Y: "y";
								case Z: "z";
								case W: "w";
							});
				}
			case TIf(econd, eif, eelse):
				add("if( ");
				addValue(econd, tabs);
				add(") ");
				addExpr(eif, tabs);
				if (eelse != null) {
					if (!isBlock(eif))
						add(";");
					add(" else ");
					addExpr(eelse, tabs);
				}
			case TDiscard:
				add("discard");
			case TReturn(e):
				if (e == null)
					add("return");
				else {
					add("return ");
					addValue(e, tabs);
				}
			case TFor(v, it, loop):
				locals.set(v.id, v);
				switch (it.e) {
					case TBinop(OpInterval, e1, e2):
						add("for(");
						add(v.name + "=");
						addValue(e1, tabs);
						add(";" + v.name + "<");
						addValue(e2, tabs);
						add(";" + v.name + "++) ");
						addBlock(loop, tabs);
					default:
						throw "assert";
				}
			case TWhile(e, loop, false):
				var old = tabs;
				tabs += "\t";
				add("do ");
				addBlock(loop, tabs);
				add(" while( ");
				addValue(e, tabs);
				add(" )");
			case TWhile(e, loop, _):
				add("while( ");
				addValue(e, tabs);
				add(" ) ");
				addBlock(loop, tabs);
			case TSwitch(_):
				add("switch(...)");
			case TContinue:
				add("continue");
			case TBreak:
				add("break");
			case TArray(e, index):
				addValue(e, tabs);
				add("[");
				addValue(index, tabs);
				add("]");
			case TArrayDecl(el):
				switch (e.t) {
					case TArray(t, _): addType(t);
					default: throw "assert";
				}
				add("[" + el.length + "]");
				add("(");
				var first = true;
				for (e in el) {
					if (first)
						first = false
					else
						add(", ");
					addValue(e, tabs);
				}
				add(")");
			case TMeta(_, _, e):
				addExpr(e, tabs);
		}
	}

	function varName(v:TVar) {
		if (v == null)
			throw "Variable name is null";

		if (v.kind == Output) {
			if (_currentStage == VERTEX)
				return "gl_Position";
			if (isES2) {
				if (outIndexes == null)
					return "gl_FragColor";
				return 'gl_FragData[${outIndexes.get(v.id)}]';
			}
		}
		var n = _stageVarNames[_currentStage].get(v.id);
		if (n != null)
			return n;
		n = v.name;
		if (KWDS.exists(n))
			n = "_" + n;
		if (allNames.exists(n)) {
			var k = 2;
			n += "_";
			while (allNames.exists(n + k))
				k++;
			n += k;
		}
		_stageVarNames[_currentStage].set(v.id, n);
		allNames.set(n, v.id);
		return n;
	}

	function newLine(e:TExpr) {
		if (isBlock(e))
			add("\n");
		else
			add(";\n");
	}

	function isBlock(e:TExpr) {
		switch (e.e) {
			case TFor(_, _, loop), TWhile(_, loop, true):
				return isBlock(loop);
			case TBlock(_):
				return true;
			default:
				return false;
		}
	}

	final BUFFER_SET = EDescriptorSetSlot.BUFFERS;

	function initVar(v:TVar, offset = -1) {
		switch (v.kind) {
			case Param, Global:
				if (v.type.match(TBuffer(_))) {
					add('layout(set=${BUFFER_SET}, binding=${_bindingCounts[BUFFER_SET]++}) ');
					//				add('layout(std140, binding=${_bufferCount}) ');
					add("uniform ");
				}
			case Input:
				add(isES2 ? "attribute " : "in ");
			case Var:
				add(isES2 ? "varying " : (_currentStage == VERTEX ? "out " : "in "));
			case Output:
				if (isES2) {
					outIndexes.set(v.id, outIndex++);
					return;
				}
				if (_currentStage == VERTEX)
					return;
				if (isES)
					add('layout(location=${outIndex++}) ');
				add("out ");
			case Function:
				add('// Function - ${v.name} : ${v.kind} \n');
				return;
			case Local:
		}

		if (_flavour == Vulkan && v.kind == Param && !v.type.match(TBuffer(_)) && offset != -1) {
			add('layout(offset=${offset}) ');
		}

		if (v.qualifiers != null)
			for (q in v.qualifiers)
				switch (q) {
					case Precision(p):
						switch (p) {
							case Low: add("lowp ");
							case Medium: add("mediump ");
							case High: add("highp ");
						}
					default:
				}
		addVar(v);
		add(';  // ${v.name} : ${v.kind} \n');
	}

	function getTypeSize(t:Type) {
		return switch (t) {
			case TInt: 4;
			case TBool: 1;
			case TFloat: 4;
			case TVec(size, k):
				switch (k) {
					case VFloat: 4 * size;
					case VInt: 4 * size;
					case VBool: size;
				}
			case TMat3x4: 4 * 3 * 4;
			default: throw 'Not supported size type ${t}';
		}
	}

	function getVarSize(v:TVar) {
		return switch (v.type) {
			case TArray(t, size):
				var old = v.type;
				v.type = t;
				var baseCount = getVarSize(v);
				v.type = old;
				baseCount * switch (size) {
					case SVar(v): throw "Not supported";
					case SConst(1): (intelDriverFix) ? 2 : 1;
					case SConst(n): n;
					default: throw 'Unrecognized size ${size}';
				}

			//			trace('Added size ${size} for ${v}');
			default:
				getTypeSize(v.type);
		}
	}

	function getSize(v:TVar) {
		var size = getVarSize(v);

		if (v.qualifiers != null)
			for (q in v.qualifiers)
				switch (q) {
					case Precision(p):
						switch (p) {
							case Low: return Std.int(size / 2);
							case Medium: Std.int(size);
							case High: Std.int(size * 2);
						}
					default:
				}

		return size;
	}

	/*
		TVoid;
		TInt;
		TBool;
		TFloat;
		TString;
		TVec( size : Int, t : VecType );
		TMat3;
		TMat4;
		TMat3x4;
		TBytes( size : Int );
		TSampler2D;
		TSampler2DArray;
		TSamplerCube;
		TStruct( vl : Array<TVar> );
		TFun( variants : Array<FunType> );
		TArray( t : Type, size : SizeDecl );
		TBuffer( t : Type, size : SizeDecl );
		TChannel( size : Int );
		TMat2;
	 */
	function getLayoutSpec(stage:EShaderStage, set:EDescriptorSetSlot, idx = -1) {
		#if PUSH_CONSTANTS
		if (_flavour == EGLSLFlavour.Metal || true) {
			if (set == EDescriptorSetSlot.PARAMS)
				return LAYOUT_PUSH_CONSTANT;
		}
		if (stage == VERTEX && set == PARAMS)
			return LAYOUT_PUSH_CONSTANT;
		#end

		if (idx != -1)
			return 'set=${set},binding=${idx}';
		return 'set=${set},binding=${stage}';
	}

	function isPushBuffer(stage:EShaderStage, set:EDescriptorSetSlot) {
		return set == EDescriptorSetSlot.PARAMS;
	}

	function isBufferableType(t:Type) {
		return switch (t) {
			case TSampler2D: false;
			case TSamplerCube: false;
			case TChannel(_): false;
			case TBuffer(_): false;
			case TArray(t, size): isBufferableType(t);
			default: true;
		}
	}
	function getBufferedParams(s:ShaderData):Array<TVar> {
		var params = s.vars.filter((x) -> x.kind == Param);
		return params.filter((x) -> isBufferableType(x.type));
	}

	function initVars(s:ShaderData, stage:EShaderStage) {
		outIndex = 0;
		uniformBuffer = 0;
		outIndexes = new Map();

		var nonBufferVars = s.vars.filter((x) -> x.type.match(TBuffer(_)) == false);
		var bufferVars = s.vars.filter((x) -> x.type.match(TBuffer(_)));

		var globals = nonBufferVars.filter((x) -> x.kind == Global);
		var params = nonBufferVars.filter((x) -> x.kind == Param);
		var buffer_params = getBufferedParams(s);

		if (globals.length > 0) {
			add('// Globals - ${EDescriptorSetSlot.GLOBALS} -> ${getLayoutSpec(stage, EDescriptorSetSlot.GLOBALS)}\n');
			add('layout( ${getLayoutSpec(stage, EDescriptorSetSlot.GLOBALS)}) uniform ${getVariableBufferName(stage, EDescriptorSetSlot.GLOBALS)} {\n');
			for (v in globals) {
				add("\t");
				initVar(v);
			}

			add('} _${getVariableBufferName( stage, EDescriptorSetSlot.GLOBALS), _flavour};\n');
		}
		if (buffer_params.length > 0) {
			add('// Params - ${EDescriptorSetSlot.PARAMS} -> ${getLayoutSpec(stage, EDescriptorSetSlot.PARAMS)}\n');

			add('layout( ${getLayoutSpec(stage, EDescriptorSetSlot.PARAMS)} ) uniform ${getVariableBufferName(stage, EDescriptorSetSlot.PARAMS, _flavour)} {\n');


			var totalSize = 0;
			for (v in buffer_params) {
				add("\t");
				#if PUSH_CONSTANTS
				initVar(v, _buildPushConstantSize + totalSize);
				#else
				initVar(v);
				#end
				totalSize += getSize(v);
			}
			trace('RENDER PARAMS OFFSET ${_buildPushConstantSize} TOTAL SIZE ${totalSize} for stage ${stage}');
			_buildPushConstantSize += totalSize;

			add('} _${getVariableBufferName(stage, EDescriptorSetSlot.PARAMS, _flavour)};\n');
		}
		for (b in bufferVars) {
			initVar(b);
		}

		var sampler_params = params.filter((x) -> switch (x.type) {
			case TSampler2D: true;
			case TSamplerCube: true;
			case TArray(t, size): t == TSampler2D || t == TSamplerCube;
			default: false;
		});

		var tex_binding_idx = 0;


		if (sampler_params.length > 0) {
			add("//Samplers & textures\n");

			// ${getVariableBufferName( stage, EDescriptorSetSlot.TEXTURES, _flavour)}_${v.name}

//			add('layout( ${getLayoutSpec(stage, EDescriptorSetSlot.TEXTURES, idx++)} ) uniform ${getVariableBufferName(stage, EDescriptorSetSlot.TEXTURES, _flavour)} {\n ');
			for (v in sampler_params) {
				add('layout( ${getLayoutSpec(stage, EDescriptorSetSlot.TEXTURES, tex_binding_idx++)} ) uniform ');
				switch (v.type) {
					case TSampler2D:
						debugTrace('RENDER adding sampler ${v.name}');
					case TArray(t, size):
						debugTrace('RENDER adding sampler array ${v.name}');
					case TSamplerCube:
						debugTrace('RENDER adding cube sampler ${v.name}');
					default:
						false;
				}

				var vt = {id: v.id + 2000, name : v.name + "Smplr", type : v.type, kind : v.kind, parent : v.parent, qualifiers : v.qualifiers};
				initVar(vt);

				var vsType = switch(v.type) {
					case TArray(t, size): TArray(TTexture2D, size);
					case TSampler2D: TTexture2D;
					default:  v.type;
				};

				var vs = {id: v.id, name : v.name, type : vsType, kind : v.kind, parent : v.parent, qualifiers : v.qualifiers};
				trace('WTF : vt ${vt.name} vs ${vs.name}');
				add('layout( ${getLayoutSpec(stage, EDescriptorSetSlot.TEXTURES, tex_binding_idx++)} ) uniform ');
				initVar(vs);


			}
//			add('} _${getVariableBufferName(stage, EDescriptorSetSlot.TEXTURES, _flavour)};\n');

		}

		var channel_params = params.filter((x) -> switch (x.type) {
			case TChannel(_): true;
			case TSamplerCube: true;
			case TArray(t, size): t.match(TChannel(_));
			default: false;
		});

		if (channel_params.length > 0) {
			add("//Samplers & textures\n");

			// ${getVariableBufferName( stage, EDescriptorSetSlot.TEXTURES, _flavour)}_${v.name}

//			add('layout( ${getLayoutSpec(stage, EDescriptorSetSlot.TEXTURES, idx++)} ) uniform ${getVariableBufferName(stage, EDescriptorSetSlot.TEXTURES, _flavour)} {\n ');
			for (v in channel_params) {
				add('layout( ${getLayoutSpec(stage, EDescriptorSetSlot.TEXTURES, tex_binding_idx++)} ) uniform ');
				switch (v.type) {
					case TChannel(size):
						debugTrace('RENDER adding channel ${v.name} ${size}');
					case TArray(t, size):
						debugTrace('RENDER adding channel array ${v.name} ${size}');
					default:
						false;
				}

				initVar(v);


			}
//			add('} _${getVariableBufferName(stage, EDescriptorSetSlot.TEXTURES, _flavour)};\n');

		}


		add("//Input\n");
		for (v in s.vars)
			if (v.kind == VarKind.Input)
				initVar(v);
		if (stage == FRAGMENT)
			for (v in s.vars)
				if (v.kind == VarKind.Var)
					initVar(v);
		add("//Output\n");
		for (v in s.vars)
			if (v.kind == VarKind.Output)
				initVar(v);
		if (stage == VERTEX)
			for (v in s.vars)
				if (v.kind == VarKind.Var)
					initVar(v);
		add("//Locals\n");
		for (v in s.vars)
			if (v.kind == VarKind.Local)
				initVar(v);

		add("// Function Locals\n");
		for (v in s.vars)
			if (v.kind == VarKind.Function)
				initVar(v);

		add("// Other Locals\n");
		for (v in s.vars) {
			switch (v.kind) {
				case Param, Global, Input, Output, Local, Var, Function:
				default:
					initVar(v);
			}
		}

		add("\n");

		if (outIndex < 2)
			outIndexes = null;
		else if (!(stage == VERTEX) && isES2)
			decl("#extension GL_EXT_draw_buffers : enable");

		add("// Done variable declaration\n");
	}

	var _buildPushConstantSize = 0;
	var _buildAllBufferedParams = [];

	function prebuild() {
		/*
			for (i in 0..._shaderData.length) {
				if (_shaderData[i] != null) {
					_buildAllBufferedParams = _buildAllBufferedParams.concat(getBufferedParams(_shaderData[i]));
				}
			}
		 */
	}

	public function build() {
		try {
			prebuild();
			buildStage(EShaderStage.VERTEX);
			buildStage(EShaderStage.FRAGMENT);
		} catch (e:haxe.Exception) {
			trace("Error building shader: " + e.message);
			trace(e.stack);

			Sys.exit(-1);
		}
	}

	public function getGLSL(stage:EShaderStage) {
		return _glslCode[stage];
	}

	function buildStage(stage:EShaderStage) {
		_currentStage = stage;
		var s = _shaderData[stage];
		locals = new Map();
		decls = [];
		buf = new StringBuf();
		exprValues = [];
		if (s.funs.length != 1)
			throw "assert";
		var f = s.funs[0];

		if (stage == EShaderStage.VERTEX)
			decl("precision highp float;");
		else
			decl("precision mediump float;");

		initVars(s, stage);

		var tmp = buf;
		buf = new StringBuf();
		add("void main(void) {\n");
		switch (f.expr.e) {
			case TBlock(el):
				for (e in el) {
					add("\t");
					addExpr(e, "\t");
					newLine(e);
				}
			default:
				addExpr(f.expr, "");
		}
		if (stage == EShaderStage.VERTEX) {
			/**
				In Heaps and DirectX, vertex output Z position is in [0,1] range
				Whereas in OpenGL it's [-1, 1].
				Given we have either [X, Y, 0, N] for zNear or [X, Y, F, F] for zFar,
				this shader operation will map [0, 1] range to [-1, 1] for correct clipping.
			**/
			// add("\tgl_Position.z += gl_Position.z - gl_Position.w;\n");
		}
		add("}");
		exprValues.push(buf.toString());
		buf = tmp;

		var locals = Lambda.array(locals);
		locals.sort(function(v1, v2) return Reflect.compare(v1.name, v2.name));
		add('// Locals\n');
		for (v in locals) {
			addVar(v);
			add(';\n');
		}
		add("\n");

		for (e in exprValues) {
			add(e);
			add("\n\n");
		}

		/*
			if( isES )
				decl("#version " + (version < 100 ? 100 : version) + (version > 150 ? " es" : ""));
			else if( version != null )
				decl("#version " + (version > 150 ? 150 : version));
			else
		 */
		decl("#version 460"); // OpenGL 3.0

		decls.push(buf.toString());
		buf = null;
		_glslCode[stage] = decls.join("\n");
	}
	/*
		public static function compile( s : ShaderData, f : EGLSLFlavour ) {
			var out = new GLSLTranscoder(f);
			#if js
			out.glES = 1;
			out.version = 100;
			#end
			return out.run(s);
		}
	 */
}
