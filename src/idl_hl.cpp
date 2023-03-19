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
#include "hl-idl-helpers.hpp"
// Need to link in helpers
//HL_API hl_type hltx_ui16;
//HL_API hl_type hltx_ui8;
HL_PRIM hl_type hltx_ui16 = { HUI16 };
HL_PRIM hl_type hltx_ui8 = { HUI8 };

#define _IDL _BYTES
#define _OPT(t) vdynamic *
#define _GET_OPT(value,t) (value)->v.t

static  hl_type *strType = nullptr;
void hl_cache_string_type( vstring *str) {
   strType = str->t;
}

vstring * hl_utf8_to_hlstr( const char *str) {
    int strLen = (int)strlen( str );
    uchar * ubuf = (uchar*)hl_gc_alloc_noptr((strLen + 1) << 1);
    hl_from_utf8( ubuf, strLen, str );

    vstring* vstr = (vstring *)hl_gc_alloc_raw(sizeof(vstring));

    vstr->bytes = ubuf;
    vstr->length = strLen;
    vstr->t = strType;
    return vstr;
}
vstring * hl_utf8_to_hlstr( const std::string &str) {
	return hl_utf8_to_hlstr(str.c_str());
}

HL_PRIM vstring * HL_NAME(getdllversion)(vstring * haxeversion) {
	strType = haxeversion->t;
	return haxeversion;
}
DEFINE_PRIM(_STRING, getdllversion, _STRING);

class HNativeBuffer {
    unsigned char *_ptr;
    int _size;

   public:
   inline unsigned char * ptr() { return _ptr; }
   inline int size() { return _size; }
   HNativeBuffer(unsigned char *ptr, int size) : _ptr(ptr), _size(size) {}
   HNativeBuffer(int size) : _ptr(new unsigned char[size]), _size(size) {}
    ~HNativeBuffer() {
        if (_ptr != nullptr)
            delete [] _ptr;
    }
};
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

inline void testvector(_h_float3 *v) {
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

static SampleCount SampleCount__values[] = { SAMPLE_COUNT_1,SAMPLE_COUNT_2,SAMPLE_COUNT_4,SAMPLE_COUNT_8,SAMPLE_COUNT_16 };
HL_PRIM int HL_NAME(SampleCount_toValue0)( int idx ) {
	return (int)SampleCount__values[idx];
}
DEFINE_PRIM(_I32, SampleCount_toValue0, _I32);
HL_PRIM int HL_NAME(SampleCount_indexToValue1)( int idx ) {
	return (int)SampleCount__values[idx];
}
DEFINE_PRIM(_I32, SampleCount_indexToValue1, _I32);
HL_PRIM int HL_NAME(SampleCount_valueToIndex1)( int value ) {
	for( int i = 0; i < 5; i++ ) if ( value == (int)SampleCount__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, SampleCount_valueToIndex1, _I32);
HL_PRIM int HL_NAME(SampleCount_fromValue1)( int value ) {
	for( int i = 0; i < 5; i++ ) if ( value == (int)SampleCount__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, SampleCount_fromValue1, _I32);
HL_PRIM int HL_NAME(SampleCount_fromIndex1)( int index ) {return index;}
DEFINE_PRIM(_I32, SampleCount_fromIndex1, _I32);
static ResourceState ResourceState__values[] = { RESOURCE_STATE_UNDEFINED,RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,RESOURCE_STATE_INDEX_BUFFER,RESOURCE_STATE_RENDER_TARGET,RESOURCE_STATE_UNORDERED_ACCESS,RESOURCE_STATE_DEPTH_WRITE,RESOURCE_STATE_DEPTH_READ,RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,RESOURCE_STATE_PIXEL_SHADER_RESOURCE,RESOURCE_STATE_SHADER_RESOURCE,RESOURCE_STATE_STREAM_OUT,RESOURCE_STATE_INDIRECT_ARGUMENT,RESOURCE_STATE_COPY_DEST,RESOURCE_STATE_COPY_SOURCE,RESOURCE_STATE_GENERIC_READ,RESOURCE_STATE_PRESENT,RESOURCE_STATE_COMMON,RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,RESOURCE_STATE_SHADING_RATE_SOURCE };
HL_PRIM int HL_NAME(ResourceState_toValue0)( int idx ) {
	return (int)ResourceState__values[idx];
}
DEFINE_PRIM(_I32, ResourceState_toValue0, _I32);
HL_PRIM int HL_NAME(ResourceState_indexToValue1)( int idx ) {
	return (int)ResourceState__values[idx];
}
DEFINE_PRIM(_I32, ResourceState_indexToValue1, _I32);
HL_PRIM int HL_NAME(ResourceState_valueToIndex1)( int value ) {
	for( int i = 0; i < 19; i++ ) if ( value == (int)ResourceState__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, ResourceState_valueToIndex1, _I32);
HL_PRIM int HL_NAME(ResourceState_fromValue1)( int value ) {
	for( int i = 0; i < 19; i++ ) if ( value == (int)ResourceState__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, ResourceState_fromValue1, _I32);
HL_PRIM int HL_NAME(ResourceState_fromIndex1)( int index ) {return index;}
DEFINE_PRIM(_I32, ResourceState_fromIndex1, _I32);
static TinyImageFormat TinyImageFormat__values[] = { TinyImageFormat_UNDEFINED,TinyImageFormat_R1_UNORM,TinyImageFormat_R2_UNORM,TinyImageFormat_R4_UNORM,TinyImageFormat_R4G4_UNORM,TinyImageFormat_G4R4_UNORM,TinyImageFormat_A8_UNORM,TinyImageFormat_R8_UNORM,TinyImageFormat_R8_SNORM,TinyImageFormat_R8_UINT,TinyImageFormat_R8_SINT,TinyImageFormat_R8_SRGB,TinyImageFormat_B2G3R3_UNORM,TinyImageFormat_R4G4B4A4_UNORM,TinyImageFormat_R4G4B4X4_UNORM,TinyImageFormat_B4G4R4A4_UNORM,TinyImageFormat_B4G4R4X4_UNORM,TinyImageFormat_A4R4G4B4_UNORM,TinyImageFormat_X4R4G4B4_UNORM,TinyImageFormat_A4B4G4R4_UNORM,TinyImageFormat_X4B4G4R4_UNORM,TinyImageFormat_R5G6B5_UNORM,TinyImageFormat_B5G6R5_UNORM,TinyImageFormat_R5G5B5A1_UNORM,TinyImageFormat_B5G5R5A1_UNORM,TinyImageFormat_A1B5G5R5_UNORM,TinyImageFormat_A1R5G5B5_UNORM,TinyImageFormat_R5G5B5X1_UNORM,TinyImageFormat_B5G5R5X1_UNORM,TinyImageFormat_X1R5G5B5_UNORM,TinyImageFormat_X1B5G5R5_UNORM,TinyImageFormat_B2G3R3A8_UNORM,TinyImageFormat_R8G8_UNORM,TinyImageFormat_R8G8_SNORM,TinyImageFormat_G8R8_UNORM,TinyImageFormat_G8R8_SNORM,TinyImageFormat_R8G8_UINT,TinyImageFormat_R8G8_SINT,TinyImageFormat_R8G8_SRGB,TinyImageFormat_R16_UNORM,TinyImageFormat_R16_SNORM,TinyImageFormat_R16_UINT,TinyImageFormat_R16_SINT,TinyImageFormat_R16_SFLOAT,TinyImageFormat_R16_SBFLOAT,TinyImageFormat_R8G8B8_UNORM,TinyImageFormat_R8G8B8_SNORM,TinyImageFormat_R8G8B8_UINT,TinyImageFormat_R8G8B8_SINT,TinyImageFormat_R8G8B8_SRGB,TinyImageFormat_B8G8R8_UNORM,TinyImageFormat_B8G8R8_SNORM,TinyImageFormat_B8G8R8_UINT,TinyImageFormat_B8G8R8_SINT,TinyImageFormat_B8G8R8_SRGB,TinyImageFormat_R8G8B8A8_UNORM,TinyImageFormat_R8G8B8A8_SNORM,TinyImageFormat_R8G8B8A8_UINT,TinyImageFormat_R8G8B8A8_SINT,TinyImageFormat_R8G8B8A8_SRGB,TinyImageFormat_B8G8R8A8_UNORM,TinyImageFormat_B8G8R8A8_SNORM,TinyImageFormat_B8G8R8A8_UINT,TinyImageFormat_B8G8R8A8_SINT,TinyImageFormat_B8G8R8A8_SRGB,TinyImageFormat_R8G8B8X8_UNORM,TinyImageFormat_B8G8R8X8_UNORM,TinyImageFormat_R16G16_UNORM,TinyImageFormat_G16R16_UNORM,TinyImageFormat_R16G16_SNORM,TinyImageFormat_G16R16_SNORM,TinyImageFormat_R16G16_UINT,TinyImageFormat_R16G16_SINT,TinyImageFormat_R16G16_SFLOAT,TinyImageFormat_R16G16_SBFLOAT,TinyImageFormat_R32_UINT,TinyImageFormat_R32_SINT,TinyImageFormat_R32_SFLOAT,TinyImageFormat_A2R10G10B10_UNORM,TinyImageFormat_A2R10G10B10_UINT,TinyImageFormat_A2R10G10B10_SNORM,TinyImageFormat_A2R10G10B10_SINT,TinyImageFormat_A2B10G10R10_UNORM,TinyImageFormat_A2B10G10R10_UINT,TinyImageFormat_A2B10G10R10_SNORM,TinyImageFormat_A2B10G10R10_SINT,TinyImageFormat_R10G10B10A2_UNORM,TinyImageFormat_R10G10B10A2_UINT,TinyImageFormat_R10G10B10A2_SNORM,TinyImageFormat_R10G10B10A2_SINT,TinyImageFormat_B10G10R10A2_UNORM,TinyImageFormat_B10G10R10A2_UINT,TinyImageFormat_B10G10R10A2_SNORM,TinyImageFormat_B10G10R10A2_SINT,TinyImageFormat_B10G11R11_UFLOAT,TinyImageFormat_E5B9G9R9_UFLOAT,TinyImageFormat_R16G16B16_UNORM,TinyImageFormat_R16G16B16_SNORM,TinyImageFormat_R16G16B16_UINT,TinyImageFormat_R16G16B16_SINT,TinyImageFormat_R16G16B16_SFLOAT,TinyImageFormat_R16G16B16_SBFLOAT,TinyImageFormat_R16G16B16A16_UNORM,TinyImageFormat_R16G16B16A16_SNORM,TinyImageFormat_R16G16B16A16_UINT,TinyImageFormat_R16G16B16A16_SINT,TinyImageFormat_R16G16B16A16_SFLOAT,TinyImageFormat_R16G16B16A16_SBFLOAT,TinyImageFormat_R32G32_UINT,TinyImageFormat_R32G32_SINT,TinyImageFormat_R32G32_SFLOAT,TinyImageFormat_R32G32B32_UINT,TinyImageFormat_R32G32B32_SINT,TinyImageFormat_R32G32B32_SFLOAT,TinyImageFormat_R32G32B32A32_UINT,TinyImageFormat_R32G32B32A32_SINT,TinyImageFormat_R32G32B32A32_SFLOAT,TinyImageFormat_R64_UINT,TinyImageFormat_R64_SINT,TinyImageFormat_R64_SFLOAT,TinyImageFormat_R64G64_UINT,TinyImageFormat_R64G64_SINT,TinyImageFormat_R64G64_SFLOAT,TinyImageFormat_R64G64B64_UINT,TinyImageFormat_R64G64B64_SINT,TinyImageFormat_R64G64B64_SFLOAT,TinyImageFormat_R64G64B64A64_UINT,TinyImageFormat_R64G64B64A64_SINT,TinyImageFormat_R64G64B64A64_SFLOAT,TinyImageFormat_D16_UNORM,TinyImageFormat_X8_D24_UNORM,TinyImageFormat_D32_SFLOAT,TinyImageFormat_S8_UINT,TinyImageFormat_D16_UNORM_S8_UINT,TinyImageFormat_D24_UNORM_S8_UINT,TinyImageFormat_D32_SFLOAT_S8_UINT,TinyImageFormat_DXBC1_RGB_UNORM,TinyImageFormat_DXBC1_RGB_SRGB,TinyImageFormat_DXBC1_RGBA_UNORM,TinyImageFormat_DXBC1_RGBA_SRGB,TinyImageFormat_DXBC2_UNORM,TinyImageFormat_DXBC2_SRGB,TinyImageFormat_DXBC3_UNORM,TinyImageFormat_DXBC3_SRGB,TinyImageFormat_DXBC4_UNORM,TinyImageFormat_DXBC4_SNORM,TinyImageFormat_DXBC5_UNORM,TinyImageFormat_DXBC5_SNORM,TinyImageFormat_DXBC6H_UFLOAT,TinyImageFormat_DXBC6H_SFLOAT,TinyImageFormat_DXBC7_UNORM,TinyImageFormat_DXBC7_SRGB,TinyImageFormat_PVRTC1_2BPP_UNORM,TinyImageFormat_PVRTC1_4BPP_UNORM,TinyImageFormat_PVRTC2_2BPP_UNORM,TinyImageFormat_PVRTC2_4BPP_UNORM,TinyImageFormat_PVRTC1_2BPP_SRGB,TinyImageFormat_PVRTC1_4BPP_SRGB,TinyImageFormat_PVRTC2_2BPP_SRGB,TinyImageFormat_PVRTC2_4BPP_SRGB,TinyImageFormat_ETC2_R8G8B8_UNORM,TinyImageFormat_ETC2_R8G8B8_SRGB,TinyImageFormat_ETC2_R8G8B8A1_UNORM,TinyImageFormat_ETC2_R8G8B8A1_SRGB,TinyImageFormat_ETC2_R8G8B8A8_UNORM,TinyImageFormat_ETC2_R8G8B8A8_SRGB,TinyImageFormat_ETC2_EAC_R11_UNORM,TinyImageFormat_ETC2_EAC_R11_SNORM,TinyImageFormat_ETC2_EAC_R11G11_UNORM,TinyImageFormat_ETC2_EAC_R11G11_SNORM,TinyImageFormat_ASTC_4x4_UNORM,TinyImageFormat_ASTC_4x4_SRGB,TinyImageFormat_ASTC_5x4_UNORM,TinyImageFormat_ASTC_5x4_SRGB,TinyImageFormat_ASTC_5x5_UNORM,TinyImageFormat_ASTC_5x5_SRGB,TinyImageFormat_ASTC_6x5_UNORM,TinyImageFormat_ASTC_6x5_SRGB,TinyImageFormat_ASTC_6x6_UNORM,TinyImageFormat_ASTC_6x6_SRGB,TinyImageFormat_ASTC_8x5_UNORM,TinyImageFormat_ASTC_8x5_SRGB,TinyImageFormat_ASTC_8x6_UNORM,TinyImageFormat_ASTC_8x6_SRGB,TinyImageFormat_ASTC_8x8_UNORM,TinyImageFormat_ASTC_8x8_SRGB,TinyImageFormat_ASTC_10x5_UNORM,TinyImageFormat_ASTC_10x5_SRGB,TinyImageFormat_ASTC_10x6_UNORM,TinyImageFormat_ASTC_10x6_SRGB,TinyImageFormat_ASTC_10x8_UNORM,TinyImageFormat_ASTC_10x8_SRGB,TinyImageFormat_ASTC_10x10_UNORM,TinyImageFormat_ASTC_10x10_SRGB,TinyImageFormat_ASTC_12x10_UNORM,TinyImageFormat_ASTC_12x10_SRGB,TinyImageFormat_ASTC_12x12_UNORM,TinyImageFormat_ASTC_12x12_SRGB,TinyImageFormat_CLUT_P4,TinyImageFormat_CLUT_P4A4,TinyImageFormat_CLUT_P8,TinyImageFormat_CLUT_P8A8,TinyImageFormat_R4G4B4A4_UNORM_PACK16,TinyImageFormat_B4G4R4A4_UNORM_PACK16,TinyImageFormat_R5G6B5_UNORM_PACK16,TinyImageFormat_B5G6R5_UNORM_PACK16,TinyImageFormat_R5G5B5A1_UNORM_PACK16,TinyImageFormat_B5G5R5A1_UNORM_PACK16,TinyImageFormat_A1R5G5B5_UNORM_PACK16,TinyImageFormat_G16B16G16R16_422_UNORM,TinyImageFormat_B16G16R16G16_422_UNORM,TinyImageFormat_R12X4G12X4B12X4A12X4_UNORM_4PACK16,TinyImageFormat_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16,TinyImageFormat_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16,TinyImageFormat_R10X6G10X6B10X6A10X6_UNORM_4PACK16,TinyImageFormat_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16,TinyImageFormat_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16,TinyImageFormat_G8B8G8R8_422_UNORM,TinyImageFormat_B8G8R8G8_422_UNORM,TinyImageFormat_G8_B8_R8_3PLANE_420_UNORM,TinyImageFormat_G8_B8R8_2PLANE_420_UNORM,TinyImageFormat_G8_B8_R8_3PLANE_422_UNORM,TinyImageFormat_G8_B8R8_2PLANE_422_UNORM,TinyImageFormat_G8_B8_R8_3PLANE_444_UNORM,TinyImageFormat_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16,TinyImageFormat_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16,TinyImageFormat_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16,TinyImageFormat_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16,TinyImageFormat_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16,TinyImageFormat_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16,TinyImageFormat_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16,TinyImageFormat_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16,TinyImageFormat_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16,TinyImageFormat_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16,TinyImageFormat_G16_B16_R16_3PLANE_420_UNORM,TinyImageFormat_G16_B16_R16_3PLANE_422_UNORM,TinyImageFormat_G16_B16_R16_3PLANE_444_UNORM,TinyImageFormat_G16_B16R16_2PLANE_420_UNORM,TinyImageFormat_G16_B16R16_2PLANE_422_UNORM };
HL_PRIM int HL_NAME(TinyImageFormat_toValue0)( int idx ) {
	return (int)TinyImageFormat__values[idx];
}
DEFINE_PRIM(_I32, TinyImageFormat_toValue0, _I32);
HL_PRIM int HL_NAME(TinyImageFormat_indexToValue1)( int idx ) {
	return (int)TinyImageFormat__values[idx];
}
DEFINE_PRIM(_I32, TinyImageFormat_indexToValue1, _I32);
HL_PRIM int HL_NAME(TinyImageFormat_valueToIndex1)( int value ) {
	for( int i = 0; i < 239; i++ ) if ( value == (int)TinyImageFormat__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, TinyImageFormat_valueToIndex1, _I32);
HL_PRIM int HL_NAME(TinyImageFormat_fromValue1)( int value ) {
	for( int i = 0; i < 239; i++ ) if ( value == (int)TinyImageFormat__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, TinyImageFormat_fromValue1, _I32);
HL_PRIM int HL_NAME(TinyImageFormat_fromIndex1)( int index ) {return index;}
DEFINE_PRIM(_I32, TinyImageFormat_fromIndex1, _I32);
static TextureCreationFlags TextureCreationFlags__values[] = { TEXTURE_CREATION_FLAG_NONE,TEXTURE_CREATION_FLAG_OWN_MEMORY_BIT,TEXTURE_CREATION_FLAG_EXPORT_BIT,TEXTURE_CREATION_FLAG_EXPORT_ADAPTER_BIT,TEXTURE_CREATION_FLAG_IMPORT_BIT,TEXTURE_CREATION_FLAG_ESRAM,TEXTURE_CREATION_FLAG_ON_TILE,TEXTURE_CREATION_FLAG_NO_COMPRESSION,TEXTURE_CREATION_FLAG_FORCE_2D,TEXTURE_CREATION_FLAG_FORCE_3D,TEXTURE_CREATION_FLAG_ALLOW_DISPLAY_TARGET,TEXTURE_CREATION_FLAG_SRGB,TEXTURE_CREATION_FLAG_NORMAL_MAP,TEXTURE_CREATION_FLAG_FAST_CLEAR,TEXTURE_CREATION_FLAG_FRAG_MASK,TEXTURE_CREATION_FLAG_VR_MULTIVIEW,TEXTURE_CREATION_FLAG_VR_FOVEATED_RENDERING };
HL_PRIM int HL_NAME(TextureCreationFlags_toValue0)( int idx ) {
	return (int)TextureCreationFlags__values[idx];
}
DEFINE_PRIM(_I32, TextureCreationFlags_toValue0, _I32);
HL_PRIM int HL_NAME(TextureCreationFlags_indexToValue1)( int idx ) {
	return (int)TextureCreationFlags__values[idx];
}
DEFINE_PRIM(_I32, TextureCreationFlags_indexToValue1, _I32);
HL_PRIM int HL_NAME(TextureCreationFlags_valueToIndex1)( int value ) {
	for( int i = 0; i < 17; i++ ) if ( value == (int)TextureCreationFlags__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, TextureCreationFlags_valueToIndex1, _I32);
HL_PRIM int HL_NAME(TextureCreationFlags_fromValue1)( int value ) {
	for( int i = 0; i < 17; i++ ) if ( value == (int)TextureCreationFlags__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, TextureCreationFlags_fromValue1, _I32);
HL_PRIM int HL_NAME(TextureCreationFlags_fromIndex1)( int index ) {return index;}
DEFINE_PRIM(_I32, TextureCreationFlags_fromIndex1, _I32);
static BlendStateTargets BlendStateTargets__values[] = { BLEND_STATE_TARGET_0,BLEND_STATE_TARGET_1,BLEND_STATE_TARGET_2,BLEND_STATE_TARGET_3,BLEND_STATE_TARGET_4,BLEND_STATE_TARGET_5,BLEND_STATE_TARGET_6,BLEND_STATE_TARGET_7,BLEND_STATE_TARGET_ALL };
HL_PRIM int HL_NAME(BlendStateTargets_toValue0)( int idx ) {
	return (int)BlendStateTargets__values[idx];
}
DEFINE_PRIM(_I32, BlendStateTargets_toValue0, _I32);
HL_PRIM int HL_NAME(BlendStateTargets_indexToValue1)( int idx ) {
	return (int)BlendStateTargets__values[idx];
}
DEFINE_PRIM(_I32, BlendStateTargets_indexToValue1, _I32);
HL_PRIM int HL_NAME(BlendStateTargets_valueToIndex1)( int value ) {
	for( int i = 0; i < 9; i++ ) if ( value == (int)BlendStateTargets__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, BlendStateTargets_valueToIndex1, _I32);
HL_PRIM int HL_NAME(BlendStateTargets_fromValue1)( int value ) {
	for( int i = 0; i < 9; i++ ) if ( value == (int)BlendStateTargets__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, BlendStateTargets_fromValue1, _I32);
HL_PRIM int HL_NAME(BlendStateTargets_fromIndex1)( int index ) {return index;}
DEFINE_PRIM(_I32, BlendStateTargets_fromIndex1, _I32);
static CullMode CullMode__values[] = { CULL_MODE_NONE,CULL_MODE_BACK,CULL_MODE_FRONT,CULL_MODE_BOTH,MAX_CULL_MODES };
HL_PRIM int HL_NAME(CullMode_toValue0)( int idx ) {
	return (int)CullMode__values[idx];
}
DEFINE_PRIM(_I32, CullMode_toValue0, _I32);
HL_PRIM int HL_NAME(CullMode_indexToValue1)( int idx ) {
	return (int)CullMode__values[idx];
}
DEFINE_PRIM(_I32, CullMode_indexToValue1, _I32);
HL_PRIM int HL_NAME(CullMode_valueToIndex1)( int value ) {
	for( int i = 0; i < 5; i++ ) if ( value == (int)CullMode__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, CullMode_valueToIndex1, _I32);
HL_PRIM int HL_NAME(CullMode_fromValue1)( int value ) {
	for( int i = 0; i < 5; i++ ) if ( value == (int)CullMode__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, CullMode_fromValue1, _I32);
HL_PRIM int HL_NAME(CullMode_fromIndex1)( int index ) {return index;}
DEFINE_PRIM(_I32, CullMode_fromIndex1, _I32);
static FrontFace FrontFace__values[] = { FRONT_FACE_CCW,FRONT_FACE_CW };
HL_PRIM int HL_NAME(FrontFace_toValue0)( int idx ) {
	return (int)FrontFace__values[idx];
}
DEFINE_PRIM(_I32, FrontFace_toValue0, _I32);
HL_PRIM int HL_NAME(FrontFace_indexToValue1)( int idx ) {
	return (int)FrontFace__values[idx];
}
DEFINE_PRIM(_I32, FrontFace_indexToValue1, _I32);
HL_PRIM int HL_NAME(FrontFace_valueToIndex1)( int value ) {
	for( int i = 0; i < 2; i++ ) if ( value == (int)FrontFace__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, FrontFace_valueToIndex1, _I32);
HL_PRIM int HL_NAME(FrontFace_fromValue1)( int value ) {
	for( int i = 0; i < 2; i++ ) if ( value == (int)FrontFace__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, FrontFace_fromValue1, _I32);
HL_PRIM int HL_NAME(FrontFace_fromIndex1)( int index ) {return index;}
DEFINE_PRIM(_I32, FrontFace_fromIndex1, _I32);
static FillMode FillMode__values[] = { FILL_MODE_SOLID,FILL_MODE_WIREFRAME,MAX_FILL_MODES };
HL_PRIM int HL_NAME(FillMode_toValue0)( int idx ) {
	return (int)FillMode__values[idx];
}
DEFINE_PRIM(_I32, FillMode_toValue0, _I32);
HL_PRIM int HL_NAME(FillMode_indexToValue1)( int idx ) {
	return (int)FillMode__values[idx];
}
DEFINE_PRIM(_I32, FillMode_indexToValue1, _I32);
HL_PRIM int HL_NAME(FillMode_valueToIndex1)( int value ) {
	for( int i = 0; i < 3; i++ ) if ( value == (int)FillMode__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, FillMode_valueToIndex1, _I32);
HL_PRIM int HL_NAME(FillMode_fromValue1)( int value ) {
	for( int i = 0; i < 3; i++ ) if ( value == (int)FillMode__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, FillMode_fromValue1, _I32);
HL_PRIM int HL_NAME(FillMode_fromIndex1)( int index ) {return index;}
DEFINE_PRIM(_I32, FillMode_fromIndex1, _I32);
static PipelineType PipelineType__values[] = { PIPELINE_TYPE_UNDEFINED,PIPELINE_TYPE_COMPUTE,PIPELINE_TYPE_GRAPHICS,PIPELINE_TYPE_RAYTRACING,PIPELINE_TYPE_COUNT };
HL_PRIM int HL_NAME(PipelineType_toValue0)( int idx ) {
	return (int)PipelineType__values[idx];
}
DEFINE_PRIM(_I32, PipelineType_toValue0, _I32);
HL_PRIM int HL_NAME(PipelineType_indexToValue1)( int idx ) {
	return (int)PipelineType__values[idx];
}
DEFINE_PRIM(_I32, PipelineType_indexToValue1, _I32);
HL_PRIM int HL_NAME(PipelineType_valueToIndex1)( int value ) {
	for( int i = 0; i < 5; i++ ) if ( value == (int)PipelineType__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, PipelineType_valueToIndex1, _I32);
HL_PRIM int HL_NAME(PipelineType_fromValue1)( int value ) {
	for( int i = 0; i < 5; i++ ) if ( value == (int)PipelineType__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, PipelineType_fromValue1, _I32);
HL_PRIM int HL_NAME(PipelineType_fromIndex1)( int index ) {return index;}
DEFINE_PRIM(_I32, PipelineType_fromIndex1, _I32);
static FilterType FilterType__values[] = { FILTER_NEAREST,FILTER_LINEAR };
HL_PRIM int HL_NAME(FilterType_toValue0)( int idx ) {
	return (int)FilterType__values[idx];
}
DEFINE_PRIM(_I32, FilterType_toValue0, _I32);
HL_PRIM int HL_NAME(FilterType_indexToValue1)( int idx ) {
	return (int)FilterType__values[idx];
}
DEFINE_PRIM(_I32, FilterType_indexToValue1, _I32);
HL_PRIM int HL_NAME(FilterType_valueToIndex1)( int value ) {
	for( int i = 0; i < 2; i++ ) if ( value == (int)FilterType__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, FilterType_valueToIndex1, _I32);
HL_PRIM int HL_NAME(FilterType_fromValue1)( int value ) {
	for( int i = 0; i < 2; i++ ) if ( value == (int)FilterType__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, FilterType_fromValue1, _I32);
HL_PRIM int HL_NAME(FilterType_fromIndex1)( int index ) {return index;}
DEFINE_PRIM(_I32, FilterType_fromIndex1, _I32);
static AddressMode AddressMode__values[] = { ADDRESS_MODE_MIRROR,ADDRESS_MODE_REPEAT,ADDRESS_MODE_CLAMP_TO_EDGE,ADDRESS_MODE_CLAMP_TO_BORDER };
HL_PRIM int HL_NAME(AddressMode_toValue0)( int idx ) {
	return (int)AddressMode__values[idx];
}
DEFINE_PRIM(_I32, AddressMode_toValue0, _I32);
HL_PRIM int HL_NAME(AddressMode_indexToValue1)( int idx ) {
	return (int)AddressMode__values[idx];
}
DEFINE_PRIM(_I32, AddressMode_indexToValue1, _I32);
HL_PRIM int HL_NAME(AddressMode_valueToIndex1)( int value ) {
	for( int i = 0; i < 4; i++ ) if ( value == (int)AddressMode__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, AddressMode_valueToIndex1, _I32);
HL_PRIM int HL_NAME(AddressMode_fromValue1)( int value ) {
	for( int i = 0; i < 4; i++ ) if ( value == (int)AddressMode__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, AddressMode_fromValue1, _I32);
HL_PRIM int HL_NAME(AddressMode_fromIndex1)( int index ) {return index;}
DEFINE_PRIM(_I32, AddressMode_fromIndex1, _I32);
static MipMapMode MipMapMode__values[] = { MIPMAP_MODE_NEAREST,MIPMAP_MODE_LINEAR };
HL_PRIM int HL_NAME(MipMapMode_toValue0)( int idx ) {
	return (int)MipMapMode__values[idx];
}
DEFINE_PRIM(_I32, MipMapMode_toValue0, _I32);
HL_PRIM int HL_NAME(MipMapMode_indexToValue1)( int idx ) {
	return (int)MipMapMode__values[idx];
}
DEFINE_PRIM(_I32, MipMapMode_indexToValue1, _I32);
HL_PRIM int HL_NAME(MipMapMode_valueToIndex1)( int value ) {
	for( int i = 0; i < 2; i++ ) if ( value == (int)MipMapMode__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, MipMapMode_valueToIndex1, _I32);
HL_PRIM int HL_NAME(MipMapMode_fromValue1)( int value ) {
	for( int i = 0; i < 2; i++ ) if ( value == (int)MipMapMode__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, MipMapMode_fromValue1, _I32);
HL_PRIM int HL_NAME(MipMapMode_fromIndex1)( int index ) {return index;}
DEFINE_PRIM(_I32, MipMapMode_fromIndex1, _I32);
static PrimitiveTopology PrimitiveTopology__values[] = { PRIMITIVE_TOPO_POINT_LIST,PRIMITIVE_TOPO_LINE_LIST,PRIMITIVE_TOPO_LINE_STRIP,PRIMITIVE_TOPO_TRI_LIST,PRIMITIVE_TOPO_TRI_STRIP,PRIMITIVE_TOPO_PATCH_LIST,PRIMITIVE_TOPO_COUNT };
HL_PRIM int HL_NAME(PrimitiveTopology_toValue0)( int idx ) {
	return (int)PrimitiveTopology__values[idx];
}
DEFINE_PRIM(_I32, PrimitiveTopology_toValue0, _I32);
HL_PRIM int HL_NAME(PrimitiveTopology_indexToValue1)( int idx ) {
	return (int)PrimitiveTopology__values[idx];
}
DEFINE_PRIM(_I32, PrimitiveTopology_indexToValue1, _I32);
HL_PRIM int HL_NAME(PrimitiveTopology_valueToIndex1)( int value ) {
	for( int i = 0; i < 7; i++ ) if ( value == (int)PrimitiveTopology__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, PrimitiveTopology_valueToIndex1, _I32);
HL_PRIM int HL_NAME(PrimitiveTopology_fromValue1)( int value ) {
	for( int i = 0; i < 7; i++ ) if ( value == (int)PrimitiveTopology__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, PrimitiveTopology_fromValue1, _I32);
HL_PRIM int HL_NAME(PrimitiveTopology_fromIndex1)( int index ) {return index;}
DEFINE_PRIM(_I32, PrimitiveTopology_fromIndex1, _I32);
static IndexType IndexType__values[] = { INDEX_TYPE_UINT32,INDEX_TYPE_UINT16 };
HL_PRIM int HL_NAME(IndexType_toValue0)( int idx ) {
	return (int)IndexType__values[idx];
}
DEFINE_PRIM(_I32, IndexType_toValue0, _I32);
HL_PRIM int HL_NAME(IndexType_indexToValue1)( int idx ) {
	return (int)IndexType__values[idx];
}
DEFINE_PRIM(_I32, IndexType_indexToValue1, _I32);
HL_PRIM int HL_NAME(IndexType_valueToIndex1)( int value ) {
	for( int i = 0; i < 2; i++ ) if ( value == (int)IndexType__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, IndexType_valueToIndex1, _I32);
HL_PRIM int HL_NAME(IndexType_fromValue1)( int value ) {
	for( int i = 0; i < 2; i++ ) if ( value == (int)IndexType__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, IndexType_fromValue1, _I32);
HL_PRIM int HL_NAME(IndexType_fromIndex1)( int index ) {return index;}
DEFINE_PRIM(_I32, IndexType_fromIndex1, _I32);
static ShaderSemantic ShaderSemantic__values[] = { SEMANTIC_UNDEFINED,SEMANTIC_POSITION,SEMANTIC_NORMAL,SEMANTIC_COLOR,SEMANTIC_TANGENT,SEMANTIC_BITANGENT,SEMANTIC_JOINTS,SEMANTIC_WEIGHTS,SEMANTIC_SHADING_RATE,SEMANTIC_TEXCOORD0,SEMANTIC_TEXCOORD1,SEMANTIC_TEXCOORD2,SEMANTIC_TEXCOORD3,SEMANTIC_TEXCOORD4,SEMANTIC_TEXCOORD5,SEMANTIC_TEXCOORD6,SEMANTIC_TEXCOORD7,SEMANTIC_TEXCOORD8,SEMANTIC_TEXCOORD9 };
HL_PRIM int HL_NAME(ShaderSemantic_toValue0)( int idx ) {
	return (int)ShaderSemantic__values[idx];
}
DEFINE_PRIM(_I32, ShaderSemantic_toValue0, _I32);
HL_PRIM int HL_NAME(ShaderSemantic_indexToValue1)( int idx ) {
	return (int)ShaderSemantic__values[idx];
}
DEFINE_PRIM(_I32, ShaderSemantic_indexToValue1, _I32);
HL_PRIM int HL_NAME(ShaderSemantic_valueToIndex1)( int value ) {
	for( int i = 0; i < 19; i++ ) if ( value == (int)ShaderSemantic__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, ShaderSemantic_valueToIndex1, _I32);
HL_PRIM int HL_NAME(ShaderSemantic_fromValue1)( int value ) {
	for( int i = 0; i < 19; i++ ) if ( value == (int)ShaderSemantic__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, ShaderSemantic_fromValue1, _I32);
HL_PRIM int HL_NAME(ShaderSemantic_fromIndex1)( int index ) {return index;}
DEFINE_PRIM(_I32, ShaderSemantic_fromIndex1, _I32);
static BlendConstant BlendConstant__values[] = { BC_ZERO,BC_ONE,BC_SRC_COLOR,BC_ONE_MINUS_SRC_COLOR,BC_DST_COLOR,BC_ONE_MINUS_DST_COLOR,BC_SRC_ALPHA,BC_ONE_MINUS_SRC_ALPHA,BC_DST_ALPHA,BC_ONE_MINUS_DST_ALPHA,BC_SRC_ALPHA_SATURATE,BC_BLEND_FACTOR,BC_ONE_MINUS_BLEND_FACTOR,MAX_BLEND_CONSTANTS };
HL_PRIM int HL_NAME(BlendConstant_toValue0)( int idx ) {
	return (int)BlendConstant__values[idx];
}
DEFINE_PRIM(_I32, BlendConstant_toValue0, _I32);
HL_PRIM int HL_NAME(BlendConstant_indexToValue1)( int idx ) {
	return (int)BlendConstant__values[idx];
}
DEFINE_PRIM(_I32, BlendConstant_indexToValue1, _I32);
HL_PRIM int HL_NAME(BlendConstant_valueToIndex1)( int value ) {
	for( int i = 0; i < 14; i++ ) if ( value == (int)BlendConstant__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, BlendConstant_valueToIndex1, _I32);
HL_PRIM int HL_NAME(BlendConstant_fromValue1)( int value ) {
	for( int i = 0; i < 14; i++ ) if ( value == (int)BlendConstant__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, BlendConstant_fromValue1, _I32);
HL_PRIM int HL_NAME(BlendConstant_fromIndex1)( int index ) {return index;}
DEFINE_PRIM(_I32, BlendConstant_fromIndex1, _I32);
static BlendMode BlendMode__values[] = { BM_ADD,BM_SUBTRACT,BM_REVERSE_SUBTRACT,BM_MIN,BM_MAX,MAX_BLEND_MODES };
HL_PRIM int HL_NAME(BlendMode_toValue0)( int idx ) {
	return (int)BlendMode__values[idx];
}
DEFINE_PRIM(_I32, BlendMode_toValue0, _I32);
HL_PRIM int HL_NAME(BlendMode_indexToValue1)( int idx ) {
	return (int)BlendMode__values[idx];
}
DEFINE_PRIM(_I32, BlendMode_indexToValue1, _I32);
HL_PRIM int HL_NAME(BlendMode_valueToIndex1)( int value ) {
	for( int i = 0; i < 6; i++ ) if ( value == (int)BlendMode__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, BlendMode_valueToIndex1, _I32);
HL_PRIM int HL_NAME(BlendMode_fromValue1)( int value ) {
	for( int i = 0; i < 6; i++ ) if ( value == (int)BlendMode__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, BlendMode_fromValue1, _I32);
HL_PRIM int HL_NAME(BlendMode_fromIndex1)( int index ) {return index;}
DEFINE_PRIM(_I32, BlendMode_fromIndex1, _I32);
static CompareMode CompareMode__values[] = { CMP_NEVER,CMP_LESS,CMP_EQUAL,CMP_LEQUAL,CMP_GREATER,CMP_NOTEQUAL,CMP_GEQUAL,CMP_ALWAYS,MAX_COMPARE_MODES };
HL_PRIM int HL_NAME(CompareMode_toValue0)( int idx ) {
	return (int)CompareMode__values[idx];
}
DEFINE_PRIM(_I32, CompareMode_toValue0, _I32);
HL_PRIM int HL_NAME(CompareMode_indexToValue1)( int idx ) {
	return (int)CompareMode__values[idx];
}
DEFINE_PRIM(_I32, CompareMode_indexToValue1, _I32);
HL_PRIM int HL_NAME(CompareMode_valueToIndex1)( int value ) {
	for( int i = 0; i < 9; i++ ) if ( value == (int)CompareMode__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, CompareMode_valueToIndex1, _I32);
HL_PRIM int HL_NAME(CompareMode_fromValue1)( int value ) {
	for( int i = 0; i < 9; i++ ) if ( value == (int)CompareMode__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, CompareMode_fromValue1, _I32);
HL_PRIM int HL_NAME(CompareMode_fromIndex1)( int index ) {return index;}
DEFINE_PRIM(_I32, CompareMode_fromIndex1, _I32);
static StencilOp StencilOp__values[] = { STENCIL_OP_KEEP,STENCIL_OP_SET_ZERO,STENCIL_OP_REPLACE,STENCIL_OP_INVERT,STENCIL_OP_INCR,STENCIL_OP_DECR,STENCIL_OP_INCR_SAT,STENCIL_OP_DECR_SAT,MAX_STENCIL_OPS };
HL_PRIM int HL_NAME(StencilOp_toValue0)( int idx ) {
	return (int)StencilOp__values[idx];
}
DEFINE_PRIM(_I32, StencilOp_toValue0, _I32);
HL_PRIM int HL_NAME(StencilOp_indexToValue1)( int idx ) {
	return (int)StencilOp__values[idx];
}
DEFINE_PRIM(_I32, StencilOp_indexToValue1, _I32);
HL_PRIM int HL_NAME(StencilOp_valueToIndex1)( int value ) {
	for( int i = 0; i < 9; i++ ) if ( value == (int)StencilOp__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, StencilOp_valueToIndex1, _I32);
HL_PRIM int HL_NAME(StencilOp_fromValue1)( int value ) {
	for( int i = 0; i < 9; i++ ) if ( value == (int)StencilOp__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, StencilOp_fromValue1, _I32);
HL_PRIM int HL_NAME(StencilOp_fromIndex1)( int index ) {return index;}
DEFINE_PRIM(_I32, StencilOp_fromIndex1, _I32);
static DescriptorUpdateFrequency DescriptorUpdateFrequency__values[] = { DESCRIPTOR_UPDATE_FREQ_NONE,DESCRIPTOR_UPDATE_FREQ_PER_FRAME,DESCRIPTOR_UPDATE_FREQ_PER_BATCH,DESCRIPTOR_UPDATE_FREQ_PER_DRAW,DESCRIPTOR_UPDATE_FREQ_COUNT };
HL_PRIM int HL_NAME(DescriptorUpdateFrequency_toValue0)( int idx ) {
	return (int)DescriptorUpdateFrequency__values[idx];
}
DEFINE_PRIM(_I32, DescriptorUpdateFrequency_toValue0, _I32);
HL_PRIM int HL_NAME(DescriptorUpdateFrequency_indexToValue1)( int idx ) {
	return (int)DescriptorUpdateFrequency__values[idx];
}
DEFINE_PRIM(_I32, DescriptorUpdateFrequency_indexToValue1, _I32);
HL_PRIM int HL_NAME(DescriptorUpdateFrequency_valueToIndex1)( int value ) {
	for( int i = 0; i < 5; i++ ) if ( value == (int)DescriptorUpdateFrequency__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, DescriptorUpdateFrequency_valueToIndex1, _I32);
HL_PRIM int HL_NAME(DescriptorUpdateFrequency_fromValue1)( int value ) {
	for( int i = 0; i < 5; i++ ) if ( value == (int)DescriptorUpdateFrequency__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, DescriptorUpdateFrequency_fromValue1, _I32);
HL_PRIM int HL_NAME(DescriptorUpdateFrequency_fromIndex1)( int index ) {return index;}
DEFINE_PRIM(_I32, DescriptorUpdateFrequency_fromIndex1, _I32);
static DescriptorType DescriptorType__values[] = { DESCRIPTOR_TYPE_UNDEFINED,DESCRIPTOR_TYPE_SAMPLER,DESCRIPTOR_TYPE_TEXTURE,DESCRIPTOR_TYPE_RW_TEXTURE,DESCRIPTOR_TYPE_BUFFER,DESCRIPTOR_TYPE_RW_BUFFER,DESCRIPTOR_TYPE_UNIFORM_BUFFER,DESCRIPTOR_TYPE_ROOT_CONSTANT,DESCRIPTOR_TYPE_VERTEX_BUFFER,DESCRIPTOR_TYPE_INDEX_BUFFER,DESCRIPTOR_TYPE_INDIRECT_BUFFER,DESCRIPTOR_TYPE_TEXTURE_CUBE };
HL_PRIM int HL_NAME(DescriptorType_toValue0)( int idx ) {
	return (int)DescriptorType__values[idx];
}
DEFINE_PRIM(_I32, DescriptorType_toValue0, _I32);
HL_PRIM int HL_NAME(DescriptorType_indexToValue1)( int idx ) {
	return (int)DescriptorType__values[idx];
}
DEFINE_PRIM(_I32, DescriptorType_indexToValue1, _I32);
HL_PRIM int HL_NAME(DescriptorType_valueToIndex1)( int value ) {
	for( int i = 0; i < 12; i++ ) if ( value == (int)DescriptorType__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, DescriptorType_valueToIndex1, _I32);
HL_PRIM int HL_NAME(DescriptorType_fromValue1)( int value ) {
	for( int i = 0; i < 12; i++ ) if ( value == (int)DescriptorType__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, DescriptorType_fromValue1, _I32);
HL_PRIM int HL_NAME(DescriptorType_fromIndex1)( int index ) {return index;}
DEFINE_PRIM(_I32, DescriptorType_fromIndex1, _I32);
static LoadActionType LoadActionType__values[] = { LOAD_ACTION_DONTCARE,LOAD_ACTION_LOAD,LOAD_ACTION_CLEAR,MAX_LOAD_ACTION };
HL_PRIM int HL_NAME(LoadActionType_toValue0)( int idx ) {
	return (int)LoadActionType__values[idx];
}
DEFINE_PRIM(_I32, LoadActionType_toValue0, _I32);
HL_PRIM int HL_NAME(LoadActionType_indexToValue1)( int idx ) {
	return (int)LoadActionType__values[idx];
}
DEFINE_PRIM(_I32, LoadActionType_indexToValue1, _I32);
HL_PRIM int HL_NAME(LoadActionType_valueToIndex1)( int value ) {
	for( int i = 0; i < 4; i++ ) if ( value == (int)LoadActionType__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, LoadActionType_valueToIndex1, _I32);
HL_PRIM int HL_NAME(LoadActionType_fromValue1)( int value ) {
	for( int i = 0; i < 4; i++ ) if ( value == (int)LoadActionType__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, LoadActionType_fromValue1, _I32);
HL_PRIM int HL_NAME(LoadActionType_fromIndex1)( int index ) {return index;}
DEFINE_PRIM(_I32, LoadActionType_fromIndex1, _I32);
static void finalize_HashBuilder( pref<HashBuilder>* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(HashBuilder_delete)( pref<HashBuilder>* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, HashBuilder_delete, _IDL);
static void finalize_StateBuilder( pref<StateBuilder>* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(StateBuilder_delete)( pref<StateBuilder>* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, StateBuilder_delete, _IDL);
static VertexAttribRate VertexAttribRate__values[] = { VERTEX_ATTRIB_RATE_VERTEX,VERTEX_ATTRIB_RATE_INSTANCE,VERTEX_ATTRIB_RATE_COUNT };
HL_PRIM int HL_NAME(VertexAttribRate_toValue0)( int idx ) {
	return (int)VertexAttribRate__values[idx];
}
DEFINE_PRIM(_I32, VertexAttribRate_toValue0, _I32);
HL_PRIM int HL_NAME(VertexAttribRate_indexToValue1)( int idx ) {
	return (int)VertexAttribRate__values[idx];
}
DEFINE_PRIM(_I32, VertexAttribRate_indexToValue1, _I32);
HL_PRIM int HL_NAME(VertexAttribRate_valueToIndex1)( int value ) {
	for( int i = 0; i < 3; i++ ) if ( value == (int)VertexAttribRate__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, VertexAttribRate_valueToIndex1, _I32);
HL_PRIM int HL_NAME(VertexAttribRate_fromValue1)( int value ) {
	for( int i = 0; i < 3; i++ ) if ( value == (int)VertexAttribRate__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, VertexAttribRate_fromValue1, _I32);
HL_PRIM int HL_NAME(VertexAttribRate_fromIndex1)( int index ) {return index;}
DEFINE_PRIM(_I32, VertexAttribRate_fromIndex1, _I32);
static void finalize_VertexLayout( pref<VertexLayout>* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(VertexLayout_delete)( pref<VertexLayout>* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, VertexLayout_delete, _IDL);
static void finalize_PipelineCache( pref<PipelineCache>* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(PipelineCache_delete)( pref<PipelineCache>* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, PipelineCache_delete, _IDL);
static void finalize_PipelineDesc( pref<HlForgePipelineDesc>* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(PipelineDesc_delete)( pref<HlForgePipelineDesc>* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, PipelineDesc_delete, _IDL);
static void finalize_BufferBinder( pref<BufferBinder>* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(BufferBinder_delete)( pref<BufferBinder>* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, BufferBinder_delete, _IDL);
static void finalize_ResourceBarrierBuilder( pref<ResourceBarrierBuilder>* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(ResourceBarrierBuilder_delete)( pref<ResourceBarrierBuilder>* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, ResourceBarrierBuilder_delete, _IDL);
static void finalize_Map64Int( pref<Map64Int>* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(Map64Int_delete)( pref<Map64Int>* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, Map64Int_delete, _IDL);
static void finalize_RenderTargetDesc( pref<RenderTargetDesc>* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(RenderTargetDesc_delete)( pref<RenderTargetDesc>* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, RenderTargetDesc_delete, _IDL);
static void finalize_SamplerDesc( pref<SamplerDesc>* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(SamplerDesc_delete)( pref<SamplerDesc>* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, SamplerDesc_delete, _IDL);
static void finalize_DescriptorSetDesc( pref<DescriptorSetDesc>* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(DescriptorSetDesc_delete)( pref<DescriptorSetDesc>* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, DescriptorSetDesc_delete, _IDL);
static DescriptorSlotMode DescriptorSlotMode__values[] = { DBM_TEXTURES,DBM_SAMPLERS,DBM_UNIFORMS };
HL_PRIM int HL_NAME(DescriptorSlotMode_toValue0)( int idx ) {
	return (int)DescriptorSlotMode__values[idx];
}
DEFINE_PRIM(_I32, DescriptorSlotMode_toValue0, _I32);
HL_PRIM int HL_NAME(DescriptorSlotMode_indexToValue1)( int idx ) {
	return (int)DescriptorSlotMode__values[idx];
}
DEFINE_PRIM(_I32, DescriptorSlotMode_indexToValue1, _I32);
HL_PRIM int HL_NAME(DescriptorSlotMode_valueToIndex1)( int value ) {
	for( int i = 0; i < 3; i++ ) if ( value == (int)DescriptorSlotMode__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, DescriptorSlotMode_valueToIndex1, _I32);
HL_PRIM int HL_NAME(DescriptorSlotMode_fromValue1)( int value ) {
	for( int i = 0; i < 3; i++ ) if ( value == (int)DescriptorSlotMode__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, DescriptorSlotMode_fromValue1, _I32);
HL_PRIM int HL_NAME(DescriptorSlotMode_fromIndex1)( int index ) {return index;}
DEFINE_PRIM(_I32, DescriptorSlotMode_fromIndex1, _I32);
static void finalize_DescriptorDataBuilder( pref<DescriptorDataBuilder>* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(DescriptorDataBuilder_delete)( pref<DescriptorDataBuilder>* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, DescriptorDataBuilder_delete, _IDL);
static void finalize_RootSignatureDesc( pref<RootSignatureFactory>* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(RootSignatureDesc_delete)( pref<RootSignatureFactory>* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, RootSignatureDesc_delete, _IDL);
static void finalize_ForgeSDLWindow( pref<ForgeSDLWindow>* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(ForgeSDLWindow_delete)( pref<ForgeSDLWindow>* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, ForgeSDLWindow_delete, _IDL);
static void finalize_SyncToken( pref<SyncToken>* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(SyncToken_delete)( pref<SyncToken>* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, SyncToken_delete, _IDL);
static void finalize_Buffer( pref<BufferExt>* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(Buffer_delete)( pref<BufferExt>* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, Buffer_delete, _IDL);
static void finalize_BufferLoadDesc( pref<BufferLoadDescExt>* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(BufferLoadDesc_delete)( pref<BufferLoadDescExt>* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, BufferLoadDesc_delete, _IDL);
static void finalize_TextureDesc( pref<TextureDesc>* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(TextureDesc_delete)( pref<TextureDesc>* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, TextureDesc_delete, _IDL);
static void finalize_TextureLoadDesc( pref<TextureLoadDesc>* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(TextureLoadDesc_delete)( pref<TextureLoadDesc>* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, TextureLoadDesc_delete, _IDL);
static AttributeSemantic AttributeSemantic__values[] = { POSITION,NORMAL,TANGENT,BITANGENT,UV0,UV1,UV2,UV3,UV4,UV5,UV6,UV7,COLOR,USER0,USER1,USER2,USER3,USER4,USER5,USER6,USER7 };
HL_PRIM int HL_NAME(AttributeSemantic_toValue0)( int idx ) {
	return (int)AttributeSemantic__values[idx];
}
DEFINE_PRIM(_I32, AttributeSemantic_toValue0, _I32);
HL_PRIM int HL_NAME(AttributeSemantic_indexToValue1)( int idx ) {
	return (int)AttributeSemantic__values[idx];
}
DEFINE_PRIM(_I32, AttributeSemantic_indexToValue1, _I32);
HL_PRIM int HL_NAME(AttributeSemantic_valueToIndex1)( int value ) {
	for( int i = 0; i < 21; i++ ) if ( value == (int)AttributeSemantic__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, AttributeSemantic_valueToIndex1, _I32);
HL_PRIM int HL_NAME(AttributeSemantic_fromValue1)( int value ) {
	for( int i = 0; i < 21; i++ ) if ( value == (int)AttributeSemantic__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, AttributeSemantic_fromValue1, _I32);
HL_PRIM int HL_NAME(AttributeSemantic_fromIndex1)( int index ) {return index;}
DEFINE_PRIM(_I32, AttributeSemantic_fromIndex1, _I32);
static AttributeType AttributeType__values[] = { ATTR_FLOAT16,ATTR_FLOAT32,ATTR_FLOAT64,ATTR_UINT8,ATTR_UINT16,ATTR_UINT32,ATTR_UINT64 };
HL_PRIM int HL_NAME(AttributeType_toValue0)( int idx ) {
	return (int)AttributeType__values[idx];
}
DEFINE_PRIM(_I32, AttributeType_toValue0, _I32);
HL_PRIM int HL_NAME(AttributeType_indexToValue1)( int idx ) {
	return (int)AttributeType__values[idx];
}
DEFINE_PRIM(_I32, AttributeType_indexToValue1, _I32);
HL_PRIM int HL_NAME(AttributeType_valueToIndex1)( int value ) {
	for( int i = 0; i < 7; i++ ) if ( value == (int)AttributeType__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, AttributeType_valueToIndex1, _I32);
HL_PRIM int HL_NAME(AttributeType_fromValue1)( int value ) {
	for( int i = 0; i < 7; i++ ) if ( value == (int)AttributeType__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, AttributeType_fromValue1, _I32);
HL_PRIM int HL_NAME(AttributeType_fromIndex1)( int index ) {return index;}
DEFINE_PRIM(_I32, AttributeType_fromIndex1, _I32);
static void finalize_PolyMesh( pref<PolyMesh>* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(PolyMesh_delete)( pref<PolyMesh>* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, PolyMesh_delete, _IDL);
HL_PRIM pref<HashBuilder>* HL_NAME(HashBuilder_new0)() {
	return alloc_ref((new HashBuilder()),HashBuilder);
}
DEFINE_PRIM(_IDL, HashBuilder_new0,);

HL_PRIM void HL_NAME(HashBuilder_reset1)(pref<HashBuilder>* _this, int64_t seed) {
	(_unref(_this)->reset(seed));
}
DEFINE_PRIM(_VOID, HashBuilder_reset1, _IDL _I64);

HL_PRIM void HL_NAME(HashBuilder_addInt81)(pref<HashBuilder>* _this, int v) {
	(_unref(_this)->addInt8(v));
}
DEFINE_PRIM(_VOID, HashBuilder_addInt81, _IDL _I32);

HL_PRIM void HL_NAME(HashBuilder_addInt161)(pref<HashBuilder>* _this, int v) {
	(_unref(_this)->addInt16(v));
}
DEFINE_PRIM(_VOID, HashBuilder_addInt161, _IDL _I32);

HL_PRIM void HL_NAME(HashBuilder_addInt321)(pref<HashBuilder>* _this, int v) {
	(_unref(_this)->addInt32(v));
}
DEFINE_PRIM(_VOID, HashBuilder_addInt321, _IDL _I32);

HL_PRIM void HL_NAME(HashBuilder_addInt641)(pref<HashBuilder>* _this, int64_t v) {
	(_unref(_this)->addInt64(v));
}
DEFINE_PRIM(_VOID, HashBuilder_addInt641, _IDL _I64);

HL_PRIM void HL_NAME(HashBuilder_addBytes3)(pref<HashBuilder>* _this, vbyte* b, int offset, int length) {
	(_unref(_this)->addBytes(b, offset, length));
}
DEFINE_PRIM(_VOID, HashBuilder_addBytes3, _IDL _BYTES _I32 _I32);

HL_PRIM int64_t HL_NAME(HashBuilder_getValue0)(pref<HashBuilder>* _this) {
	return (_unref(_this)->getValue());
}
DEFINE_PRIM(_I64, HashBuilder_getValue0, _IDL);

HL_PRIM int HL_NAME(BlendStateDesc_get_SrcFactors)( pref<BlendStateDesc>* _this, int index ) {
	return _unref(_this)->mSrcFactors[index];
}
DEFINE_PRIM(_I32,BlendStateDesc_get_SrcFactors,_IDL _I32);
HL_PRIM int HL_NAME(BlendStateDesc_set_SrcFactors)( pref<BlendStateDesc>* _this, int index, int value ) {
	_unref(_this)->mSrcFactors[index] = (BlendConstant)(BlendConstant__values[value]);
	return value;
}
DEFINE_PRIM(_I32,BlendStateDesc_set_SrcFactors,_IDL _I32 _I32); // Array setter

HL_PRIM int HL_NAME(BlendStateDesc_get_DstFactors)( pref<BlendStateDesc>* _this, int index ) {
	return _unref(_this)->mDstFactors[index];
}
DEFINE_PRIM(_I32,BlendStateDesc_get_DstFactors,_IDL _I32);
HL_PRIM int HL_NAME(BlendStateDesc_set_DstFactors)( pref<BlendStateDesc>* _this, int index, int value ) {
	_unref(_this)->mDstFactors[index] = (BlendConstant)(BlendConstant__values[value]);
	return value;
}
DEFINE_PRIM(_I32,BlendStateDesc_set_DstFactors,_IDL _I32 _I32); // Array setter

HL_PRIM int HL_NAME(BlendStateDesc_get_SrcAlphaFactors)( pref<BlendStateDesc>* _this, int index ) {
	return _unref(_this)->mSrcAlphaFactors[index];
}
DEFINE_PRIM(_I32,BlendStateDesc_get_SrcAlphaFactors,_IDL _I32);
HL_PRIM int HL_NAME(BlendStateDesc_set_SrcAlphaFactors)( pref<BlendStateDesc>* _this, int index, int value ) {
	_unref(_this)->mSrcAlphaFactors[index] = (BlendConstant)(BlendConstant__values[value]);
	return value;
}
DEFINE_PRIM(_I32,BlendStateDesc_set_SrcAlphaFactors,_IDL _I32 _I32); // Array setter

HL_PRIM int HL_NAME(BlendStateDesc_get_DstAlphaFactors)( pref<BlendStateDesc>* _this, int index ) {
	return _unref(_this)->mDstAlphaFactors[index];
}
DEFINE_PRIM(_I32,BlendStateDesc_get_DstAlphaFactors,_IDL _I32);
HL_PRIM int HL_NAME(BlendStateDesc_set_DstAlphaFactors)( pref<BlendStateDesc>* _this, int index, int value ) {
	_unref(_this)->mDstAlphaFactors[index] = (BlendConstant)(BlendConstant__values[value]);
	return value;
}
DEFINE_PRIM(_I32,BlendStateDesc_set_DstAlphaFactors,_IDL _I32 _I32); // Array setter

HL_PRIM int HL_NAME(BlendStateDesc_get_BlendModes)( pref<BlendStateDesc>* _this, int index ) {
	return _unref(_this)->mBlendModes[index];
}
DEFINE_PRIM(_I32,BlendStateDesc_get_BlendModes,_IDL _I32);
HL_PRIM int HL_NAME(BlendStateDesc_set_BlendModes)( pref<BlendStateDesc>* _this, int index, int value ) {
	_unref(_this)->mBlendModes[index] = (BlendMode)(BlendMode__values[value]);
	return value;
}
DEFINE_PRIM(_I32,BlendStateDesc_set_BlendModes,_IDL _I32 _I32); // Array setter

HL_PRIM int HL_NAME(BlendStateDesc_get_BlendAlphaModes)( pref<BlendStateDesc>* _this, int index ) {
	return _unref(_this)->mBlendAlphaModes[index];
}
DEFINE_PRIM(_I32,BlendStateDesc_get_BlendAlphaModes,_IDL _I32);
HL_PRIM int HL_NAME(BlendStateDesc_set_BlendAlphaModes)( pref<BlendStateDesc>* _this, int index, int value ) {
	_unref(_this)->mBlendAlphaModes[index] = (BlendMode)(BlendMode__values[value]);
	return value;
}
DEFINE_PRIM(_I32,BlendStateDesc_set_BlendAlphaModes,_IDL _I32 _I32); // Array setter

HL_PRIM int HL_NAME(BlendStateDesc_get_Masks)( pref<BlendStateDesc>* _this, int index ) {
	return _unref(_this)->mMasks[index];
}
DEFINE_PRIM(_I32,BlendStateDesc_get_Masks,_IDL _I32);
HL_PRIM int HL_NAME(BlendStateDesc_set_Masks)( pref<BlendStateDesc>* _this, int index, int value ) {
	_unref(_this)->mMasks[index] = (value);
	return value;
}
DEFINE_PRIM(_I32,BlendStateDesc_set_Masks,_IDL _I32 _I32); // Array setter

HL_PRIM int HL_NAME(BlendStateDesc_get_renderTargetMask)( pref<BlendStateDesc>* _this ) {
	return _unref(_this)->mRenderTargetMask;
}
DEFINE_PRIM(_I32,BlendStateDesc_get_renderTargetMask,_IDL);
HL_PRIM int HL_NAME(BlendStateDesc_set_renderTargetMask)( pref<BlendStateDesc>* _this, int value ) {
	_unref(_this)->mRenderTargetMask = (BlendStateTargets)(value);
	return value;
}
DEFINE_PRIM(_I32,BlendStateDesc_set_renderTargetMask,_IDL _I32);

HL_PRIM void HL_NAME(BlendStateDesc_setRenderTarget2)(pref<BlendStateDesc>* _this, int target, bool mask) {
	(forge_blend_state_desc_set_rt( _unref(_this) , BlendStateTargets__values[target], mask));
}
DEFINE_PRIM(_VOID, BlendStateDesc_setRenderTarget2, _IDL _I32 _BOOL);

HL_PRIM bool HL_NAME(BlendStateDesc_get_alphaToCoverage)( pref<BlendStateDesc>* _this ) {
	return _unref(_this)->mAlphaToCoverage;
}
DEFINE_PRIM(_BOOL,BlendStateDesc_get_alphaToCoverage,_IDL);
HL_PRIM bool HL_NAME(BlendStateDesc_set_alphaToCoverage)( pref<BlendStateDesc>* _this, bool value ) {
	_unref(_this)->mAlphaToCoverage = (value);
	return value;
}
DEFINE_PRIM(_BOOL,BlendStateDesc_set_alphaToCoverage,_IDL _BOOL);

HL_PRIM bool HL_NAME(BlendStateDesc_get_independentBlend)( pref<BlendStateDesc>* _this ) {
	return _unref(_this)->mIndependentBlend;
}
DEFINE_PRIM(_BOOL,BlendStateDesc_get_independentBlend,_IDL);
HL_PRIM bool HL_NAME(BlendStateDesc_set_independentBlend)( pref<BlendStateDesc>* _this, bool value ) {
	_unref(_this)->mIndependentBlend = (value);
	return value;
}
DEFINE_PRIM(_BOOL,BlendStateDesc_set_independentBlend,_IDL _BOOL);

HL_PRIM bool HL_NAME(DepthStateDesc_get_depthTest)( pref<DepthStateDesc>* _this ) {
	return _unref(_this)->mDepthTest;
}
DEFINE_PRIM(_BOOL,DepthStateDesc_get_depthTest,_IDL);
HL_PRIM bool HL_NAME(DepthStateDesc_set_depthTest)( pref<DepthStateDesc>* _this, bool value ) {
	_unref(_this)->mDepthTest = (value);
	return value;
}
DEFINE_PRIM(_BOOL,DepthStateDesc_set_depthTest,_IDL _BOOL);

HL_PRIM bool HL_NAME(DepthStateDesc_get_depthWrite)( pref<DepthStateDesc>* _this ) {
	return _unref(_this)->mDepthWrite;
}
DEFINE_PRIM(_BOOL,DepthStateDesc_get_depthWrite,_IDL);
HL_PRIM bool HL_NAME(DepthStateDesc_set_depthWrite)( pref<DepthStateDesc>* _this, bool value ) {
	_unref(_this)->mDepthWrite = (value);
	return value;
}
DEFINE_PRIM(_BOOL,DepthStateDesc_set_depthWrite,_IDL _BOOL);

HL_PRIM int HL_NAME(DepthStateDesc_get_depthFunc)( pref<DepthStateDesc>* _this ) {
	return HL_NAME(CompareMode_valueToIndex1)(_unref(_this)->mDepthFunc);
}
DEFINE_PRIM(_I32,DepthStateDesc_get_depthFunc,_IDL);
HL_PRIM int HL_NAME(DepthStateDesc_set_depthFunc)( pref<DepthStateDesc>* _this, int value ) {
	_unref(_this)->mDepthFunc = (CompareMode)HL_NAME(CompareMode_indexToValue1)(value);
	return value;
}
DEFINE_PRIM(_I32,DepthStateDesc_set_depthFunc,_IDL _I32);

HL_PRIM bool HL_NAME(DepthStateDesc_get_stencilTest)( pref<DepthStateDesc>* _this ) {
	return _unref(_this)->mStencilTest;
}
DEFINE_PRIM(_BOOL,DepthStateDesc_get_stencilTest,_IDL);
HL_PRIM bool HL_NAME(DepthStateDesc_set_stencilTest)( pref<DepthStateDesc>* _this, bool value ) {
	_unref(_this)->mStencilTest = (value);
	return value;
}
DEFINE_PRIM(_BOOL,DepthStateDesc_set_stencilTest,_IDL _BOOL);

HL_PRIM int HL_NAME(DepthStateDesc_get_stencilReadMask)( pref<DepthStateDesc>* _this ) {
	return _unref(_this)->mStencilReadMask;
}
DEFINE_PRIM(_I32,DepthStateDesc_get_stencilReadMask,_IDL);
HL_PRIM int HL_NAME(DepthStateDesc_set_stencilReadMask)( pref<DepthStateDesc>* _this, int value ) {
	_unref(_this)->mStencilReadMask = (value);
	return value;
}
DEFINE_PRIM(_I32,DepthStateDesc_set_stencilReadMask,_IDL _I32);

HL_PRIM int HL_NAME(DepthStateDesc_get_stencilWriteMask)( pref<DepthStateDesc>* _this ) {
	return _unref(_this)->mStencilWriteMask;
}
DEFINE_PRIM(_I32,DepthStateDesc_get_stencilWriteMask,_IDL);
HL_PRIM int HL_NAME(DepthStateDesc_set_stencilWriteMask)( pref<DepthStateDesc>* _this, int value ) {
	_unref(_this)->mStencilWriteMask = (value);
	return value;
}
DEFINE_PRIM(_I32,DepthStateDesc_set_stencilWriteMask,_IDL _I32);

HL_PRIM int HL_NAME(DepthStateDesc_get_stencilFrontFunc)( pref<DepthStateDesc>* _this ) {
	return HL_NAME(CompareMode_valueToIndex1)(_unref(_this)->mStencilFrontFunc);
}
DEFINE_PRIM(_I32,DepthStateDesc_get_stencilFrontFunc,_IDL);
HL_PRIM int HL_NAME(DepthStateDesc_set_stencilFrontFunc)( pref<DepthStateDesc>* _this, int value ) {
	_unref(_this)->mStencilFrontFunc = (CompareMode)HL_NAME(CompareMode_indexToValue1)(value);
	return value;
}
DEFINE_PRIM(_I32,DepthStateDesc_set_stencilFrontFunc,_IDL _I32);

HL_PRIM int HL_NAME(DepthStateDesc_get_stencilFrontFail)( pref<DepthStateDesc>* _this ) {
	return HL_NAME(StencilOp_valueToIndex1)(_unref(_this)->mStencilFrontFail);
}
DEFINE_PRIM(_I32,DepthStateDesc_get_stencilFrontFail,_IDL);
HL_PRIM int HL_NAME(DepthStateDesc_set_stencilFrontFail)( pref<DepthStateDesc>* _this, int value ) {
	_unref(_this)->mStencilFrontFail = (StencilOp)HL_NAME(StencilOp_indexToValue1)(value);
	return value;
}
DEFINE_PRIM(_I32,DepthStateDesc_set_stencilFrontFail,_IDL _I32);

HL_PRIM int HL_NAME(DepthStateDesc_get_depthFrontFail)( pref<DepthStateDesc>* _this ) {
	return HL_NAME(StencilOp_valueToIndex1)(_unref(_this)->mDepthFrontFail);
}
DEFINE_PRIM(_I32,DepthStateDesc_get_depthFrontFail,_IDL);
HL_PRIM int HL_NAME(DepthStateDesc_set_depthFrontFail)( pref<DepthStateDesc>* _this, int value ) {
	_unref(_this)->mDepthFrontFail = (StencilOp)HL_NAME(StencilOp_indexToValue1)(value);
	return value;
}
DEFINE_PRIM(_I32,DepthStateDesc_set_depthFrontFail,_IDL _I32);

HL_PRIM int HL_NAME(DepthStateDesc_get_stencilFrontPass)( pref<DepthStateDesc>* _this ) {
	return HL_NAME(StencilOp_valueToIndex1)(_unref(_this)->mStencilFrontPass);
}
DEFINE_PRIM(_I32,DepthStateDesc_get_stencilFrontPass,_IDL);
HL_PRIM int HL_NAME(DepthStateDesc_set_stencilFrontPass)( pref<DepthStateDesc>* _this, int value ) {
	_unref(_this)->mStencilFrontPass = (StencilOp)HL_NAME(StencilOp_indexToValue1)(value);
	return value;
}
DEFINE_PRIM(_I32,DepthStateDesc_set_stencilFrontPass,_IDL _I32);

HL_PRIM int HL_NAME(DepthStateDesc_get_stencilBackFunc)( pref<DepthStateDesc>* _this ) {
	return HL_NAME(CompareMode_valueToIndex1)(_unref(_this)->mStencilBackFunc);
}
DEFINE_PRIM(_I32,DepthStateDesc_get_stencilBackFunc,_IDL);
HL_PRIM int HL_NAME(DepthStateDesc_set_stencilBackFunc)( pref<DepthStateDesc>* _this, int value ) {
	_unref(_this)->mStencilBackFunc = (CompareMode)HL_NAME(CompareMode_indexToValue1)(value);
	return value;
}
DEFINE_PRIM(_I32,DepthStateDesc_set_stencilBackFunc,_IDL _I32);

HL_PRIM int HL_NAME(DepthStateDesc_get_stencilBackFail)( pref<DepthStateDesc>* _this ) {
	return HL_NAME(StencilOp_valueToIndex1)(_unref(_this)->mStencilBackFail);
}
DEFINE_PRIM(_I32,DepthStateDesc_get_stencilBackFail,_IDL);
HL_PRIM int HL_NAME(DepthStateDesc_set_stencilBackFail)( pref<DepthStateDesc>* _this, int value ) {
	_unref(_this)->mStencilBackFail = (StencilOp)HL_NAME(StencilOp_indexToValue1)(value);
	return value;
}
DEFINE_PRIM(_I32,DepthStateDesc_set_stencilBackFail,_IDL _I32);

HL_PRIM int HL_NAME(DepthStateDesc_get_depthBackFail)( pref<DepthStateDesc>* _this ) {
	return HL_NAME(StencilOp_valueToIndex1)(_unref(_this)->mDepthBackFail);
}
DEFINE_PRIM(_I32,DepthStateDesc_get_depthBackFail,_IDL);
HL_PRIM int HL_NAME(DepthStateDesc_set_depthBackFail)( pref<DepthStateDesc>* _this, int value ) {
	_unref(_this)->mDepthBackFail = (StencilOp)HL_NAME(StencilOp_indexToValue1)(value);
	return value;
}
DEFINE_PRIM(_I32,DepthStateDesc_set_depthBackFail,_IDL _I32);

HL_PRIM int HL_NAME(DepthStateDesc_get_stencilBackPass)( pref<DepthStateDesc>* _this ) {
	return HL_NAME(StencilOp_valueToIndex1)(_unref(_this)->mStencilBackPass);
}
DEFINE_PRIM(_I32,DepthStateDesc_get_stencilBackPass,_IDL);
HL_PRIM int HL_NAME(DepthStateDesc_set_stencilBackPass)( pref<DepthStateDesc>* _this, int value ) {
	_unref(_this)->mStencilBackPass = (StencilOp)HL_NAME(StencilOp_indexToValue1)(value);
	return value;
}
DEFINE_PRIM(_I32,DepthStateDesc_set_stencilBackPass,_IDL _I32);

HL_PRIM int HL_NAME(RasterizerStateDesc_get_cullMode)( pref<RasterizerStateDesc>* _this ) {
	return HL_NAME(CullMode_valueToIndex1)(_unref(_this)->mCullMode);
}
DEFINE_PRIM(_I32,RasterizerStateDesc_get_cullMode,_IDL);
HL_PRIM int HL_NAME(RasterizerStateDesc_set_cullMode)( pref<RasterizerStateDesc>* _this, int value ) {
	_unref(_this)->mCullMode = (CullMode)HL_NAME(CullMode_indexToValue1)(value);
	return value;
}
DEFINE_PRIM(_I32,RasterizerStateDesc_set_cullMode,_IDL _I32);

HL_PRIM int HL_NAME(RasterizerStateDesc_get_depthBias)( pref<RasterizerStateDesc>* _this ) {
	return _unref(_this)->mDepthBias;
}
DEFINE_PRIM(_I32,RasterizerStateDesc_get_depthBias,_IDL);
HL_PRIM int HL_NAME(RasterizerStateDesc_set_depthBias)( pref<RasterizerStateDesc>* _this, int value ) {
	_unref(_this)->mDepthBias = (value);
	return value;
}
DEFINE_PRIM(_I32,RasterizerStateDesc_set_depthBias,_IDL _I32);

HL_PRIM float HL_NAME(RasterizerStateDesc_get_slopeScaledDepthBias)( pref<RasterizerStateDesc>* _this ) {
	return _unref(_this)->mSlopeScaledDepthBias;
}
DEFINE_PRIM(_F32,RasterizerStateDesc_get_slopeScaledDepthBias,_IDL);
HL_PRIM float HL_NAME(RasterizerStateDesc_set_slopeScaledDepthBias)( pref<RasterizerStateDesc>* _this, float value ) {
	_unref(_this)->mSlopeScaledDepthBias = (value);
	return value;
}
DEFINE_PRIM(_F32,RasterizerStateDesc_set_slopeScaledDepthBias,_IDL _F32);

HL_PRIM int HL_NAME(RasterizerStateDesc_get_fillMode)( pref<RasterizerStateDesc>* _this ) {
	return HL_NAME(FillMode_valueToIndex1)(_unref(_this)->mFillMode);
}
DEFINE_PRIM(_I32,RasterizerStateDesc_get_fillMode,_IDL);
HL_PRIM int HL_NAME(RasterizerStateDesc_set_fillMode)( pref<RasterizerStateDesc>* _this, int value ) {
	_unref(_this)->mFillMode = (FillMode)HL_NAME(FillMode_indexToValue1)(value);
	return value;
}
DEFINE_PRIM(_I32,RasterizerStateDesc_set_fillMode,_IDL _I32);

HL_PRIM int HL_NAME(RasterizerStateDesc_get_frontFace)( pref<RasterizerStateDesc>* _this ) {
	return HL_NAME(FrontFace_valueToIndex1)(_unref(_this)->mFrontFace);
}
DEFINE_PRIM(_I32,RasterizerStateDesc_get_frontFace,_IDL);
HL_PRIM int HL_NAME(RasterizerStateDesc_set_frontFace)( pref<RasterizerStateDesc>* _this, int value ) {
	_unref(_this)->mFrontFace = (FrontFace)HL_NAME(FrontFace_indexToValue1)(value);
	return value;
}
DEFINE_PRIM(_I32,RasterizerStateDesc_set_frontFace,_IDL _I32);

HL_PRIM bool HL_NAME(RasterizerStateDesc_get_multiSample)( pref<RasterizerStateDesc>* _this ) {
	return _unref(_this)->mMultiSample;
}
DEFINE_PRIM(_BOOL,RasterizerStateDesc_get_multiSample,_IDL);
HL_PRIM bool HL_NAME(RasterizerStateDesc_set_multiSample)( pref<RasterizerStateDesc>* _this, bool value ) {
	_unref(_this)->mMultiSample = (value);
	return value;
}
DEFINE_PRIM(_BOOL,RasterizerStateDesc_set_multiSample,_IDL _BOOL);

HL_PRIM bool HL_NAME(RasterizerStateDesc_get_scissor)( pref<RasterizerStateDesc>* _this ) {
	return _unref(_this)->mScissor;
}
DEFINE_PRIM(_BOOL,RasterizerStateDesc_get_scissor,_IDL);
HL_PRIM bool HL_NAME(RasterizerStateDesc_set_scissor)( pref<RasterizerStateDesc>* _this, bool value ) {
	_unref(_this)->mScissor = (value);
	return value;
}
DEFINE_PRIM(_BOOL,RasterizerStateDesc_set_scissor,_IDL _BOOL);

HL_PRIM bool HL_NAME(RasterizerStateDesc_get_depthClampEnable)( pref<RasterizerStateDesc>* _this ) {
	return _unref(_this)->mDepthClampEnable;
}
DEFINE_PRIM(_BOOL,RasterizerStateDesc_get_depthClampEnable,_IDL);
HL_PRIM bool HL_NAME(RasterizerStateDesc_set_depthClampEnable)( pref<RasterizerStateDesc>* _this, bool value ) {
	_unref(_this)->mDepthClampEnable = (value);
	return value;
}
DEFINE_PRIM(_BOOL,RasterizerStateDesc_set_depthClampEnable,_IDL _BOOL);

HL_PRIM pref<StateBuilder>* HL_NAME(StateBuilder_new0)() {
	return alloc_ref((new StateBuilder()),StateBuilder);
}
DEFINE_PRIM(_IDL, StateBuilder_new0,);

HL_PRIM void HL_NAME(StateBuilder_reset0)(pref<StateBuilder>* _this) {
	(_unref(_this)->reset());
}
DEFINE_PRIM(_VOID, StateBuilder_reset0, _IDL);

HL_PRIM HL_CONST pref<BlendStateDesc>* HL_NAME(StateBuilder_blend0)(pref<StateBuilder>* _this) {
	return alloc_ref_const((_unref(_this)->blend()),BlendStateDesc);
}
DEFINE_PRIM(_IDL, StateBuilder_blend0, _IDL);

HL_PRIM HL_CONST pref<DepthStateDesc>* HL_NAME(StateBuilder_depth0)(pref<StateBuilder>* _this) {
	return alloc_ref_const((_unref(_this)->depth()),DepthStateDesc);
}
DEFINE_PRIM(_IDL, StateBuilder_depth0, _IDL);

HL_PRIM HL_CONST pref<RasterizerStateDesc>* HL_NAME(StateBuilder_raster0)(pref<StateBuilder>* _this) {
	return alloc_ref_const((_unref(_this)->raster()),RasterizerStateDesc);
}
DEFINE_PRIM(_IDL, StateBuilder_raster0, _IDL);

HL_PRIM void HL_NAME(StateBuilder_addToHash1)(pref<StateBuilder>* _this, pref<HashBuilder>* hash) {
	(_unref(_this)->addToHash(_unref_ptr_safe(hash)));
}
DEFINE_PRIM(_VOID, StateBuilder_addToHash1, _IDL _IDL);

HL_PRIM int HL_NAME(VertexAttrib_get_mSemantic)( pref<VertexAttrib>* _this ) {
	return HL_NAME(ShaderSemantic_valueToIndex1)(_unref(_this)->mSemantic);
}
DEFINE_PRIM(_I32,VertexAttrib_get_mSemantic,_IDL);
HL_PRIM int HL_NAME(VertexAttrib_set_mSemantic)( pref<VertexAttrib>* _this, int value ) {
	_unref(_this)->mSemantic = (ShaderSemantic)HL_NAME(ShaderSemantic_indexToValue1)(value);
	return value;
}
DEFINE_PRIM(_I32,VertexAttrib_set_mSemantic,_IDL _I32);

HL_PRIM unsigned int HL_NAME(VertexAttrib_get_mSemanticNameLength)( pref<VertexAttrib>* _this ) {
	return _unref(_this)->mSemanticNameLength;
}
DEFINE_PRIM(_I32,VertexAttrib_get_mSemanticNameLength,_IDL);
HL_PRIM unsigned int HL_NAME(VertexAttrib_set_mSemanticNameLength)( pref<VertexAttrib>* _this, unsigned int value ) {
	_unref(_this)->mSemanticNameLength = (value);
	return value;
}
DEFINE_PRIM(_I32,VertexAttrib_set_mSemanticNameLength,_IDL _I32);

HL_PRIM void HL_NAME(VertexAttrib_setSemanticName1)(pref<VertexAttrib>* _this, vstring * name) {
	const char* name__cstr = (name == nullptr) ? "" : hl_to_utf8( name->bytes ); // Should be garbage collected
	(forge_vertex_attrib_set_semantic( _unref(_this) , name__cstr));
}
DEFINE_PRIM(_VOID, VertexAttrib_setSemanticName1, _IDL _STRING);

HL_PRIM int HL_NAME(VertexAttrib_get_mFormat)( pref<VertexAttrib>* _this ) {
	return HL_NAME(TinyImageFormat_valueToIndex1)(_unref(_this)->mFormat);
}
DEFINE_PRIM(_I32,VertexAttrib_get_mFormat,_IDL);
HL_PRIM int HL_NAME(VertexAttrib_set_mFormat)( pref<VertexAttrib>* _this, int value ) {
	_unref(_this)->mFormat = (TinyImageFormat)HL_NAME(TinyImageFormat_indexToValue1)(value);
	return value;
}
DEFINE_PRIM(_I32,VertexAttrib_set_mFormat,_IDL _I32);

HL_PRIM unsigned int HL_NAME(VertexAttrib_get_mBinding)( pref<VertexAttrib>* _this ) {
	return _unref(_this)->mBinding;
}
DEFINE_PRIM(_I32,VertexAttrib_get_mBinding,_IDL);
HL_PRIM unsigned int HL_NAME(VertexAttrib_set_mBinding)( pref<VertexAttrib>* _this, unsigned int value ) {
	_unref(_this)->mBinding = (value);
	return value;
}
DEFINE_PRIM(_I32,VertexAttrib_set_mBinding,_IDL _I32);

HL_PRIM unsigned int HL_NAME(VertexAttrib_get_mLocation)( pref<VertexAttrib>* _this ) {
	return _unref(_this)->mLocation;
}
DEFINE_PRIM(_I32,VertexAttrib_get_mLocation,_IDL);
HL_PRIM unsigned int HL_NAME(VertexAttrib_set_mLocation)( pref<VertexAttrib>* _this, unsigned int value ) {
	_unref(_this)->mLocation = (value);
	return value;
}
DEFINE_PRIM(_I32,VertexAttrib_set_mLocation,_IDL _I32);

HL_PRIM unsigned int HL_NAME(VertexAttrib_get_mOffset)( pref<VertexAttrib>* _this ) {
	return _unref(_this)->mOffset;
}
DEFINE_PRIM(_I32,VertexAttrib_get_mOffset,_IDL);
HL_PRIM unsigned int HL_NAME(VertexAttrib_set_mOffset)( pref<VertexAttrib>* _this, unsigned int value ) {
	_unref(_this)->mOffset = (value);
	return value;
}
DEFINE_PRIM(_I32,VertexAttrib_set_mOffset,_IDL _I32);

HL_PRIM int HL_NAME(VertexAttrib_get_mRate)( pref<VertexAttrib>* _this ) {
	return HL_NAME(VertexAttribRate_valueToIndex1)(_unref(_this)->mRate);
}
DEFINE_PRIM(_I32,VertexAttrib_get_mRate,_IDL);
HL_PRIM int HL_NAME(VertexAttrib_set_mRate)( pref<VertexAttrib>* _this, int value ) {
	_unref(_this)->mRate = (VertexAttribRate)HL_NAME(VertexAttribRate_indexToValue1)(value);
	return value;
}
DEFINE_PRIM(_I32,VertexAttrib_set_mRate,_IDL _I32);

HL_PRIM pref<VertexLayout>* HL_NAME(VertexLayout_new0)() {
	return alloc_ref((new VertexLayout()),VertexLayout);
}
DEFINE_PRIM(_IDL, VertexLayout_new0,);

HL_PRIM unsigned int HL_NAME(VertexLayout_get_attribCount)( pref<VertexLayout>* _this ) {
	return _unref(_this)->mAttribCount;
}
DEFINE_PRIM(_I32,VertexLayout_get_attribCount,_IDL);
HL_PRIM unsigned int HL_NAME(VertexLayout_set_attribCount)( pref<VertexLayout>* _this, unsigned int value ) {
	_unref(_this)->mAttribCount = (value);
	return value;
}
DEFINE_PRIM(_I32,VertexLayout_set_attribCount,_IDL _I32);

HL_PRIM HL_CONST pref<VertexAttrib>* HL_NAME(VertexLayout_attrib1)(pref<VertexLayout>* _this, int idx) {
	return alloc_ref_const((forge_vertex_layout_get_attrib( _unref(_this) , idx)),VertexAttrib);
}
DEFINE_PRIM(_IDL, VertexLayout_attrib1, _IDL _I32);

HL_PRIM void HL_NAME(VertexLayout_setStride2)(pref<VertexLayout>* _this, int idx, int stride) {
	(forge_vertex_layout_set_stride( _unref(_this) , idx, stride));
}
DEFINE_PRIM(_VOID, VertexLayout_setStride2, _IDL _I32 _I32);

HL_PRIM int HL_NAME(VertexLayout_getStride1)(pref<VertexLayout>* _this, int idx) {
	return (forge_vertex_layout_get_stride( _unref(_this) , idx));
}
DEFINE_PRIM(_I32, VertexLayout_getStride1, _IDL _I32);

HL_PRIM bool HL_NAME(Globals_initialize1)(vstring * name) {
	const char* name__cstr = (name == nullptr) ? "" : hl_to_utf8( name->bytes ); // Should be garbage collected
	auto ___retvalue = (hlForgeInitialize(name__cstr));
	return (___retvalue);
}
DEFINE_PRIM(_BOOL, Globals_initialize1, _STRING);

HL_PRIM void HL_NAME(Globals_waitForAllResourceLoads0)() {
	(waitForAllResourceLoads());
}
DEFINE_PRIM(_VOID, Globals_waitForAllResourceLoads0,);

HL_PRIM int HL_NAME(RenderTarget_get_sampleCount)( pref<RenderTarget>* _this ) {
	return HL_NAME(SampleCount_valueToIndex1)(_unref(_this)->mSampleCount);
}
DEFINE_PRIM(_I32,RenderTarget_get_sampleCount,_IDL);
HL_PRIM int HL_NAME(RenderTarget_set_sampleCount)( pref<RenderTarget>* _this, int value ) {
	_unref(_this)->mSampleCount = (SampleCount)HL_NAME(SampleCount_indexToValue1)(value);
	return value;
}
DEFINE_PRIM(_I32,RenderTarget_set_sampleCount,_IDL _I32);

HL_PRIM unsigned int HL_NAME(RenderTarget_get_sampleQuality)( pref<RenderTarget>* _this ) {
	return _unref(_this)->mSampleQuality;
}
DEFINE_PRIM(_I32,RenderTarget_get_sampleQuality,_IDL);
HL_PRIM unsigned int HL_NAME(RenderTarget_set_sampleQuality)( pref<RenderTarget>* _this, unsigned int value ) {
	_unref(_this)->mSampleQuality = (value);
	return value;
}
DEFINE_PRIM(_I32,RenderTarget_set_sampleQuality,_IDL _I32);

HL_PRIM unsigned int HL_NAME(RenderTarget_get_width)( pref<RenderTarget>* _this ) {
	return _unref(_this)->mWidth;
}
DEFINE_PRIM(_I32,RenderTarget_get_width,_IDL);
HL_PRIM unsigned int HL_NAME(RenderTarget_set_width)( pref<RenderTarget>* _this, unsigned int value ) {
	_unref(_this)->mWidth = (value);
	return value;
}
DEFINE_PRIM(_I32,RenderTarget_set_width,_IDL _I32);

HL_PRIM unsigned int HL_NAME(RenderTarget_get_height)( pref<RenderTarget>* _this ) {
	return _unref(_this)->mHeight;
}
DEFINE_PRIM(_I32,RenderTarget_get_height,_IDL);
HL_PRIM unsigned int HL_NAME(RenderTarget_set_height)( pref<RenderTarget>* _this, unsigned int value ) {
	_unref(_this)->mHeight = (value);
	return value;
}
DEFINE_PRIM(_I32,RenderTarget_set_height,_IDL _I32);

HL_PRIM int HL_NAME(RenderTarget_get_format)( pref<RenderTarget>* _this ) {
	return HL_NAME(TinyImageFormat_valueToIndex1)(_unref(_this)->mFormat);
}
DEFINE_PRIM(_I32,RenderTarget_get_format,_IDL);
HL_PRIM int HL_NAME(RenderTarget_set_format)( pref<RenderTarget>* _this, int value ) {
	_unref(_this)->mFormat = (TinyImageFormat)HL_NAME(TinyImageFormat_indexToValue1)(value);
	return value;
}
DEFINE_PRIM(_I32,RenderTarget_set_format,_IDL _I32);

HL_PRIM HL_CONST pref<Texture>* HL_NAME(RenderTarget_getTexture0)(pref<RenderTarget>* _this) {
	return alloc_ref_const((forge_render_target_get_texture( _unref(_this) )),Texture);
}
DEFINE_PRIM(_IDL, RenderTarget_getTexture0, _IDL);

HL_PRIM void HL_NAME(RenderTarget_setClearColor4)(pref<RenderTarget>* _this, float r, float g, float b, float a) {
	(forge_render_target_set_clear_colour( _unref(_this) , r, g, b, a));
}
DEFINE_PRIM(_VOID, RenderTarget_setClearColor4, _IDL _F32 _F32 _F32 _F32);

HL_PRIM void HL_NAME(RenderTarget_setClearDepthNormalized2)(pref<RenderTarget>* _this, float depth, int stencil) {
	(forge_render_target_set_clear_depth( _unref(_this) , depth, stencil));
}
DEFINE_PRIM(_VOID, RenderTarget_setClearDepthNormalized2, _IDL _F32 _I32);

HL_PRIM void HL_NAME(RenderTarget_captureAsBuffer2)(pref<RenderTarget>* _this, pref<Buffer>* pTransferBuffer, pref<Semaphore>* semaphore) {
	(forge_render_target_capture( _unref(_this) , _unref_ptr_safe(pTransferBuffer), _unref_ptr_safe(semaphore)));
}
DEFINE_PRIM(_VOID, RenderTarget_captureAsBuffer2, _IDL _IDL _IDL);

HL_PRIM int HL_NAME(RenderTarget_captureSize0)(pref<RenderTarget>* _this) {
	return (forge_render_target_capture_size( _unref(_this) ));
}
DEFINE_PRIM(_I32, RenderTarget_captureSize0, _IDL);

HL_PRIM HL_CONST pref<Shader>* HL_NAME(GraphicsPipelineDesc_get_pShaderProgram)( pref<GraphicsPipelineDesc>* _this ) {
	return alloc_ref_const(_unref(_this)->pShaderProgram,Shader);
}
DEFINE_PRIM(_IDL,GraphicsPipelineDesc_get_pShaderProgram,_IDL);
HL_PRIM HL_CONST pref<Shader>* HL_NAME(GraphicsPipelineDesc_set_pShaderProgram)( pref<GraphicsPipelineDesc>* _this, HL_CONST pref<Shader>* value ) {
	_unref(_this)->pShaderProgram = _unref_ptr_safe(value);
	return value;
}
DEFINE_PRIM(_IDL,GraphicsPipelineDesc_set_pShaderProgram,_IDL _IDL);

HL_PRIM HL_CONST pref<RootSignature>* HL_NAME(GraphicsPipelineDesc_get_pRootSignature)( pref<GraphicsPipelineDesc>* _this ) {
	return alloc_ref_const(_unref(_this)->pRootSignature,RootSignature);
}
DEFINE_PRIM(_IDL,GraphicsPipelineDesc_get_pRootSignature,_IDL);
HL_PRIM HL_CONST pref<RootSignature>* HL_NAME(GraphicsPipelineDesc_set_pRootSignature)( pref<GraphicsPipelineDesc>* _this, HL_CONST pref<RootSignature>* value ) {
	_unref(_this)->pRootSignature = _unref_ptr_safe(value);
	return value;
}
DEFINE_PRIM(_IDL,GraphicsPipelineDesc_set_pRootSignature,_IDL _IDL);

HL_PRIM HL_CONST pref<VertexLayout>* HL_NAME(GraphicsPipelineDesc_get_pVertexLayout)( pref<GraphicsPipelineDesc>* _this ) {
	return alloc_ref_const(_unref(_this)->pVertexLayout,VertexLayout);
}
DEFINE_PRIM(_IDL,GraphicsPipelineDesc_get_pVertexLayout,_IDL);
HL_PRIM HL_CONST pref<VertexLayout>* HL_NAME(GraphicsPipelineDesc_set_pVertexLayout)( pref<GraphicsPipelineDesc>* _this, HL_CONST pref<VertexLayout>* value ) {
	_unref(_this)->pVertexLayout = _unref_ptr_safe(value);
	return value;
}
DEFINE_PRIM(_IDL,GraphicsPipelineDesc_set_pVertexLayout,_IDL _IDL);

HL_PRIM HL_CONST pref<BlendStateDesc>* HL_NAME(GraphicsPipelineDesc_get_pBlendState)( pref<GraphicsPipelineDesc>* _this ) {
	return alloc_ref_const(_unref(_this)->pBlendState,BlendStateDesc);
}
DEFINE_PRIM(_IDL,GraphicsPipelineDesc_get_pBlendState,_IDL);
HL_PRIM HL_CONST pref<BlendStateDesc>* HL_NAME(GraphicsPipelineDesc_set_pBlendState)( pref<GraphicsPipelineDesc>* _this, HL_CONST pref<BlendStateDesc>* value ) {
	_unref(_this)->pBlendState = _unref_ptr_safe(value);
	return value;
}
DEFINE_PRIM(_IDL,GraphicsPipelineDesc_set_pBlendState,_IDL _IDL);

HL_PRIM HL_CONST pref<DepthStateDesc>* HL_NAME(GraphicsPipelineDesc_get_pDepthState)( pref<GraphicsPipelineDesc>* _this ) {
	return alloc_ref_const(_unref(_this)->pDepthState,DepthStateDesc);
}
DEFINE_PRIM(_IDL,GraphicsPipelineDesc_get_pDepthState,_IDL);
HL_PRIM HL_CONST pref<DepthStateDesc>* HL_NAME(GraphicsPipelineDesc_set_pDepthState)( pref<GraphicsPipelineDesc>* _this, HL_CONST pref<DepthStateDesc>* value ) {
	_unref(_this)->pDepthState = _unref_ptr_safe(value);
	return value;
}
DEFINE_PRIM(_IDL,GraphicsPipelineDesc_set_pDepthState,_IDL _IDL);

HL_PRIM HL_CONST pref<RasterizerStateDesc>* HL_NAME(GraphicsPipelineDesc_get_pRasterizerState)( pref<GraphicsPipelineDesc>* _this ) {
	return alloc_ref_const(_unref(_this)->pRasterizerState,RasterizerStateDesc);
}
DEFINE_PRIM(_IDL,GraphicsPipelineDesc_get_pRasterizerState,_IDL);
HL_PRIM HL_CONST pref<RasterizerStateDesc>* HL_NAME(GraphicsPipelineDesc_set_pRasterizerState)( pref<GraphicsPipelineDesc>* _this, HL_CONST pref<RasterizerStateDesc>* value ) {
	_unref(_this)->pRasterizerState = _unref_ptr_safe(value);
	return value;
}
DEFINE_PRIM(_IDL,GraphicsPipelineDesc_set_pRasterizerState,_IDL _IDL);

HL_PRIM unsigned int HL_NAME(GraphicsPipelineDesc_get_mRenderTargetCount)( pref<GraphicsPipelineDesc>* _this ) {
	return _unref(_this)->mRenderTargetCount;
}
DEFINE_PRIM(_I32,GraphicsPipelineDesc_get_mRenderTargetCount,_IDL);
HL_PRIM unsigned int HL_NAME(GraphicsPipelineDesc_set_mRenderTargetCount)( pref<GraphicsPipelineDesc>* _this, unsigned int value ) {
	_unref(_this)->mRenderTargetCount = (value);
	return value;
}
DEFINE_PRIM(_I32,GraphicsPipelineDesc_set_mRenderTargetCount,_IDL _I32);

HL_PRIM int HL_NAME(GraphicsPipelineDesc_get_sampleCount)( pref<GraphicsPipelineDesc>* _this ) {
	return HL_NAME(SampleCount_valueToIndex1)(_unref(_this)->mSampleCount);
}
DEFINE_PRIM(_I32,GraphicsPipelineDesc_get_sampleCount,_IDL);
HL_PRIM int HL_NAME(GraphicsPipelineDesc_set_sampleCount)( pref<GraphicsPipelineDesc>* _this, int value ) {
	_unref(_this)->mSampleCount = (SampleCount)HL_NAME(SampleCount_indexToValue1)(value);
	return value;
}
DEFINE_PRIM(_I32,GraphicsPipelineDesc_set_sampleCount,_IDL _I32);

HL_PRIM unsigned int HL_NAME(GraphicsPipelineDesc_get_sampleQuality)( pref<GraphicsPipelineDesc>* _this ) {
	return _unref(_this)->mSampleQuality;
}
DEFINE_PRIM(_I32,GraphicsPipelineDesc_get_sampleQuality,_IDL);
HL_PRIM unsigned int HL_NAME(GraphicsPipelineDesc_set_sampleQuality)( pref<GraphicsPipelineDesc>* _this, unsigned int value ) {
	_unref(_this)->mSampleQuality = (value);
	return value;
}
DEFINE_PRIM(_I32,GraphicsPipelineDesc_set_sampleQuality,_IDL _I32);

HL_PRIM int HL_NAME(GraphicsPipelineDesc_get_depthStencilFormat)( pref<GraphicsPipelineDesc>* _this ) {
	return HL_NAME(TinyImageFormat_valueToIndex1)(_unref(_this)->mDepthStencilFormat);
}
DEFINE_PRIM(_I32,GraphicsPipelineDesc_get_depthStencilFormat,_IDL);
HL_PRIM int HL_NAME(GraphicsPipelineDesc_set_depthStencilFormat)( pref<GraphicsPipelineDesc>* _this, int value ) {
	_unref(_this)->mDepthStencilFormat = (TinyImageFormat)HL_NAME(TinyImageFormat_indexToValue1)(value);
	return value;
}
DEFINE_PRIM(_I32,GraphicsPipelineDesc_set_depthStencilFormat,_IDL _I32);

HL_PRIM int HL_NAME(GraphicsPipelineDesc_get_mPrimitiveTopo)( pref<GraphicsPipelineDesc>* _this ) {
	return HL_NAME(PrimitiveTopology_valueToIndex1)(_unref(_this)->mPrimitiveTopo);
}
DEFINE_PRIM(_I32,GraphicsPipelineDesc_get_mPrimitiveTopo,_IDL);
HL_PRIM int HL_NAME(GraphicsPipelineDesc_set_mPrimitiveTopo)( pref<GraphicsPipelineDesc>* _this, int value ) {
	_unref(_this)->mPrimitiveTopo = (PrimitiveTopology)HL_NAME(PrimitiveTopology_indexToValue1)(value);
	return value;
}
DEFINE_PRIM(_I32,GraphicsPipelineDesc_set_mPrimitiveTopo,_IDL _I32);

HL_PRIM bool HL_NAME(GraphicsPipelineDesc_get_mSupportIndirectCommandBuffer)( pref<GraphicsPipelineDesc>* _this ) {
	return _unref(_this)->mSupportIndirectCommandBuffer;
}
DEFINE_PRIM(_BOOL,GraphicsPipelineDesc_get_mSupportIndirectCommandBuffer,_IDL);
HL_PRIM bool HL_NAME(GraphicsPipelineDesc_set_mSupportIndirectCommandBuffer)( pref<GraphicsPipelineDesc>* _this, bool value ) {
	_unref(_this)->mSupportIndirectCommandBuffer = (value);
	return value;
}
DEFINE_PRIM(_BOOL,GraphicsPipelineDesc_set_mSupportIndirectCommandBuffer,_IDL _BOOL);

HL_PRIM bool HL_NAME(GraphicsPipelineDesc_get_mVRFoveatedRendering)( pref<GraphicsPipelineDesc>* _this ) {
	return _unref(_this)->mVRFoveatedRendering;
}
DEFINE_PRIM(_BOOL,GraphicsPipelineDesc_get_mVRFoveatedRendering,_IDL);
HL_PRIM bool HL_NAME(GraphicsPipelineDesc_set_mVRFoveatedRendering)( pref<GraphicsPipelineDesc>* _this, bool value ) {
	_unref(_this)->mVRFoveatedRendering = (value);
	return value;
}
DEFINE_PRIM(_BOOL,GraphicsPipelineDesc_set_mVRFoveatedRendering,_IDL _BOOL);

HL_PRIM HL_CONST pref<Shader>* HL_NAME(ComputePipelineDesc_get_shaderProgram)( pref<ComputePipelineDesc>* _this ) {
	return alloc_ref_const(_unref(_this)->pShaderProgram,Shader);
}
DEFINE_PRIM(_IDL,ComputePipelineDesc_get_shaderProgram,_IDL);
HL_PRIM HL_CONST pref<Shader>* HL_NAME(ComputePipelineDesc_set_shaderProgram)( pref<ComputePipelineDesc>* _this, HL_CONST pref<Shader>* value ) {
	_unref(_this)->pShaderProgram = _unref_ptr_safe(value);
	return value;
}
DEFINE_PRIM(_IDL,ComputePipelineDesc_set_shaderProgram,_IDL _IDL);

HL_PRIM HL_CONST pref<RootSignature>* HL_NAME(ComputePipelineDesc_get_rootSignature)( pref<ComputePipelineDesc>* _this ) {
	return alloc_ref_const(_unref(_this)->pRootSignature,RootSignature);
}
DEFINE_PRIM(_IDL,ComputePipelineDesc_get_rootSignature,_IDL);
HL_PRIM HL_CONST pref<RootSignature>* HL_NAME(ComputePipelineDesc_set_rootSignature)( pref<ComputePipelineDesc>* _this, HL_CONST pref<RootSignature>* value ) {
	_unref(_this)->pRootSignature = _unref_ptr_safe(value);
	return value;
}
DEFINE_PRIM(_IDL,ComputePipelineDesc_set_rootSignature,_IDL _IDL);

HL_PRIM pref<HlForgePipelineDesc>* HL_NAME(PipelineDesc_new0)() {
	return alloc_ref((new HlForgePipelineDesc()),PipelineDesc);
}
DEFINE_PRIM(_IDL, PipelineDesc_new0,);

HL_PRIM HL_CONST pref<GraphicsPipelineDesc>* HL_NAME(PipelineDesc_graphicsPipeline0)(pref<HlForgePipelineDesc>* _this) {
	return alloc_ref_const((_unref(_this)->graphicsPipeline()),GraphicsPipelineDesc);
}
DEFINE_PRIM(_IDL, PipelineDesc_graphicsPipeline0, _IDL);

HL_PRIM HL_CONST pref<ComputePipelineDesc>* HL_NAME(PipelineDesc_computePipeline0)(pref<HlForgePipelineDesc>* _this) {
	return alloc_ref_const((_unref(_this)->computePipeline()),ComputePipelineDesc);
}
DEFINE_PRIM(_IDL, PipelineDesc_computePipeline0, _IDL);

HL_PRIM pref<PipelineCache>* HL_NAME(PipelineDesc_get_pCache)( pref<HlForgePipelineDesc>* _this ) {
	return alloc_ref(_unref(_this)->pCache,PipelineCache);
}
DEFINE_PRIM(_IDL,PipelineDesc_get_pCache,_IDL);
HL_PRIM pref<PipelineCache>* HL_NAME(PipelineDesc_set_pCache)( pref<HlForgePipelineDesc>* _this, pref<PipelineCache>* value ) {
	_unref(_this)->pCache = _unref_ptr_safe(value);
	return value;
}
DEFINE_PRIM(_IDL,PipelineDesc_set_pCache,_IDL _IDL);

HL_PRIM unsigned int HL_NAME(PipelineDesc_get_mExtensionCount)( pref<HlForgePipelineDesc>* _this ) {
	return _unref(_this)->mExtensionCount;
}
DEFINE_PRIM(_I32,PipelineDesc_get_mExtensionCount,_IDL);
HL_PRIM unsigned int HL_NAME(PipelineDesc_set_mExtensionCount)( pref<HlForgePipelineDesc>* _this, unsigned int value ) {
	_unref(_this)->mExtensionCount = (value);
	return value;
}
DEFINE_PRIM(_I32,PipelineDesc_set_mExtensionCount,_IDL _I32);

HL_PRIM void HL_NAME(PipelineDesc_setName1)(pref<HlForgePipelineDesc>* _this, vstring * name) {
	const char* name__cstr = (name == nullptr) ? "" : hl_to_utf8( name->bytes ); // Should be garbage collected
	(_unref(_this)->setName(name__cstr));
}
DEFINE_PRIM(_VOID, PipelineDesc_setName1, _IDL _STRING);

HL_PRIM void HL_NAME(PipelineDesc_reset0)(pref<HlForgePipelineDesc>* _this) {
	(_unref(_this)->reset());
}
DEFINE_PRIM(_VOID, PipelineDesc_reset0, _IDL);

HL_PRIM void HL_NAME(PipelineDesc_setRenderTargetGlobals2)(pref<HlForgePipelineDesc>* _this, int sampleCount, int sampleQuality) {
	(_unref(_this)->setRenderTargetGlobals(SampleCount__values[sampleCount], sampleQuality));
}
DEFINE_PRIM(_VOID, PipelineDesc_setRenderTargetGlobals2, _IDL _I32 _I32);

HL_PRIM int HL_NAME(PipelineDesc_addGraphicsRenderTarget1)(pref<HlForgePipelineDesc>* _this, int format) {
	return (_unref(_this)->addGraphicsRenderTarget(TinyImageFormat__values[format]));
}
DEFINE_PRIM(_I32, PipelineDesc_addGraphicsRenderTarget1, _IDL _I32);

HL_PRIM pref<BufferBinder>* HL_NAME(BufferBinder_new0)() {
	return alloc_ref((new BufferBinder()),BufferBinder);
}
DEFINE_PRIM(_IDL, BufferBinder_new0,);

HL_PRIM void HL_NAME(BufferBinder_reset0)(pref<BufferBinder>* _this) {
	(_unref(_this)->reset());
}
DEFINE_PRIM(_VOID, BufferBinder_reset0, _IDL);

HL_PRIM int HL_NAME(BufferBinder_add3)(pref<BufferBinder>* _this, pref<BufferExt>* buf, int stride, int offset) {
	return (_unref(_this)->add(_unref_ptr_safe(buf), stride, offset));
}
DEFINE_PRIM(_I32, BufferBinder_add3, _IDL _IDL _I32 _I32);

HL_PRIM pref<ResourceBarrierBuilder>* HL_NAME(ResourceBarrierBuilder_new0)() {
	return alloc_ref((new ResourceBarrierBuilder()),ResourceBarrierBuilder);
}
DEFINE_PRIM(_IDL, ResourceBarrierBuilder_new0,);

HL_PRIM int HL_NAME(ResourceBarrierBuilder_addRTBarrier3)(pref<ResourceBarrierBuilder>* _this, pref<RenderTarget>* rt, int src, int dst) {
	return (_unref(_this)->addRTBarrier(_unref_ptr_safe(rt), ResourceState__values[src], ResourceState__values[dst]));
}
DEFINE_PRIM(_I32, ResourceBarrierBuilder_addRTBarrier3, _IDL _IDL _I32 _I32);

HL_PRIM void HL_NAME(ResourceBarrierBuilder_insert1)(pref<ResourceBarrierBuilder>* _this, pref<Cmd>* cmd) {
	(_unref(_this)->insert(_unref_ptr_safe(cmd)));
}
DEFINE_PRIM(_VOID, ResourceBarrierBuilder_insert1, _IDL _IDL);

HL_PRIM void HL_NAME(Cmd_bind4)(pref<Cmd>* _this, pref<RenderTarget>* rt, pref<RenderTarget>* depthstencil, int color, int depth) {
	(forge_render_target_bind( _unref(_this) , _unref_ptr_safe(rt), _unref_ptr_safe(depthstencil), LoadActionType__values[color], LoadActionType__values[depth]));
}
DEFINE_PRIM(_VOID, Cmd_bind4, _IDL _IDL _IDL _I32 _I32);

HL_PRIM void HL_NAME(Cmd_unbindRenderTarget0)(pref<Cmd>* _this) {
	(forge_cmd_unbind( _unref(_this) ));
}
DEFINE_PRIM(_VOID, Cmd_unbindRenderTarget0, _IDL);

HL_PRIM void HL_NAME(Cmd_begin0)(pref<Cmd>* _this) {
	(beginCmd( _unref(_this) ));
}
DEFINE_PRIM(_VOID, Cmd_begin0, _IDL);

HL_PRIM void HL_NAME(Cmd_end0)(pref<Cmd>* _this) {
	(endCmd( _unref(_this) ));
}
DEFINE_PRIM(_VOID, Cmd_end0, _IDL);

HL_PRIM void HL_NAME(Cmd_pushConstants3)(pref<Cmd>* _this, pref<RootSignature>* rs, int index, vbyte* data) {
	(cmdBindPushConstants( _unref(_this) , _unref_ptr_safe(rs), index, data));
}
DEFINE_PRIM(_VOID, Cmd_pushConstants3, _IDL _IDL _I32 _BYTES);

HL_PRIM void HL_NAME(Cmd_bindIndexBuffer3)(pref<Cmd>* _this, pref<BufferExt>* b, int format, int offset) {
	(BufferExt::bindAsIndex( _unref(_this) , _unref_ptr_safe(b), IndexType__values[format], offset));
}
DEFINE_PRIM(_VOID, Cmd_bindIndexBuffer3, _IDL _IDL _I32 _I32);

HL_PRIM void HL_NAME(Cmd_bindVertexBuffer1)(pref<Cmd>* _this, pref<BufferBinder>* binder) {
	(BufferBinder::bindAsVertex( _unref(_this) , _unref_ptr_safe(binder)));
}
DEFINE_PRIM(_VOID, Cmd_bindVertexBuffer1, _IDL _IDL);

HL_PRIM void HL_NAME(Cmd_drawIndexed3)(pref<Cmd>* _this, int vertCount, unsigned int first_index, unsigned int first_vertex) {
	(cmdDrawIndexed( _unref(_this) , vertCount, first_index, first_vertex));
}
DEFINE_PRIM(_VOID, Cmd_drawIndexed3, _IDL _I32 _I32 _I32);

HL_PRIM void HL_NAME(Cmd_bindPipeline1)(pref<Cmd>* _this, pref<Pipeline>* pipeline) {
	(cmdBindPipeline( _unref(_this) , _unref_ptr_safe(pipeline)));
}
DEFINE_PRIM(_VOID, Cmd_bindPipeline1, _IDL _IDL);

HL_PRIM void HL_NAME(Cmd_bindDescriptorSet2)(pref<Cmd>* _this, int index, pref<DescriptorSet>* set) {
	(cmdBindDescriptorSet( _unref(_this) , index, _unref_ptr_safe(set)));
}
DEFINE_PRIM(_VOID, Cmd_bindDescriptorSet2, _IDL _I32 _IDL);

HL_PRIM void HL_NAME(Cmd_insertBarrier1)(pref<Cmd>* _this, pref<ResourceBarrierBuilder>* barrier) {
	(forge_cmd_insert_barrier( _unref(_this) , _unref_ptr_safe(barrier)));
}
DEFINE_PRIM(_VOID, Cmd_insertBarrier1, _IDL _IDL);

HL_PRIM void HL_NAME(Cmd_dispatch3)(pref<Cmd>* _this, int groupx, int groupy, int groupz) {
	(cmdDispatch( _unref(_this) , groupx, groupy, groupz));
}
DEFINE_PRIM(_VOID, Cmd_dispatch3, _IDL _I32 _I32 _I32);

HL_PRIM pref<Map64Int>* HL_NAME(Map64Int_new0)() {
	return alloc_ref((new Map64Int()),Map64Int);
}
DEFINE_PRIM(_IDL, Map64Int_new0,);

HL_PRIM bool HL_NAME(Map64Int_exists1)(pref<Map64Int>* _this, int64_t key) {
	return (_unref(_this)->exists(key));
}
DEFINE_PRIM(_BOOL, Map64Int_exists1, _IDL _I64);

HL_PRIM void HL_NAME(Map64Int_set2)(pref<Map64Int>* _this, int64_t key, int value) {
	(_unref(_this)->set(key, value));
}
DEFINE_PRIM(_VOID, Map64Int_set2, _IDL _I64 _I32);

HL_PRIM int HL_NAME(Map64Int_get1)(pref<Map64Int>* _this, int64_t key) {
	return (_unref(_this)->get(key));
}
DEFINE_PRIM(_I32, Map64Int_get1, _IDL _I64);

HL_PRIM int HL_NAME(Map64Int_size0)(pref<Map64Int>* _this) {
	return (_unref(_this)->size());
}
DEFINE_PRIM(_I32, Map64Int_size0, _IDL);

HL_PRIM void HL_NAME(Queue_waitIdle0)(pref<Queue>* _this) {
	(waitQueueIdle( _unref(_this) ));
}
DEFINE_PRIM(_VOID, Queue_waitIdle0, _IDL);

HL_PRIM void HL_NAME(Queue_submit4)(pref<Queue>* _this, pref<Cmd>* cmd, pref<Semaphore>* signalSemphor, pref<Semaphore>* wait, pref<Fence>* signalFence) {
	(forge_queue_submit_cmd( _unref(_this) , _unref_ptr_safe(cmd), _unref_ptr_safe(signalSemphor), _unref_ptr_safe(wait), _unref_ptr_safe(signalFence)));
}
DEFINE_PRIM(_VOID, Queue_submit4, _IDL _IDL _IDL _IDL _IDL);

HL_PRIM HL_CONST pref<RenderTarget>* HL_NAME(SwapChain_getRenderTarget1)(pref<SwapChain>* _this, int rtidx) {
	return alloc_ref_const((forge_swap_chain_get_render_target( _unref(_this) , rtidx)),RenderTarget);
}
DEFINE_PRIM(_IDL, SwapChain_getRenderTarget1, _IDL _I32);

HL_PRIM bool HL_NAME(SwapChain_isVSync0)(pref<SwapChain>* _this) {
	return (isVSync( _unref(_this) ));
}
DEFINE_PRIM(_BOOL, SwapChain_isVSync0, _IDL);

HL_PRIM pref<RenderTargetDesc>* HL_NAME(RenderTargetDesc_new0)() {
	auto ___retvalue = alloc_ref((new RenderTargetDesc()),RenderTargetDesc);
	*(___retvalue->value) = {};
	return (___retvalue);
}
DEFINE_PRIM(_IDL, RenderTargetDesc_new0,);

HL_PRIM unsigned int HL_NAME(RenderTargetDesc_get_flags)( pref<RenderTargetDesc>* _this ) {
	return _unref(_this)->mFlags;
}
DEFINE_PRIM(_I32,RenderTargetDesc_get_flags,_IDL);
HL_PRIM unsigned int HL_NAME(RenderTargetDesc_set_flags)( pref<RenderTargetDesc>* _this, unsigned int value ) {
	_unref(_this)->mFlags = (TextureCreationFlags)(value);
	return value;
}
DEFINE_PRIM(_I32,RenderTargetDesc_set_flags,_IDL _I32);

HL_PRIM unsigned int HL_NAME(RenderTargetDesc_get_width)( pref<RenderTargetDesc>* _this ) {
	return _unref(_this)->mWidth;
}
DEFINE_PRIM(_I32,RenderTargetDesc_get_width,_IDL);
HL_PRIM unsigned int HL_NAME(RenderTargetDesc_set_width)( pref<RenderTargetDesc>* _this, unsigned int value ) {
	_unref(_this)->mWidth = (value);
	return value;
}
DEFINE_PRIM(_I32,RenderTargetDesc_set_width,_IDL _I32);

HL_PRIM unsigned int HL_NAME(RenderTargetDesc_get_height)( pref<RenderTargetDesc>* _this ) {
	return _unref(_this)->mHeight;
}
DEFINE_PRIM(_I32,RenderTargetDesc_get_height,_IDL);
HL_PRIM unsigned int HL_NAME(RenderTargetDesc_set_height)( pref<RenderTargetDesc>* _this, unsigned int value ) {
	_unref(_this)->mHeight = (value);
	return value;
}
DEFINE_PRIM(_I32,RenderTargetDesc_set_height,_IDL _I32);

HL_PRIM unsigned int HL_NAME(RenderTargetDesc_get_depth)( pref<RenderTargetDesc>* _this ) {
	return _unref(_this)->mDepth;
}
DEFINE_PRIM(_I32,RenderTargetDesc_get_depth,_IDL);
HL_PRIM unsigned int HL_NAME(RenderTargetDesc_set_depth)( pref<RenderTargetDesc>* _this, unsigned int value ) {
	_unref(_this)->mDepth = (value);
	return value;
}
DEFINE_PRIM(_I32,RenderTargetDesc_set_depth,_IDL _I32);

HL_PRIM unsigned int HL_NAME(RenderTargetDesc_get_arraySize)( pref<RenderTargetDesc>* _this ) {
	return _unref(_this)->mArraySize;
}
DEFINE_PRIM(_I32,RenderTargetDesc_get_arraySize,_IDL);
HL_PRIM unsigned int HL_NAME(RenderTargetDesc_set_arraySize)( pref<RenderTargetDesc>* _this, unsigned int value ) {
	_unref(_this)->mArraySize = (value);
	return value;
}
DEFINE_PRIM(_I32,RenderTargetDesc_set_arraySize,_IDL _I32);

HL_PRIM unsigned int HL_NAME(RenderTargetDesc_get_mipLevels)( pref<RenderTargetDesc>* _this ) {
	return _unref(_this)->mMipLevels;
}
DEFINE_PRIM(_I32,RenderTargetDesc_get_mipLevels,_IDL);
HL_PRIM unsigned int HL_NAME(RenderTargetDesc_set_mipLevels)( pref<RenderTargetDesc>* _this, unsigned int value ) {
	_unref(_this)->mMipLevels = (value);
	return value;
}
DEFINE_PRIM(_I32,RenderTargetDesc_set_mipLevels,_IDL _I32);

HL_PRIM int HL_NAME(RenderTargetDesc_get_sampleCount)( pref<RenderTargetDesc>* _this ) {
	return HL_NAME(SampleCount_valueToIndex1)(_unref(_this)->mSampleCount);
}
DEFINE_PRIM(_I32,RenderTargetDesc_get_sampleCount,_IDL);
HL_PRIM int HL_NAME(RenderTargetDesc_set_sampleCount)( pref<RenderTargetDesc>* _this, int value ) {
	_unref(_this)->mSampleCount = (SampleCount)HL_NAME(SampleCount_indexToValue1)(value);
	return value;
}
DEFINE_PRIM(_I32,RenderTargetDesc_set_sampleCount,_IDL _I32);

HL_PRIM int HL_NAME(RenderTargetDesc_get_startState)( pref<RenderTargetDesc>* _this ) {
	return HL_NAME(ResourceState_valueToIndex1)(_unref(_this)->mStartState);
}
DEFINE_PRIM(_I32,RenderTargetDesc_get_startState,_IDL);
HL_PRIM int HL_NAME(RenderTargetDesc_set_startState)( pref<RenderTargetDesc>* _this, int value ) {
	_unref(_this)->mStartState = (ResourceState)HL_NAME(ResourceState_indexToValue1)(value);
	return value;
}
DEFINE_PRIM(_I32,RenderTargetDesc_set_startState,_IDL _I32);

HL_PRIM int HL_NAME(RenderTargetDesc_get_format)( pref<RenderTargetDesc>* _this ) {
	return HL_NAME(TinyImageFormat_valueToIndex1)(_unref(_this)->mFormat);
}
DEFINE_PRIM(_I32,RenderTargetDesc_get_format,_IDL);
HL_PRIM int HL_NAME(RenderTargetDesc_set_format)( pref<RenderTargetDesc>* _this, int value ) {
	_unref(_this)->mFormat = (TinyImageFormat)HL_NAME(TinyImageFormat_indexToValue1)(value);
	return value;
}
DEFINE_PRIM(_I32,RenderTargetDesc_set_format,_IDL _I32);

HL_PRIM unsigned int HL_NAME(RenderTargetDesc_get_sampleQuality)( pref<RenderTargetDesc>* _this ) {
	return _unref(_this)->mSampleQuality;
}
DEFINE_PRIM(_I32,RenderTargetDesc_get_sampleQuality,_IDL);
HL_PRIM unsigned int HL_NAME(RenderTargetDesc_set_sampleQuality)( pref<RenderTargetDesc>* _this, unsigned int value ) {
	_unref(_this)->mSampleQuality = (value);
	return value;
}
DEFINE_PRIM(_I32,RenderTargetDesc_set_sampleQuality,_IDL _I32);

HL_PRIM int HL_NAME(RenderTargetDesc_get_descriptors)( pref<RenderTargetDesc>* _this ) {
	return HL_NAME(DescriptorType_valueToIndex1)(_unref(_this)->mDescriptors);
}
DEFINE_PRIM(_I32,RenderTargetDesc_get_descriptors,_IDL);
HL_PRIM int HL_NAME(RenderTargetDesc_set_descriptors)( pref<RenderTargetDesc>* _this, int value ) {
	_unref(_this)->mDescriptors = (DescriptorType)HL_NAME(DescriptorType_indexToValue1)(value);
	return value;
}
DEFINE_PRIM(_I32,RenderTargetDesc_set_descriptors,_IDL _I32);

HL_PRIM void* HL_NAME(RenderTargetDesc_get_nativeHandle)( pref<RenderTargetDesc>* _this ) {
	return (void*)_unref(_this)->pNativeHandle;
}
DEFINE_PRIM(_BYTES,RenderTargetDesc_get_nativeHandle,_IDL);
HL_PRIM void* HL_NAME(RenderTargetDesc_set_nativeHandle)( pref<RenderTargetDesc>* _this, void* value ) {
	_unref(_this)->pNativeHandle = (value);
	return value;
}
DEFINE_PRIM(_BYTES,RenderTargetDesc_set_nativeHandle,_IDL _BYTES);

HL_PRIM unsigned int HL_NAME(RenderTargetDesc_get_mSharedNodeIndexCount)( pref<RenderTargetDesc>* _this ) {
	return _unref(_this)->mSharedNodeIndexCount;
}
DEFINE_PRIM(_I32,RenderTargetDesc_get_mSharedNodeIndexCount,_IDL);
HL_PRIM unsigned int HL_NAME(RenderTargetDesc_set_mSharedNodeIndexCount)( pref<RenderTargetDesc>* _this, unsigned int value ) {
	_unref(_this)->mSharedNodeIndexCount = (value);
	return value;
}
DEFINE_PRIM(_I32,RenderTargetDesc_set_mSharedNodeIndexCount,_IDL _I32);

HL_PRIM unsigned int HL_NAME(RenderTargetDesc_get_nodeIndex)( pref<RenderTargetDesc>* _this ) {
	return _unref(_this)->mNodeIndex;
}
DEFINE_PRIM(_I32,RenderTargetDesc_get_nodeIndex,_IDL);
HL_PRIM unsigned int HL_NAME(RenderTargetDesc_set_nodeIndex)( pref<RenderTargetDesc>* _this, unsigned int value ) {
	_unref(_this)->mNodeIndex = (value);
	return value;
}
DEFINE_PRIM(_I32,RenderTargetDesc_set_nodeIndex,_IDL _I32);

HL_PRIM int HL_NAME(RootSignature_getDescriptorIndexFromName1)(pref<RootSignature>* _this, vstring * name) {
	const char* name__cstr = (name == nullptr) ? "" : hl_to_utf8( name->bytes ); // Should be garbage collected
	auto ___retvalue = (getDescriptorIndexFromName( _unref(_this) , name__cstr));
	return (___retvalue);
}
DEFINE_PRIM(_I32, RootSignature_getDescriptorIndexFromName1, _IDL _STRING);

HL_PRIM pref<SamplerDesc>* HL_NAME(SamplerDesc_new0)() {
	return alloc_ref((new SamplerDesc()),SamplerDesc);
}
DEFINE_PRIM(_IDL, SamplerDesc_new0,);

HL_PRIM int HL_NAME(SamplerDesc_get_mMinFilter)( pref<SamplerDesc>* _this ) {
	return HL_NAME(FilterType_valueToIndex1)(_unref(_this)->mMinFilter);
}
DEFINE_PRIM(_I32,SamplerDesc_get_mMinFilter,_IDL);
HL_PRIM int HL_NAME(SamplerDesc_set_mMinFilter)( pref<SamplerDesc>* _this, int value ) {
	_unref(_this)->mMinFilter = (FilterType)HL_NAME(FilterType_indexToValue1)(value);
	return value;
}
DEFINE_PRIM(_I32,SamplerDesc_set_mMinFilter,_IDL _I32);

HL_PRIM int HL_NAME(SamplerDesc_get_mMagFilter)( pref<SamplerDesc>* _this ) {
	return HL_NAME(FilterType_valueToIndex1)(_unref(_this)->mMagFilter);
}
DEFINE_PRIM(_I32,SamplerDesc_get_mMagFilter,_IDL);
HL_PRIM int HL_NAME(SamplerDesc_set_mMagFilter)( pref<SamplerDesc>* _this, int value ) {
	_unref(_this)->mMagFilter = (FilterType)HL_NAME(FilterType_indexToValue1)(value);
	return value;
}
DEFINE_PRIM(_I32,SamplerDesc_set_mMagFilter,_IDL _I32);

HL_PRIM int HL_NAME(SamplerDesc_get_mMipMapMode)( pref<SamplerDesc>* _this ) {
	return HL_NAME(MipMapMode_valueToIndex1)(_unref(_this)->mMipMapMode);
}
DEFINE_PRIM(_I32,SamplerDesc_get_mMipMapMode,_IDL);
HL_PRIM int HL_NAME(SamplerDesc_set_mMipMapMode)( pref<SamplerDesc>* _this, int value ) {
	_unref(_this)->mMipMapMode = (MipMapMode)HL_NAME(MipMapMode_indexToValue1)(value);
	return value;
}
DEFINE_PRIM(_I32,SamplerDesc_set_mMipMapMode,_IDL _I32);

HL_PRIM int HL_NAME(SamplerDesc_get_mAddressU)( pref<SamplerDesc>* _this ) {
	return HL_NAME(AddressMode_valueToIndex1)(_unref(_this)->mAddressU);
}
DEFINE_PRIM(_I32,SamplerDesc_get_mAddressU,_IDL);
HL_PRIM int HL_NAME(SamplerDesc_set_mAddressU)( pref<SamplerDesc>* _this, int value ) {
	_unref(_this)->mAddressU = (AddressMode)HL_NAME(AddressMode_indexToValue1)(value);
	return value;
}
DEFINE_PRIM(_I32,SamplerDesc_set_mAddressU,_IDL _I32);

HL_PRIM int HL_NAME(SamplerDesc_get_mAddressV)( pref<SamplerDesc>* _this ) {
	return HL_NAME(AddressMode_valueToIndex1)(_unref(_this)->mAddressV);
}
DEFINE_PRIM(_I32,SamplerDesc_get_mAddressV,_IDL);
HL_PRIM int HL_NAME(SamplerDesc_set_mAddressV)( pref<SamplerDesc>* _this, int value ) {
	_unref(_this)->mAddressV = (AddressMode)HL_NAME(AddressMode_indexToValue1)(value);
	return value;
}
DEFINE_PRIM(_I32,SamplerDesc_set_mAddressV,_IDL _I32);

HL_PRIM int HL_NAME(SamplerDesc_get_mAddressW)( pref<SamplerDesc>* _this ) {
	return HL_NAME(AddressMode_valueToIndex1)(_unref(_this)->mAddressW);
}
DEFINE_PRIM(_I32,SamplerDesc_get_mAddressW,_IDL);
HL_PRIM int HL_NAME(SamplerDesc_set_mAddressW)( pref<SamplerDesc>* _this, int value ) {
	_unref(_this)->mAddressW = (AddressMode)HL_NAME(AddressMode_indexToValue1)(value);
	return value;
}
DEFINE_PRIM(_I32,SamplerDesc_set_mAddressW,_IDL _I32);

HL_PRIM float HL_NAME(SamplerDesc_get_mMipLodBias)( pref<SamplerDesc>* _this ) {
	return _unref(_this)->mMipLodBias;
}
DEFINE_PRIM(_F32,SamplerDesc_get_mMipLodBias,_IDL);
HL_PRIM float HL_NAME(SamplerDesc_set_mMipLodBias)( pref<SamplerDesc>* _this, float value ) {
	_unref(_this)->mMipLodBias = (value);
	return value;
}
DEFINE_PRIM(_F32,SamplerDesc_set_mMipLodBias,_IDL _F32);

HL_PRIM bool HL_NAME(SamplerDesc_get_mSetLodRange)( pref<SamplerDesc>* _this ) {
	return _unref(_this)->mSetLodRange;
}
DEFINE_PRIM(_BOOL,SamplerDesc_get_mSetLodRange,_IDL);
HL_PRIM bool HL_NAME(SamplerDesc_set_mSetLodRange)( pref<SamplerDesc>* _this, bool value ) {
	_unref(_this)->mSetLodRange = (value);
	return value;
}
DEFINE_PRIM(_BOOL,SamplerDesc_set_mSetLodRange,_IDL _BOOL);

HL_PRIM float HL_NAME(SamplerDesc_get_mMinLod)( pref<SamplerDesc>* _this ) {
	return _unref(_this)->mMinLod;
}
DEFINE_PRIM(_F32,SamplerDesc_get_mMinLod,_IDL);
HL_PRIM float HL_NAME(SamplerDesc_set_mMinLod)( pref<SamplerDesc>* _this, float value ) {
	_unref(_this)->mMinLod = (value);
	return value;
}
DEFINE_PRIM(_F32,SamplerDesc_set_mMinLod,_IDL _F32);

HL_PRIM float HL_NAME(SamplerDesc_get_mMaxLod)( pref<SamplerDesc>* _this ) {
	return _unref(_this)->mMaxLod;
}
DEFINE_PRIM(_F32,SamplerDesc_get_mMaxLod,_IDL);
HL_PRIM float HL_NAME(SamplerDesc_set_mMaxLod)( pref<SamplerDesc>* _this, float value ) {
	_unref(_this)->mMaxLod = (value);
	return value;
}
DEFINE_PRIM(_F32,SamplerDesc_set_mMaxLod,_IDL _F32);

HL_PRIM float HL_NAME(SamplerDesc_get_mMaxAnisotropy)( pref<SamplerDesc>* _this ) {
	return _unref(_this)->mMaxAnisotropy;
}
DEFINE_PRIM(_F32,SamplerDesc_get_mMaxAnisotropy,_IDL);
HL_PRIM float HL_NAME(SamplerDesc_set_mMaxAnisotropy)( pref<SamplerDesc>* _this, float value ) {
	_unref(_this)->mMaxAnisotropy = (value);
	return value;
}
DEFINE_PRIM(_F32,SamplerDesc_set_mMaxAnisotropy,_IDL _F32);

HL_PRIM int HL_NAME(SamplerDesc_get_mCompareFunc)( pref<SamplerDesc>* _this ) {
	return HL_NAME(CompareMode_valueToIndex1)(_unref(_this)->mCompareFunc);
}
DEFINE_PRIM(_I32,SamplerDesc_get_mCompareFunc,_IDL);
HL_PRIM int HL_NAME(SamplerDesc_set_mCompareFunc)( pref<SamplerDesc>* _this, int value ) {
	_unref(_this)->mCompareFunc = (CompareMode)HL_NAME(CompareMode_indexToValue1)(value);
	return value;
}
DEFINE_PRIM(_I32,SamplerDesc_set_mCompareFunc,_IDL _I32);

HL_PRIM pref<DescriptorSetDesc>* HL_NAME(DescriptorSetDesc_new0)() {
	return alloc_ref((new DescriptorSetDesc()),DescriptorSetDesc);
}
DEFINE_PRIM(_IDL, DescriptorSetDesc_new0,);

HL_PRIM HL_CONST pref<RootSignature>* HL_NAME(DescriptorSetDesc_get_pRootSignature)( pref<DescriptorSetDesc>* _this ) {
	return alloc_ref_const(_unref(_this)->pRootSignature,RootSignature);
}
DEFINE_PRIM(_IDL,DescriptorSetDesc_get_pRootSignature,_IDL);
HL_PRIM HL_CONST pref<RootSignature>* HL_NAME(DescriptorSetDesc_set_pRootSignature)( pref<DescriptorSetDesc>* _this, HL_CONST pref<RootSignature>* value ) {
	_unref(_this)->pRootSignature = _unref_ptr_safe(value);
	return value;
}
DEFINE_PRIM(_IDL,DescriptorSetDesc_set_pRootSignature,_IDL _IDL);

HL_PRIM int HL_NAME(DescriptorSetDesc_get_setIndex)( pref<DescriptorSetDesc>* _this ) {
	return _unref(_this)->mUpdateFrequency;
}
DEFINE_PRIM(_I32,DescriptorSetDesc_get_setIndex,_IDL);
HL_PRIM int HL_NAME(DescriptorSetDesc_set_setIndex)( pref<DescriptorSetDesc>* _this, int value ) {
	_unref(_this)->mUpdateFrequency = (DescriptorUpdateFrequency)(value);
	return value;
}
DEFINE_PRIM(_I32,DescriptorSetDesc_set_setIndex,_IDL _I32);

HL_PRIM unsigned int HL_NAME(DescriptorSetDesc_get_maxSets)( pref<DescriptorSetDesc>* _this ) {
	return _unref(_this)->mMaxSets;
}
DEFINE_PRIM(_I32,DescriptorSetDesc_get_maxSets,_IDL);
HL_PRIM unsigned int HL_NAME(DescriptorSetDesc_set_maxSets)( pref<DescriptorSetDesc>* _this, unsigned int value ) {
	_unref(_this)->mMaxSets = (value);
	return value;
}
DEFINE_PRIM(_I32,DescriptorSetDesc_set_maxSets,_IDL _I32);

HL_PRIM unsigned int HL_NAME(DescriptorSetDesc_get_nodeIndex)( pref<DescriptorSetDesc>* _this ) {
	return _unref(_this)->mNodeIndex;
}
DEFINE_PRIM(_I32,DescriptorSetDesc_get_nodeIndex,_IDL);
HL_PRIM unsigned int HL_NAME(DescriptorSetDesc_set_nodeIndex)( pref<DescriptorSetDesc>* _this, unsigned int value ) {
	_unref(_this)->mNodeIndex = (value);
	return value;
}
DEFINE_PRIM(_I32,DescriptorSetDesc_set_nodeIndex,_IDL _I32);

HL_PRIM pref<DescriptorDataBuilder>* HL_NAME(DescriptorDataBuilder_new0)() {
	return alloc_ref((new DescriptorDataBuilder()),DescriptorDataBuilder);
}
DEFINE_PRIM(_IDL, DescriptorDataBuilder_new0,);

HL_PRIM void HL_NAME(DescriptorDataBuilder_clear0)(pref<DescriptorDataBuilder>* _this) {
	(_unref(_this)->clear());
}
DEFINE_PRIM(_VOID, DescriptorDataBuilder_clear0, _IDL);

HL_PRIM void HL_NAME(DescriptorDataBuilder_clearSlotData1)(pref<DescriptorDataBuilder>* _this, int index) {
	(_unref(_this)->clearSlotData(index));
}
DEFINE_PRIM(_VOID, DescriptorDataBuilder_clearSlotData1, _IDL _I32);

HL_PRIM int HL_NAME(DescriptorDataBuilder_addSlot1)(pref<DescriptorDataBuilder>* _this, int descriptorType) {
	return (_unref(_this)->addSlot(DescriptorSlotMode__values[descriptorType]));
}
DEFINE_PRIM(_I32, DescriptorDataBuilder_addSlot1, _IDL _I32);

HL_PRIM void HL_NAME(DescriptorDataBuilder_setSlotBindName2)(pref<DescriptorDataBuilder>* _this, int slot, vstring * name) {
	const char* name__cstr = (name == nullptr) ? "" : hl_to_utf8( name->bytes ); // Should be garbage collected
	(_unref(_this)->setSlotBindName(slot, name__cstr));
}
DEFINE_PRIM(_VOID, DescriptorDataBuilder_setSlotBindName2, _IDL _I32 _STRING);

HL_PRIM void HL_NAME(DescriptorDataBuilder_setSlotBindIndex2)(pref<DescriptorDataBuilder>* _this, int slot, int index) {
	(_unref(_this)->setSlotBindIndex(slot, index));
}
DEFINE_PRIM(_VOID, DescriptorDataBuilder_setSlotBindIndex2, _IDL _I32 _I32);

HL_PRIM void HL_NAME(DescriptorDataBuilder_addSlotTexture2)(pref<DescriptorDataBuilder>* _this, int slot, pref<Texture>* tex) {
	(_unref(_this)->addSlotData(slot, _unref_ptr_safe(tex)));
}
DEFINE_PRIM(_VOID, DescriptorDataBuilder_addSlotTexture2, _IDL _I32 _IDL);

HL_PRIM void HL_NAME(DescriptorDataBuilder_addSlotSampler2)(pref<DescriptorDataBuilder>* _this, int slot, pref<Sampler>* sampler) {
	(_unref(_this)->addSlotData(slot, _unref_ptr_safe(sampler)));
}
DEFINE_PRIM(_VOID, DescriptorDataBuilder_addSlotSampler2, _IDL _I32 _IDL);

HL_PRIM void HL_NAME(DescriptorDataBuilder_addSlotUniformBuffer2)(pref<DescriptorDataBuilder>* _this, int slot, pref<Buffer>* uniformBuffer) {
	(_unref(_this)->addSlotData(slot, _unref_ptr_safe(uniformBuffer)));
}
DEFINE_PRIM(_VOID, DescriptorDataBuilder_addSlotUniformBuffer2, _IDL _I32 _IDL);

HL_PRIM void HL_NAME(DescriptorDataBuilder_setSlotUAVMipSlice2)(pref<DescriptorDataBuilder>* _this, int slot, int idx) {
	(_unref(_this)->setSlotUAVMipSlice(slot, idx));
}
DEFINE_PRIM(_VOID, DescriptorDataBuilder_setSlotUAVMipSlice2, _IDL _I32 _I32);

HL_PRIM void HL_NAME(DescriptorDataBuilder_update3)(pref<DescriptorDataBuilder>* _this, pref<Renderer>* renderer, int index, pref<DescriptorSet>* set) {
	(_unref(_this)->update(_unref_ptr_safe(renderer), index, _unref_ptr_safe(set)));
}
DEFINE_PRIM(_VOID, DescriptorDataBuilder_update3, _IDL _IDL _I32 _IDL);

HL_PRIM void HL_NAME(DescriptorDataBuilder_bind3)(pref<DescriptorDataBuilder>* _this, pref<Cmd>* cmd, int index, pref<DescriptorSet>* set) {
	(_unref(_this)->bind(_unref_ptr_safe(cmd), index, _unref_ptr_safe(set)));
}
DEFINE_PRIM(_VOID, DescriptorDataBuilder_bind3, _IDL _IDL _I32 _IDL);

HL_PRIM pref<RootSignatureFactory>* HL_NAME(RootSignatureDesc_new0)() {
	return alloc_ref((new RootSignatureFactory()),RootSignatureDesc);
}
DEFINE_PRIM(_IDL, RootSignatureDesc_new0,);

HL_PRIM void HL_NAME(RootSignatureDesc_addShader1)(pref<RootSignatureFactory>* _this, pref<Shader>* shader) {
	(_unref(_this)->addShader(_unref_ptr_safe(shader)));
}
DEFINE_PRIM(_VOID, RootSignatureDesc_addShader1, _IDL _IDL);

HL_PRIM void HL_NAME(RootSignatureDesc_addSampler2)(pref<RootSignatureFactory>* _this, pref<Sampler>* sampler, vstring * name) {
	const char* name__cstr = (name == nullptr) ? "" : hl_to_utf8( name->bytes ); // Should be garbage collected
	(_unref(_this)->addSampler(_unref_ptr_safe(sampler), name__cstr));
}
DEFINE_PRIM(_VOID, RootSignatureDesc_addSampler2, _IDL _IDL _STRING);

HL_PRIM HL_CONST pref<Queue>* HL_NAME(Renderer_createQueue0)(pref<Renderer>* _this) {
	return alloc_ref_const((createQueue( _unref(_this) )),Queue);
}
DEFINE_PRIM(_IDL, Renderer_createQueue0, _IDL);

HL_PRIM void HL_NAME(Renderer_removeQueue1)(pref<Renderer>* _this, pref<Queue>* pGraphicsQueue) {
	(removeQueue( _unref(_this) , _unref_ptr_safe(pGraphicsQueue)));
}
DEFINE_PRIM(_VOID, Renderer_removeQueue1, _IDL _IDL);

HL_PRIM HL_CONST pref<RenderTarget>* HL_NAME(Renderer_createRenderTarget1)(pref<Renderer>* _this, pref<RenderTargetDesc>* desc) {
	return alloc_ref_const((forge_sdl_create_render_target( _unref(_this) , _unref_ptr_safe(desc))),RenderTarget);
}
DEFINE_PRIM(_IDL, Renderer_createRenderTarget1, _IDL _IDL);

HL_PRIM void HL_NAME(Renderer_initResourceLoaderInterface0)(pref<Renderer>* _this) {
	(forge_init_loader( _unref(_this) ));
}
DEFINE_PRIM(_VOID, Renderer_initResourceLoaderInterface0, _IDL);

HL_PRIM HL_CONST pref<CmdPool>* HL_NAME(Renderer_createCommandPool1)(pref<Renderer>* _this, pref<Queue>* queue) {
	return alloc_ref_const((forge_sdl_renderer_create_cmd_pool( _unref(_this) , _unref_ptr_safe(queue))),CmdPool);
}
DEFINE_PRIM(_IDL, Renderer_createCommandPool1, _IDL _IDL);

HL_PRIM HL_CONST pref<Cmd>* HL_NAME(Renderer_createCommand1)(pref<Renderer>* _this, pref<CmdPool>* pool) {
	return alloc_ref_const((forge_sdl_renderer_create_cmd( _unref(_this) , _unref_ptr_safe(pool))),Cmd);
}
DEFINE_PRIM(_IDL, Renderer_createCommand1, _IDL _IDL);

HL_PRIM HL_CONST pref<Fence>* HL_NAME(Renderer_createFence0)(pref<Renderer>* _this) {
	return alloc_ref_const((forge_sdl_renderer_create_fence( _unref(_this) )),Fence);
}
DEFINE_PRIM(_IDL, Renderer_createFence0, _IDL);

HL_PRIM HL_CONST pref<Semaphore>* HL_NAME(Renderer_createSemaphore0)(pref<Renderer>* _this) {
	return alloc_ref_const((forge_sdl_renderer_create_semaphore( _unref(_this) )),Semaphore);
}
DEFINE_PRIM(_IDL, Renderer_createSemaphore0, _IDL);

HL_PRIM unsigned int HL_NAME(Renderer_acquireNextImage4)(pref<Renderer>* _this, pref<SwapChain>* pSwapChain, pref<Semaphore>* pImageAcquiredSemaphore, pref<Fence>* fence) {
	unsigned int __tmpret;
acquireNextImage( _unref(_this) , _unref_ptr_safe(pSwapChain), _unref_ptr_safe(pImageAcquiredSemaphore), _unref_ptr_safe(fence), &__tmpret);
	return (__tmpret);
}
DEFINE_PRIM(_I32, Renderer_acquireNextImage4, _IDL _IDL _IDL _IDL);

HL_PRIM HL_CONST pref<Shader>* HL_NAME(Renderer_loadComputeShader1)(pref<Renderer>* _this, vstring * fileName) {
	const char* fileName__cstr = (fileName == nullptr) ? "" : hl_to_utf8( fileName->bytes ); // Should be garbage collected
	auto ___retvalue = alloc_ref_const((forge_load_compute_shader_file( _unref(_this) , fileName__cstr)),Shader);
	return (___retvalue);
}
DEFINE_PRIM(_IDL, Renderer_loadComputeShader1, _IDL _STRING);

HL_PRIM pref<DescriptorSet>* HL_NAME(Renderer_addDescriptorSet2)(pref<Renderer>* _this, pref<DescriptorSetDesc>* desc) {
	DescriptorSet* __tmpret;
addDescriptorSet( _unref(_this) , _unref_ptr_safe(desc), &__tmpret);
	return alloc_ref_const( __tmpret, DescriptorSet );
}
DEFINE_PRIM(_IDL, Renderer_addDescriptorSet2, _IDL _IDL);

HL_PRIM HL_CONST pref<DescriptorSet>* HL_NAME(Renderer_createDescriptorSet4)(pref<Renderer>* _this, pref<RootSignature>* sig, int setIndex, unsigned int maxSets, unsigned int nodeIndex) {
	return alloc_ref_const((forge_renderer_create_descriptor_set( _unref(_this) , _unref_ptr_safe(sig), setIndex, maxSets, nodeIndex)),DescriptorSet);
}
DEFINE_PRIM(_IDL, Renderer_createDescriptorSet4, _IDL _IDL _I32 _I32 _I32);

HL_PRIM pref<Sampler>* HL_NAME(Renderer_createSampler2)(pref<Renderer>* _this, pref<SamplerDesc>* desc) {
	Sampler* __tmpret;
addSampler( _unref(_this) , _unref_ptr_safe(desc), &__tmpret);
	return alloc_ref_const( __tmpret, Sampler );
}
DEFINE_PRIM(_IDL, Renderer_createSampler2, _IDL _IDL);

HL_PRIM void HL_NAME(Renderer_waitFence1)(pref<Renderer>* _this, pref<Fence>* fence) {
	(forge_renderer_wait_fence( _unref(_this) , _unref_ptr_safe(fence)));
}
DEFINE_PRIM(_VOID, Renderer_waitFence1, _IDL _IDL);

HL_PRIM HL_CONST pref<Shader>* HL_NAME(Renderer_createShader2)(pref<Renderer>* _this, vstring * vertShaderPath, vstring * fragShaderPath) {
	const char* vertShaderPath__cstr = (vertShaderPath == nullptr) ? "" : hl_to_utf8( vertShaderPath->bytes ); // Should be garbage collected
	const char* fragShaderPath__cstr = (fragShaderPath == nullptr) ? "" : hl_to_utf8( fragShaderPath->bytes ); // Should be garbage collected
	auto ___retvalue = alloc_ref_const((forge_renderer_shader_create( _unref(_this) , vertShaderPath__cstr, fragShaderPath__cstr)),Shader);
	return (___retvalue);
}
DEFINE_PRIM(_IDL, Renderer_createShader2, _IDL _STRING _STRING);

HL_PRIM bool HL_NAME(Renderer_captureAsBytes6)(pref<Renderer>* _this, pref<Cmd>* submittableCmd, pref<RenderTarget>* renderTarget, pref<Queue>* queue, int renderTargetCurrentState, vbyte* buffer, int bufferSize) {
	return (forge_render_target_capture_2( _unref(_this) , _unref_ptr_safe(submittableCmd), _unref_ptr_safe(renderTarget), _unref_ptr_safe(queue), ResourceState__values[renderTargetCurrentState], buffer, bufferSize));
}
DEFINE_PRIM(_BOOL, Renderer_captureAsBytes6, _IDL _IDL _IDL _IDL _I32 _BYTES _I32);

HL_PRIM void HL_NAME(Renderer_resetCmdPool1)(pref<Renderer>* _this, pref<CmdPool>* pool) {
	(resetCmdPool( _unref(_this) , _unref_ptr_safe(pool)));
}
DEFINE_PRIM(_VOID, Renderer_resetCmdPool1, _IDL _IDL);

HL_PRIM pref<Pipeline>* HL_NAME(Renderer_createPipeline2)(pref<Renderer>* _this, pref<HlForgePipelineDesc>* desc) {
	Pipeline* __tmpret;
addPipeline( _unref(_this) , _unref_ptr_safe(desc), &__tmpret);
	return alloc_ref_const( __tmpret, Pipeline );
}
DEFINE_PRIM(_IDL, Renderer_createPipeline2, _IDL _IDL);

HL_PRIM HL_CONST pref<RootSignature>* HL_NAME(Renderer_createRootSigSimple1)(pref<Renderer>* _this, pref<Shader>* shader) {
	return alloc_ref_const((forge_renderer_createRootSignatureSimple( _unref(_this) , _unref_ptr_safe(shader))),RootSignature);
}
DEFINE_PRIM(_IDL, Renderer_createRootSigSimple1, _IDL _IDL);

HL_PRIM HL_CONST pref<RootSignature>* HL_NAME(Renderer_createRootSig1)(pref<Renderer>* _this, pref<RootSignatureFactory>* desc) {
	return alloc_ref_const((forge_renderer_createRootSignature( _unref(_this) , _unref_ptr_safe(desc))),RootSignature);
}
DEFINE_PRIM(_IDL, Renderer_createRootSig1, _IDL _IDL);

HL_PRIM void HL_NAME(Renderer_toggleVSync1)(pref<Renderer>* _this, pref<SwapChain>* sc) {
	(::toggleVSync( _unref(_this) , &_unref(sc)));
}
DEFINE_PRIM(_VOID, Renderer_toggleVSync1, _IDL _IDL);

HL_PRIM HL_CONST void HL_NAME(Renderer_destroySwapChain1)(pref<Renderer>* _this, pref<SwapChain>* swapChain) {
	(forge_renderer_destroySwapChain( _unref(_this) , _unref_ptr_safe(swapChain)));
}
DEFINE_PRIM(_VOID, Renderer_destroySwapChain1, _IDL _IDL);

HL_PRIM HL_CONST void HL_NAME(Renderer_destroyRenderTarget1)(pref<Renderer>* _this, pref<RenderTarget>* swapChain) {
	(forge_renderer_destroyRenderTarget( _unref(_this) , _unref_ptr_safe(swapChain)));
}
DEFINE_PRIM(_VOID, Renderer_destroyRenderTarget1, _IDL _IDL);

HL_PRIM void HL_NAME(Renderer_fillDescriptorSet4)(pref<Renderer>* _this, pref<BufferExt>* buf, pref<DescriptorSet>* ds, int mode, int slotIndex) {
	(forge_renderer_fill_descriptor_set( _unref(_this) , _unref_ptr_safe(buf), _unref_ptr_safe(ds), DescriptorSlotMode__values[mode], slotIndex));
}
DEFINE_PRIM(_VOID, Renderer_fillDescriptorSet4, _IDL _IDL _IDL _I32 _I32);

HL_PRIM pref<ForgeSDLWindow>* HL_NAME(ForgeSDLWindow_new1)(pref<SDL_Window>* sdlWindow) {
	return alloc_ref((new ForgeSDLWindow(_unref_ptr_safe(sdlWindow))),ForgeSDLWindow);
}
DEFINE_PRIM(_IDL, ForgeSDLWindow_new1, _IDL);

HL_PRIM HL_CONST pref<SwapChain>* HL_NAME(ForgeSDLWindow_createSwapChain6)(pref<ForgeSDLWindow>* _this, pref<Renderer>* renderer, pref<Queue>* queue, int width, int height, int count, bool hdr10) {
	return alloc_ref_const((_unref(_this)->createSwapChain(_unref_ptr_safe(renderer), _unref_ptr_safe(queue), width, height, count, hdr10)),SwapChain);
}
DEFINE_PRIM(_IDL, ForgeSDLWindow_createSwapChain6, _IDL _IDL _IDL _I32 _I32 _I32 _BOOL);

HL_PRIM HL_CONST pref<Renderer>* HL_NAME(ForgeSDLWindow_renderer0)(pref<ForgeSDLWindow>* _this) {
	return alloc_ref_const((_unref(_this)->renderer()),Renderer);
}
DEFINE_PRIM(_IDL, ForgeSDLWindow_renderer0, _IDL);

HL_PRIM void HL_NAME(ForgeSDLWindow_present4)(pref<ForgeSDLWindow>* _this, pref<Queue>* pGraphicsQueue, pref<SwapChain>* pSwapChain, int swapchainImageIndex, pref<Semaphore>* pRenderCompleteSemaphore) {
	(_unref(_this)->present(_unref_ptr_safe(pGraphicsQueue), _unref_ptr_safe(pSwapChain), swapchainImageIndex, _unref_ptr_safe(pRenderCompleteSemaphore)));
}
DEFINE_PRIM(_VOID, ForgeSDLWindow_present4, _IDL _IDL _IDL _I32 _IDL);

HL_PRIM void HL_NAME(Buffer_updateRegion4)(pref<BufferExt>* _this, vbyte* data, int toffset, int size, int soffset) {
	(_unref(_this)->updateRegion(data, toffset, size, soffset));
}
DEFINE_PRIM(_VOID, Buffer_updateRegion4, _IDL _BYTES _I32 _I32 _I32);

HL_PRIM void HL_NAME(Buffer_update1)(pref<BufferExt>* _this, vbyte* data) {
	(_unref(_this)->update(data));
}
DEFINE_PRIM(_VOID, Buffer_update1, _IDL _BYTES);

HL_PRIM void HL_NAME(Buffer_dispose0)(pref<BufferExt>* _this) {
	(_unref(_this)->dispose());
}
DEFINE_PRIM(_VOID, Buffer_dispose0, _IDL);

HL_PRIM vbyte* HL_NAME(Buffer_getCpuAddress0)(pref<BufferExt>* _this) {
	return (_unref(_this)->getCpuAddress());
}
DEFINE_PRIM(_BYTES, Buffer_getCpuAddress0, _IDL);

HL_PRIM int HL_NAME(Buffer_getSize0)(pref<BufferExt>* _this) {
	return (_unref(_this)->getSize());
}
DEFINE_PRIM(_I32, Buffer_getSize0, _IDL);

HL_PRIM int HL_NAME(Buffer_next0)(pref<BufferExt>* _this) {
	return (_unref(_this)->next());
}
DEFINE_PRIM(_I32, Buffer_next0, _IDL);

HL_PRIM int HL_NAME(Buffer_currentIdx0)(pref<BufferExt>* _this) {
	return (_unref(_this)->currentIdx());
}
DEFINE_PRIM(_I32, Buffer_currentIdx0, _IDL);

HL_PRIM void HL_NAME(Buffer_setCurrent1)(pref<BufferExt>* _this, int idx) {
	(_unref(_this)->setCurrent(idx));
}
DEFINE_PRIM(_VOID, Buffer_setCurrent1, _IDL _I32);

HL_PRIM HL_CONST pref<Buffer>* HL_NAME(Buffer_get1)(pref<BufferExt>* _this, int idx) {
	return alloc_ref_const((_unref(_this)->get(idx)),InternalBuffer);
}
DEFINE_PRIM(_IDL, Buffer_get1, _IDL _I32);

HL_PRIM HL_CONST pref<Buffer>* HL_NAME(Buffer_current0)(pref<BufferExt>* _this) {
	return alloc_ref_const((_unref(_this)->current()),InternalBuffer);
}
DEFINE_PRIM(_IDL, Buffer_current0, _IDL);

HL_PRIM pref<BufferLoadDescExt>* HL_NAME(BufferLoadDesc_new0)() {
	auto ___retvalue = alloc_ref((new BufferLoadDescExt()),BufferLoadDesc);
	*(___retvalue->value) = {};
	return (___retvalue);
}
DEFINE_PRIM(_IDL, BufferLoadDesc_new0,);

HL_PRIM bool HL_NAME(BufferLoadDesc_get_forceReset)( pref<BufferLoadDescExt>* _this ) {
	return _unref(_this)->mForceReset;
}
DEFINE_PRIM(_BOOL,BufferLoadDesc_get_forceReset,_IDL);
HL_PRIM bool HL_NAME(BufferLoadDesc_set_forceReset)( pref<BufferLoadDescExt>* _this, bool value ) {
	_unref(_this)->mForceReset = (value);
	return value;
}
DEFINE_PRIM(_BOOL,BufferLoadDesc_set_forceReset,_IDL _BOOL);

HL_PRIM pref<BufferExt>* HL_NAME(BufferLoadDesc_load1)(pref<BufferLoadDescExt>* _this, pref<SyncToken>* syncToken) {
	return alloc_ref((_unref(_this)->load(_unref_ptr_safe(syncToken))),Buffer);
}
DEFINE_PRIM(_IDL, BufferLoadDesc_load1, _IDL _IDL);

HL_PRIM void HL_NAME(BufferLoadDesc_setIndexBuffer2)(pref<BufferLoadDescExt>* _this, int size, vbyte* data) {
	(_unref(_this)->setIndexBuffer(size, data));
}
DEFINE_PRIM(_VOID, BufferLoadDesc_setIndexBuffer2, _IDL _I32 _BYTES);

HL_PRIM void HL_NAME(BufferLoadDesc_setVertexBuffer2)(pref<BufferLoadDescExt>* _this, int size, vbyte* data) {
	(_unref(_this)->setVertexBuffer(size, data));
}
DEFINE_PRIM(_VOID, BufferLoadDesc_setVertexBuffer2, _IDL _I32 _BYTES);

HL_PRIM void HL_NAME(BufferLoadDesc_setUniformBuffer2)(pref<BufferLoadDescExt>* _this, int size, vbyte* data) {
	(_unref(_this)->setUniformBuffer(size, data));
}
DEFINE_PRIM(_VOID, BufferLoadDesc_setUniformBuffer2, _IDL _I32 _BYTES);

HL_PRIM void HL_NAME(BufferLoadDesc_setDynamic1)(pref<BufferLoadDescExt>* _this, int depth) {
	(_unref(_this)->setDynamic(depth));
}
DEFINE_PRIM(_VOID, BufferLoadDesc_setDynamic1, _IDL _I32);

HL_PRIM void HL_NAME(BufferLoadDesc_setUsage1)(pref<BufferLoadDescExt>* _this, bool shared) {
	(_unref(_this)->setUsage(shared));
}
DEFINE_PRIM(_VOID, BufferLoadDesc_setUsage1, _IDL _BOOL);

HL_PRIM void HL_NAME(Texture_upload2)(pref<Texture>* _this, vbyte* data, int size) {
	(forge_sdl_texture_upload( _unref(_this) , data, size));
}
DEFINE_PRIM(_VOID, Texture_upload2, _IDL _BYTES _I32);

HL_PRIM void HL_NAME(Texture_uploadMip3)(pref<Texture>* _this, int mip, vbyte* data, int size) {
	(forge_texture_upload_mip( _unref(_this) , mip, data, size));
}
DEFINE_PRIM(_VOID, Texture_uploadMip3, _IDL _I32 _BYTES _I32);

HL_PRIM void HL_NAME(Texture_uploadLayerMip4)(pref<Texture>* _this, int layer, int mip, vbyte* data, int size) {
	(forge_texture_upload_layer_mip( _unref(_this) , layer, mip, data, size));
}
DEFINE_PRIM(_VOID, Texture_uploadLayerMip4, _IDL _I32 _I32 _BYTES _I32);

HL_PRIM void HL_NAME(Texture_dispose0)(pref<Texture>* _this) {
	(removeResource( _unref(_this) ));
}
DEFINE_PRIM(_VOID, Texture_dispose0, _IDL);

HL_PRIM pref<TextureDesc>* HL_NAME(TextureDesc_new0)() {
	auto ___retvalue = alloc_ref((new TextureDesc()),TextureDesc);
	*(___retvalue->value) = {};
	return (___retvalue);
}
DEFINE_PRIM(_IDL, TextureDesc_new0,);

HL_PRIM HL_CONST pref<Texture>* HL_NAME(TextureDesc_load2)(pref<TextureDesc>* _this, vstring * name, pref<SyncToken>* syncToken) {
	const char* name__cstr = (name == nullptr) ? "" : hl_to_utf8( name->bytes ); // Should be garbage collected
	auto ___retvalue = alloc_ref_const((forge_texture_load_from_desc( _unref(_this) , name__cstr, _unref_ptr_safe(syncToken))),Texture);
	return (___retvalue);
}
DEFINE_PRIM(_IDL, TextureDesc_load2, _IDL _STRING _IDL);

HL_PRIM void* HL_NAME(TextureDesc_get_nativeHandle)( pref<TextureDesc>* _this ) {
	return (void*)_unref(_this)->pNativeHandle;
}
DEFINE_PRIM(_BYTES,TextureDesc_get_nativeHandle,_IDL);
HL_PRIM void* HL_NAME(TextureDesc_set_nativeHandle)( pref<TextureDesc>* _this, void* value ) {
	_unref(_this)->pNativeHandle = (value);
	return value;
}
DEFINE_PRIM(_BYTES,TextureDesc_set_nativeHandle,_IDL _BYTES);

HL_PRIM unsigned int HL_NAME(TextureDesc_get_flags)( pref<TextureDesc>* _this ) {
	return _unref(_this)->mFlags;
}
DEFINE_PRIM(_I32,TextureDesc_get_flags,_IDL);
HL_PRIM unsigned int HL_NAME(TextureDesc_set_flags)( pref<TextureDesc>* _this, unsigned int value ) {
	_unref(_this)->mFlags = (TextureCreationFlags)(value);
	return value;
}
DEFINE_PRIM(_I32,TextureDesc_set_flags,_IDL _I32);

HL_PRIM unsigned int HL_NAME(TextureDesc_get_width)( pref<TextureDesc>* _this ) {
	return _unref(_this)->mWidth;
}
DEFINE_PRIM(_I32,TextureDesc_get_width,_IDL);
HL_PRIM unsigned int HL_NAME(TextureDesc_set_width)( pref<TextureDesc>* _this, unsigned int value ) {
	_unref(_this)->mWidth = (value);
	return value;
}
DEFINE_PRIM(_I32,TextureDesc_set_width,_IDL _I32);

HL_PRIM unsigned int HL_NAME(TextureDesc_get_height)( pref<TextureDesc>* _this ) {
	return _unref(_this)->mHeight;
}
DEFINE_PRIM(_I32,TextureDesc_get_height,_IDL);
HL_PRIM unsigned int HL_NAME(TextureDesc_set_height)( pref<TextureDesc>* _this, unsigned int value ) {
	_unref(_this)->mHeight = (value);
	return value;
}
DEFINE_PRIM(_I32,TextureDesc_set_height,_IDL _I32);

HL_PRIM unsigned int HL_NAME(TextureDesc_get_depth)( pref<TextureDesc>* _this ) {
	return _unref(_this)->mDepth;
}
DEFINE_PRIM(_I32,TextureDesc_get_depth,_IDL);
HL_PRIM unsigned int HL_NAME(TextureDesc_set_depth)( pref<TextureDesc>* _this, unsigned int value ) {
	_unref(_this)->mDepth = (value);
	return value;
}
DEFINE_PRIM(_I32,TextureDesc_set_depth,_IDL _I32);

HL_PRIM unsigned int HL_NAME(TextureDesc_get_arraySize)( pref<TextureDesc>* _this ) {
	return _unref(_this)->mArraySize;
}
DEFINE_PRIM(_I32,TextureDesc_get_arraySize,_IDL);
HL_PRIM unsigned int HL_NAME(TextureDesc_set_arraySize)( pref<TextureDesc>* _this, unsigned int value ) {
	_unref(_this)->mArraySize = (value);
	return value;
}
DEFINE_PRIM(_I32,TextureDesc_set_arraySize,_IDL _I32);

HL_PRIM unsigned int HL_NAME(TextureDesc_get_mipLevels)( pref<TextureDesc>* _this ) {
	return _unref(_this)->mMipLevels;
}
DEFINE_PRIM(_I32,TextureDesc_get_mipLevels,_IDL);
HL_PRIM unsigned int HL_NAME(TextureDesc_set_mipLevels)( pref<TextureDesc>* _this, unsigned int value ) {
	_unref(_this)->mMipLevels = (value);
	return value;
}
DEFINE_PRIM(_I32,TextureDesc_set_mipLevels,_IDL _I32);

HL_PRIM int HL_NAME(TextureDesc_get_sampleCount)( pref<TextureDesc>* _this ) {
	return HL_NAME(SampleCount_valueToIndex1)(_unref(_this)->mSampleCount);
}
DEFINE_PRIM(_I32,TextureDesc_get_sampleCount,_IDL);
HL_PRIM int HL_NAME(TextureDesc_set_sampleCount)( pref<TextureDesc>* _this, int value ) {
	_unref(_this)->mSampleCount = (SampleCount)HL_NAME(SampleCount_indexToValue1)(value);
	return value;
}
DEFINE_PRIM(_I32,TextureDesc_set_sampleCount,_IDL _I32);

HL_PRIM unsigned int HL_NAME(TextureDesc_get_sampleQuality)( pref<TextureDesc>* _this ) {
	return _unref(_this)->mSampleQuality;
}
DEFINE_PRIM(_I32,TextureDesc_get_sampleQuality,_IDL);
HL_PRIM unsigned int HL_NAME(TextureDesc_set_sampleQuality)( pref<TextureDesc>* _this, unsigned int value ) {
	_unref(_this)->mSampleQuality = (value);
	return value;
}
DEFINE_PRIM(_I32,TextureDesc_set_sampleQuality,_IDL _I32);

HL_PRIM int HL_NAME(TextureDesc_get_format)( pref<TextureDesc>* _this ) {
	return HL_NAME(TinyImageFormat_valueToIndex1)(_unref(_this)->mFormat);
}
DEFINE_PRIM(_I32,TextureDesc_get_format,_IDL);
HL_PRIM int HL_NAME(TextureDesc_set_format)( pref<TextureDesc>* _this, int value ) {
	_unref(_this)->mFormat = (TinyImageFormat)HL_NAME(TinyImageFormat_indexToValue1)(value);
	return value;
}
DEFINE_PRIM(_I32,TextureDesc_set_format,_IDL _I32);

HL_PRIM int HL_NAME(TextureDesc_get_startState)( pref<TextureDesc>* _this ) {
	return HL_NAME(ResourceState_valueToIndex1)(_unref(_this)->mStartState);
}
DEFINE_PRIM(_I32,TextureDesc_get_startState,_IDL);
HL_PRIM int HL_NAME(TextureDesc_set_startState)( pref<TextureDesc>* _this, int value ) {
	_unref(_this)->mStartState = (ResourceState)HL_NAME(ResourceState_indexToValue1)(value);
	return value;
}
DEFINE_PRIM(_I32,TextureDesc_set_startState,_IDL _I32);

HL_PRIM unsigned int HL_NAME(TextureDesc_get_descriptors)( pref<TextureDesc>* _this ) {
	return _unref(_this)->mDescriptors;
}
DEFINE_PRIM(_I32,TextureDesc_get_descriptors,_IDL);
HL_PRIM unsigned int HL_NAME(TextureDesc_set_descriptors)( pref<TextureDesc>* _this, unsigned int value ) {
	_unref(_this)->mDescriptors = (DescriptorType)(value);
	return value;
}
DEFINE_PRIM(_I32,TextureDesc_set_descriptors,_IDL _I32);

HL_PRIM unsigned int HL_NAME(TextureDesc_get_sharedNodeIndexCount)( pref<TextureDesc>* _this ) {
	return _unref(_this)->mSharedNodeIndexCount;
}
DEFINE_PRIM(_I32,TextureDesc_get_sharedNodeIndexCount,_IDL);
HL_PRIM unsigned int HL_NAME(TextureDesc_set_sharedNodeIndexCount)( pref<TextureDesc>* _this, unsigned int value ) {
	_unref(_this)->mSharedNodeIndexCount = (value);
	return value;
}
DEFINE_PRIM(_I32,TextureDesc_set_sharedNodeIndexCount,_IDL _I32);

HL_PRIM unsigned int HL_NAME(TextureDesc_get_nodeIndex)( pref<TextureDesc>* _this ) {
	return _unref(_this)->mNodeIndex;
}
DEFINE_PRIM(_I32,TextureDesc_get_nodeIndex,_IDL);
HL_PRIM unsigned int HL_NAME(TextureDesc_set_nodeIndex)( pref<TextureDesc>* _this, unsigned int value ) {
	_unref(_this)->mNodeIndex = (value);
	return value;
}
DEFINE_PRIM(_I32,TextureDesc_set_nodeIndex,_IDL _I32);

HL_PRIM pref<TextureLoadDesc>* HL_NAME(TextureLoadDesc_new0)() {
	auto ___retvalue = alloc_ref((new TextureLoadDesc()),TextureLoadDesc);
	*(___retvalue->value) = {};
	return (___retvalue);
}
DEFINE_PRIM(_IDL, TextureLoadDesc_new0,);

HL_PRIM HL_CONST pref<Texture>* HL_NAME(TextureLoadDesc_load1)(pref<TextureLoadDesc>* _this, pref<SyncToken>* syncToken) {
	return alloc_ref_const((forge_texture_load( _unref(_this) , _unref_ptr_safe(syncToken))),Texture);
}
DEFINE_PRIM(_IDL, TextureLoadDesc_load1, _IDL _IDL);

HL_PRIM int HL_NAME(TextureLoadDesc_get_creationFlag)( pref<TextureLoadDesc>* _this ) {
	return HL_NAME(TextureCreationFlags_valueToIndex1)(_unref(_this)->mCreationFlag);
}
DEFINE_PRIM(_I32,TextureLoadDesc_get_creationFlag,_IDL);
HL_PRIM int HL_NAME(TextureLoadDesc_set_creationFlag)( pref<TextureLoadDesc>* _this, int value ) {
	_unref(_this)->mCreationFlag = (TextureCreationFlags)HL_NAME(TextureCreationFlags_indexToValue1)(value);
	return value;
}
DEFINE_PRIM(_I32,TextureLoadDesc_set_creationFlag,_IDL _I32);

HL_PRIM vstring * HL_NAME(Tools_glslToNative3)(vstring * source, vstring * path, bool fragmentShader) {
	const char* source__cstr = (source == nullptr) ? "" : hl_to_utf8( source->bytes ); // Should be garbage collected
	const char* path__cstr = (path == nullptr) ? "" : hl_to_utf8( path->bytes ); // Should be garbage collected
	auto ___retvalue = (forge_translate_glsl_native(source__cstr, path__cstr, fragmentShader));
	return hl_utf8_to_hlstr(___retvalue);
}
DEFINE_PRIM(_STRING, Tools_glslToNative3, _STRING _STRING _BOOL);

HL_PRIM void HL_NAME(Tools_nativeToBin2)(vstring * sourcePath, vstring * outpath) {
	const char* sourcePath__cstr = (sourcePath == nullptr) ? "" : hl_to_utf8( sourcePath->bytes ); // Should be garbage collected
	const char* outpath__cstr = (outpath == nullptr) ? "" : hl_to_utf8( outpath->bytes ); // Should be garbage collected
	(hl_compile_native_to_bin(sourcePath__cstr, outpath__cstr));
}
DEFINE_PRIM(_VOID, Tools_nativeToBin2, _STRING _STRING);

HL_PRIM pref<PolyMesh>* HL_NAME(PolyMesh_new0)() {
	return alloc_ref((new PolyMesh()),PolyMesh);
}
DEFINE_PRIM(_IDL, PolyMesh_new0,);

HL_PRIM void HL_NAME(PolyMesh_reserve1)(pref<PolyMesh>* _this, int polynodes) {
	(_unref(_this)->reserve(polynodes));
}
DEFINE_PRIM(_VOID, PolyMesh_reserve1, _IDL _I32);

HL_PRIM int HL_NAME(PolyMesh_addAttribute4)(pref<PolyMesh>* _this, vstring * name, int semantic, int type, int dimensions) {
	const char* name__cstr = (name == nullptr) ? "" : hl_to_utf8( name->bytes ); // Should be garbage collected
	auto ___retvalue = (_unref(_this)->addAttribute(name__cstr, AttributeSemantic__values[semantic], AttributeType__values[type], dimensions));
	return (___retvalue);
}
DEFINE_PRIM(_I32, PolyMesh_addAttribute4, _IDL _STRING _I32 _I32 _I32);

HL_PRIM int HL_NAME(PolyMesh_beginPolygon1)(pref<PolyMesh>* _this, int count) {
	return (_unref(_this)->beginPolygon(count));
}
DEFINE_PRIM(_I32, PolyMesh_beginPolygon1, _IDL _I32);

HL_PRIM int HL_NAME(PolyMesh_beginPolyNode0)(pref<PolyMesh>* _this) {
	return (_unref(_this)->beginPolyNode());
}
DEFINE_PRIM(_I32, PolyMesh_beginPolyNode0, _IDL);

HL_PRIM void HL_NAME(PolyMesh_setNodeAddtribute1f2)(pref<PolyMesh>* _this, int attr, float x) {
	(_unref(_this)->setNodeAddtribute1f(attr, x));
}
DEFINE_PRIM(_VOID, PolyMesh_setNodeAddtribute1f2, _IDL _I32 _F32);

HL_PRIM void HL_NAME(PolyMesh_setNodeAddtribute2f3)(pref<PolyMesh>* _this, int attr, float x, float y) {
	(_unref(_this)->setNodeAddtribute2f(attr, x, y));
}
DEFINE_PRIM(_VOID, PolyMesh_setNodeAddtribute2f3, _IDL _I32 _F32 _F32);

HL_PRIM void HL_NAME(PolyMesh_setNodeAddtribute3f4)(pref<PolyMesh>* _this, int attr, float x, float y, float z) {
	(_unref(_this)->setNodeAddtribute3f(attr, x, y, z));
}
DEFINE_PRIM(_VOID, PolyMesh_setNodeAddtribute3f4, _IDL _I32 _F32 _F32 _F32);

HL_PRIM void HL_NAME(PolyMesh_setNodeAddtribute4f5)(pref<PolyMesh>* _this, int attr, float x, float y, float z, float w) {
	(_unref(_this)->setNodeAddtribute4f(attr, x, y, z, w));
}
DEFINE_PRIM(_VOID, PolyMesh_setNodeAddtribute4f5, _IDL _I32 _F32 _F32 _F32 _F32);

HL_PRIM void HL_NAME(PolyMesh_setNodeAddtribute1d2)(pref<PolyMesh>* _this, int attr, double x) {
	(_unref(_this)->setNodeAddtribute1d(attr, x));
}
DEFINE_PRIM(_VOID, PolyMesh_setNodeAddtribute1d2, _IDL _I32 _F64);

HL_PRIM void HL_NAME(PolyMesh_setNodeAddtribute2d3)(pref<PolyMesh>* _this, int attr, double x, double y) {
	(_unref(_this)->setNodeAddtribute2d(attr, x, y));
}
DEFINE_PRIM(_VOID, PolyMesh_setNodeAddtribute2d3, _IDL _I32 _F64 _F64);

HL_PRIM void HL_NAME(PolyMesh_setNodeAddtribute3d4)(pref<PolyMesh>* _this, int attr, double x, double y, double z) {
	(_unref(_this)->setNodeAddtribute3d(attr, x, y, z));
}
DEFINE_PRIM(_VOID, PolyMesh_setNodeAddtribute3d4, _IDL _I32 _F64 _F64 _F64);

HL_PRIM void HL_NAME(PolyMesh_setNodeAddtribute4d5)(pref<PolyMesh>* _this, int attr, double x, double y, double z, double w) {
	(_unref(_this)->setNodeAddtribute4d(attr, x, y, z, w));
}
DEFINE_PRIM(_VOID, PolyMesh_setNodeAddtribute4d5, _IDL _I32 _F64 _F64 _F64 _F64);

HL_PRIM int HL_NAME(PolyMesh_endPolyNode0)(pref<PolyMesh>* _this) {
	return (_unref(_this)->endPolyNode());
}
DEFINE_PRIM(_I32, PolyMesh_endPolyNode0, _IDL);

HL_PRIM int HL_NAME(PolyMesh_endPolygon0)(pref<PolyMesh>* _this) {
	return (_unref(_this)->endPolygon());
}
DEFINE_PRIM(_I32, PolyMesh_endPolygon0, _IDL);

HL_PRIM int HL_NAME(PolyMesh_numPolyNodes0)(pref<PolyMesh>* _this) {
	return (_unref(_this)->numPolyNodes());
}
DEFINE_PRIM(_I32, PolyMesh_numPolyNodes0, _IDL);

HL_PRIM int HL_NAME(PolyMesh_numVerts0)(pref<PolyMesh>* _this) {
	return (_unref(_this)->numVerts());
}
DEFINE_PRIM(_I32, PolyMesh_numVerts0, _IDL);

HL_PRIM int HL_NAME(PolyMesh_numPolys0)(pref<PolyMesh>* _this) {
	return (_unref(_this)->numPolys());
}
DEFINE_PRIM(_I32, PolyMesh_numPolys0, _IDL);

HL_PRIM int HL_NAME(PolyMesh_getIndices1)(pref<PolyMesh>* _this, vbyte* indices) {
	return (_unref(_this)->getIndices((int*)indices));
}
DEFINE_PRIM(_I32, PolyMesh_getIndices1, _IDL _BYTES);

HL_PRIM int HL_NAME(PolyMesh_getStride0)(pref<PolyMesh>* _this) {
	return (_unref(_this)->getStride());
}
DEFINE_PRIM(_I32, PolyMesh_getStride0, _IDL);

HL_PRIM void HL_NAME(PolyMesh_getInterleavedVertices1)(pref<PolyMesh>* _this, vbyte* data) {
	(_unref(_this)->getInterleavedVertices(data));
}
DEFINE_PRIM(_VOID, PolyMesh_getInterleavedVertices1, _IDL _BYTES);

HL_PRIM int HL_NAME(PolyMesh_numAttributes0)(pref<PolyMesh>* _this) {
	return (_unref(_this)->numAttributes());
}
DEFINE_PRIM(_I32, PolyMesh_numAttributes0, _IDL);

HL_PRIM int HL_NAME(PolyMesh_getAttributeIndexBySemantic1)(pref<PolyMesh>* _this, int semantic) {
	return (_unref(_this)->getAttributeIndexBySemantic(AttributeSemantic__values[semantic]));
}
DEFINE_PRIM(_I32, PolyMesh_getAttributeIndexBySemantic1, _IDL _I32);

HL_PRIM int HL_NAME(PolyMesh_getAttributeOffset1)(pref<PolyMesh>* _this, int index) {
	return (_unref(_this)->getAttributeOffset(index));
}
DEFINE_PRIM(_I32, PolyMesh_getAttributeOffset1, _IDL _I32);

HL_PRIM int HL_NAME(PolyMesh_getAttributeSemantic1)(pref<PolyMesh>* _this, int index) {
	return HL_NAME(AttributeSemantic_valueToIndex1)(_unref(_this)->getAttributeSemantic(index));
}
DEFINE_PRIM(_I32, PolyMesh_getAttributeSemantic1, _IDL _I32);

HL_PRIM int HL_NAME(PolyMesh_getAttributeType1)(pref<PolyMesh>* _this, int index) {
	return HL_NAME(AttributeType_valueToIndex1)(_unref(_this)->getAttributeType(index));
}
DEFINE_PRIM(_I32, PolyMesh_getAttributeType1, _IDL _I32);

HL_PRIM int HL_NAME(PolyMesh_getAttributeDimensions1)(pref<PolyMesh>* _this, int index) {
	return (_unref(_this)->getAttributeDimensions(index));
}
DEFINE_PRIM(_I32, PolyMesh_getAttributeDimensions1, _IDL _I32);

HL_PRIM vstring * HL_NAME(PolyMesh_getAttributeName1)(pref<PolyMesh>* _this, int index) {
	return hl_utf8_to_hlstr(_unref(_this)->getAttributeName(index));
}
DEFINE_PRIM(_STRING, PolyMesh_getAttributeName1, _IDL _I32);

HL_PRIM void HL_NAME(PolyMesh_removeDuplicateVerts0)(pref<PolyMesh>* _this) {
	(_unref(_this)->removeDuplicateVerts());
}
DEFINE_PRIM(_VOID, PolyMesh_removeDuplicateVerts0, _IDL);

HL_PRIM void HL_NAME(PolyMesh_convexTriangulate0)(pref<PolyMesh>* _this) {
	(_unref(_this)->convexTriangulate());
}
DEFINE_PRIM(_VOID, PolyMesh_convexTriangulate0, _IDL);

HL_PRIM void HL_NAME(PolyMesh_optimizeTriangleIndices0)(pref<PolyMesh>* _this) {
	(_unref(_this)->optimizeTriangleIndices());
}
DEFINE_PRIM(_VOID, PolyMesh_optimizeTriangleIndices0, _IDL);

HL_PRIM bool HL_NAME(PolyMesh_optimizeTriangleIndicesForOverdraw0)(pref<PolyMesh>* _this) {
	return (_unref(_this)->optimizeTriangleIndicesForOverdraw());
}
DEFINE_PRIM(_BOOL, PolyMesh_optimizeTriangleIndicesForOverdraw0, _IDL);

HL_PRIM void HL_NAME(PolyMesh_optimizeTriangleMemoryFetch0)(pref<PolyMesh>* _this) {
	(_unref(_this)->optimizeTriangleMemoryFetch());
}
DEFINE_PRIM(_VOID, PolyMesh_optimizeTriangleMemoryFetch0, _IDL);

}
