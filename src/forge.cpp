#ifdef EMSCRIPTEN

#include <emscripten.h>
#define HL_PRIM
#define HL_NAME(n)	EMSCRIPTEN_KEEPALIVE eb_##n
#define DEFINE_PRIM(ret, name, args)
#define _OPT(t) t*
#define _GET_OPT(value,t) *value
#define alloc_ref(r, _) r
#define alloc_ref_const(r,_) r
#define _ref(t)			t
#define _unref(v)		v
#define free_ref(v) delete (v)
#define HL_CONST const

#else

#define HL_NAME(x) forge_##x
#include <hl.h>

// Need to link in helpers
HL_API hl_type hltx_ui16;
HL_API hl_type hltx_ui8;

#define _IDL _BYTES
#define _OPT(t) vdynamic *
#define _GET_OPT(value,t) (value)->v.t
template <typename T> struct pref {
	void (*finalize)( pref<T> * );
	T *value;
};

#define _ref(t) pref<t>
#define _unref(v) v->value
#define _unref_ptr_safe(v) (v != nullptr ? v->value : nullptr)
#define alloc_ref(r,t) _alloc_ref(r,finalize_##t)
#define alloc_ref_const(r, _) _alloc_const(r)
#define HL_CONST

template<typename T> void free_ref( pref<T> *r ) {
	if( !r->finalize ) hl_error("delete() is not allowed on const value.");
	delete r->value;
	r->value = NULL;
	r->finalize = NULL;
}

template<typename T> void free_ref( pref<T> *r, void (*deleteFunc)(T*) ) {
	if( !r->finalize ) hl_error("delete() is not allowed on const value.");
	deleteFunc( r->value );
	r->value = NULL;
	r->finalize = NULL;
}

// Float vector
struct _hl_float2 {
	float x;
	float y;
};

struct _hl_float3 {
	float x;
	float y;
	float z;
};

struct _hl_float4 {
	float x;
	float y;
	float z;
	float w;
};

// int vector
struct _hl_int2 {
	int x;
	int y;
};

struct _hl_int3 {
	int x;
	int y;
	int z;
};

struct _hl_int4 {
	int x;
	int y;
	int z;
	int w;
};

// double vector
struct _hl_double2 {
	double x;
	double y;
};

struct _hl_double3 {
	double x;
	double y;
	double z;
};

struct _hl_double4 {
	double x;
	double y;
	double z;
	double w;
};

inline void testvector(_hl_float3 *v) {
  printf("v: %f %f %f\n", v->x, v->y, v->z);
}
template<typename T> pref<T> *_alloc_ref( T *value, void (*finalize)( pref<T> * ) ) {
	if (value == nullptr) return nullptr;
	pref<T> *r = (pref<T>*)hl_gc_alloc_finalizer(sizeof(pref<T>));
	r->finalize = finalize;
	r->value = value;
	return r;
}

template<typename T> pref<T> *_alloc_const( const T *value ) {
	if (value == nullptr) return nullptr;
	pref<T> *r = (pref<T>*)hl_gc_alloc_noptr(sizeof(pref<T>));
	r->finalize = NULL;
	r->value = (T*)value;
	return r;
}

inline static varray* _idc_alloc_array(float *src, int count) {
	if (src == nullptr) return nullptr;

	varray *a = NULL;
	float *p;
	a = hl_alloc_array(&hlt_f32, count);
	p = hl_aptr(a, float);

	for (int i = 0; i < count; i++) {
		p[i] = src[i];
	}
	return a;
}
inline static varray* _idc_alloc_array(unsigned char *src, int count) {
	if (src == nullptr) return nullptr;

	varray *a = NULL;
	float *p;
	a = hl_alloc_array(&hltx_ui8, count);
	p = hl_aptr(a, float);

	for (int i = 0; i < count; i++) {
		p[i] = src[i];
	}
	return a;
}

inline static varray* _idc_alloc_array( char *src, int count) {
	return _idc_alloc_array((unsigned char *)src, count);
}

inline static varray* _idc_alloc_array(int *src, int count) {
	if (src == nullptr) return nullptr;

	varray *a = NULL;
	int *p;
	a = hl_alloc_array(&hlt_i32, count);
	p = hl_aptr(a, int);

	for (int i = 0; i < count; i++) {
		p[i] = src[i];
	}
	return a;

}

inline static varray* _idc_alloc_array(double *src, int count) {
	if (src == nullptr) return nullptr;

	varray *a = NULL;
	double *p;
	a = hl_alloc_array(&hlt_f64, count);
	p = hl_aptr(a, double);

	for (int i = 0; i < count; i++) {
		p[i] = src[i];
	}
	return a;
}


inline static varray* _idc_alloc_array(const unsigned short *src, int count) {
	if (src == nullptr) return nullptr;

	varray *a = NULL;
	unsigned short *p;
	a = hl_alloc_array(&hltx_ui16, count);
	p = hl_aptr(a, unsigned short);

	for (int i = 0; i < count; i++) {
		p[i] = src[i];
	}
	return a;
}

inline static varray* _idc_alloc_array(unsigned short *src, int count) {
	if (src == nullptr) return nullptr;

	varray *a = NULL;
	unsigned short *p;
	a = hl_alloc_array(&hltx_ui16, count);
	p = hl_aptr(a, unsigned short);

	for (int i = 0; i < count; i++) {
		p[i] = src[i];
	}
	return a;
}

inline static void _idc_copy_array( float *dst, varray *src, int count) {
	float *p = hl_aptr(src, float);
	for (int i = 0; i < count; i++) {
		dst[i] = p[i];
	}
}

inline static void _idc_copy_array( varray *dst, float *src,  int count) {
	float *p = hl_aptr(dst, float);
	for (int i = 0; i < count; i++) {
		p[i] = src[i];
	}
}


inline static void _idc_copy_array( int *dst, varray *src, int count) {
	int *p = hl_aptr(src, int);
	for (int i = 0; i < count; i++) {
		dst[i] = p[i];
	}
}

inline static void _idc_copy_array( unsigned short *dst, varray *src) {
	unsigned short *p = hl_aptr(src, unsigned short);
	for (int i = 0; i < src->size; i++) {
		dst[i] = p[i];
	}
}

inline static void _idc_copy_array( const unsigned short *cdst, varray *src) {
	unsigned short *p = hl_aptr(src, unsigned short);
	unsigned short *dst = (unsigned short *)cdst;
	for (int i = 0; i < src->size; i++) {
		dst[i] = p[i];
	}
}

inline static void _idc_copy_array( varray *dst, int *src,  int count) {
	int *p = hl_aptr(dst, int);
	for (int i = 0; i < count; i++) {
		p[i] = src[i];
	}
}


inline static void _idc_copy_array( double *dst, varray *src, int count) {
	double *p = hl_aptr(src, double);
	for (int i = 0; i < count; i++) {
		dst[i] = p[i];
	}
}

inline static void _idc_copy_array( varray *dst, double *src,  int count) {
	double *p = hl_aptr(dst, double);
	for (int i = 0; i < count; i++) {
		p[i] = src[i];
	}
}

#endif

#ifdef _WIN32
#pragma warning(disable:4305)
#pragma warning(disable:4244)
#pragma warning(disable:4316)
#endif


#include "hl-forge.h"





extern "C" {

static void finalize_SyncToken( _ref(SyncToken)* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(SyncToken_delete)( _ref(SyncToken)* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, SyncToken_delete, _IDL);
static void finalize_BufferLoadDesc( _ref(BufferLoadDesc)* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(BufferLoadDesc_delete)( _ref(BufferLoadDesc)* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, BufferLoadDesc_delete, _IDL);
HL_PRIM bool HL_NAME(Globals_initialize1)(vstring * name) {
	const char* name__cstr = (name == nullptr) ? "" : hl_to_utf8( name->bytes ); // Should be garbage collected
	auto ___retvalue = (hlForgeInitialize(name__cstr));
	return ___retvalue;
}
DEFINE_PRIM(_BOOL, Globals_initialize1, _STRING);

HL_PRIM void HL_NAME(Queue_waitIdle0)(_ref(Queue)* _this) {
	(waitQueueIdle( _unref(_this) ));
}
DEFINE_PRIM(_VOID, Queue_waitIdle0, _IDL);

HL_PRIM HL_CONST _ref(Renderer)* HL_NAME(Renderer_create1)(vstring * name) {
	const char* name__cstr = (name == nullptr) ? "" : hl_to_utf8( name->bytes ); // Should be garbage collected
	auto ___retvalue = alloc_ref_const((createRenderer(name__cstr)),Renderer);
	return ___retvalue;
}
DEFINE_PRIM(_IDL, Renderer_create1, _STRING);

HL_PRIM HL_CONST _ref(Queue)* HL_NAME(Renderer_createQueue0)(_ref(Renderer)* _this) {
	return alloc_ref_const((createQueue( _unref(_this) )),Queue);
}
DEFINE_PRIM(_IDL, Renderer_createQueue0, _IDL);

HL_PRIM void HL_NAME(Renderer_removeQueue1)(_ref(Renderer)* _this, _ref(Queue)* pGraphicsQueue) {
	(removeQueue( _unref(_this) , _unref_ptr_safe(pGraphicsQueue)));
}
DEFINE_PRIM(_VOID, Renderer_removeQueue1, _IDL _IDL);

HL_PRIM HL_CONST _ref(SwapChain)* HL_NAME(Window_createSwapChain6)(_ref(SDL_Window)* _this, _ref(Renderer)* renderer, _ref(Queue)* queue, int width, int height, int count, bool hdr10) {
	return alloc_ref_const((createSwapChain( _unref(_this) , _unref_ptr_safe(renderer), _unref_ptr_safe(queue), width, height, count, hdr10)),SwapChain);
}
DEFINE_PRIM(_IDL, Window_createSwapChain6, _IDL _IDL _IDL _I32 _I32 _I32 _BOOL);

HL_PRIM void HL_NAME(Buffer_updateRegion4)(_ref(Buffer)* _this, vbyte* data, int toffset, int size, int soffset) {
	(forge_sdl_buffer_update_region( _unref(_this) , data, toffset, size, soffset));
}
DEFINE_PRIM(_VOID, Buffer_updateRegion4, _IDL _BYTES _I32 _I32 _I32);

HL_PRIM void HL_NAME(Buffer_update1)(_ref(Buffer)* _this, vbyte* data) {
	(forge_sdl_buffer_update( _unref(_this) , data));
}
DEFINE_PRIM(_VOID, Buffer_update1, _IDL _BYTES);

HL_PRIM _ref(BufferLoadDesc)* HL_NAME(BufferLoadDesc_new0)() {
	return alloc_ref((new BufferLoadDesc()),BufferLoadDesc);
}
DEFINE_PRIM(_IDL, BufferLoadDesc_new0,);

HL_PRIM bool HL_NAME(BufferLoadDesc_get_forceReset)( _ref(BufferLoadDesc)* _this ) {
	return _unref(_this)->mForceReset;
}
DEFINE_PRIM(_BOOL,BufferLoadDesc_get_forceReset,_IDL);
HL_PRIM bool HL_NAME(BufferLoadDesc_set_forceReset)( _ref(BufferLoadDesc)* _this, bool value ) {
	_unref(_this)->mForceReset = (value);
	return value;
}
DEFINE_PRIM(_BOOL,BufferLoadDesc_set_forceReset,_IDL _BOOL);

HL_PRIM HL_CONST _ref(Buffer)* HL_NAME(BufferLoadDesc_load1)(_ref(BufferLoadDesc)* _this, _ref(SyncToken)* syncToken) {
	return alloc_ref_const((forge_sdl_buffer_load( _unref(_this) , _unref_ptr_safe(syncToken))),Buffer);
}
DEFINE_PRIM(_IDL, BufferLoadDesc_load1, _IDL _IDL);

HL_PRIM void HL_NAME(BufferLoadDesc_setIndexbuffer3)(_ref(BufferLoadDesc)* _this, int size, vbyte* data, bool shared) {
	(forge_sdl_buffer_load_desc_set_index_buffer( _unref(_this) , size, data, shared));
}
DEFINE_PRIM(_VOID, BufferLoadDesc_setIndexbuffer3, _IDL _I32 _BYTES _BOOL);

}
