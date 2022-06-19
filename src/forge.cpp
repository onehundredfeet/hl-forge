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

static SampleCount SampleCount__values[] = { SAMPLE_COUNT_1,SAMPLE_COUNT_2,SAMPLE_COUNT_4,SAMPLE_COUNT_8,SAMPLE_COUNT_16 };
HL_PRIM int HL_NAME(SampleCount_toValue0)( int idx ) {
	return SampleCount__values[idx];
}
DEFINE_PRIM(_I32, SampleCount_toValue0, _I32);
HL_PRIM int HL_NAME(SampleCount_indexToValue0)( int idx ) {
	return SampleCount__values[idx];
}
DEFINE_PRIM(_I32, SampleCount_indexToValue0, _I32);
HL_PRIM int HL_NAME(SampleCount_valueToIndex0)( int value ) {
	for( int i = 0; i < 5; i++ ) if ( value == (int)SampleCount__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, SampleCount_valueToIndex0, _I32);
static ResourceState ResourceState__values[] = { RESOURCE_STATE_UNDEFINED,RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,RESOURCE_STATE_INDEX_BUFFER,RESOURCE_STATE_RENDER_TARGET,RESOURCE_STATE_UNORDERED_ACCESS,RESOURCE_STATE_DEPTH_WRITE,RESOURCE_STATE_DEPTH_READ,RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,RESOURCE_STATE_PIXEL_SHADER_RESOURCE,RESOURCE_STATE_SHADER_RESOURCE,RESOURCE_STATE_STREAM_OUT,RESOURCE_STATE_INDIRECT_ARGUMENT,RESOURCE_STATE_COPY_DEST,RESOURCE_STATE_COPY_SOURCE,RESOURCE_STATE_GENERIC_READ,RESOURCE_STATE_PRESENT,RESOURCE_STATE_COMMON,RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,RESOURCE_STATE_SHADING_RATE_SOURCE };
HL_PRIM int HL_NAME(ResourceState_toValue0)( int idx ) {
	return ResourceState__values[idx];
}
DEFINE_PRIM(_I32, ResourceState_toValue0, _I32);
HL_PRIM int HL_NAME(ResourceState_indexToValue0)( int idx ) {
	return ResourceState__values[idx];
}
DEFINE_PRIM(_I32, ResourceState_indexToValue0, _I32);
HL_PRIM int HL_NAME(ResourceState_valueToIndex0)( int value ) {
	for( int i = 0; i < 19; i++ ) if ( value == (int)ResourceState__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, ResourceState_valueToIndex0, _I32);
static TinyImageFormat TinyImageFormat__values[] = { TinyImageFormat_UNDEFINED,TinyImageFormat_R1_UNORM,TinyImageFormat_R2_UNORM,TinyImageFormat_R4_UNORM,TinyImageFormat_R4G4_UNORM,TinyImageFormat_G4R4_UNORM,TinyImageFormat_A8_UNORM,TinyImageFormat_R8_UNORM,TinyImageFormat_R8_SNORM,TinyImageFormat_R8_UINT,TinyImageFormat_R8_SINT,TinyImageFormat_R8_SRGB,TinyImageFormat_B2G3R3_UNORM,TinyImageFormat_R4G4B4A4_UNORM,TinyImageFormat_R4G4B4X4_UNORM,TinyImageFormat_B4G4R4A4_UNORM,TinyImageFormat_B4G4R4X4_UNORM,TinyImageFormat_A4R4G4B4_UNORM,TinyImageFormat_X4R4G4B4_UNORM,TinyImageFormat_A4B4G4R4_UNORM,TinyImageFormat_X4B4G4R4_UNORM,TinyImageFormat_R5G6B5_UNORM,TinyImageFormat_B5G6R5_UNORM,TinyImageFormat_R5G5B5A1_UNORM,TinyImageFormat_B5G5R5A1_UNORM,TinyImageFormat_A1B5G5R5_UNORM,TinyImageFormat_A1R5G5B5_UNORM,TinyImageFormat_R5G5B5X1_UNORM,TinyImageFormat_B5G5R5X1_UNORM,TinyImageFormat_X1R5G5B5_UNORM,TinyImageFormat_X1B5G5R5_UNORM,TinyImageFormat_B2G3R3A8_UNORM,TinyImageFormat_R8G8_UNORM,TinyImageFormat_R8G8_SNORM,TinyImageFormat_G8R8_UNORM,TinyImageFormat_G8R8_SNORM,TinyImageFormat_R8G8_UINT,TinyImageFormat_R8G8_SINT,TinyImageFormat_R8G8_SRGB,TinyImageFormat_R16_UNORM,TinyImageFormat_R16_SNORM,TinyImageFormat_R16_UINT,TinyImageFormat_R16_SINT,TinyImageFormat_R16_SFLOAT,TinyImageFormat_R16_SBFLOAT,TinyImageFormat_R8G8B8_UNORM,TinyImageFormat_R8G8B8_SNORM,TinyImageFormat_R8G8B8_UINT,TinyImageFormat_R8G8B8_SINT,TinyImageFormat_R8G8B8_SRGB,TinyImageFormat_B8G8R8_UNORM,TinyImageFormat_B8G8R8_SNORM,TinyImageFormat_B8G8R8_UINT,TinyImageFormat_B8G8R8_SINT,TinyImageFormat_B8G8R8_SRGB,TinyImageFormat_R8G8B8A8_UNORM,TinyImageFormat_R8G8B8A8_SNORM,TinyImageFormat_R8G8B8A8_UINT,TinyImageFormat_R8G8B8A8_SINT,TinyImageFormat_R8G8B8A8_SRGB,TinyImageFormat_B8G8R8A8_UNORM,TinyImageFormat_B8G8R8A8_SNORM,TinyImageFormat_B8G8R8A8_UINT,TinyImageFormat_B8G8R8A8_SINT,TinyImageFormat_B8G8R8A8_SRGB,TinyImageFormat_R8G8B8X8_UNORM,TinyImageFormat_B8G8R8X8_UNORM,TinyImageFormat_R16G16_UNORM,TinyImageFormat_G16R16_UNORM,TinyImageFormat_R16G16_SNORM,TinyImageFormat_G16R16_SNORM,TinyImageFormat_R16G16_UINT,TinyImageFormat_R16G16_SINT,TinyImageFormat_R16G16_SFLOAT,TinyImageFormat_R16G16_SBFLOAT,TinyImageFormat_R32_UINT,TinyImageFormat_R32_SINT,TinyImageFormat_R32_SFLOAT,TinyImageFormat_A2R10G10B10_UNORM,TinyImageFormat_A2R10G10B10_UINT,TinyImageFormat_A2R10G10B10_SNORM,TinyImageFormat_A2R10G10B10_SINT,TinyImageFormat_A2B10G10R10_UNORM,TinyImageFormat_A2B10G10R10_UINT,TinyImageFormat_A2B10G10R10_SNORM,TinyImageFormat_A2B10G10R10_SINT,TinyImageFormat_R10G10B10A2_UNORM,TinyImageFormat_R10G10B10A2_UINT,TinyImageFormat_R10G10B10A2_SNORM,TinyImageFormat_R10G10B10A2_SINT,TinyImageFormat_B10G10R10A2_UNORM,TinyImageFormat_B10G10R10A2_UINT,TinyImageFormat_B10G10R10A2_SNORM,TinyImageFormat_B10G10R10A2_SINT,TinyImageFormat_B10G11R11_UFLOAT,TinyImageFormat_E5B9G9R9_UFLOAT,TinyImageFormat_R16G16B16_UNORM,TinyImageFormat_R16G16B16_SNORM,TinyImageFormat_R16G16B16_UINT,TinyImageFormat_R16G16B16_SINT,TinyImageFormat_R16G16B16_SFLOAT,TinyImageFormat_R16G16B16_SBFLOAT,TinyImageFormat_R16G16B16A16_UNORM,TinyImageFormat_R16G16B16A16_SNORM,TinyImageFormat_R16G16B16A16_UINT,TinyImageFormat_R16G16B16A16_SINT,TinyImageFormat_R16G16B16A16_SFLOAT,TinyImageFormat_R16G16B16A16_SBFLOAT,TinyImageFormat_R32G32_UINT,TinyImageFormat_R32G32_SINT,TinyImageFormat_R32G32_SFLOAT,TinyImageFormat_R32G32B32_UINT,TinyImageFormat_R32G32B32_SINT,TinyImageFormat_R32G32B32_SFLOAT,TinyImageFormat_R32G32B32A32_UINT,TinyImageFormat_R32G32B32A32_SINT,TinyImageFormat_R32G32B32A32_SFLOAT,TinyImageFormat_R64_UINT,TinyImageFormat_R64_SINT,TinyImageFormat_R64_SFLOAT,TinyImageFormat_R64G64_UINT,TinyImageFormat_R64G64_SINT,TinyImageFormat_R64G64_SFLOAT,TinyImageFormat_R64G64B64_UINT,TinyImageFormat_R64G64B64_SINT,TinyImageFormat_R64G64B64_SFLOAT,TinyImageFormat_R64G64B64A64_UINT,TinyImageFormat_R64G64B64A64_SINT,TinyImageFormat_R64G64B64A64_SFLOAT,TinyImageFormat_D16_UNORM,TinyImageFormat_X8_D24_UNORM,TinyImageFormat_D32_SFLOAT,TinyImageFormat_S8_UINT,TinyImageFormat_D16_UNORM_S8_UINT,TinyImageFormat_D24_UNORM_S8_UINT,TinyImageFormat_D32_SFLOAT_S8_UINT,TinyImageFormat_DXBC1_RGB_UNORM,TinyImageFormat_DXBC1_RGB_SRGB,TinyImageFormat_DXBC1_RGBA_UNORM,TinyImageFormat_DXBC1_RGBA_SRGB,TinyImageFormat_DXBC2_UNORM,TinyImageFormat_DXBC2_SRGB,TinyImageFormat_DXBC3_UNORM,TinyImageFormat_DXBC3_SRGB,TinyImageFormat_DXBC4_UNORM,TinyImageFormat_DXBC4_SNORM,TinyImageFormat_DXBC5_UNORM,TinyImageFormat_DXBC5_SNORM,TinyImageFormat_DXBC6H_UFLOAT,TinyImageFormat_DXBC6H_SFLOAT,TinyImageFormat_DXBC7_UNORM,TinyImageFormat_DXBC7_SRGB,TinyImageFormat_PVRTC1_2BPP_UNORM,TinyImageFormat_PVRTC1_4BPP_UNORM,TinyImageFormat_PVRTC2_2BPP_UNORM,TinyImageFormat_PVRTC2_4BPP_UNORM,TinyImageFormat_PVRTC1_2BPP_SRGB,TinyImageFormat_PVRTC1_4BPP_SRGB,TinyImageFormat_PVRTC2_2BPP_SRGB,TinyImageFormat_PVRTC2_4BPP_SRGB,TinyImageFormat_ETC2_R8G8B8_UNORM,TinyImageFormat_ETC2_R8G8B8_SRGB,TinyImageFormat_ETC2_R8G8B8A1_UNORM,TinyImageFormat_ETC2_R8G8B8A1_SRGB,TinyImageFormat_ETC2_R8G8B8A8_UNORM,TinyImageFormat_ETC2_R8G8B8A8_SRGB,TinyImageFormat_ETC2_EAC_R11_UNORM,TinyImageFormat_ETC2_EAC_R11_SNORM,TinyImageFormat_ETC2_EAC_R11G11_UNORM,TinyImageFormat_ETC2_EAC_R11G11_SNORM,TinyImageFormat_ASTC_4x4_UNORM,TinyImageFormat_ASTC_4x4_SRGB,TinyImageFormat_ASTC_5x4_UNORM,TinyImageFormat_ASTC_5x4_SRGB,TinyImageFormat_ASTC_5x5_UNORM,TinyImageFormat_ASTC_5x5_SRGB,TinyImageFormat_ASTC_6x5_UNORM,TinyImageFormat_ASTC_6x5_SRGB,TinyImageFormat_ASTC_6x6_UNORM,TinyImageFormat_ASTC_6x6_SRGB,TinyImageFormat_ASTC_8x5_UNORM,TinyImageFormat_ASTC_8x5_SRGB,TinyImageFormat_ASTC_8x6_UNORM,TinyImageFormat_ASTC_8x6_SRGB,TinyImageFormat_ASTC_8x8_UNORM,TinyImageFormat_ASTC_8x8_SRGB,TinyImageFormat_ASTC_10x5_UNORM,TinyImageFormat_ASTC_10x5_SRGB,TinyImageFormat_ASTC_10x6_UNORM,TinyImageFormat_ASTC_10x6_SRGB,TinyImageFormat_ASTC_10x8_UNORM,TinyImageFormat_ASTC_10x8_SRGB,TinyImageFormat_ASTC_10x10_UNORM,TinyImageFormat_ASTC_10x10_SRGB,TinyImageFormat_ASTC_12x10_UNORM,TinyImageFormat_ASTC_12x10_SRGB,TinyImageFormat_ASTC_12x12_UNORM,TinyImageFormat_ASTC_12x12_SRGB,TinyImageFormat_CLUT_P4,TinyImageFormat_CLUT_P4A4,TinyImageFormat_CLUT_P8,TinyImageFormat_CLUT_P8A8,TinyImageFormat_R4G4B4A4_UNORM_PACK16,TinyImageFormat_B4G4R4A4_UNORM_PACK16,TinyImageFormat_R5G6B5_UNORM_PACK16,TinyImageFormat_B5G6R5_UNORM_PACK16,TinyImageFormat_R5G5B5A1_UNORM_PACK16,TinyImageFormat_B5G5R5A1_UNORM_PACK16,TinyImageFormat_A1R5G5B5_UNORM_PACK16,TinyImageFormat_G16B16G16R16_422_UNORM,TinyImageFormat_B16G16R16G16_422_UNORM,TinyImageFormat_R12X4G12X4B12X4A12X4_UNORM_4PACK16,TinyImageFormat_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16,TinyImageFormat_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16,TinyImageFormat_R10X6G10X6B10X6A10X6_UNORM_4PACK16,TinyImageFormat_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16,TinyImageFormat_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16,TinyImageFormat_G8B8G8R8_422_UNORM,TinyImageFormat_B8G8R8G8_422_UNORM,TinyImageFormat_G8_B8_R8_3PLANE_420_UNORM,TinyImageFormat_G8_B8R8_2PLANE_420_UNORM,TinyImageFormat_G8_B8_R8_3PLANE_422_UNORM,TinyImageFormat_G8_B8R8_2PLANE_422_UNORM,TinyImageFormat_G8_B8_R8_3PLANE_444_UNORM,TinyImageFormat_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16,TinyImageFormat_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16,TinyImageFormat_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16,TinyImageFormat_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16,TinyImageFormat_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16,TinyImageFormat_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16,TinyImageFormat_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16,TinyImageFormat_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16,TinyImageFormat_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16,TinyImageFormat_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16,TinyImageFormat_G16_B16_R16_3PLANE_420_UNORM,TinyImageFormat_G16_B16_R16_3PLANE_422_UNORM,TinyImageFormat_G16_B16_R16_3PLANE_444_UNORM,TinyImageFormat_G16_B16R16_2PLANE_420_UNORM,TinyImageFormat_G16_B16R16_2PLANE_422_UNORM };
HL_PRIM int HL_NAME(TinyImageFormat_toValue0)( int idx ) {
	return TinyImageFormat__values[idx];
}
DEFINE_PRIM(_I32, TinyImageFormat_toValue0, _I32);
HL_PRIM int HL_NAME(TinyImageFormat_indexToValue0)( int idx ) {
	return TinyImageFormat__values[idx];
}
DEFINE_PRIM(_I32, TinyImageFormat_indexToValue0, _I32);
HL_PRIM int HL_NAME(TinyImageFormat_valueToIndex0)( int value ) {
	for( int i = 0; i < 239; i++ ) if ( value == (int)TinyImageFormat__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, TinyImageFormat_valueToIndex0, _I32);
static TextureCreationFlags TextureCreationFlags__values[] = { TEXTURE_CREATION_FLAG_NONE,TEXTURE_CREATION_FLAG_OWN_MEMORY_BIT,TEXTURE_CREATION_FLAG_EXPORT_BIT,TEXTURE_CREATION_FLAG_EXPORT_ADAPTER_BIT,TEXTURE_CREATION_FLAG_IMPORT_BIT,TEXTURE_CREATION_FLAG_ESRAM,TEXTURE_CREATION_FLAG_ON_TILE,TEXTURE_CREATION_FLAG_NO_COMPRESSION,TEXTURE_CREATION_FLAG_FORCE_2D,TEXTURE_CREATION_FLAG_FORCE_3D,TEXTURE_CREATION_FLAG_ALLOW_DISPLAY_TARGET,TEXTURE_CREATION_FLAG_SRGB,TEXTURE_CREATION_FLAG_NORMAL_MAP,TEXTURE_CREATION_FLAG_FAST_CLEAR,TEXTURE_CREATION_FLAG_FRAG_MASK,TEXTURE_CREATION_FLAG_VR_MULTIVIEW,TEXTURE_CREATION_FLAG_VR_FOVEATED_RENDERING };
HL_PRIM int HL_NAME(TextureCreationFlags_toValue0)( int idx ) {
	return TextureCreationFlags__values[idx];
}
DEFINE_PRIM(_I32, TextureCreationFlags_toValue0, _I32);
HL_PRIM int HL_NAME(TextureCreationFlags_indexToValue0)( int idx ) {
	return TextureCreationFlags__values[idx];
}
DEFINE_PRIM(_I32, TextureCreationFlags_indexToValue0, _I32);
HL_PRIM int HL_NAME(TextureCreationFlags_valueToIndex0)( int value ) {
	for( int i = 0; i < 17; i++ ) if ( value == (int)TextureCreationFlags__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, TextureCreationFlags_valueToIndex0, _I32);
static void finalize_RenderTargetDesc( _ref(RenderTargetDesc)* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(RenderTargetDesc_delete)( _ref(RenderTargetDesc)* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, RenderTargetDesc_delete, _IDL);
static void finalize_RootSignatureDesc( _ref(RootSignatureFactory)* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(RootSignatureDesc_delete)( _ref(RootSignatureFactory)* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, RootSignatureDesc_delete, _IDL);
static void finalize_ForgeSDLWindow( _ref(ForgeSDLWindow)* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(ForgeSDLWindow_delete)( _ref(ForgeSDLWindow)* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, ForgeSDLWindow_delete, _IDL);
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
static void finalize_TextureDesc( _ref(TextureDesc)* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(TextureDesc_delete)( _ref(TextureDesc)* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, TextureDesc_delete, _IDL);
static void finalize_TextureLoadDesc( _ref(TextureLoadDesc)* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(TextureLoadDesc_delete)( _ref(TextureLoadDesc)* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, TextureLoadDesc_delete, _IDL);
HL_PRIM bool HL_NAME(Globals_initialize1)(vstring * name) {
	const char* name__cstr = (name == nullptr) ? "" : hl_to_utf8( name->bytes ); // Should be garbage collected
	auto ___retvalue = (hlForgeInitialize(name__cstr));
	return ___retvalue;
}
DEFINE_PRIM(_BOOL, Globals_initialize1, _STRING);

HL_PRIM void HL_NAME(Globals_waitForAllResourceLoads0)() {
	(waitForAllResourceLoads());
}
DEFINE_PRIM(_VOID, Globals_waitForAllResourceLoads0,);

HL_PRIM void HL_NAME(Cmd_clear2)(_ref(Cmd)* _this, _ref(RenderTarget)* rt, _ref(RenderTarget)* depthstencil) {
	(forge_render_target_clear( _unref(_this) , _unref_ptr_safe(rt), _unref_ptr_safe(depthstencil)));
}
DEFINE_PRIM(_VOID, Cmd_clear2, _IDL _IDL _IDL);

HL_PRIM void HL_NAME(Cmd_begin0)(_ref(Cmd)* _this) {
	(beginCmd( _unref(_this) ));
}
DEFINE_PRIM(_VOID, Cmd_begin0, _IDL);

HL_PRIM void HL_NAME(Cmd_end0)(_ref(Cmd)* _this) {
	(endCmd( _unref(_this) ));
}
DEFINE_PRIM(_VOID, Cmd_end0, _IDL);

HL_PRIM void HL_NAME(Queue_waitIdle0)(_ref(Queue)* _this) {
	(waitQueueIdle( _unref(_this) ));
}
DEFINE_PRIM(_VOID, Queue_waitIdle0, _IDL);

HL_PRIM void HL_NAME(Queue_submit4)(_ref(Queue)* _this, _ref(Cmd)* cmd, _ref(Semaphore)* signalSemphor, _ref(Semaphore)* wait, _ref(Fence)* signalFence) {
	(forge_queue_submit_cmd( _unref(_this) , _unref_ptr_safe(cmd), _unref_ptr_safe(signalSemphor), _unref_ptr_safe(wait), _unref_ptr_safe(signalFence)));
}
DEFINE_PRIM(_VOID, Queue_submit4, _IDL _IDL _IDL _IDL _IDL);

HL_PRIM HL_CONST _ref(RenderTarget)* HL_NAME(SwapChain_getRenderTarget1)(_ref(SwapChain)* _this, int rtidx) {
	return alloc_ref_const((forge_swap_chain_get_render_target( _unref(_this) , rtidx)),RenderTarget);
}
DEFINE_PRIM(_IDL, SwapChain_getRenderTarget1, _IDL _I32);

HL_PRIM _ref(RenderTargetDesc)* HL_NAME(RenderTargetDesc_new0)() {
	auto ___retvalue = alloc_ref((new RenderTargetDesc()),RenderTargetDesc);
	*(___retvalue->value) = {};
	return ___retvalue;
}
DEFINE_PRIM(_IDL, RenderTargetDesc_new0,);

HL_PRIM unsigned int HL_NAME(RenderTargetDesc_get_flags)( _ref(RenderTargetDesc)* _this ) {
	return _unref(_this)->mFlags;
}
DEFINE_PRIM(_I32,RenderTargetDesc_get_flags,_IDL);
HL_PRIM unsigned int HL_NAME(RenderTargetDesc_set_flags)( _ref(RenderTargetDesc)* _this, unsigned int value ) {
	_unref(_this)->mFlags = (TextureCreationFlags)(value);
	return value;
}
DEFINE_PRIM(_I32,RenderTargetDesc_set_flags,_IDL _I32);

HL_PRIM unsigned int HL_NAME(RenderTargetDesc_get_width)( _ref(RenderTargetDesc)* _this ) {
	return _unref(_this)->mWidth;
}
DEFINE_PRIM(_I32,RenderTargetDesc_get_width,_IDL);
HL_PRIM unsigned int HL_NAME(RenderTargetDesc_set_width)( _ref(RenderTargetDesc)* _this, unsigned int value ) {
	_unref(_this)->mWidth = (value);
	return value;
}
DEFINE_PRIM(_I32,RenderTargetDesc_set_width,_IDL _I32);

HL_PRIM unsigned int HL_NAME(RenderTargetDesc_get_height)( _ref(RenderTargetDesc)* _this ) {
	return _unref(_this)->mHeight;
}
DEFINE_PRIM(_I32,RenderTargetDesc_get_height,_IDL);
HL_PRIM unsigned int HL_NAME(RenderTargetDesc_set_height)( _ref(RenderTargetDesc)* _this, unsigned int value ) {
	_unref(_this)->mHeight = (value);
	return value;
}
DEFINE_PRIM(_I32,RenderTargetDesc_set_height,_IDL _I32);

HL_PRIM unsigned int HL_NAME(RenderTargetDesc_get_depth)( _ref(RenderTargetDesc)* _this ) {
	return _unref(_this)->mDepth;
}
DEFINE_PRIM(_I32,RenderTargetDesc_get_depth,_IDL);
HL_PRIM unsigned int HL_NAME(RenderTargetDesc_set_depth)( _ref(RenderTargetDesc)* _this, unsigned int value ) {
	_unref(_this)->mDepth = (value);
	return value;
}
DEFINE_PRIM(_I32,RenderTargetDesc_set_depth,_IDL _I32);

HL_PRIM unsigned int HL_NAME(RenderTargetDesc_get_arraySize)( _ref(RenderTargetDesc)* _this ) {
	return _unref(_this)->mArraySize;
}
DEFINE_PRIM(_I32,RenderTargetDesc_get_arraySize,_IDL);
HL_PRIM unsigned int HL_NAME(RenderTargetDesc_set_arraySize)( _ref(RenderTargetDesc)* _this, unsigned int value ) {
	_unref(_this)->mArraySize = (value);
	return value;
}
DEFINE_PRIM(_I32,RenderTargetDesc_set_arraySize,_IDL _I32);

HL_PRIM unsigned int HL_NAME(RenderTargetDesc_get_mipLevels)( _ref(RenderTargetDesc)* _this ) {
	return _unref(_this)->mMipLevels;
}
DEFINE_PRIM(_I32,RenderTargetDesc_get_mipLevels,_IDL);
HL_PRIM unsigned int HL_NAME(RenderTargetDesc_set_mipLevels)( _ref(RenderTargetDesc)* _this, unsigned int value ) {
	_unref(_this)->mMipLevels = (value);
	return value;
}
DEFINE_PRIM(_I32,RenderTargetDesc_set_mipLevels,_IDL _I32);

HL_PRIM int HL_NAME(RenderTargetDesc_get_sampleCount)( _ref(RenderTargetDesc)* _this ) {
	return HL_NAME(SampleCount_valueToIndex0)(_unref(_this)->mSampleCount);
}
DEFINE_PRIM(_I32,RenderTargetDesc_get_sampleCount,_IDL);
HL_PRIM int HL_NAME(RenderTargetDesc_set_sampleCount)( _ref(RenderTargetDesc)* _this, int value ) {
	_unref(_this)->mSampleCount = (SampleCount)HL_NAME(SampleCount_indexToValue0)(value);
	return value;
}
DEFINE_PRIM(_I32,RenderTargetDesc_set_sampleCount,_IDL _I32);

HL_PRIM int HL_NAME(RenderTargetDesc_get_startState)( _ref(RenderTargetDesc)* _this ) {
	return HL_NAME(ResourceState_valueToIndex0)(_unref(_this)->mStartState);
}
DEFINE_PRIM(_I32,RenderTargetDesc_get_startState,_IDL);
HL_PRIM int HL_NAME(RenderTargetDesc_set_startState)( _ref(RenderTargetDesc)* _this, int value ) {
	_unref(_this)->mStartState = (ResourceState)HL_NAME(ResourceState_indexToValue0)(value);
	return value;
}
DEFINE_PRIM(_I32,RenderTargetDesc_set_startState,_IDL _I32);

HL_PRIM int HL_NAME(RenderTargetDesc_get_format)( _ref(RenderTargetDesc)* _this ) {
	return HL_NAME(TinyImageFormat_valueToIndex0)(_unref(_this)->mFormat);
}
DEFINE_PRIM(_I32,RenderTargetDesc_get_format,_IDL);
HL_PRIM int HL_NAME(RenderTargetDesc_set_format)( _ref(RenderTargetDesc)* _this, int value ) {
	_unref(_this)->mFormat = (TinyImageFormat)HL_NAME(TinyImageFormat_indexToValue0)(value);
	return value;
}
DEFINE_PRIM(_I32,RenderTargetDesc_set_format,_IDL _I32);

HL_PRIM unsigned int HL_NAME(RenderTargetDesc_get_sampleQuality)( _ref(RenderTargetDesc)* _this ) {
	return _unref(_this)->mSampleQuality;
}
DEFINE_PRIM(_I32,RenderTargetDesc_get_sampleQuality,_IDL);
HL_PRIM unsigned int HL_NAME(RenderTargetDesc_set_sampleQuality)( _ref(RenderTargetDesc)* _this, unsigned int value ) {
	_unref(_this)->mSampleQuality = (value);
	return value;
}
DEFINE_PRIM(_I32,RenderTargetDesc_set_sampleQuality,_IDL _I32);

HL_PRIM unsigned int HL_NAME(RenderTargetDesc_get_mSharedNodeIndexCount)( _ref(RenderTargetDesc)* _this ) {
	return _unref(_this)->mSharedNodeIndexCount;
}
DEFINE_PRIM(_I32,RenderTargetDesc_get_mSharedNodeIndexCount,_IDL);
HL_PRIM unsigned int HL_NAME(RenderTargetDesc_set_mSharedNodeIndexCount)( _ref(RenderTargetDesc)* _this, unsigned int value ) {
	_unref(_this)->mSharedNodeIndexCount = (value);
	return value;
}
DEFINE_PRIM(_I32,RenderTargetDesc_set_mSharedNodeIndexCount,_IDL _I32);

HL_PRIM unsigned int HL_NAME(RenderTargetDesc_get_nodeIndex)( _ref(RenderTargetDesc)* _this ) {
	return _unref(_this)->mNodeIndex;
}
DEFINE_PRIM(_I32,RenderTargetDesc_get_nodeIndex,_IDL);
HL_PRIM unsigned int HL_NAME(RenderTargetDesc_set_nodeIndex)( _ref(RenderTargetDesc)* _this, unsigned int value ) {
	_unref(_this)->mNodeIndex = (value);
	return value;
}
DEFINE_PRIM(_I32,RenderTargetDesc_set_nodeIndex,_IDL _I32);

HL_PRIM _ref(RootSignatureFactory)* HL_NAME(RootSignatureDesc_new0)() {
	return alloc_ref((new RootSignatureFactory()),RootSignatureDesc);
}
DEFINE_PRIM(_IDL, RootSignatureDesc_new0,);

HL_PRIM void HL_NAME(RootSignatureDesc_addShader1)(_ref(RootSignatureFactory)* _this, _ref(Shader)* shader) {
	(_unref(_this)->addShader(_unref_ptr_safe(shader)));
}
DEFINE_PRIM(_VOID, RootSignatureDesc_addShader1, _IDL _IDL);

HL_PRIM void HL_NAME(RootSignatureDesc_addSampler1)(_ref(RootSignatureFactory)* _this, _ref(Sampler)* sampler) {
	(_unref(_this)->addSampler(_unref_ptr_safe(sampler)));
}
DEFINE_PRIM(_VOID, RootSignatureDesc_addSampler1, _IDL _IDL);

HL_PRIM HL_CONST _ref(Queue)* HL_NAME(Renderer_createQueue0)(_ref(Renderer)* _this) {
	return alloc_ref_const((createQueue( _unref(_this) )),Queue);
}
DEFINE_PRIM(_IDL, Renderer_createQueue0, _IDL);

HL_PRIM void HL_NAME(Renderer_removeQueue1)(_ref(Renderer)* _this, _ref(Queue)* pGraphicsQueue) {
	(removeQueue( _unref(_this) , _unref_ptr_safe(pGraphicsQueue)));
}
DEFINE_PRIM(_VOID, Renderer_removeQueue1, _IDL _IDL);

HL_PRIM HL_CONST _ref(RenderTarget)* HL_NAME(Renderer_createRenderTarget1)(_ref(Renderer)* _this, _ref(RenderTargetDesc)* desc) {
	return alloc_ref_const((forge_sdl_create_render_target( _unref(_this) , _unref_ptr_safe(desc))),RenderTarget);
}
DEFINE_PRIM(_IDL, Renderer_createRenderTarget1, _IDL _IDL);

HL_PRIM void HL_NAME(Renderer_initResourceLoaderInterface0)(_ref(Renderer)* _this) {
	(forge_init_loader( _unref(_this) ));
}
DEFINE_PRIM(_VOID, Renderer_initResourceLoaderInterface0, _IDL);

HL_PRIM HL_CONST _ref(CmdPool)* HL_NAME(Renderer_createCommandPool1)(_ref(Renderer)* _this, _ref(Queue)* queue) {
	return alloc_ref_const((forge_sdl_renderer_create_cmd_pool( _unref(_this) , _unref_ptr_safe(queue))),CmdPool);
}
DEFINE_PRIM(_IDL, Renderer_createCommandPool1, _IDL _IDL);

HL_PRIM HL_CONST _ref(Cmd)* HL_NAME(Renderer_createCommand1)(_ref(Renderer)* _this, _ref(CmdPool)* pool) {
	return alloc_ref_const((forge_sdl_renderer_create_cmd( _unref(_this) , _unref_ptr_safe(pool))),Cmd);
}
DEFINE_PRIM(_IDL, Renderer_createCommand1, _IDL _IDL);

HL_PRIM HL_CONST _ref(Fence)* HL_NAME(Renderer_createFence0)(_ref(Renderer)* _this) {
	return alloc_ref_const((forge_sdl_renderer_create_fence( _unref(_this) )),Fence);
}
DEFINE_PRIM(_IDL, Renderer_createFence0, _IDL);

HL_PRIM HL_CONST _ref(Semaphore)* HL_NAME(Renderer_createSemaphore0)(_ref(Renderer)* _this) {
	return alloc_ref_const((forge_sdl_renderer_create_semaphore( _unref(_this) )),Semaphore);
}
DEFINE_PRIM(_IDL, Renderer_createSemaphore0, _IDL);

HL_PRIM unsigned int HL_NAME(Renderer_acquireNextImage4)(_ref(Renderer)* _this, _ref(SwapChain)* pSwapChain, _ref(Semaphore)* pImageAcquiredSemaphore, _ref(Fence)* fence) {
	unsigned int __tmpret;
acquireNextImage( _unref(_this) , _unref_ptr_safe(pSwapChain), _unref_ptr_safe(pImageAcquiredSemaphore), _unref_ptr_safe(fence), &__tmpret);
	return __tmpret;
}
DEFINE_PRIM(_I32, Renderer_acquireNextImage4, _IDL _IDL _IDL _IDL);

HL_PRIM void HL_NAME(Renderer_waitFence1)(_ref(Renderer)* _this, _ref(Fence)* fence) {
	(forge_renderer_wait_fence( _unref(_this) , _unref_ptr_safe(fence)));
}
DEFINE_PRIM(_VOID, Renderer_waitFence1, _IDL _IDL);

HL_PRIM HL_CONST _ref(Shader)* HL_NAME(Renderer_createShader2)(_ref(Renderer)* _this, vstring * vertShaderPath, vstring * fragShaderPath) {
	const char* vertShaderPath__cstr = (vertShaderPath == nullptr) ? "" : hl_to_utf8( vertShaderPath->bytes ); // Should be garbage collected
	const char* fragShaderPath__cstr = (fragShaderPath == nullptr) ? "" : hl_to_utf8( fragShaderPath->bytes ); // Should be garbage collected
	auto ___retvalue = alloc_ref_const((forge_renderer_shader_create( _unref(_this) , vertShaderPath__cstr, fragShaderPath__cstr)),Shader);
	return ___retvalue;
}
DEFINE_PRIM(_IDL, Renderer_createShader2, _IDL _STRING _STRING);

HL_PRIM void HL_NAME(Renderer_resetCmdPool1)(_ref(Renderer)* _this, _ref(CmdPool)* pool) {
	(resetCmdPool( _unref(_this) , _unref_ptr_safe(pool)));
}
DEFINE_PRIM(_VOID, Renderer_resetCmdPool1, _IDL _IDL);

HL_PRIM _ref(Pipeline)* HL_NAME(Renderer_createPipeline2)(_ref(Renderer)* _this, _ref(PipelineDesc)* desc) {
	Pipeline* __tmpret;
addPipeline( _unref(_this) , _unref_ptr_safe(desc), &__tmpret);
	return alloc_ref_const( __tmpret, Pipeline );
}
DEFINE_PRIM(_IDL, Renderer_createPipeline2, _IDL _IDL);

HL_PRIM HL_CONST _ref(RootSignature)* HL_NAME(Renderer_createRootSigSimple1)(_ref(Renderer)* _this, _ref(Shader)* shader) {
	return alloc_ref_const((forge_renderer_createRootSignatureSimple( _unref(_this) , _unref_ptr_safe(shader))),RootSignature);
}
DEFINE_PRIM(_IDL, Renderer_createRootSigSimple1, _IDL _IDL);

HL_PRIM HL_CONST _ref(RootSignature)* HL_NAME(Renderer_createRootSig1)(_ref(Renderer)* _this, _ref(RootSignatureFactory)* desc) {
	return alloc_ref_const((forge_renderer_createRootSignature( _unref(_this) , _unref_ptr_safe(desc))),RootSignature);
}
DEFINE_PRIM(_IDL, Renderer_createRootSig1, _IDL _IDL);

HL_PRIM _ref(ForgeSDLWindow)* HL_NAME(ForgeSDLWindow_new1)(_ref(SDL_Window)* sdlWindow) {
	return alloc_ref((new ForgeSDLWindow(_unref_ptr_safe(sdlWindow))),ForgeSDLWindow);
}
DEFINE_PRIM(_IDL, ForgeSDLWindow_new1, _IDL);

HL_PRIM HL_CONST _ref(SwapChain)* HL_NAME(ForgeSDLWindow_createSwapChain6)(_ref(ForgeSDLWindow)* _this, _ref(Renderer)* renderer, _ref(Queue)* queue, int width, int height, int count, bool hdr10) {
	return alloc_ref_const((_unref(_this)->createSwapChain(_unref_ptr_safe(renderer), _unref_ptr_safe(queue), width, height, count, hdr10)),SwapChain);
}
DEFINE_PRIM(_IDL, ForgeSDLWindow_createSwapChain6, _IDL _IDL _IDL _I32 _I32 _I32 _BOOL);

HL_PRIM HL_CONST _ref(Renderer)* HL_NAME(ForgeSDLWindow_renderer0)(_ref(ForgeSDLWindow)* _this) {
	return alloc_ref_const((_unref(_this)->renderer()),Renderer);
}
DEFINE_PRIM(_IDL, ForgeSDLWindow_renderer0, _IDL);

HL_PRIM void HL_NAME(ForgeSDLWindow_present4)(_ref(ForgeSDLWindow)* _this, _ref(Queue)* pGraphicsQueue, _ref(SwapChain)* pSwapChain, int swapchainImageIndex, _ref(Semaphore)* pRenderCompleteSemaphore) {
	(_unref(_this)->present(_unref_ptr_safe(pGraphicsQueue), _unref_ptr_safe(pSwapChain), swapchainImageIndex, _unref_ptr_safe(pRenderCompleteSemaphore)));
}
DEFINE_PRIM(_VOID, ForgeSDLWindow_present4, _IDL _IDL _IDL _I32 _IDL);

HL_PRIM void HL_NAME(Buffer_updateRegion4)(_ref(Buffer)* _this, vbyte* data, int toffset, int size, int soffset) {
	(forge_sdl_buffer_update_region( _unref(_this) , data, toffset, size, soffset));
}
DEFINE_PRIM(_VOID, Buffer_updateRegion4, _IDL _BYTES _I32 _I32 _I32);

HL_PRIM void HL_NAME(Buffer_update1)(_ref(Buffer)* _this, vbyte* data) {
	(forge_sdl_buffer_update( _unref(_this) , data));
}
DEFINE_PRIM(_VOID, Buffer_update1, _IDL _BYTES);

HL_PRIM _ref(BufferLoadDesc)* HL_NAME(BufferLoadDesc_new0)() {
	auto ___retvalue = alloc_ref((new BufferLoadDesc()),BufferLoadDesc);
	*(___retvalue->value) = {};
	return ___retvalue;
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

HL_PRIM void HL_NAME(BufferLoadDesc_setVertexbuffer3)(_ref(BufferLoadDesc)* _this, int size, vbyte* data, bool shared) {
	(forge_sdl_buffer_load_desc_set_vertex_buffer( _unref(_this) , size, data, shared));
}
DEFINE_PRIM(_VOID, BufferLoadDesc_setVertexbuffer3, _IDL _I32 _BYTES _BOOL);

HL_PRIM void HL_NAME(Texture_upload2)(_ref(Texture)* _this, vbyte* data, int size) {
	(forge_sdl_texture_upload( _unref(_this) , data, size));
}
DEFINE_PRIM(_VOID, Texture_upload2, _IDL _BYTES _I32);

HL_PRIM _ref(TextureDesc)* HL_NAME(TextureDesc_new0)() {
	auto ___retvalue = alloc_ref((new TextureDesc()),TextureDesc);
	*(___retvalue->value) = {};
	return ___retvalue;
}
DEFINE_PRIM(_IDL, TextureDesc_new0,);

HL_PRIM HL_CONST _ref(Texture)* HL_NAME(TextureDesc_load2)(_ref(TextureDesc)* _this, vstring * name, _ref(SyncToken)* syncToken) {
	const char* name__cstr = (name == nullptr) ? "" : hl_to_utf8( name->bytes ); // Should be garbage collected
	auto ___retvalue = alloc_ref_const((forge_texture_load_from_desc( _unref(_this) , name__cstr, _unref_ptr_safe(syncToken))),Texture);
	return ___retvalue;
}
DEFINE_PRIM(_IDL, TextureDesc_load2, _IDL _STRING _IDL);

HL_PRIM int HL_NAME(TextureDesc_get_flags)( _ref(TextureDesc)* _this ) {
	return HL_NAME(TextureCreationFlags_valueToIndex0)(_unref(_this)->mFlags);
}
DEFINE_PRIM(_I32,TextureDesc_get_flags,_IDL);
HL_PRIM int HL_NAME(TextureDesc_set_flags)( _ref(TextureDesc)* _this, int value ) {
	_unref(_this)->mFlags = (TextureCreationFlags)HL_NAME(TextureCreationFlags_indexToValue0)(value);
	return value;
}
DEFINE_PRIM(_I32,TextureDesc_set_flags,_IDL _I32);

HL_PRIM unsigned int HL_NAME(TextureDesc_get_width)( _ref(TextureDesc)* _this ) {
	return _unref(_this)->mWidth;
}
DEFINE_PRIM(_I32,TextureDesc_get_width,_IDL);
HL_PRIM unsigned int HL_NAME(TextureDesc_set_width)( _ref(TextureDesc)* _this, unsigned int value ) {
	_unref(_this)->mWidth = (value);
	return value;
}
DEFINE_PRIM(_I32,TextureDesc_set_width,_IDL _I32);

HL_PRIM unsigned int HL_NAME(TextureDesc_get_height)( _ref(TextureDesc)* _this ) {
	return _unref(_this)->mHeight;
}
DEFINE_PRIM(_I32,TextureDesc_get_height,_IDL);
HL_PRIM unsigned int HL_NAME(TextureDesc_set_height)( _ref(TextureDesc)* _this, unsigned int value ) {
	_unref(_this)->mHeight = (value);
	return value;
}
DEFINE_PRIM(_I32,TextureDesc_set_height,_IDL _I32);

HL_PRIM unsigned int HL_NAME(TextureDesc_get_depth)( _ref(TextureDesc)* _this ) {
	return _unref(_this)->mDepth;
}
DEFINE_PRIM(_I32,TextureDesc_get_depth,_IDL);
HL_PRIM unsigned int HL_NAME(TextureDesc_set_depth)( _ref(TextureDesc)* _this, unsigned int value ) {
	_unref(_this)->mDepth = (value);
	return value;
}
DEFINE_PRIM(_I32,TextureDesc_set_depth,_IDL _I32);

HL_PRIM unsigned int HL_NAME(TextureDesc_get_arraySize)( _ref(TextureDesc)* _this ) {
	return _unref(_this)->mArraySize;
}
DEFINE_PRIM(_I32,TextureDesc_get_arraySize,_IDL);
HL_PRIM unsigned int HL_NAME(TextureDesc_set_arraySize)( _ref(TextureDesc)* _this, unsigned int value ) {
	_unref(_this)->mArraySize = (value);
	return value;
}
DEFINE_PRIM(_I32,TextureDesc_set_arraySize,_IDL _I32);

HL_PRIM unsigned int HL_NAME(TextureDesc_get_mipLevels)( _ref(TextureDesc)* _this ) {
	return _unref(_this)->mMipLevels;
}
DEFINE_PRIM(_I32,TextureDesc_get_mipLevels,_IDL);
HL_PRIM unsigned int HL_NAME(TextureDesc_set_mipLevels)( _ref(TextureDesc)* _this, unsigned int value ) {
	_unref(_this)->mMipLevels = (value);
	return value;
}
DEFINE_PRIM(_I32,TextureDesc_set_mipLevels,_IDL _I32);

HL_PRIM int HL_NAME(TextureDesc_get_sampleCount)( _ref(TextureDesc)* _this ) {
	return HL_NAME(SampleCount_valueToIndex0)(_unref(_this)->mSampleCount);
}
DEFINE_PRIM(_I32,TextureDesc_get_sampleCount,_IDL);
HL_PRIM int HL_NAME(TextureDesc_set_sampleCount)( _ref(TextureDesc)* _this, int value ) {
	_unref(_this)->mSampleCount = (SampleCount)HL_NAME(SampleCount_indexToValue0)(value);
	return value;
}
DEFINE_PRIM(_I32,TextureDesc_set_sampleCount,_IDL _I32);

HL_PRIM unsigned int HL_NAME(TextureDesc_get_sampleQuality)( _ref(TextureDesc)* _this ) {
	return _unref(_this)->mSampleQuality;
}
DEFINE_PRIM(_I32,TextureDesc_get_sampleQuality,_IDL);
HL_PRIM unsigned int HL_NAME(TextureDesc_set_sampleQuality)( _ref(TextureDesc)* _this, unsigned int value ) {
	_unref(_this)->mSampleQuality = (value);
	return value;
}
DEFINE_PRIM(_I32,TextureDesc_set_sampleQuality,_IDL _I32);

HL_PRIM int HL_NAME(TextureDesc_get_format)( _ref(TextureDesc)* _this ) {
	return HL_NAME(TinyImageFormat_valueToIndex0)(_unref(_this)->mFormat);
}
DEFINE_PRIM(_I32,TextureDesc_get_format,_IDL);
HL_PRIM int HL_NAME(TextureDesc_set_format)( _ref(TextureDesc)* _this, int value ) {
	_unref(_this)->mFormat = (TinyImageFormat)HL_NAME(TinyImageFormat_indexToValue0)(value);
	return value;
}
DEFINE_PRIM(_I32,TextureDesc_set_format,_IDL _I32);

HL_PRIM int HL_NAME(TextureDesc_get_startState)( _ref(TextureDesc)* _this ) {
	return HL_NAME(ResourceState_valueToIndex0)(_unref(_this)->mStartState);
}
DEFINE_PRIM(_I32,TextureDesc_get_startState,_IDL);
HL_PRIM int HL_NAME(TextureDesc_set_startState)( _ref(TextureDesc)* _this, int value ) {
	_unref(_this)->mStartState = (ResourceState)HL_NAME(ResourceState_indexToValue0)(value);
	return value;
}
DEFINE_PRIM(_I32,TextureDesc_set_startState,_IDL _I32);

HL_PRIM unsigned int HL_NAME(TextureDesc_get_sharedNodeIndexCount)( _ref(TextureDesc)* _this ) {
	return _unref(_this)->mSharedNodeIndexCount;
}
DEFINE_PRIM(_I32,TextureDesc_get_sharedNodeIndexCount,_IDL);
HL_PRIM unsigned int HL_NAME(TextureDesc_set_sharedNodeIndexCount)( _ref(TextureDesc)* _this, unsigned int value ) {
	_unref(_this)->mSharedNodeIndexCount = (value);
	return value;
}
DEFINE_PRIM(_I32,TextureDesc_set_sharedNodeIndexCount,_IDL _I32);

HL_PRIM unsigned int HL_NAME(TextureDesc_get_nodeIndex)( _ref(TextureDesc)* _this ) {
	return _unref(_this)->mNodeIndex;
}
DEFINE_PRIM(_I32,TextureDesc_get_nodeIndex,_IDL);
HL_PRIM unsigned int HL_NAME(TextureDesc_set_nodeIndex)( _ref(TextureDesc)* _this, unsigned int value ) {
	_unref(_this)->mNodeIndex = (value);
	return value;
}
DEFINE_PRIM(_I32,TextureDesc_set_nodeIndex,_IDL _I32);

HL_PRIM _ref(TextureLoadDesc)* HL_NAME(TextureLoadDesc_new0)() {
	auto ___retvalue = alloc_ref((new TextureLoadDesc()),TextureLoadDesc);
	*(___retvalue->value) = {};
	return ___retvalue;
}
DEFINE_PRIM(_IDL, TextureLoadDesc_new0,);

HL_PRIM HL_CONST _ref(Texture)* HL_NAME(TextureLoadDesc_load1)(_ref(TextureLoadDesc)* _this, _ref(SyncToken)* syncToken) {
	return alloc_ref_const((forge_texture_load( _unref(_this) , _unref_ptr_safe(syncToken))),Texture);
}
DEFINE_PRIM(_IDL, TextureLoadDesc_load1, _IDL _IDL);

HL_PRIM int HL_NAME(TextureLoadDesc_get_creationFlag)( _ref(TextureLoadDesc)* _this ) {
	return HL_NAME(TextureCreationFlags_valueToIndex0)(_unref(_this)->mCreationFlag);
}
DEFINE_PRIM(_I32,TextureLoadDesc_get_creationFlag,_IDL);
HL_PRIM int HL_NAME(TextureLoadDesc_set_creationFlag)( _ref(TextureLoadDesc)* _this, int value ) {
	_unref(_this)->mCreationFlag = (TextureCreationFlags)HL_NAME(TextureCreationFlags_indexToValue0)(value);
	return value;
}
DEFINE_PRIM(_I32,TextureLoadDesc_set_creationFlag,_IDL _I32);

}
