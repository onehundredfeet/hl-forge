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
static BlendStateTargets BlendStateTargets__values[] = { BLEND_STATE_TARGET_0,BLEND_STATE_TARGET_1,BLEND_STATE_TARGET_2,BLEND_STATE_TARGET_3,BLEND_STATE_TARGET_4,BLEND_STATE_TARGET_5,BLEND_STATE_TARGET_6,BLEND_STATE_TARGET_7,BLEND_STATE_TARGET_ALL };
HL_PRIM int HL_NAME(BlendStateTargets_toValue0)( int idx ) {
	return BlendStateTargets__values[idx];
}
DEFINE_PRIM(_I32, BlendStateTargets_toValue0, _I32);
HL_PRIM int HL_NAME(BlendStateTargets_indexToValue0)( int idx ) {
	return BlendStateTargets__values[idx];
}
DEFINE_PRIM(_I32, BlendStateTargets_indexToValue0, _I32);
HL_PRIM int HL_NAME(BlendStateTargets_valueToIndex0)( int value ) {
	for( int i = 0; i < 9; i++ ) if ( value == (int)BlendStateTargets__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, BlendStateTargets_valueToIndex0, _I32);
static CullMode CullMode__values[] = { CULL_MODE_NONE,CULL_MODE_BACK,CULL_MODE_FRONT,CULL_MODE_BOTH,MAX_CULL_MODES };
HL_PRIM int HL_NAME(CullMode_toValue0)( int idx ) {
	return CullMode__values[idx];
}
DEFINE_PRIM(_I32, CullMode_toValue0, _I32);
HL_PRIM int HL_NAME(CullMode_indexToValue0)( int idx ) {
	return CullMode__values[idx];
}
DEFINE_PRIM(_I32, CullMode_indexToValue0, _I32);
HL_PRIM int HL_NAME(CullMode_valueToIndex0)( int value ) {
	for( int i = 0; i < 5; i++ ) if ( value == (int)CullMode__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, CullMode_valueToIndex0, _I32);
static FrontFace FrontFace__values[] = { FRONT_FACE_CCW,FRONT_FACE_CW };
HL_PRIM int HL_NAME(FrontFace_toValue0)( int idx ) {
	return FrontFace__values[idx];
}
DEFINE_PRIM(_I32, FrontFace_toValue0, _I32);
HL_PRIM int HL_NAME(FrontFace_indexToValue0)( int idx ) {
	return FrontFace__values[idx];
}
DEFINE_PRIM(_I32, FrontFace_indexToValue0, _I32);
HL_PRIM int HL_NAME(FrontFace_valueToIndex0)( int value ) {
	for( int i = 0; i < 2; i++ ) if ( value == (int)FrontFace__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, FrontFace_valueToIndex0, _I32);
static FillMode FillMode__values[] = { FILL_MODE_SOLID,FILL_MODE_WIREFRAME,MAX_FILL_MODES };
HL_PRIM int HL_NAME(FillMode_toValue0)( int idx ) {
	return FillMode__values[idx];
}
DEFINE_PRIM(_I32, FillMode_toValue0, _I32);
HL_PRIM int HL_NAME(FillMode_indexToValue0)( int idx ) {
	return FillMode__values[idx];
}
DEFINE_PRIM(_I32, FillMode_indexToValue0, _I32);
HL_PRIM int HL_NAME(FillMode_valueToIndex0)( int value ) {
	for( int i = 0; i < 3; i++ ) if ( value == (int)FillMode__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, FillMode_valueToIndex0, _I32);
static PipelineType PipelineType__values[] = { PIPELINE_TYPE_UNDEFINED,PIPELINE_TYPE_COMPUTE,PIPELINE_TYPE_GRAPHICS,PIPELINE_TYPE_RAYTRACING,PIPELINE_TYPE_COUNT };
HL_PRIM int HL_NAME(PipelineType_toValue0)( int idx ) {
	return PipelineType__values[idx];
}
DEFINE_PRIM(_I32, PipelineType_toValue0, _I32);
HL_PRIM int HL_NAME(PipelineType_indexToValue0)( int idx ) {
	return PipelineType__values[idx];
}
DEFINE_PRIM(_I32, PipelineType_indexToValue0, _I32);
HL_PRIM int HL_NAME(PipelineType_valueToIndex0)( int value ) {
	for( int i = 0; i < 5; i++ ) if ( value == (int)PipelineType__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, PipelineType_valueToIndex0, _I32);
static FilterType FilterType__values[] = { FILTER_NEAREST,FILTER_LINEAR };
HL_PRIM int HL_NAME(FilterType_toValue0)( int idx ) {
	return FilterType__values[idx];
}
DEFINE_PRIM(_I32, FilterType_toValue0, _I32);
HL_PRIM int HL_NAME(FilterType_indexToValue0)( int idx ) {
	return FilterType__values[idx];
}
DEFINE_PRIM(_I32, FilterType_indexToValue0, _I32);
HL_PRIM int HL_NAME(FilterType_valueToIndex0)( int value ) {
	for( int i = 0; i < 2; i++ ) if ( value == (int)FilterType__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, FilterType_valueToIndex0, _I32);
static AddressMode AddressMode__values[] = { ADDRESS_MODE_MIRROR,ADDRESS_MODE_REPEAT,ADDRESS_MODE_CLAMP_TO_EDGE,ADDRESS_MODE_CLAMP_TO_BORDER };
HL_PRIM int HL_NAME(AddressMode_toValue0)( int idx ) {
	return AddressMode__values[idx];
}
DEFINE_PRIM(_I32, AddressMode_toValue0, _I32);
HL_PRIM int HL_NAME(AddressMode_indexToValue0)( int idx ) {
	return AddressMode__values[idx];
}
DEFINE_PRIM(_I32, AddressMode_indexToValue0, _I32);
HL_PRIM int HL_NAME(AddressMode_valueToIndex0)( int value ) {
	for( int i = 0; i < 4; i++ ) if ( value == (int)AddressMode__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, AddressMode_valueToIndex0, _I32);
static MipMapMode MipMapMode__values[] = { MIPMAP_MODE_NEAREST,MIPMAP_MODE_LINEAR };
HL_PRIM int HL_NAME(MipMapMode_toValue0)( int idx ) {
	return MipMapMode__values[idx];
}
DEFINE_PRIM(_I32, MipMapMode_toValue0, _I32);
HL_PRIM int HL_NAME(MipMapMode_indexToValue0)( int idx ) {
	return MipMapMode__values[idx];
}
DEFINE_PRIM(_I32, MipMapMode_indexToValue0, _I32);
HL_PRIM int HL_NAME(MipMapMode_valueToIndex0)( int value ) {
	for( int i = 0; i < 2; i++ ) if ( value == (int)MipMapMode__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, MipMapMode_valueToIndex0, _I32);
static PrimitiveTopology PrimitiveTopology__values[] = { PRIMITIVE_TOPO_POINT_LIST,PRIMITIVE_TOPO_LINE_LIST,PRIMITIVE_TOPO_LINE_STRIP,PRIMITIVE_TOPO_TRI_LIST,PRIMITIVE_TOPO_TRI_STRIP,PRIMITIVE_TOPO_PATCH_LIST,PRIMITIVE_TOPO_COUNT };
HL_PRIM int HL_NAME(PrimitiveTopology_toValue0)( int idx ) {
	return PrimitiveTopology__values[idx];
}
DEFINE_PRIM(_I32, PrimitiveTopology_toValue0, _I32);
HL_PRIM int HL_NAME(PrimitiveTopology_indexToValue0)( int idx ) {
	return PrimitiveTopology__values[idx];
}
DEFINE_PRIM(_I32, PrimitiveTopology_indexToValue0, _I32);
HL_PRIM int HL_NAME(PrimitiveTopology_valueToIndex0)( int value ) {
	for( int i = 0; i < 7; i++ ) if ( value == (int)PrimitiveTopology__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, PrimitiveTopology_valueToIndex0, _I32);
static IndexType IndexType__values[] = { INDEX_TYPE_UINT32,INDEX_TYPE_UINT16 };
HL_PRIM int HL_NAME(IndexType_toValue0)( int idx ) {
	return IndexType__values[idx];
}
DEFINE_PRIM(_I32, IndexType_toValue0, _I32);
HL_PRIM int HL_NAME(IndexType_indexToValue0)( int idx ) {
	return IndexType__values[idx];
}
DEFINE_PRIM(_I32, IndexType_indexToValue0, _I32);
HL_PRIM int HL_NAME(IndexType_valueToIndex0)( int value ) {
	for( int i = 0; i < 2; i++ ) if ( value == (int)IndexType__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, IndexType_valueToIndex0, _I32);
static ShaderSemantic ShaderSemantic__values[] = { SEMANTIC_UNDEFINED,SEMANTIC_POSITION,SEMANTIC_NORMAL,SEMANTIC_COLOR,SEMANTIC_TANGENT,SEMANTIC_BITANGENT,SEMANTIC_JOINTS,SEMANTIC_WEIGHTS,SEMANTIC_SHADING_RATE,SEMANTIC_TEXCOORD0,SEMANTIC_TEXCOORD1,SEMANTIC_TEXCOORD2,SEMANTIC_TEXCOORD3,SEMANTIC_TEXCOORD4,SEMANTIC_TEXCOORD5,SEMANTIC_TEXCOORD6,SEMANTIC_TEXCOORD7,SEMANTIC_TEXCOORD8,SEMANTIC_TEXCOORD9 };
HL_PRIM int HL_NAME(ShaderSemantic_toValue0)( int idx ) {
	return ShaderSemantic__values[idx];
}
DEFINE_PRIM(_I32, ShaderSemantic_toValue0, _I32);
HL_PRIM int HL_NAME(ShaderSemantic_indexToValue0)( int idx ) {
	return ShaderSemantic__values[idx];
}
DEFINE_PRIM(_I32, ShaderSemantic_indexToValue0, _I32);
HL_PRIM int HL_NAME(ShaderSemantic_valueToIndex0)( int value ) {
	for( int i = 0; i < 19; i++ ) if ( value == (int)ShaderSemantic__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, ShaderSemantic_valueToIndex0, _I32);
static BlendConstant BlendConstant__values[] = { BC_ZERO,BC_ONE,BC_SRC_COLOR,BC_ONE_MINUS_SRC_COLOR,BC_DST_COLOR,BC_ONE_MINUS_DST_COLOR,BC_SRC_ALPHA,BC_ONE_MINUS_SRC_ALPHA,BC_DST_ALPHA,BC_ONE_MINUS_DST_ALPHA,BC_SRC_ALPHA_SATURATE,BC_BLEND_FACTOR,BC_ONE_MINUS_BLEND_FACTOR,MAX_BLEND_CONSTANTS };
HL_PRIM int HL_NAME(BlendConstant_toValue0)( int idx ) {
	return BlendConstant__values[idx];
}
DEFINE_PRIM(_I32, BlendConstant_toValue0, _I32);
HL_PRIM int HL_NAME(BlendConstant_indexToValue0)( int idx ) {
	return BlendConstant__values[idx];
}
DEFINE_PRIM(_I32, BlendConstant_indexToValue0, _I32);
HL_PRIM int HL_NAME(BlendConstant_valueToIndex0)( int value ) {
	for( int i = 0; i < 14; i++ ) if ( value == (int)BlendConstant__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, BlendConstant_valueToIndex0, _I32);
static BlendMode BlendMode__values[] = { BM_ADD,BM_SUBTRACT,BM_REVERSE_SUBTRACT,BM_MIN,BM_MAX,MAX_BLEND_MODES };
HL_PRIM int HL_NAME(BlendMode_toValue0)( int idx ) {
	return BlendMode__values[idx];
}
DEFINE_PRIM(_I32, BlendMode_toValue0, _I32);
HL_PRIM int HL_NAME(BlendMode_indexToValue0)( int idx ) {
	return BlendMode__values[idx];
}
DEFINE_PRIM(_I32, BlendMode_indexToValue0, _I32);
HL_PRIM int HL_NAME(BlendMode_valueToIndex0)( int value ) {
	for( int i = 0; i < 6; i++ ) if ( value == (int)BlendMode__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, BlendMode_valueToIndex0, _I32);
static CompareMode CompareMode__values[] = { CMP_NEVER,CMP_LESS,CMP_EQUAL,CMP_LEQUAL,CMP_GREATER,CMP_NOTEQUAL,CMP_GEQUAL,CMP_ALWAYS,MAX_COMPARE_MODES };
HL_PRIM int HL_NAME(CompareMode_toValue0)( int idx ) {
	return CompareMode__values[idx];
}
DEFINE_PRIM(_I32, CompareMode_toValue0, _I32);
HL_PRIM int HL_NAME(CompareMode_indexToValue0)( int idx ) {
	return CompareMode__values[idx];
}
DEFINE_PRIM(_I32, CompareMode_indexToValue0, _I32);
HL_PRIM int HL_NAME(CompareMode_valueToIndex0)( int value ) {
	for( int i = 0; i < 9; i++ ) if ( value == (int)CompareMode__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, CompareMode_valueToIndex0, _I32);
static StencilOp StencilOp__values[] = { STENCIL_OP_KEEP,STENCIL_OP_SET_ZERO,STENCIL_OP_REPLACE,STENCIL_OP_INVERT,STENCIL_OP_INCR,STENCIL_OP_DECR,STENCIL_OP_INCR_SAT,STENCIL_OP_DECR_SAT,MAX_STENCIL_OPS };
HL_PRIM int HL_NAME(StencilOp_toValue0)( int idx ) {
	return StencilOp__values[idx];
}
DEFINE_PRIM(_I32, StencilOp_toValue0, _I32);
HL_PRIM int HL_NAME(StencilOp_indexToValue0)( int idx ) {
	return StencilOp__values[idx];
}
DEFINE_PRIM(_I32, StencilOp_indexToValue0, _I32);
HL_PRIM int HL_NAME(StencilOp_valueToIndex0)( int value ) {
	for( int i = 0; i < 9; i++ ) if ( value == (int)StencilOp__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, StencilOp_valueToIndex0, _I32);
static DescriptorUpdateFrequency DescriptorUpdateFrequency__values[] = { DESCRIPTOR_UPDATE_FREQ_NONE,DESCRIPTOR_UPDATE_FREQ_PER_FRAME,DESCRIPTOR_UPDATE_FREQ_PER_BATCH,DESCRIPTOR_UPDATE_FREQ_PER_DRAW,DESCRIPTOR_UPDATE_FREQ_COUNT };
HL_PRIM int HL_NAME(DescriptorUpdateFrequency_toValue0)( int idx ) {
	return DescriptorUpdateFrequency__values[idx];
}
DEFINE_PRIM(_I32, DescriptorUpdateFrequency_toValue0, _I32);
HL_PRIM int HL_NAME(DescriptorUpdateFrequency_indexToValue0)( int idx ) {
	return DescriptorUpdateFrequency__values[idx];
}
DEFINE_PRIM(_I32, DescriptorUpdateFrequency_indexToValue0, _I32);
HL_PRIM int HL_NAME(DescriptorUpdateFrequency_valueToIndex0)( int value ) {
	for( int i = 0; i < 5; i++ ) if ( value == (int)DescriptorUpdateFrequency__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, DescriptorUpdateFrequency_valueToIndex0, _I32);
static DescriptorType DescriptorType__values[] = { DESCRIPTOR_TYPE_UNDEFINED,DESCRIPTOR_TYPE_SAMPLER,DESCRIPTOR_TYPE_TEXTURE,DESCRIPTOR_TYPE_RW_TEXTURE,DESCRIPTOR_TYPE_BUFFER,DESCRIPTOR_TYPE_RW_BUFFER,DESCRIPTOR_TYPE_UNIFORM_BUFFER,DESCRIPTOR_TYPE_ROOT_CONSTANT,DESCRIPTOR_TYPE_VERTEX_BUFFER,DESCRIPTOR_TYPE_INDEX_BUFFER,DESCRIPTOR_TYPE_INDIRECT_BUFFER,DESCRIPTOR_TYPE_TEXTURE_CUBE };
HL_PRIM int HL_NAME(DescriptorType_toValue0)( int idx ) {
	return DescriptorType__values[idx];
}
DEFINE_PRIM(_I32, DescriptorType_toValue0, _I32);
HL_PRIM int HL_NAME(DescriptorType_indexToValue0)( int idx ) {
	return DescriptorType__values[idx];
}
DEFINE_PRIM(_I32, DescriptorType_indexToValue0, _I32);
HL_PRIM int HL_NAME(DescriptorType_valueToIndex0)( int value ) {
	for( int i = 0; i < 12; i++ ) if ( value == (int)DescriptorType__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, DescriptorType_valueToIndex0, _I32);
static void finalize_StateBuilder( _ref(StateBuilder)* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(StateBuilder_delete)( _ref(StateBuilder)* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, StateBuilder_delete, _IDL);
static VertexAttribRate VertexAttribRate__values[] = { VERTEX_ATTRIB_RATE_VERTEX,VERTEX_ATTRIB_RATE_INSTANCE,VERTEX_ATTRIB_RATE_COUNT };
HL_PRIM int HL_NAME(VertexAttribRate_toValue0)( int idx ) {
	return VertexAttribRate__values[idx];
}
DEFINE_PRIM(_I32, VertexAttribRate_toValue0, _I32);
HL_PRIM int HL_NAME(VertexAttribRate_indexToValue0)( int idx ) {
	return VertexAttribRate__values[idx];
}
DEFINE_PRIM(_I32, VertexAttribRate_indexToValue0, _I32);
HL_PRIM int HL_NAME(VertexAttribRate_valueToIndex0)( int value ) {
	for( int i = 0; i < 3; i++ ) if ( value == (int)VertexAttribRate__values[i]) return i; return -1;
}
DEFINE_PRIM(_I32, VertexAttribRate_valueToIndex0, _I32);
static void finalize_VertexLayout( _ref(VertexLayout)* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(VertexLayout_delete)( _ref(VertexLayout)* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, VertexLayout_delete, _IDL);
static void finalize_PipelineCache( _ref(PipelineCache)* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(PipelineCache_delete)( _ref(PipelineCache)* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, PipelineCache_delete, _IDL);
static void finalize_PipelineDesc( _ref(HlForgePipelineDesc)* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(PipelineDesc_delete)( _ref(HlForgePipelineDesc)* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, PipelineDesc_delete, _IDL);
static void finalize_BufferBinder( _ref(BufferBinder)* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(BufferBinder_delete)( _ref(BufferBinder)* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, BufferBinder_delete, _IDL);
static void finalize_Map64Int( _ref(Map64Int)* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(Map64Int_delete)( _ref(Map64Int)* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, Map64Int_delete, _IDL);
static void finalize_RenderTargetDesc( _ref(RenderTargetDesc)* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(RenderTargetDesc_delete)( _ref(RenderTargetDesc)* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, RenderTargetDesc_delete, _IDL);
static void finalize_SamplerDesc( _ref(SamplerDesc)* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(SamplerDesc_delete)( _ref(SamplerDesc)* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, SamplerDesc_delete, _IDL);
static void finalize_DescriptorSetDesc( _ref(DescriptorSetDesc)* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(DescriptorSetDesc_delete)( _ref(DescriptorSetDesc)* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, DescriptorSetDesc_delete, _IDL);
static void finalize_DescriptorDataBuilder( _ref(DescriptorDataBuilder)* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(DescriptorDataBuilder_delete)( _ref(DescriptorDataBuilder)* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, DescriptorDataBuilder_delete, _IDL);
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
HL_PRIM int HL_NAME(BlendStateDesc_get_SrcFactors)( _ref(BlendStateDesc)* _this, int index ) {
	return _unref(_this)->mSrcFactors[index];
}
DEFINE_PRIM(_I32,BlendStateDesc_get_SrcFactors,_IDL _I32);
HL_PRIM int HL_NAME(BlendStateDesc_set_SrcFactors)( _ref(BlendStateDesc)* _this, int index, int value ) {
	_unref(_this)->mSrcFactors[index] = (BlendConstant)(BlendConstant__values[value]);
	return value;
}
DEFINE_PRIM(_I32,BlendStateDesc_set_SrcFactors,_IDL _I32 _I32); // Array setter

HL_PRIM int HL_NAME(BlendStateDesc_get_DstFactors)( _ref(BlendStateDesc)* _this, int index ) {
	return _unref(_this)->mDstFactors[index];
}
DEFINE_PRIM(_I32,BlendStateDesc_get_DstFactors,_IDL _I32);
HL_PRIM int HL_NAME(BlendStateDesc_set_DstFactors)( _ref(BlendStateDesc)* _this, int index, int value ) {
	_unref(_this)->mDstFactors[index] = (BlendConstant)(BlendConstant__values[value]);
	return value;
}
DEFINE_PRIM(_I32,BlendStateDesc_set_DstFactors,_IDL _I32 _I32); // Array setter

HL_PRIM int HL_NAME(BlendStateDesc_get_SrcAlphaFactors)( _ref(BlendStateDesc)* _this, int index ) {
	return _unref(_this)->mSrcAlphaFactors[index];
}
DEFINE_PRIM(_I32,BlendStateDesc_get_SrcAlphaFactors,_IDL _I32);
HL_PRIM int HL_NAME(BlendStateDesc_set_SrcAlphaFactors)( _ref(BlendStateDesc)* _this, int index, int value ) {
	_unref(_this)->mSrcAlphaFactors[index] = (BlendConstant)(BlendConstant__values[value]);
	return value;
}
DEFINE_PRIM(_I32,BlendStateDesc_set_SrcAlphaFactors,_IDL _I32 _I32); // Array setter

HL_PRIM int HL_NAME(BlendStateDesc_get_DstAlphaFactors)( _ref(BlendStateDesc)* _this, int index ) {
	return _unref(_this)->mDstAlphaFactors[index];
}
DEFINE_PRIM(_I32,BlendStateDesc_get_DstAlphaFactors,_IDL _I32);
HL_PRIM int HL_NAME(BlendStateDesc_set_DstAlphaFactors)( _ref(BlendStateDesc)* _this, int index, int value ) {
	_unref(_this)->mDstAlphaFactors[index] = (BlendConstant)(BlendConstant__values[value]);
	return value;
}
DEFINE_PRIM(_I32,BlendStateDesc_set_DstAlphaFactors,_IDL _I32 _I32); // Array setter

HL_PRIM int HL_NAME(BlendStateDesc_get_BlendModes)( _ref(BlendStateDesc)* _this, int index ) {
	return _unref(_this)->mBlendModes[index];
}
DEFINE_PRIM(_I32,BlendStateDesc_get_BlendModes,_IDL _I32);
HL_PRIM int HL_NAME(BlendStateDesc_set_BlendModes)( _ref(BlendStateDesc)* _this, int index, int value ) {
	_unref(_this)->mBlendModes[index] = (BlendMode)(BlendMode__values[value]);
	return value;
}
DEFINE_PRIM(_I32,BlendStateDesc_set_BlendModes,_IDL _I32 _I32); // Array setter

HL_PRIM int HL_NAME(BlendStateDesc_get_BlendAlphaModes)( _ref(BlendStateDesc)* _this, int index ) {
	return _unref(_this)->mBlendAlphaModes[index];
}
DEFINE_PRIM(_I32,BlendStateDesc_get_BlendAlphaModes,_IDL _I32);
HL_PRIM int HL_NAME(BlendStateDesc_set_BlendAlphaModes)( _ref(BlendStateDesc)* _this, int index, int value ) {
	_unref(_this)->mBlendAlphaModes[index] = (BlendMode)(BlendMode__values[value]);
	return value;
}
DEFINE_PRIM(_I32,BlendStateDesc_set_BlendAlphaModes,_IDL _I32 _I32); // Array setter

HL_PRIM int HL_NAME(BlendStateDesc_get_Masks)( _ref(BlendStateDesc)* _this, int index ) {
	return _unref(_this)->mMasks[index];
}
DEFINE_PRIM(_I32,BlendStateDesc_get_Masks,_IDL _I32);
HL_PRIM int HL_NAME(BlendStateDesc_set_Masks)( _ref(BlendStateDesc)* _this, int index, int value ) {
	_unref(_this)->mMasks[index] = (value);
	return value;
}
DEFINE_PRIM(_I32,BlendStateDesc_set_Masks,_IDL _I32 _I32); // Array setter

HL_PRIM int HL_NAME(BlendStateDesc_get_renderTargetMask)( _ref(BlendStateDesc)* _this ) {
	return _unref(_this)->mRenderTargetMask;
}
DEFINE_PRIM(_I32,BlendStateDesc_get_renderTargetMask,_IDL);
HL_PRIM int HL_NAME(BlendStateDesc_set_renderTargetMask)( _ref(BlendStateDesc)* _this, int value ) {
	_unref(_this)->mRenderTargetMask = (BlendStateTargets)(value);
	return value;
}
DEFINE_PRIM(_I32,BlendStateDesc_set_renderTargetMask,_IDL _I32);

HL_PRIM void HL_NAME(BlendStateDesc_setRenderTarget2)(_ref(BlendStateDesc)* _this, int target, bool mask) {
	(forge_blend_state_desc_set_rt( _unref(_this) , BlendStateTargets__values[target], mask));
}
DEFINE_PRIM(_VOID, BlendStateDesc_setRenderTarget2, _IDL _I32 _BOOL);

HL_PRIM bool HL_NAME(BlendStateDesc_get_alphaToCoverage)( _ref(BlendStateDesc)* _this ) {
	return _unref(_this)->mAlphaToCoverage;
}
DEFINE_PRIM(_BOOL,BlendStateDesc_get_alphaToCoverage,_IDL);
HL_PRIM bool HL_NAME(BlendStateDesc_set_alphaToCoverage)( _ref(BlendStateDesc)* _this, bool value ) {
	_unref(_this)->mAlphaToCoverage = (value);
	return value;
}
DEFINE_PRIM(_BOOL,BlendStateDesc_set_alphaToCoverage,_IDL _BOOL);

HL_PRIM bool HL_NAME(BlendStateDesc_get_independentBlend)( _ref(BlendStateDesc)* _this ) {
	return _unref(_this)->mIndependentBlend;
}
DEFINE_PRIM(_BOOL,BlendStateDesc_get_independentBlend,_IDL);
HL_PRIM bool HL_NAME(BlendStateDesc_set_independentBlend)( _ref(BlendStateDesc)* _this, bool value ) {
	_unref(_this)->mIndependentBlend = (value);
	return value;
}
DEFINE_PRIM(_BOOL,BlendStateDesc_set_independentBlend,_IDL _BOOL);

HL_PRIM bool HL_NAME(DepthStateDesc_get_depthTest)( _ref(DepthStateDesc)* _this ) {
	return _unref(_this)->mDepthTest;
}
DEFINE_PRIM(_BOOL,DepthStateDesc_get_depthTest,_IDL);
HL_PRIM bool HL_NAME(DepthStateDesc_set_depthTest)( _ref(DepthStateDesc)* _this, bool value ) {
	_unref(_this)->mDepthTest = (value);
	return value;
}
DEFINE_PRIM(_BOOL,DepthStateDesc_set_depthTest,_IDL _BOOL);

HL_PRIM bool HL_NAME(DepthStateDesc_get_depthWrite)( _ref(DepthStateDesc)* _this ) {
	return _unref(_this)->mDepthWrite;
}
DEFINE_PRIM(_BOOL,DepthStateDesc_get_depthWrite,_IDL);
HL_PRIM bool HL_NAME(DepthStateDesc_set_depthWrite)( _ref(DepthStateDesc)* _this, bool value ) {
	_unref(_this)->mDepthWrite = (value);
	return value;
}
DEFINE_PRIM(_BOOL,DepthStateDesc_set_depthWrite,_IDL _BOOL);

HL_PRIM int HL_NAME(DepthStateDesc_get_depthFunc)( _ref(DepthStateDesc)* _this ) {
	return HL_NAME(CompareMode_valueToIndex0)(_unref(_this)->mDepthFunc);
}
DEFINE_PRIM(_I32,DepthStateDesc_get_depthFunc,_IDL);
HL_PRIM int HL_NAME(DepthStateDesc_set_depthFunc)( _ref(DepthStateDesc)* _this, int value ) {
	_unref(_this)->mDepthFunc = (CompareMode)HL_NAME(CompareMode_indexToValue0)(value);
	return value;
}
DEFINE_PRIM(_I32,DepthStateDesc_set_depthFunc,_IDL _I32);

HL_PRIM bool HL_NAME(DepthStateDesc_get_stencilTest)( _ref(DepthStateDesc)* _this ) {
	return _unref(_this)->mStencilTest;
}
DEFINE_PRIM(_BOOL,DepthStateDesc_get_stencilTest,_IDL);
HL_PRIM bool HL_NAME(DepthStateDesc_set_stencilTest)( _ref(DepthStateDesc)* _this, bool value ) {
	_unref(_this)->mStencilTest = (value);
	return value;
}
DEFINE_PRIM(_BOOL,DepthStateDesc_set_stencilTest,_IDL _BOOL);

HL_PRIM int HL_NAME(DepthStateDesc_get_stencilReadMask)( _ref(DepthStateDesc)* _this ) {
	return _unref(_this)->mStencilReadMask;
}
DEFINE_PRIM(_I32,DepthStateDesc_get_stencilReadMask,_IDL);
HL_PRIM int HL_NAME(DepthStateDesc_set_stencilReadMask)( _ref(DepthStateDesc)* _this, int value ) {
	_unref(_this)->mStencilReadMask = (value);
	return value;
}
DEFINE_PRIM(_I32,DepthStateDesc_set_stencilReadMask,_IDL _I32);

HL_PRIM int HL_NAME(DepthStateDesc_get_stencilWriteMask)( _ref(DepthStateDesc)* _this ) {
	return _unref(_this)->mStencilWriteMask;
}
DEFINE_PRIM(_I32,DepthStateDesc_get_stencilWriteMask,_IDL);
HL_PRIM int HL_NAME(DepthStateDesc_set_stencilWriteMask)( _ref(DepthStateDesc)* _this, int value ) {
	_unref(_this)->mStencilWriteMask = (value);
	return value;
}
DEFINE_PRIM(_I32,DepthStateDesc_set_stencilWriteMask,_IDL _I32);

HL_PRIM int HL_NAME(DepthStateDesc_get_stencilFrontFunc)( _ref(DepthStateDesc)* _this ) {
	return HL_NAME(CompareMode_valueToIndex0)(_unref(_this)->mStencilFrontFunc);
}
DEFINE_PRIM(_I32,DepthStateDesc_get_stencilFrontFunc,_IDL);
HL_PRIM int HL_NAME(DepthStateDesc_set_stencilFrontFunc)( _ref(DepthStateDesc)* _this, int value ) {
	_unref(_this)->mStencilFrontFunc = (CompareMode)HL_NAME(CompareMode_indexToValue0)(value);
	return value;
}
DEFINE_PRIM(_I32,DepthStateDesc_set_stencilFrontFunc,_IDL _I32);

HL_PRIM int HL_NAME(DepthStateDesc_get_stencilFrontFail)( _ref(DepthStateDesc)* _this ) {
	return HL_NAME(StencilOp_valueToIndex0)(_unref(_this)->mStencilFrontFail);
}
DEFINE_PRIM(_I32,DepthStateDesc_get_stencilFrontFail,_IDL);
HL_PRIM int HL_NAME(DepthStateDesc_set_stencilFrontFail)( _ref(DepthStateDesc)* _this, int value ) {
	_unref(_this)->mStencilFrontFail = (StencilOp)HL_NAME(StencilOp_indexToValue0)(value);
	return value;
}
DEFINE_PRIM(_I32,DepthStateDesc_set_stencilFrontFail,_IDL _I32);

HL_PRIM int HL_NAME(DepthStateDesc_get_depthFrontFail)( _ref(DepthStateDesc)* _this ) {
	return HL_NAME(StencilOp_valueToIndex0)(_unref(_this)->mDepthFrontFail);
}
DEFINE_PRIM(_I32,DepthStateDesc_get_depthFrontFail,_IDL);
HL_PRIM int HL_NAME(DepthStateDesc_set_depthFrontFail)( _ref(DepthStateDesc)* _this, int value ) {
	_unref(_this)->mDepthFrontFail = (StencilOp)HL_NAME(StencilOp_indexToValue0)(value);
	return value;
}
DEFINE_PRIM(_I32,DepthStateDesc_set_depthFrontFail,_IDL _I32);

HL_PRIM int HL_NAME(DepthStateDesc_get_stencilFrontPass)( _ref(DepthStateDesc)* _this ) {
	return HL_NAME(StencilOp_valueToIndex0)(_unref(_this)->mStencilFrontPass);
}
DEFINE_PRIM(_I32,DepthStateDesc_get_stencilFrontPass,_IDL);
HL_PRIM int HL_NAME(DepthStateDesc_set_stencilFrontPass)( _ref(DepthStateDesc)* _this, int value ) {
	_unref(_this)->mStencilFrontPass = (StencilOp)HL_NAME(StencilOp_indexToValue0)(value);
	return value;
}
DEFINE_PRIM(_I32,DepthStateDesc_set_stencilFrontPass,_IDL _I32);

HL_PRIM int HL_NAME(DepthStateDesc_get_stencilBackFunc)( _ref(DepthStateDesc)* _this ) {
	return HL_NAME(CompareMode_valueToIndex0)(_unref(_this)->mStencilBackFunc);
}
DEFINE_PRIM(_I32,DepthStateDesc_get_stencilBackFunc,_IDL);
HL_PRIM int HL_NAME(DepthStateDesc_set_stencilBackFunc)( _ref(DepthStateDesc)* _this, int value ) {
	_unref(_this)->mStencilBackFunc = (CompareMode)HL_NAME(CompareMode_indexToValue0)(value);
	return value;
}
DEFINE_PRIM(_I32,DepthStateDesc_set_stencilBackFunc,_IDL _I32);

HL_PRIM int HL_NAME(DepthStateDesc_get_stencilBackFail)( _ref(DepthStateDesc)* _this ) {
	return HL_NAME(StencilOp_valueToIndex0)(_unref(_this)->mStencilBackFail);
}
DEFINE_PRIM(_I32,DepthStateDesc_get_stencilBackFail,_IDL);
HL_PRIM int HL_NAME(DepthStateDesc_set_stencilBackFail)( _ref(DepthStateDesc)* _this, int value ) {
	_unref(_this)->mStencilBackFail = (StencilOp)HL_NAME(StencilOp_indexToValue0)(value);
	return value;
}
DEFINE_PRIM(_I32,DepthStateDesc_set_stencilBackFail,_IDL _I32);

HL_PRIM int HL_NAME(DepthStateDesc_get_depthBackFail)( _ref(DepthStateDesc)* _this ) {
	return HL_NAME(StencilOp_valueToIndex0)(_unref(_this)->mDepthBackFail);
}
DEFINE_PRIM(_I32,DepthStateDesc_get_depthBackFail,_IDL);
HL_PRIM int HL_NAME(DepthStateDesc_set_depthBackFail)( _ref(DepthStateDesc)* _this, int value ) {
	_unref(_this)->mDepthBackFail = (StencilOp)HL_NAME(StencilOp_indexToValue0)(value);
	return value;
}
DEFINE_PRIM(_I32,DepthStateDesc_set_depthBackFail,_IDL _I32);

HL_PRIM int HL_NAME(DepthStateDesc_get_stencilBackPass)( _ref(DepthStateDesc)* _this ) {
	return HL_NAME(StencilOp_valueToIndex0)(_unref(_this)->mStencilBackPass);
}
DEFINE_PRIM(_I32,DepthStateDesc_get_stencilBackPass,_IDL);
HL_PRIM int HL_NAME(DepthStateDesc_set_stencilBackPass)( _ref(DepthStateDesc)* _this, int value ) {
	_unref(_this)->mStencilBackPass = (StencilOp)HL_NAME(StencilOp_indexToValue0)(value);
	return value;
}
DEFINE_PRIM(_I32,DepthStateDesc_set_stencilBackPass,_IDL _I32);

HL_PRIM int HL_NAME(RasterizerStateDesc_get_cullMode)( _ref(RasterizerStateDesc)* _this ) {
	return HL_NAME(CullMode_valueToIndex0)(_unref(_this)->mCullMode);
}
DEFINE_PRIM(_I32,RasterizerStateDesc_get_cullMode,_IDL);
HL_PRIM int HL_NAME(RasterizerStateDesc_set_cullMode)( _ref(RasterizerStateDesc)* _this, int value ) {
	_unref(_this)->mCullMode = (CullMode)HL_NAME(CullMode_indexToValue0)(value);
	return value;
}
DEFINE_PRIM(_I32,RasterizerStateDesc_set_cullMode,_IDL _I32);

HL_PRIM int HL_NAME(RasterizerStateDesc_get_depthBias)( _ref(RasterizerStateDesc)* _this ) {
	return _unref(_this)->mDepthBias;
}
DEFINE_PRIM(_I32,RasterizerStateDesc_get_depthBias,_IDL);
HL_PRIM int HL_NAME(RasterizerStateDesc_set_depthBias)( _ref(RasterizerStateDesc)* _this, int value ) {
	_unref(_this)->mDepthBias = (value);
	return value;
}
DEFINE_PRIM(_I32,RasterizerStateDesc_set_depthBias,_IDL _I32);

HL_PRIM float HL_NAME(RasterizerStateDesc_get_slopeScaledDepthBias)( _ref(RasterizerStateDesc)* _this ) {
	return _unref(_this)->mSlopeScaledDepthBias;
}
DEFINE_PRIM(_F32,RasterizerStateDesc_get_slopeScaledDepthBias,_IDL);
HL_PRIM float HL_NAME(RasterizerStateDesc_set_slopeScaledDepthBias)( _ref(RasterizerStateDesc)* _this, float value ) {
	_unref(_this)->mSlopeScaledDepthBias = (value);
	return value;
}
DEFINE_PRIM(_F32,RasterizerStateDesc_set_slopeScaledDepthBias,_IDL _F32);

HL_PRIM int HL_NAME(RasterizerStateDesc_get_fillMode)( _ref(RasterizerStateDesc)* _this ) {
	return HL_NAME(FillMode_valueToIndex0)(_unref(_this)->mFillMode);
}
DEFINE_PRIM(_I32,RasterizerStateDesc_get_fillMode,_IDL);
HL_PRIM int HL_NAME(RasterizerStateDesc_set_fillMode)( _ref(RasterizerStateDesc)* _this, int value ) {
	_unref(_this)->mFillMode = (FillMode)HL_NAME(FillMode_indexToValue0)(value);
	return value;
}
DEFINE_PRIM(_I32,RasterizerStateDesc_set_fillMode,_IDL _I32);

HL_PRIM int HL_NAME(RasterizerStateDesc_get_frontFace)( _ref(RasterizerStateDesc)* _this ) {
	return HL_NAME(FrontFace_valueToIndex0)(_unref(_this)->mFrontFace);
}
DEFINE_PRIM(_I32,RasterizerStateDesc_get_frontFace,_IDL);
HL_PRIM int HL_NAME(RasterizerStateDesc_set_frontFace)( _ref(RasterizerStateDesc)* _this, int value ) {
	_unref(_this)->mFrontFace = (FrontFace)HL_NAME(FrontFace_indexToValue0)(value);
	return value;
}
DEFINE_PRIM(_I32,RasterizerStateDesc_set_frontFace,_IDL _I32);

HL_PRIM bool HL_NAME(RasterizerStateDesc_get_multiSample)( _ref(RasterizerStateDesc)* _this ) {
	return _unref(_this)->mMultiSample;
}
DEFINE_PRIM(_BOOL,RasterizerStateDesc_get_multiSample,_IDL);
HL_PRIM bool HL_NAME(RasterizerStateDesc_set_multiSample)( _ref(RasterizerStateDesc)* _this, bool value ) {
	_unref(_this)->mMultiSample = (value);
	return value;
}
DEFINE_PRIM(_BOOL,RasterizerStateDesc_set_multiSample,_IDL _BOOL);

HL_PRIM bool HL_NAME(RasterizerStateDesc_get_scissor)( _ref(RasterizerStateDesc)* _this ) {
	return _unref(_this)->mScissor;
}
DEFINE_PRIM(_BOOL,RasterizerStateDesc_get_scissor,_IDL);
HL_PRIM bool HL_NAME(RasterizerStateDesc_set_scissor)( _ref(RasterizerStateDesc)* _this, bool value ) {
	_unref(_this)->mScissor = (value);
	return value;
}
DEFINE_PRIM(_BOOL,RasterizerStateDesc_set_scissor,_IDL _BOOL);

HL_PRIM bool HL_NAME(RasterizerStateDesc_get_depthClampEnable)( _ref(RasterizerStateDesc)* _this ) {
	return _unref(_this)->mDepthClampEnable;
}
DEFINE_PRIM(_BOOL,RasterizerStateDesc_get_depthClampEnable,_IDL);
HL_PRIM bool HL_NAME(RasterizerStateDesc_set_depthClampEnable)( _ref(RasterizerStateDesc)* _this, bool value ) {
	_unref(_this)->mDepthClampEnable = (value);
	return value;
}
DEFINE_PRIM(_BOOL,RasterizerStateDesc_set_depthClampEnable,_IDL _BOOL);

HL_PRIM _ref(StateBuilder)* HL_NAME(StateBuilder_new0)() {
	return alloc_ref((new StateBuilder()),StateBuilder);
}
DEFINE_PRIM(_IDL, StateBuilder_new0,);

HL_PRIM void HL_NAME(StateBuilder_reset0)(_ref(StateBuilder)* _this) {
	(_unref(_this)->reset());
}
DEFINE_PRIM(_VOID, StateBuilder_reset0, _IDL);

HL_PRIM HL_CONST _ref(BlendStateDesc)* HL_NAME(StateBuilder_blend0)(_ref(StateBuilder)* _this) {
	return alloc_ref_const((_unref(_this)->blend()),BlendStateDesc);
}
DEFINE_PRIM(_IDL, StateBuilder_blend0, _IDL);

HL_PRIM HL_CONST _ref(DepthStateDesc)* HL_NAME(StateBuilder_depth0)(_ref(StateBuilder)* _this) {
	return alloc_ref_const((_unref(_this)->depth()),DepthStateDesc);
}
DEFINE_PRIM(_IDL, StateBuilder_depth0, _IDL);

HL_PRIM HL_CONST _ref(RasterizerStateDesc)* HL_NAME(StateBuilder_raster0)(_ref(StateBuilder)* _this) {
	return alloc_ref_const((_unref(_this)->raster()),RasterizerStateDesc);
}
DEFINE_PRIM(_IDL, StateBuilder_raster0, _IDL);

HL_PRIM int64_t HL_NAME(StateBuilder_getSignature1)(_ref(StateBuilder)* _this, int shaderID) {
	return (_unref(_this)->getSignature(shaderID));
}
DEFINE_PRIM(_I64, StateBuilder_getSignature1, _IDL _I32);

HL_PRIM int HL_NAME(VertexAttrib_get_mSemantic)( _ref(VertexAttrib)* _this ) {
	return HL_NAME(ShaderSemantic_valueToIndex0)(_unref(_this)->mSemantic);
}
DEFINE_PRIM(_I32,VertexAttrib_get_mSemantic,_IDL);
HL_PRIM int HL_NAME(VertexAttrib_set_mSemantic)( _ref(VertexAttrib)* _this, int value ) {
	_unref(_this)->mSemantic = (ShaderSemantic)HL_NAME(ShaderSemantic_indexToValue0)(value);
	return value;
}
DEFINE_PRIM(_I32,VertexAttrib_set_mSemantic,_IDL _I32);

HL_PRIM unsigned int HL_NAME(VertexAttrib_get_mSemanticNameLength)( _ref(VertexAttrib)* _this ) {
	return _unref(_this)->mSemanticNameLength;
}
DEFINE_PRIM(_I32,VertexAttrib_get_mSemanticNameLength,_IDL);
HL_PRIM unsigned int HL_NAME(VertexAttrib_set_mSemanticNameLength)( _ref(VertexAttrib)* _this, unsigned int value ) {
	_unref(_this)->mSemanticNameLength = (value);
	return value;
}
DEFINE_PRIM(_I32,VertexAttrib_set_mSemanticNameLength,_IDL _I32);

HL_PRIM void HL_NAME(VertexAttrib_setSemanticName1)(_ref(VertexAttrib)* _this, vstring * name) {
	const char* name__cstr = (name == nullptr) ? "" : hl_to_utf8( name->bytes ); // Should be garbage collected
	(forge_vertex_attrib_set_semantic( _unref(_this) , name__cstr));
}
DEFINE_PRIM(_VOID, VertexAttrib_setSemanticName1, _IDL _STRING);

HL_PRIM int HL_NAME(VertexAttrib_get_mFormat)( _ref(VertexAttrib)* _this ) {
	return HL_NAME(TinyImageFormat_valueToIndex0)(_unref(_this)->mFormat);
}
DEFINE_PRIM(_I32,VertexAttrib_get_mFormat,_IDL);
HL_PRIM int HL_NAME(VertexAttrib_set_mFormat)( _ref(VertexAttrib)* _this, int value ) {
	_unref(_this)->mFormat = (TinyImageFormat)HL_NAME(TinyImageFormat_indexToValue0)(value);
	return value;
}
DEFINE_PRIM(_I32,VertexAttrib_set_mFormat,_IDL _I32);

HL_PRIM unsigned int HL_NAME(VertexAttrib_get_mBinding)( _ref(VertexAttrib)* _this ) {
	return _unref(_this)->mBinding;
}
DEFINE_PRIM(_I32,VertexAttrib_get_mBinding,_IDL);
HL_PRIM unsigned int HL_NAME(VertexAttrib_set_mBinding)( _ref(VertexAttrib)* _this, unsigned int value ) {
	_unref(_this)->mBinding = (value);
	return value;
}
DEFINE_PRIM(_I32,VertexAttrib_set_mBinding,_IDL _I32);

HL_PRIM unsigned int HL_NAME(VertexAttrib_get_mLocation)( _ref(VertexAttrib)* _this ) {
	return _unref(_this)->mLocation;
}
DEFINE_PRIM(_I32,VertexAttrib_get_mLocation,_IDL);
HL_PRIM unsigned int HL_NAME(VertexAttrib_set_mLocation)( _ref(VertexAttrib)* _this, unsigned int value ) {
	_unref(_this)->mLocation = (value);
	return value;
}
DEFINE_PRIM(_I32,VertexAttrib_set_mLocation,_IDL _I32);

HL_PRIM unsigned int HL_NAME(VertexAttrib_get_mOffset)( _ref(VertexAttrib)* _this ) {
	return _unref(_this)->mOffset;
}
DEFINE_PRIM(_I32,VertexAttrib_get_mOffset,_IDL);
HL_PRIM unsigned int HL_NAME(VertexAttrib_set_mOffset)( _ref(VertexAttrib)* _this, unsigned int value ) {
	_unref(_this)->mOffset = (value);
	return value;
}
DEFINE_PRIM(_I32,VertexAttrib_set_mOffset,_IDL _I32);

HL_PRIM int HL_NAME(VertexAttrib_get_mRate)( _ref(VertexAttrib)* _this ) {
	return HL_NAME(VertexAttribRate_valueToIndex0)(_unref(_this)->mRate);
}
DEFINE_PRIM(_I32,VertexAttrib_get_mRate,_IDL);
HL_PRIM int HL_NAME(VertexAttrib_set_mRate)( _ref(VertexAttrib)* _this, int value ) {
	_unref(_this)->mRate = (VertexAttribRate)HL_NAME(VertexAttribRate_indexToValue0)(value);
	return value;
}
DEFINE_PRIM(_I32,VertexAttrib_set_mRate,_IDL _I32);

HL_PRIM _ref(VertexLayout)* HL_NAME(VertexLayout_new0)() {
	return alloc_ref((new VertexLayout()),VertexLayout);
}
DEFINE_PRIM(_IDL, VertexLayout_new0,);

HL_PRIM unsigned int HL_NAME(VertexLayout_get_attribCount)( _ref(VertexLayout)* _this ) {
	return _unref(_this)->mAttribCount;
}
DEFINE_PRIM(_I32,VertexLayout_get_attribCount,_IDL);
HL_PRIM unsigned int HL_NAME(VertexLayout_set_attribCount)( _ref(VertexLayout)* _this, unsigned int value ) {
	_unref(_this)->mAttribCount = (value);
	return value;
}
DEFINE_PRIM(_I32,VertexLayout_set_attribCount,_IDL _I32);

HL_PRIM HL_CONST _ref(VertexAttrib)* HL_NAME(VertexLayout_attrib1)(_ref(VertexLayout)* _this, int idx) {
	return alloc_ref_const((forge_vertex_layout_get_attrib( _unref(_this) , idx)),VertexAttrib);
}
DEFINE_PRIM(_IDL, VertexLayout_attrib1, _IDL _I32);

HL_PRIM unsigned int HL_NAME(VertexLayout_get_strides)( _ref(VertexLayout)* _this, int index ) {
	return _unref(_this)->mStrides[index];
}
DEFINE_PRIM(_I32,VertexLayout_get_strides,_IDL _I32);
HL_PRIM unsigned int HL_NAME(VertexLayout_set_strides)( _ref(VertexLayout)* _this, int index, unsigned int value ) {
	_unref(_this)->mStrides[index] = (value);
	return value;
}
DEFINE_PRIM(_I32,VertexLayout_set_strides,_IDL _I32 _I32); // Array setter

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

HL_PRIM int HL_NAME(RenderTarget_get_sampleCount)( _ref(RenderTarget)* _this ) {
	return HL_NAME(SampleCount_valueToIndex0)(_unref(_this)->mSampleCount);
}
DEFINE_PRIM(_I32,RenderTarget_get_sampleCount,_IDL);
HL_PRIM int HL_NAME(RenderTarget_set_sampleCount)( _ref(RenderTarget)* _this, int value ) {
	_unref(_this)->mSampleCount = (SampleCount)HL_NAME(SampleCount_indexToValue0)(value);
	return value;
}
DEFINE_PRIM(_I32,RenderTarget_set_sampleCount,_IDL _I32);

HL_PRIM unsigned int HL_NAME(RenderTarget_get_sampleQuality)( _ref(RenderTarget)* _this ) {
	return _unref(_this)->mSampleQuality;
}
DEFINE_PRIM(_I32,RenderTarget_get_sampleQuality,_IDL);
HL_PRIM unsigned int HL_NAME(RenderTarget_set_sampleQuality)( _ref(RenderTarget)* _this, unsigned int value ) {
	_unref(_this)->mSampleQuality = (value);
	return value;
}
DEFINE_PRIM(_I32,RenderTarget_set_sampleQuality,_IDL _I32);

HL_PRIM int HL_NAME(RenderTarget_get_format)( _ref(RenderTarget)* _this ) {
	return HL_NAME(TinyImageFormat_valueToIndex0)(_unref(_this)->mFormat);
}
DEFINE_PRIM(_I32,RenderTarget_get_format,_IDL);
HL_PRIM int HL_NAME(RenderTarget_set_format)( _ref(RenderTarget)* _this, int value ) {
	_unref(_this)->mFormat = (TinyImageFormat)HL_NAME(TinyImageFormat_indexToValue0)(value);
	return value;
}
DEFINE_PRIM(_I32,RenderTarget_set_format,_IDL _I32);

HL_PRIM HL_CONST _ref(Shader)* HL_NAME(GraphicsPipelineDesc_get_pShaderProgram)( _ref(GraphicsPipelineDesc)* _this ) {
	return alloc_ref_const(_unref(_this)->pShaderProgram,Shader);
}
DEFINE_PRIM(_IDL,GraphicsPipelineDesc_get_pShaderProgram,_IDL);
HL_PRIM HL_CONST _ref(Shader)* HL_NAME(GraphicsPipelineDesc_set_pShaderProgram)( _ref(GraphicsPipelineDesc)* _this, HL_CONST _ref(Shader)* value ) {
	_unref(_this)->pShaderProgram = _unref(value);
	return value;
}
DEFINE_PRIM(_IDL,GraphicsPipelineDesc_set_pShaderProgram,_IDL _IDL);

HL_PRIM HL_CONST _ref(RootSignature)* HL_NAME(GraphicsPipelineDesc_get_pRootSignature)( _ref(GraphicsPipelineDesc)* _this ) {
	return alloc_ref_const(_unref(_this)->pRootSignature,RootSignature);
}
DEFINE_PRIM(_IDL,GraphicsPipelineDesc_get_pRootSignature,_IDL);
HL_PRIM HL_CONST _ref(RootSignature)* HL_NAME(GraphicsPipelineDesc_set_pRootSignature)( _ref(GraphicsPipelineDesc)* _this, HL_CONST _ref(RootSignature)* value ) {
	_unref(_this)->pRootSignature = _unref(value);
	return value;
}
DEFINE_PRIM(_IDL,GraphicsPipelineDesc_set_pRootSignature,_IDL _IDL);

HL_PRIM HL_CONST _ref(VertexLayout)* HL_NAME(GraphicsPipelineDesc_get_pVertexLayout)( _ref(GraphicsPipelineDesc)* _this ) {
	return alloc_ref_const(_unref(_this)->pVertexLayout,VertexLayout);
}
DEFINE_PRIM(_IDL,GraphicsPipelineDesc_get_pVertexLayout,_IDL);
HL_PRIM HL_CONST _ref(VertexLayout)* HL_NAME(GraphicsPipelineDesc_set_pVertexLayout)( _ref(GraphicsPipelineDesc)* _this, HL_CONST _ref(VertexLayout)* value ) {
	_unref(_this)->pVertexLayout = _unref(value);
	return value;
}
DEFINE_PRIM(_IDL,GraphicsPipelineDesc_set_pVertexLayout,_IDL _IDL);

HL_PRIM HL_CONST _ref(BlendStateDesc)* HL_NAME(GraphicsPipelineDesc_get_pBlendState)( _ref(GraphicsPipelineDesc)* _this ) {
	return alloc_ref_const(_unref(_this)->pBlendState,BlendStateDesc);
}
DEFINE_PRIM(_IDL,GraphicsPipelineDesc_get_pBlendState,_IDL);
HL_PRIM HL_CONST _ref(BlendStateDesc)* HL_NAME(GraphicsPipelineDesc_set_pBlendState)( _ref(GraphicsPipelineDesc)* _this, HL_CONST _ref(BlendStateDesc)* value ) {
	_unref(_this)->pBlendState = _unref(value);
	return value;
}
DEFINE_PRIM(_IDL,GraphicsPipelineDesc_set_pBlendState,_IDL _IDL);

HL_PRIM HL_CONST _ref(DepthStateDesc)* HL_NAME(GraphicsPipelineDesc_get_pDepthState)( _ref(GraphicsPipelineDesc)* _this ) {
	return alloc_ref_const(_unref(_this)->pDepthState,DepthStateDesc);
}
DEFINE_PRIM(_IDL,GraphicsPipelineDesc_get_pDepthState,_IDL);
HL_PRIM HL_CONST _ref(DepthStateDesc)* HL_NAME(GraphicsPipelineDesc_set_pDepthState)( _ref(GraphicsPipelineDesc)* _this, HL_CONST _ref(DepthStateDesc)* value ) {
	_unref(_this)->pDepthState = _unref(value);
	return value;
}
DEFINE_PRIM(_IDL,GraphicsPipelineDesc_set_pDepthState,_IDL _IDL);

HL_PRIM HL_CONST _ref(RasterizerStateDesc)* HL_NAME(GraphicsPipelineDesc_get_pRasterizerState)( _ref(GraphicsPipelineDesc)* _this ) {
	return alloc_ref_const(_unref(_this)->pRasterizerState,RasterizerStateDesc);
}
DEFINE_PRIM(_IDL,GraphicsPipelineDesc_get_pRasterizerState,_IDL);
HL_PRIM HL_CONST _ref(RasterizerStateDesc)* HL_NAME(GraphicsPipelineDesc_set_pRasterizerState)( _ref(GraphicsPipelineDesc)* _this, HL_CONST _ref(RasterizerStateDesc)* value ) {
	_unref(_this)->pRasterizerState = _unref(value);
	return value;
}
DEFINE_PRIM(_IDL,GraphicsPipelineDesc_set_pRasterizerState,_IDL _IDL);

HL_PRIM unsigned int HL_NAME(GraphicsPipelineDesc_get_mRenderTargetCount)( _ref(GraphicsPipelineDesc)* _this ) {
	return _unref(_this)->mRenderTargetCount;
}
DEFINE_PRIM(_I32,GraphicsPipelineDesc_get_mRenderTargetCount,_IDL);
HL_PRIM unsigned int HL_NAME(GraphicsPipelineDesc_set_mRenderTargetCount)( _ref(GraphicsPipelineDesc)* _this, unsigned int value ) {
	_unref(_this)->mRenderTargetCount = (value);
	return value;
}
DEFINE_PRIM(_I32,GraphicsPipelineDesc_set_mRenderTargetCount,_IDL _I32);

HL_PRIM int HL_NAME(GraphicsPipelineDesc_get_sampleCount)( _ref(GraphicsPipelineDesc)* _this ) {
	return HL_NAME(SampleCount_valueToIndex0)(_unref(_this)->mSampleCount);
}
DEFINE_PRIM(_I32,GraphicsPipelineDesc_get_sampleCount,_IDL);
HL_PRIM int HL_NAME(GraphicsPipelineDesc_set_sampleCount)( _ref(GraphicsPipelineDesc)* _this, int value ) {
	_unref(_this)->mSampleCount = (SampleCount)HL_NAME(SampleCount_indexToValue0)(value);
	return value;
}
DEFINE_PRIM(_I32,GraphicsPipelineDesc_set_sampleCount,_IDL _I32);

HL_PRIM unsigned int HL_NAME(GraphicsPipelineDesc_get_sampleQuality)( _ref(GraphicsPipelineDesc)* _this ) {
	return _unref(_this)->mSampleQuality;
}
DEFINE_PRIM(_I32,GraphicsPipelineDesc_get_sampleQuality,_IDL);
HL_PRIM unsigned int HL_NAME(GraphicsPipelineDesc_set_sampleQuality)( _ref(GraphicsPipelineDesc)* _this, unsigned int value ) {
	_unref(_this)->mSampleQuality = (value);
	return value;
}
DEFINE_PRIM(_I32,GraphicsPipelineDesc_set_sampleQuality,_IDL _I32);

HL_PRIM int HL_NAME(GraphicsPipelineDesc_get_depthStencilFormat)( _ref(GraphicsPipelineDesc)* _this ) {
	return HL_NAME(TinyImageFormat_valueToIndex0)(_unref(_this)->mDepthStencilFormat);
}
DEFINE_PRIM(_I32,GraphicsPipelineDesc_get_depthStencilFormat,_IDL);
HL_PRIM int HL_NAME(GraphicsPipelineDesc_set_depthStencilFormat)( _ref(GraphicsPipelineDesc)* _this, int value ) {
	_unref(_this)->mDepthStencilFormat = (TinyImageFormat)HL_NAME(TinyImageFormat_indexToValue0)(value);
	return value;
}
DEFINE_PRIM(_I32,GraphicsPipelineDesc_set_depthStencilFormat,_IDL _I32);

HL_PRIM int HL_NAME(GraphicsPipelineDesc_get_mPrimitiveTopo)( _ref(GraphicsPipelineDesc)* _this ) {
	return HL_NAME(PrimitiveTopology_valueToIndex0)(_unref(_this)->mPrimitiveTopo);
}
DEFINE_PRIM(_I32,GraphicsPipelineDesc_get_mPrimitiveTopo,_IDL);
HL_PRIM int HL_NAME(GraphicsPipelineDesc_set_mPrimitiveTopo)( _ref(GraphicsPipelineDesc)* _this, int value ) {
	_unref(_this)->mPrimitiveTopo = (PrimitiveTopology)HL_NAME(PrimitiveTopology_indexToValue0)(value);
	return value;
}
DEFINE_PRIM(_I32,GraphicsPipelineDesc_set_mPrimitiveTopo,_IDL _I32);

HL_PRIM bool HL_NAME(GraphicsPipelineDesc_get_mSupportIndirectCommandBuffer)( _ref(GraphicsPipelineDesc)* _this ) {
	return _unref(_this)->mSupportIndirectCommandBuffer;
}
DEFINE_PRIM(_BOOL,GraphicsPipelineDesc_get_mSupportIndirectCommandBuffer,_IDL);
HL_PRIM bool HL_NAME(GraphicsPipelineDesc_set_mSupportIndirectCommandBuffer)( _ref(GraphicsPipelineDesc)* _this, bool value ) {
	_unref(_this)->mSupportIndirectCommandBuffer = (value);
	return value;
}
DEFINE_PRIM(_BOOL,GraphicsPipelineDesc_set_mSupportIndirectCommandBuffer,_IDL _BOOL);

HL_PRIM bool HL_NAME(GraphicsPipelineDesc_get_mVRFoveatedRendering)( _ref(GraphicsPipelineDesc)* _this ) {
	return _unref(_this)->mVRFoveatedRendering;
}
DEFINE_PRIM(_BOOL,GraphicsPipelineDesc_get_mVRFoveatedRendering,_IDL);
HL_PRIM bool HL_NAME(GraphicsPipelineDesc_set_mVRFoveatedRendering)( _ref(GraphicsPipelineDesc)* _this, bool value ) {
	_unref(_this)->mVRFoveatedRendering = (value);
	return value;
}
DEFINE_PRIM(_BOOL,GraphicsPipelineDesc_set_mVRFoveatedRendering,_IDL _BOOL);

HL_PRIM _ref(HlForgePipelineDesc)* HL_NAME(PipelineDesc_new0)() {
	return alloc_ref((new HlForgePipelineDesc()),PipelineDesc);
}
DEFINE_PRIM(_IDL, PipelineDesc_new0,);

HL_PRIM HL_CONST _ref(GraphicsPipelineDesc)* HL_NAME(PipelineDesc_graphicsPipeline0)(_ref(HlForgePipelineDesc)* _this) {
	return alloc_ref_const((_unref(_this)->graphicsPipeline()),GraphicsPipelineDesc);
}
DEFINE_PRIM(_IDL, PipelineDesc_graphicsPipeline0, _IDL);

HL_PRIM _ref(PipelineCache)* HL_NAME(PipelineDesc_get_pCache)( _ref(HlForgePipelineDesc)* _this ) {
	return alloc_ref(_unref(_this)->pCache,PipelineCache);
}
DEFINE_PRIM(_IDL,PipelineDesc_get_pCache,_IDL);
HL_PRIM _ref(PipelineCache)* HL_NAME(PipelineDesc_set_pCache)( _ref(HlForgePipelineDesc)* _this, _ref(PipelineCache)* value ) {
	_unref(_this)->pCache = _unref(value);
	return value;
}
DEFINE_PRIM(_IDL,PipelineDesc_set_pCache,_IDL _IDL);

HL_PRIM unsigned int HL_NAME(PipelineDesc_get_mExtensionCount)( _ref(HlForgePipelineDesc)* _this ) {
	return _unref(_this)->mExtensionCount;
}
DEFINE_PRIM(_I32,PipelineDesc_get_mExtensionCount,_IDL);
HL_PRIM unsigned int HL_NAME(PipelineDesc_set_mExtensionCount)( _ref(HlForgePipelineDesc)* _this, unsigned int value ) {
	_unref(_this)->mExtensionCount = (value);
	return value;
}
DEFINE_PRIM(_I32,PipelineDesc_set_mExtensionCount,_IDL _I32);

HL_PRIM void HL_NAME(PipelineDesc_setName1)(_ref(HlForgePipelineDesc)* _this, vstring * name) {
	const char* name__cstr = (name == nullptr) ? "" : hl_to_utf8( name->bytes ); // Should be garbage collected
	(_unref(_this)->setName(name__cstr));
}
DEFINE_PRIM(_VOID, PipelineDesc_setName1, _IDL _STRING);

HL_PRIM int HL_NAME(PipelineDesc_addGraphicsRenderTarget1)(_ref(HlForgePipelineDesc)* _this, _ref(RenderTarget)* rt) {
	return (_unref(_this)->addGraphicsRenderTarget(_unref_ptr_safe(rt)));
}
DEFINE_PRIM(_I32, PipelineDesc_addGraphicsRenderTarget1, _IDL _IDL);

HL_PRIM _ref(BufferBinder)* HL_NAME(BufferBinder_new0)() {
	return alloc_ref((new BufferBinder()),BufferBinder);
}
DEFINE_PRIM(_IDL, BufferBinder_new0,);

HL_PRIM void HL_NAME(BufferBinder_reset0)(_ref(BufferBinder)* _this) {
	(_unref(_this)->reset());
}
DEFINE_PRIM(_VOID, BufferBinder_reset0, _IDL);

HL_PRIM int HL_NAME(BufferBinder_add3)(_ref(BufferBinder)* _this, _ref(Buffer)* buf, int stride, int offset) {
	return (_unref(_this)->add(_unref_ptr_safe(buf), stride, offset));
}
DEFINE_PRIM(_I32, BufferBinder_add3, _IDL _IDL _I32 _I32);

HL_PRIM void HL_NAME(Cmd_clear2)(_ref(Cmd)* _this, _ref(RenderTarget)* rt, _ref(RenderTarget)* depthstencil) {
	(forge_render_target_clear( _unref(_this) , _unref_ptr_safe(rt), _unref_ptr_safe(depthstencil)));
}
DEFINE_PRIM(_VOID, Cmd_clear2, _IDL _IDL _IDL);

HL_PRIM void HL_NAME(Cmd_unbindRenderTarget0)(_ref(Cmd)* _this) {
	(forge_cmd_unbind( _unref(_this) ));
}
DEFINE_PRIM(_VOID, Cmd_unbindRenderTarget0, _IDL);

HL_PRIM void HL_NAME(Cmd_begin0)(_ref(Cmd)* _this) {
	(beginCmd( _unref(_this) ));
}
DEFINE_PRIM(_VOID, Cmd_begin0, _IDL);

HL_PRIM void HL_NAME(Cmd_end0)(_ref(Cmd)* _this) {
	(endCmd( _unref(_this) ));
}
DEFINE_PRIM(_VOID, Cmd_end0, _IDL);

HL_PRIM void HL_NAME(Cmd_pushConstants3)(_ref(Cmd)* _this, _ref(RootSignature)* rs, int index, vbyte* data) {
	(cmdBindPushConstants( _unref(_this) , _unref_ptr_safe(rs), index, data));
}
DEFINE_PRIM(_VOID, Cmd_pushConstants3, _IDL _IDL _I32 _BYTES);

HL_PRIM void HL_NAME(Cmd_bindIndexBuffer3)(_ref(Cmd)* _this, _ref(Buffer)* b, int format, int offset) {
	(cmdBindIndexBuffer( _unref(_this) , _unref_ptr_safe(b), IndexType__values[format], offset));
}
DEFINE_PRIM(_VOID, Cmd_bindIndexBuffer3, _IDL _IDL _I32 _I32);

HL_PRIM void HL_NAME(Cmd_bindVertexBuffer1)(_ref(Cmd)* _this, _ref(BufferBinder)* binder) {
	(BufferBinder::bind( _unref(_this) , _unref_ptr_safe(binder)));
}
DEFINE_PRIM(_VOID, Cmd_bindVertexBuffer1, _IDL _IDL);

HL_PRIM void HL_NAME(Cmd_drawIndexed3)(_ref(Cmd)* _this, int vertCount, unsigned int first_index, unsigned int first_vertex) {
	(cmdDrawIndexed( _unref(_this) , vertCount, first_index, first_vertex));
}
DEFINE_PRIM(_VOID, Cmd_drawIndexed3, _IDL _I32 _I32 _I32);

HL_PRIM void HL_NAME(Cmd_bindPipeline1)(_ref(Cmd)* _this, _ref(Pipeline)* pipeline) {
	(cmdBindPipeline( _unref(_this) , _unref_ptr_safe(pipeline)));
}
DEFINE_PRIM(_VOID, Cmd_bindPipeline1, _IDL _IDL);

HL_PRIM void HL_NAME(Cmd_bindDescriptorSet2)(_ref(Cmd)* _this, int index, _ref(DescriptorSet)* set) {
	(cmdBindDescriptorSet( _unref(_this) , index, _unref_ptr_safe(set)));
}
DEFINE_PRIM(_VOID, Cmd_bindDescriptorSet2, _IDL _I32 _IDL);

HL_PRIM void HL_NAME(Cmd_renderBarrier1)(_ref(Cmd)* _this, _ref(RenderTarget)* rt) {
	(forge_cmd_wait_for_render( _unref(_this) , _unref_ptr_safe(rt)));
}
DEFINE_PRIM(_VOID, Cmd_renderBarrier1, _IDL _IDL);

HL_PRIM void HL_NAME(Cmd_presentBarrier1)(_ref(Cmd)* _this, _ref(RenderTarget)* rt) {
	(forge_cmd_wait_for_present( _unref(_this) , _unref_ptr_safe(rt)));
}
DEFINE_PRIM(_VOID, Cmd_presentBarrier1, _IDL _IDL);

HL_PRIM _ref(Map64Int)* HL_NAME(Map64Int_new0)() {
	return alloc_ref((new Map64Int()),Map64Int);
}
DEFINE_PRIM(_IDL, Map64Int_new0,);

HL_PRIM bool HL_NAME(Map64Int_exists1)(_ref(Map64Int)* _this, int64_t key) {
	return (_unref(_this)->exists(key));
}
DEFINE_PRIM(_BOOL, Map64Int_exists1, _IDL _I64);

HL_PRIM void HL_NAME(Map64Int_set2)(_ref(Map64Int)* _this, int64_t key, int value) {
	(_unref(_this)->set(key, value));
}
DEFINE_PRIM(_VOID, Map64Int_set2, _IDL _I64 _I32);

HL_PRIM int HL_NAME(Map64Int_get1)(_ref(Map64Int)* _this, int64_t key) {
	return (_unref(_this)->get(key));
}
DEFINE_PRIM(_I32, Map64Int_get1, _IDL _I64);

HL_PRIM int HL_NAME(Map64Int_size0)(_ref(Map64Int)* _this) {
	return (_unref(_this)->size());
}
DEFINE_PRIM(_I32, Map64Int_size0, _IDL);

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

HL_PRIM bool HL_NAME(SwapChain_isVSync0)(_ref(SwapChain)* _this) {
	return (isVSync( _unref(_this) ));
}
DEFINE_PRIM(_BOOL, SwapChain_isVSync0, _IDL);

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

HL_PRIM int HL_NAME(RootSignature_getDescriptorIndexFromName1)(_ref(RootSignature)* _this, vstring * name) {
	const char* name__cstr = (name == nullptr) ? "" : hl_to_utf8( name->bytes ); // Should be garbage collected
	auto ___retvalue = (getDescriptorIndexFromName( _unref(_this) , name__cstr));
	return ___retvalue;
}
DEFINE_PRIM(_I32, RootSignature_getDescriptorIndexFromName1, _IDL _STRING);

HL_PRIM _ref(SamplerDesc)* HL_NAME(SamplerDesc_new0)() {
	return alloc_ref((new SamplerDesc()),SamplerDesc);
}
DEFINE_PRIM(_IDL, SamplerDesc_new0,);

HL_PRIM int HL_NAME(SamplerDesc_get_mMinFilter)( _ref(SamplerDesc)* _this ) {
	return HL_NAME(FilterType_valueToIndex0)(_unref(_this)->mMinFilter);
}
DEFINE_PRIM(_I32,SamplerDesc_get_mMinFilter,_IDL);
HL_PRIM int HL_NAME(SamplerDesc_set_mMinFilter)( _ref(SamplerDesc)* _this, int value ) {
	_unref(_this)->mMinFilter = (FilterType)HL_NAME(FilterType_indexToValue0)(value);
	return value;
}
DEFINE_PRIM(_I32,SamplerDesc_set_mMinFilter,_IDL _I32);

HL_PRIM int HL_NAME(SamplerDesc_get_mMagFilter)( _ref(SamplerDesc)* _this ) {
	return HL_NAME(FilterType_valueToIndex0)(_unref(_this)->mMagFilter);
}
DEFINE_PRIM(_I32,SamplerDesc_get_mMagFilter,_IDL);
HL_PRIM int HL_NAME(SamplerDesc_set_mMagFilter)( _ref(SamplerDesc)* _this, int value ) {
	_unref(_this)->mMagFilter = (FilterType)HL_NAME(FilterType_indexToValue0)(value);
	return value;
}
DEFINE_PRIM(_I32,SamplerDesc_set_mMagFilter,_IDL _I32);

HL_PRIM int HL_NAME(SamplerDesc_get_mMipMapMode)( _ref(SamplerDesc)* _this ) {
	return HL_NAME(MipMapMode_valueToIndex0)(_unref(_this)->mMipMapMode);
}
DEFINE_PRIM(_I32,SamplerDesc_get_mMipMapMode,_IDL);
HL_PRIM int HL_NAME(SamplerDesc_set_mMipMapMode)( _ref(SamplerDesc)* _this, int value ) {
	_unref(_this)->mMipMapMode = (MipMapMode)HL_NAME(MipMapMode_indexToValue0)(value);
	return value;
}
DEFINE_PRIM(_I32,SamplerDesc_set_mMipMapMode,_IDL _I32);

HL_PRIM int HL_NAME(SamplerDesc_get_mAddressU)( _ref(SamplerDesc)* _this ) {
	return HL_NAME(AddressMode_valueToIndex0)(_unref(_this)->mAddressU);
}
DEFINE_PRIM(_I32,SamplerDesc_get_mAddressU,_IDL);
HL_PRIM int HL_NAME(SamplerDesc_set_mAddressU)( _ref(SamplerDesc)* _this, int value ) {
	_unref(_this)->mAddressU = (AddressMode)HL_NAME(AddressMode_indexToValue0)(value);
	return value;
}
DEFINE_PRIM(_I32,SamplerDesc_set_mAddressU,_IDL _I32);

HL_PRIM int HL_NAME(SamplerDesc_get_mAddressV)( _ref(SamplerDesc)* _this ) {
	return HL_NAME(AddressMode_valueToIndex0)(_unref(_this)->mAddressV);
}
DEFINE_PRIM(_I32,SamplerDesc_get_mAddressV,_IDL);
HL_PRIM int HL_NAME(SamplerDesc_set_mAddressV)( _ref(SamplerDesc)* _this, int value ) {
	_unref(_this)->mAddressV = (AddressMode)HL_NAME(AddressMode_indexToValue0)(value);
	return value;
}
DEFINE_PRIM(_I32,SamplerDesc_set_mAddressV,_IDL _I32);

HL_PRIM int HL_NAME(SamplerDesc_get_mAddressW)( _ref(SamplerDesc)* _this ) {
	return HL_NAME(AddressMode_valueToIndex0)(_unref(_this)->mAddressW);
}
DEFINE_PRIM(_I32,SamplerDesc_get_mAddressW,_IDL);
HL_PRIM int HL_NAME(SamplerDesc_set_mAddressW)( _ref(SamplerDesc)* _this, int value ) {
	_unref(_this)->mAddressW = (AddressMode)HL_NAME(AddressMode_indexToValue0)(value);
	return value;
}
DEFINE_PRIM(_I32,SamplerDesc_set_mAddressW,_IDL _I32);

HL_PRIM float HL_NAME(SamplerDesc_get_mMipLodBias)( _ref(SamplerDesc)* _this ) {
	return _unref(_this)->mMipLodBias;
}
DEFINE_PRIM(_F32,SamplerDesc_get_mMipLodBias,_IDL);
HL_PRIM float HL_NAME(SamplerDesc_set_mMipLodBias)( _ref(SamplerDesc)* _this, float value ) {
	_unref(_this)->mMipLodBias = (value);
	return value;
}
DEFINE_PRIM(_F32,SamplerDesc_set_mMipLodBias,_IDL _F32);

HL_PRIM bool HL_NAME(SamplerDesc_get_mSetLodRange)( _ref(SamplerDesc)* _this ) {
	return _unref(_this)->mSetLodRange;
}
DEFINE_PRIM(_BOOL,SamplerDesc_get_mSetLodRange,_IDL);
HL_PRIM bool HL_NAME(SamplerDesc_set_mSetLodRange)( _ref(SamplerDesc)* _this, bool value ) {
	_unref(_this)->mSetLodRange = (value);
	return value;
}
DEFINE_PRIM(_BOOL,SamplerDesc_set_mSetLodRange,_IDL _BOOL);

HL_PRIM float HL_NAME(SamplerDesc_get_mMinLod)( _ref(SamplerDesc)* _this ) {
	return _unref(_this)->mMinLod;
}
DEFINE_PRIM(_F32,SamplerDesc_get_mMinLod,_IDL);
HL_PRIM float HL_NAME(SamplerDesc_set_mMinLod)( _ref(SamplerDesc)* _this, float value ) {
	_unref(_this)->mMinLod = (value);
	return value;
}
DEFINE_PRIM(_F32,SamplerDesc_set_mMinLod,_IDL _F32);

HL_PRIM float HL_NAME(SamplerDesc_get_mMaxLod)( _ref(SamplerDesc)* _this ) {
	return _unref(_this)->mMaxLod;
}
DEFINE_PRIM(_F32,SamplerDesc_get_mMaxLod,_IDL);
HL_PRIM float HL_NAME(SamplerDesc_set_mMaxLod)( _ref(SamplerDesc)* _this, float value ) {
	_unref(_this)->mMaxLod = (value);
	return value;
}
DEFINE_PRIM(_F32,SamplerDesc_set_mMaxLod,_IDL _F32);

HL_PRIM float HL_NAME(SamplerDesc_get_mMaxAnisotropy)( _ref(SamplerDesc)* _this ) {
	return _unref(_this)->mMaxAnisotropy;
}
DEFINE_PRIM(_F32,SamplerDesc_get_mMaxAnisotropy,_IDL);
HL_PRIM float HL_NAME(SamplerDesc_set_mMaxAnisotropy)( _ref(SamplerDesc)* _this, float value ) {
	_unref(_this)->mMaxAnisotropy = (value);
	return value;
}
DEFINE_PRIM(_F32,SamplerDesc_set_mMaxAnisotropy,_IDL _F32);

HL_PRIM int HL_NAME(SamplerDesc_get_mCompareFunc)( _ref(SamplerDesc)* _this ) {
	return HL_NAME(CompareMode_valueToIndex0)(_unref(_this)->mCompareFunc);
}
DEFINE_PRIM(_I32,SamplerDesc_get_mCompareFunc,_IDL);
HL_PRIM int HL_NAME(SamplerDesc_set_mCompareFunc)( _ref(SamplerDesc)* _this, int value ) {
	_unref(_this)->mCompareFunc = (CompareMode)HL_NAME(CompareMode_indexToValue0)(value);
	return value;
}
DEFINE_PRIM(_I32,SamplerDesc_set_mCompareFunc,_IDL _I32);

HL_PRIM _ref(DescriptorSetDesc)* HL_NAME(DescriptorSetDesc_new0)() {
	return alloc_ref((new DescriptorSetDesc()),DescriptorSetDesc);
}
DEFINE_PRIM(_IDL, DescriptorSetDesc_new0,);

HL_PRIM HL_CONST _ref(RootSignature)* HL_NAME(DescriptorSetDesc_get_pRootSignature)( _ref(DescriptorSetDesc)* _this ) {
	return alloc_ref_const(_unref(_this)->pRootSignature,RootSignature);
}
DEFINE_PRIM(_IDL,DescriptorSetDesc_get_pRootSignature,_IDL);
HL_PRIM HL_CONST _ref(RootSignature)* HL_NAME(DescriptorSetDesc_set_pRootSignature)( _ref(DescriptorSetDesc)* _this, HL_CONST _ref(RootSignature)* value ) {
	_unref(_this)->pRootSignature = _unref(value);
	return value;
}
DEFINE_PRIM(_IDL,DescriptorSetDesc_set_pRootSignature,_IDL _IDL);

HL_PRIM int HL_NAME(DescriptorSetDesc_get_updateFrequency)( _ref(DescriptorSetDesc)* _this ) {
	return HL_NAME(DescriptorUpdateFrequency_valueToIndex0)(_unref(_this)->mUpdateFrequency);
}
DEFINE_PRIM(_I32,DescriptorSetDesc_get_updateFrequency,_IDL);
HL_PRIM int HL_NAME(DescriptorSetDesc_set_updateFrequency)( _ref(DescriptorSetDesc)* _this, int value ) {
	_unref(_this)->mUpdateFrequency = (DescriptorUpdateFrequency)HL_NAME(DescriptorUpdateFrequency_indexToValue0)(value);
	return value;
}
DEFINE_PRIM(_I32,DescriptorSetDesc_set_updateFrequency,_IDL _I32);

HL_PRIM unsigned int HL_NAME(DescriptorSetDesc_get_maxSets)( _ref(DescriptorSetDesc)* _this ) {
	return _unref(_this)->mMaxSets;
}
DEFINE_PRIM(_I32,DescriptorSetDesc_get_maxSets,_IDL);
HL_PRIM unsigned int HL_NAME(DescriptorSetDesc_set_maxSets)( _ref(DescriptorSetDesc)* _this, unsigned int value ) {
	_unref(_this)->mMaxSets = (value);
	return value;
}
DEFINE_PRIM(_I32,DescriptorSetDesc_set_maxSets,_IDL _I32);

HL_PRIM unsigned int HL_NAME(DescriptorSetDesc_get_nodeIndex)( _ref(DescriptorSetDesc)* _this ) {
	return _unref(_this)->mNodeIndex;
}
DEFINE_PRIM(_I32,DescriptorSetDesc_get_nodeIndex,_IDL);
HL_PRIM unsigned int HL_NAME(DescriptorSetDesc_set_nodeIndex)( _ref(DescriptorSetDesc)* _this, unsigned int value ) {
	_unref(_this)->mNodeIndex = (value);
	return value;
}
DEFINE_PRIM(_I32,DescriptorSetDesc_set_nodeIndex,_IDL _I32);

HL_PRIM _ref(DescriptorDataBuilder)* HL_NAME(DescriptorDataBuilder_new0)() {
	return alloc_ref((new DescriptorDataBuilder()),DescriptorDataBuilder);
}
DEFINE_PRIM(_IDL, DescriptorDataBuilder_new0,);

HL_PRIM void HL_NAME(DescriptorDataBuilder_clear0)(_ref(DescriptorDataBuilder)* _this) {
	(_unref(_this)->clear());
}
DEFINE_PRIM(_VOID, DescriptorDataBuilder_clear0, _IDL);

HL_PRIM void HL_NAME(DescriptorDataBuilder_clearSlotData1)(_ref(DescriptorDataBuilder)* _this, int index) {
	(_unref(_this)->clearSlotData(index));
}
DEFINE_PRIM(_VOID, DescriptorDataBuilder_clearSlotData1, _IDL _I32);

HL_PRIM int HL_NAME(DescriptorDataBuilder_addSlot2)(_ref(DescriptorDataBuilder)* _this, vstring * name, int type) {
	const char* name__cstr = (name == nullptr) ? "" : hl_to_utf8( name->bytes ); // Should be garbage collected
	auto ___retvalue = (_unref(_this)->addSlot(name__cstr, DescriptorType__values[type]));
	return ___retvalue;
}
DEFINE_PRIM(_I32, DescriptorDataBuilder_addSlot2, _IDL _STRING _I32);

HL_PRIM void HL_NAME(DescriptorDataBuilder_addSlotTexture2)(_ref(DescriptorDataBuilder)* _this, int slot, _ref(Texture)* tex) {
	(_unref(_this)->addSlotData(slot, _unref_ptr_safe(tex)));
}
DEFINE_PRIM(_VOID, DescriptorDataBuilder_addSlotTexture2, _IDL _I32 _IDL);

HL_PRIM void HL_NAME(DescriptorDataBuilder_addSlotSampler2)(_ref(DescriptorDataBuilder)* _this, int slot, _ref(Sampler)* sampler) {
	(_unref(_this)->addSlotData(slot, _unref_ptr_safe(sampler)));
}
DEFINE_PRIM(_VOID, DescriptorDataBuilder_addSlotSampler2, _IDL _I32 _IDL);

HL_PRIM void HL_NAME(DescriptorDataBuilder_update3)(_ref(DescriptorDataBuilder)* _this, _ref(Renderer)* renderer, int index, _ref(DescriptorSet)* set) {
	(_unref(_this)->update(_unref_ptr_safe(renderer), index, _unref_ptr_safe(set)));
}
DEFINE_PRIM(_VOID, DescriptorDataBuilder_update3, _IDL _IDL _I32 _IDL);

HL_PRIM void HL_NAME(DescriptorDataBuilder_bind3)(_ref(DescriptorDataBuilder)* _this, _ref(Cmd)* cmd, int index, _ref(DescriptorSet)* set) {
	(_unref(_this)->bind(_unref_ptr_safe(cmd), index, _unref_ptr_safe(set)));
}
DEFINE_PRIM(_VOID, DescriptorDataBuilder_bind3, _IDL _IDL _I32 _IDL);

HL_PRIM _ref(RootSignatureFactory)* HL_NAME(RootSignatureDesc_new0)() {
	return alloc_ref((new RootSignatureFactory()),RootSignatureDesc);
}
DEFINE_PRIM(_IDL, RootSignatureDesc_new0,);

HL_PRIM void HL_NAME(RootSignatureDesc_addShader1)(_ref(RootSignatureFactory)* _this, _ref(Shader)* shader) {
	(_unref(_this)->addShader(_unref_ptr_safe(shader)));
}
DEFINE_PRIM(_VOID, RootSignatureDesc_addShader1, _IDL _IDL);

HL_PRIM void HL_NAME(RootSignatureDesc_addSampler2)(_ref(RootSignatureFactory)* _this, _ref(Sampler)* sampler, vstring * name) {
	const char* name__cstr = (name == nullptr) ? "" : hl_to_utf8( name->bytes ); // Should be garbage collected
	(_unref(_this)->addSampler(_unref_ptr_safe(sampler), name__cstr));
}
DEFINE_PRIM(_VOID, RootSignatureDesc_addSampler2, _IDL _IDL _STRING);

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

HL_PRIM _ref(DescriptorSet)* HL_NAME(Renderer_addDescriptorSet2)(_ref(Renderer)* _this, _ref(DescriptorSetDesc)* desc) {
	DescriptorSet* __tmpret;
addDescriptorSet( _unref(_this) , _unref_ptr_safe(desc), &__tmpret);
	return alloc_ref_const( __tmpret, DescriptorSet );
}
DEFINE_PRIM(_IDL, Renderer_addDescriptorSet2, _IDL _IDL);

HL_PRIM HL_CONST _ref(DescriptorSet)* HL_NAME(Renderer_createDescriptorSet4)(_ref(Renderer)* _this, _ref(RootSignature)* sig, int updateFrequency, unsigned int maxSets, unsigned int nodeIndex) {
	return alloc_ref_const((forge_renderer_create_descriptor_set( _unref(_this) , _unref_ptr_safe(sig), DescriptorUpdateFrequency__values[updateFrequency], maxSets, nodeIndex)),DescriptorSet);
}
DEFINE_PRIM(_IDL, Renderer_createDescriptorSet4, _IDL _IDL _I32 _I32 _I32);

HL_PRIM _ref(Sampler)* HL_NAME(Renderer_createSampler2)(_ref(Renderer)* _this, _ref(SamplerDesc)* desc) {
	Sampler* __tmpret;
addSampler( _unref(_this) , _unref_ptr_safe(desc), &__tmpret);
	return alloc_ref_const( __tmpret, Sampler );
}
DEFINE_PRIM(_IDL, Renderer_createSampler2, _IDL _IDL);

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

HL_PRIM _ref(Pipeline)* HL_NAME(Renderer_createPipeline2)(_ref(Renderer)* _this, _ref(HlForgePipelineDesc)* desc) {
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

HL_PRIM void HL_NAME(Renderer_toggleVSync1)(_ref(Renderer)* _this, _ref(SwapChain)* sc) {
	(::toggleVSync( _unref(_this) , &_unref(sc)));
}
DEFINE_PRIM(_VOID, Renderer_toggleVSync1, _IDL _IDL);

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

HL_PRIM void HL_NAME(Buffer_dispose0)(_ref(Buffer)* _this) {
	(removeResource( _unref(_this) ));
}
DEFINE_PRIM(_VOID, Buffer_dispose0, _IDL);

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
