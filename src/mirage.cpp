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

#define HL_NAME(x) mirage_##x
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

#include <mirage/mirage.h>

#include "hl-mirage.h"





extern "C" {

static void finalize_PointField2Df( _ref(mirage::PointField<cinolib::vec2d>)* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(PointField2Df_delete)( _ref(mirage::PointField<cinolib::vec2d>)* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, PointField2Df_delete, _IDL);
static void finalize_CellLibrary( _ref(mirage::CellLibrary)* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(CellLibrary_delete)( _ref(mirage::CellLibrary)* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, CellLibrary_delete, _IDL);
static void finalize_Trimesh( _ref(cinolib::Trimesh<>)* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(Trimesh_delete)( _ref(cinolib::Trimesh<>)* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, Trimesh_delete, _IDL);
static void finalize_QuadrangulateSettings( _ref(mirage::QuadrangulateSettings)* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(QuadrangulateSettings_delete)( _ref(mirage::QuadrangulateSettings)* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, QuadrangulateSettings_delete, _IDL);
static void finalize_CellMesh( _ref(mirage::CellMesh<>)* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(CellMesh_delete)( _ref(mirage::CellMesh<>)* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, CellMesh_delete, _IDL);
static void finalize_VolumizeSettings( _ref(mirage::VolumizeSettings)* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(VolumizeSettings_delete)( _ref(mirage::VolumizeSettings)* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, VolumizeSettings_delete, _IDL);
static void finalize_CellPolyhedraMesh( _ref(mirage::CellPolyhedraMesh)* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(CellPolyhedraMesh_delete)( _ref(mirage::CellPolyhedraMesh)* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, CellPolyhedraMesh_delete, _IDL);
static void finalize_Polygonmesh( _ref(cinolib::Polygonmesh<>)* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(Polygonmesh_delete)( _ref(cinolib::Polygonmesh<>)* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, Polygonmesh_delete, _IDL);
static void finalize_IndexedTriMesh( _ref(mirage::IndexedTriMesh<>)* _this ) { free_ref(_this ); }
HL_PRIM void HL_NAME(IndexedTriMesh_delete)( _ref(mirage::IndexedTriMesh<>)* _this ) {
	free_ref(_this );
}
DEFINE_PRIM(_VOID, IndexedTriMesh_delete, _IDL);
HL_PRIM _ref(mirage::PointField<cinolib::vec2d>)* HL_NAME(PointField2Df_new0)() {
	return alloc_ref((new mirage::PointField<cinolib::vec2d>()),PointField2Df);
}
DEFINE_PRIM(_IDL, PointField2Df_new0,);

HL_PRIM _ref(mirage::CellLibrary)* HL_NAME(CellLibrary_new0)() {
	return alloc_ref((new mirage::CellLibrary()),CellLibrary);
}
DEFINE_PRIM(_IDL, CellLibrary_new0,);

HL_PRIM bool HL_NAME(CellLibrary_load1)(_ref(mirage::CellLibrary)* _this, vstring * path) {
	const char* path__cstr = (path == nullptr) ? "" : hl_to_utf8( path->bytes ); // Should be garbage collected
	auto ___retvalue = (_unref(_this)->load(path__cstr));
	return ___retvalue;
}
DEFINE_PRIM(_BOOL, CellLibrary_load1, _IDL _STRING);

HL_PRIM bool HL_NAME(CellLibrary_filter1)(_ref(mirage::CellLibrary)* _this, vstring * path) {
	const char* path__cstr = (path == nullptr) ? "" : hl_to_utf8( path->bytes ); // Should be garbage collected
	auto ___retvalue = (_unref(_this)->filter(path__cstr));
	return ___retvalue;
}
DEFINE_PRIM(_BOOL, CellLibrary_filter1, _IDL _STRING);

HL_PRIM int HL_NAME(CellLibrary_getTagID1)(_ref(mirage::CellLibrary)* _this, vstring * tag) {
	const char* tag__cstr = (tag == nullptr) ? "" : hl_to_utf8( tag->bytes ); // Should be garbage collected
	auto ___retvalue = (_unref(_this)->getTagID(tag__cstr));
	return ___retvalue;
}
DEFINE_PRIM(_I32, CellLibrary_getTagID1, _IDL _STRING);

HL_PRIM void HL_NAME(Noise_poissonGrid6)(_ref(mirage::PointField<cinolib::vec2d>)* pf, double radius, _hl_double2* min, _hl_double2* max, unsigned int seed, int max_iterations) {
	(mirage::Noise::poisson_grid(*_unref(pf), radius, *(cinolib::vec2d *)(double*)min, *(cinolib::vec2d *)(double*)max, seed, max_iterations));
}
DEFINE_PRIM(_VOID, Noise_poissonGrid6, _IDL _F64 _STRUCT _STRUCT _I32 _I32);

HL_PRIM _ref(cinolib::Trimesh<>)* HL_NAME(Trimesh_new0)() {
	return alloc_ref((new cinolib::Trimesh<>()),Trimesh);
}
DEFINE_PRIM(_IDL, Trimesh_new0,);

HL_PRIM float HL_NAME(Trimesh_edgeAvgLength0)(_ref(cinolib::Trimesh<>)* _this) {
	return (_unref(_this)->edge_avg_length());
}
DEFINE_PRIM(_F32, Trimesh_edgeAvgLength0, _IDL);

HL_PRIM _ref(mirage::QuadrangulateSettings)* HL_NAME(QuadrangulateSettings_new0)() {
	return alloc_ref((new mirage::QuadrangulateSettings()),QuadrangulateSettings);
}
DEFINE_PRIM(_IDL, QuadrangulateSettings_new0,);

HL_PRIM bool HL_NAME(QuadrangulateSettings_get_pureQuad)( _ref(mirage::QuadrangulateSettings)* _this ) {
	return _unref(_this)->pure_quad;
}
DEFINE_PRIM(_BOOL,QuadrangulateSettings_get_pureQuad,_IDL);
HL_PRIM bool HL_NAME(QuadrangulateSettings_set_pureQuad)( _ref(mirage::QuadrangulateSettings)* _this, bool value ) {
	_unref(_this)->pure_quad = (value);
	return value;
}
DEFINE_PRIM(_BOOL,QuadrangulateSettings_set_pureQuad,_IDL _BOOL);

HL_PRIM bool HL_NAME(QuadrangulateSettings_get_alignToBoundaries)( _ref(mirage::QuadrangulateSettings)* _this ) {
	return _unref(_this)->align_to_boundaries;
}
DEFINE_PRIM(_BOOL,QuadrangulateSettings_get_alignToBoundaries,_IDL);
HL_PRIM bool HL_NAME(QuadrangulateSettings_set_alignToBoundaries)( _ref(mirage::QuadrangulateSettings)* _this, bool value ) {
	_unref(_this)->align_to_boundaries = (value);
	return value;
}
DEFINE_PRIM(_BOOL,QuadrangulateSettings_set_alignToBoundaries,_IDL _BOOL);

HL_PRIM int HL_NAME(QuadrangulateSettings_get_smoothIter)( _ref(mirage::QuadrangulateSettings)* _this ) {
	return _unref(_this)->smooth_iter;
}
DEFINE_PRIM(_I32,QuadrangulateSettings_get_smoothIter,_IDL);
HL_PRIM int HL_NAME(QuadrangulateSettings_set_smoothIter)( _ref(mirage::QuadrangulateSettings)* _this, int value ) {
	_unref(_this)->smooth_iter = (value);
	return value;
}
DEFINE_PRIM(_I32,QuadrangulateSettings_set_smoothIter,_IDL _I32);

HL_PRIM bool HL_NAME(QuadrangulateSettings_get_extrinsic)( _ref(mirage::QuadrangulateSettings)* _this ) {
	return _unref(_this)->extrinsic;
}
DEFINE_PRIM(_BOOL,QuadrangulateSettings_get_extrinsic,_IDL);
HL_PRIM bool HL_NAME(QuadrangulateSettings_set_extrinsic)( _ref(mirage::QuadrangulateSettings)* _this, bool value ) {
	_unref(_this)->extrinsic = (value);
	return value;
}
DEFINE_PRIM(_BOOL,QuadrangulateSettings_set_extrinsic,_IDL _BOOL);

HL_PRIM _ref(mirage::CellMesh<>)* HL_NAME(CellMesh_new0)() {
	return alloc_ref((new mirage::CellMesh<>()),CellMesh);
}
DEFINE_PRIM(_IDL, CellMesh_new0,);

HL_PRIM _ref(mirage::VolumizeSettings)* HL_NAME(VolumizeSettings_new0)() {
	return alloc_ref((new mirage::VolumizeSettings()),VolumizeSettings);
}
DEFINE_PRIM(_IDL, VolumizeSettings_new0,);

HL_PRIM double HL_NAME(VolumizeSettings_get_cellHeight)( _ref(mirage::VolumizeSettings)* _this ) {
	return _unref(_this)->cellHeight;
}
DEFINE_PRIM(_F64,VolumizeSettings_get_cellHeight,_IDL);
HL_PRIM double HL_NAME(VolumizeSettings_set_cellHeight)( _ref(mirage::VolumizeSettings)* _this, double value ) {
	_unref(_this)->cellHeight = (value);
	return value;
}
DEFINE_PRIM(_F64,VolumizeSettings_set_cellHeight,_IDL _F64);

HL_PRIM int HL_NAME(VolumizeSettings_get_verticalCellCount)( _ref(mirage::VolumizeSettings)* _this ) {
	return _unref(_this)->verticalCellCount;
}
DEFINE_PRIM(_I32,VolumizeSettings_get_verticalCellCount,_IDL);
HL_PRIM int HL_NAME(VolumizeSettings_set_verticalCellCount)( _ref(mirage::VolumizeSettings)* _this, int value ) {
	_unref(_this)->verticalCellCount = (value);
	return value;
}
DEFINE_PRIM(_I32,VolumizeSettings_set_verticalCellCount,_IDL _I32);

HL_PRIM void HL_NAME(VolumizeSettings_addBottomTag3)(_ref(mirage::VolumizeSettings)* _this, unsigned int tag, bool provides, bool requires) {
	(_unref(_this)->addBottomTag(tag, provides, requires));
}
DEFINE_PRIM(_VOID, VolumizeSettings_addBottomTag3, _IDL _I32 _BOOL _BOOL);

HL_PRIM void HL_NAME(VolumizeSettings_addSideTag3)(_ref(mirage::VolumizeSettings)* _this, unsigned int tag, bool provides, bool requires) {
	(_unref(_this)->addSideTag(tag, provides, requires));
}
DEFINE_PRIM(_VOID, VolumizeSettings_addSideTag3, _IDL _I32 _BOOL _BOOL);

HL_PRIM void HL_NAME(VolumizeSettings_addTopTag3)(_ref(mirage::VolumizeSettings)* _this, unsigned int tag, bool provides, bool requires) {
	(_unref(_this)->addTopTag(tag, provides, requires));
}
DEFINE_PRIM(_VOID, VolumizeSettings_addTopTag3, _IDL _I32 _BOOL _BOOL);

HL_PRIM _ref(mirage::CellPolyhedraMesh)* HL_NAME(CellPolyhedraMesh_new0)() {
	return alloc_ref((new mirage::CellPolyhedraMesh()),CellPolyhedraMesh);
}
DEFINE_PRIM(_IDL, CellPolyhedraMesh_new0,);

HL_PRIM void HL_NAME(Surfacing_pointsToTrimesh2)(_ref(mirage::PointField<cinolib::vec2d>)* pf, _ref(cinolib::Trimesh<>)* out_mesh) {
	(mirage::Surfacing::points_to_trimesh(*_unref(pf), *_unref(out_mesh)));
}
DEFINE_PRIM(_VOID, Surfacing_pointsToTrimesh2, _IDL _IDL);

HL_PRIM void HL_NAME(Surfacing_quadrangulateTrimesh3)(_ref(cinolib::Trimesh<>)* triMeshInput, _ref(mirage::QuadrangulateSettings)* settings, _ref(mirage::CellMesh<>)* quadMesh) {
	(mirage::Surfacing::quadrangulate_trimesh<>(*_unref(triMeshInput), *_unref(settings), *_unref(quadMesh)));
}
DEFINE_PRIM(_VOID, Surfacing_quadrangulateTrimesh3, _IDL _IDL _IDL);

HL_PRIM void HL_NAME(Translate_pointsToTrimesh2)(_ref(mirage::PointField<cinolib::vec2d>)* pf, _ref(cinolib::Trimesh<>)* out_mesh) {
	(mirage::Surfacing::points_to_trimesh(*_unref(pf), *_unref(out_mesh)));
}
DEFINE_PRIM(_VOID, Translate_pointsToTrimesh2, _IDL _IDL);

HL_PRIM void HL_NAME(Translate_polyMeshToIndexedTriMesh2)(_ref(cinolib::Polygonmesh<>)* pm, _ref(mirage::IndexedTriMesh<>)* idm) {
	(mirage::Translation::cino_to_itm(*_unref(pm), *_unref(idm)));
}
DEFINE_PRIM(_VOID, Translate_polyMeshToIndexedTriMesh2, _IDL _IDL);

HL_PRIM _ref(cinolib::Polygonmesh<>)* HL_NAME(Polygonmesh_new0)() {
	return alloc_ref((new cinolib::Polygonmesh<>()),Polygonmesh);
}
DEFINE_PRIM(_IDL, Polygonmesh_new0,);

HL_PRIM _ref(mirage::IndexedTriMesh<>)* HL_NAME(IndexedTriMesh_new0)() {
	return alloc_ref((new mirage::IndexedTriMesh<>()),IndexedTriMesh);
}
DEFINE_PRIM(_IDL, IndexedTriMesh_new0,);

HL_PRIM int HL_NAME(IndexedTriMesh_vertexCount0)(_ref(mirage::IndexedTriMesh<>)* _this) {
	return (_unref(_this)->vertexCount());
}
DEFINE_PRIM(_I32, IndexedTriMesh_vertexCount0, _IDL);

HL_PRIM int HL_NAME(IndexedTriMesh_triangleCount0)(_ref(mirage::IndexedTriMesh<>)* _this) {
	return (_unref(_this)->triangleCount());
}
DEFINE_PRIM(_I32, IndexedTriMesh_triangleCount0, _IDL);

HL_PRIM void HL_NAME(IndexedTriMesh_getPositions1)(_ref(mirage::IndexedTriMesh<>)* _this, vbyte* data) {
	(_unref(_this)->getPositions((float*)data));
}
DEFINE_PRIM(_VOID, IndexedTriMesh_getPositions1, _IDL _BYTES);

HL_PRIM void HL_NAME(IndexedTriMesh_getVertexNormals1)(_ref(mirage::IndexedTriMesh<>)* _this, vbyte* data) {
	(_unref(_this)->getVertexNormals((float*)data));
}
DEFINE_PRIM(_VOID, IndexedTriMesh_getVertexNormals1, _IDL _BYTES);

HL_PRIM void HL_NAME(IndexedTriMesh_getIndices1)(_ref(mirage::IndexedTriMesh<>)* _this, vbyte* data) {
	(_unref(_this)->getIndices((unsigned int*)data));
}
DEFINE_PRIM(_VOID, IndexedTriMesh_getIndices1, _IDL _BYTES);

HL_PRIM void HL_NAME(Cells_volumizeSockets3)(_ref(mirage::CellMesh<>)* cellSockets, _ref(mirage::VolumizeSettings)* settings, _ref(mirage::CellPolyhedraMesh)* outMesh) {
	(mirage::Cells::volumize_sockets(*_unref(cellSockets), *_unref(settings), *_unref(outMesh)));
}
DEFINE_PRIM(_VOID, Cells_volumizeSockets3, _IDL _IDL _IDL);

HL_PRIM void HL_NAME(Cells_seedCellPossibilities2)(_ref(mirage::CellPolyhedraMesh)* cellSockets, _ref(mirage::CellLibrary)* lib) {
	(mirage::Cells::seedCellPossibilities(*_unref(cellSockets), *_unref(lib)));
}
DEFINE_PRIM(_VOID, Cells_seedCellPossibilities2, _IDL _IDL);

HL_PRIM void HL_NAME(Cells_solve_cells2)(_ref(mirage::CellPolyhedraMesh)* cellSockets, _ref(mirage::CellLibrary)* lib) {
	(mirage::Cells::solve_cells(*_unref(cellSockets), *_unref(lib)));
}
DEFINE_PRIM(_VOID, Cells_solve_cells2, _IDL _IDL);

HL_PRIM void HL_NAME(Cells_buildWorldMesh3D4)(_ref(mirage::CellPolyhedraMesh)* cellSockets, _ref(mirage::CellLibrary)* cellLib, float cellHeight, _ref(cinolib::Polygonmesh<>)* worldMesh) {
	(mirage::Cells::buildWorldMesh3D(*_unref(cellSockets), *_unref(cellLib), cellHeight, *_unref(worldMesh)));
}
DEFINE_PRIM(_VOID, Cells_buildWorldMesh3D4, _IDL _IDL _F32 _IDL);

HL_PRIM void HL_NAME(Cells_heuristicTest1)(vclosure* fnptr) {
		if (fnptr->hasValue) hl_error("Only static callbacks supported");
(heuristicTest((void (*)())fnptr->fun));
}
DEFINE_PRIM(_VOID, Cells_heuristicTest1, _FUN(_VOID,_NO_ARG));

HL_PRIM void HL_NAME(Cells_heuristicTest21)(vclosure* fnptr) {
		if (fnptr->hasValue) hl_error("Only static callbacks supported");
(heuristicTest2((float (*)(int))fnptr->fun));
}
DEFINE_PRIM(_VOID, Cells_heuristicTest21, _FUN(_F32,_I32));

}
