#ifndef __HL_FORGE_H_
#define __HL_FORGE_H_
#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <fp16.h>
#include <xxhash.h>

#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#ifndef SDL_MAJOR_VERSION
#error "SDL2 SDK not found in hl/include/sdl/"
#endif

#include <meshoptimizer/src/meshoptimizer.h>

#define TWIN _ABSTRACT(sdl_window)

void heuristicTest(void (*fn)());
void heuristicTest2(float (*fn)(int));

bool hlForgeInitialize(const char *name);

#include <Renderer/IRenderer.h>
#include <Renderer/IResourceLoader.h>

class ForgeSDLWindow {
   public:
    ForgeSDLWindow(SDL_Window *window);
    SwapChain *createSwapChain(Renderer *renderer, Queue *queue, int width, int height, int chainCount, bool hdr10);
    SDL_Window *window;
    SDL_SysWMinfo wmInfo;
    Renderer *renderer() { return _renderer; }
    Renderer *_renderer;
    SDL_MetalView _view;
    CAMetalLayer *_layer;
    void present(Queue *pGraphicsQueue, SwapChain *pSwapChain, int swapchainImageIndex, Semaphore *pRenderCompleteSemaphore);
};

class HashBuilder {
   private:
    XXH64_state_t _state;

   public:
    HashBuilder() {
        // _state = XXH64_createState();
    }
    ~HashBuilder() {
        // XXH64_freeState(_state);
    }

    void reset(int64_t seed) {
        XXH64_reset(&_state, seed);
    }
    void addInt8(int v) {
        int8_t x = (int8_t)v;
        XXH64_update(&_state, &x, sizeof(int8_t));
    }
    void addInt16(int v) {
        int16_t x = (int16_t)v;
        XXH64_update(&_state, &x, sizeof(int16_t));
    }
    void addInt32(int v) {
        int32_t x = (int32_t)v;
        XXH64_update(&_state, &x, sizeof(int32_t));
    }
    void addInt64(int64_t v) {
        XXH64_update(&_state, &v, sizeof(int64_t));
    }
    void addBytes(const uint8_t *b, int offset, int length) {
        XXH64_update(&_state, &b[offset], length);
    }
    int64 getValue() {
        return XXH64_digest(&_state);
    }
};

class RootSignatureFactory {
   public:
    RootSignatureFactory();
    ~RootSignatureFactory();
    RootSignature *create(Renderer *pRenderer);
    void addShader(Shader *);
    void addSampler(Sampler *sampler, const char *name);
    std::vector<Shader *> _shaders;
    std::vector<Sampler *> _samplers;
    std::vector<std::string> _names;
};

class StateBuilder {
   public:
    StateBuilder() { reset(); }
    ~StateBuilder() {}
    void reset() {
        memset(this, 0, sizeof(StateBuilder));
    }

    void addToHash(HashBuilder *hb) {
        hb->addBytes((uint8_t *)this, 0, sizeof(StateBuilder));
    }
    uint64_t getSignature(int shaderID, RenderTarget *rt, RenderTarget *depth);

    DepthStateDesc _depth;
    RasterizerStateDesc _raster;
    BlendStateDesc _blend;

    inline DepthStateDesc *depth() { return &_depth; }
    inline RasterizerStateDesc *raster() { return &_raster; }
    inline BlendStateDesc *blend() { return &_blend; }
};

class BufferBinder {
    std::vector<Buffer *> _buffers;
    std::vector<uint32_t> _strides;
    std::vector<uint64_t> _offsets;

   public:
    BufferBinder() {}
    ~BufferBinder() {}

    void reset() {
        _buffers.clear();
        _strides.clear();
        _offsets.clear();
    }

    int add(Buffer *b, int stride, int offset) {
        _buffers.push_back(b);
        _strides.push_back(stride);
        _offsets.push_back(offset);
        return _buffers.size() - 1;
    }

    static void bind(Cmd *cmd, BufferBinder *binder) {
        if (binder->_buffers.size() == 0) {
            printf("Warning: binding 0 size buffer\n");
        }
        // these strides don't seem to matter on Metal ENABLE_DRAW_INDEX_BASE_VERTEX_FALLBACK
        cmdBindVertexBuffer(cmd, binder->_buffers.size(), &binder->_buffers[0], &binder->_strides[0], &binder->_offsets[0]);
    }
};

class Map64Int {
   public:
    Map64Int() {}
    ~Map64Int() {}

    std::map<int64_t, int> _map;

    inline bool exists(int64 key) {
        return _map.find(key) != _map.end();
    }
    inline void set(int64 key, int value) {
        //        printf("Signature : Setting key %lld, unsigned %llu n", key, (* ((uint64 *) &key)));

        _map[key] = value;
    }
    inline int get(int64 key) {
        auto x = _map.find(key);
        if (x != _map.end()) {
            return x->second;
        }
        return -1;
    }
    inline int size() {
        return _map.size();
    }
};

class HlForgePipelineDesc : public PipelineDesc {
   public:
    HlForgePipelineDesc() {
        mGraphicsDesc = {};
    }
    inline GraphicsPipelineDesc *graphicsPipeline() {
        this->mType = PIPELINE_TYPE_GRAPHICS;
        return &this->mGraphicsDesc;
    }

    inline ComputePipelineDesc *computePipeline() {
        this->mType = PIPELINE_TYPE_COMPUTE;
        return &this->mComputeDesc;
    }

    void setName(const char *name) {
        _name = name;
        this->pName = _name.c_str();
    }

    void reset() {
        mGraphicsDesc.mRenderTargetCount = 0;
        _formats.clear();
    }
    void setRenderTargetGlobals(SampleCount sampleCount, int sampleQuality) {
        mGraphicsDesc.mSampleCount = sampleCount;
        mGraphicsDesc.mSampleQuality = sampleQuality;
    }

    int addGraphicsRenderTarget(TinyImageFormat format) {
        auto idx = mGraphicsDesc.mRenderTargetCount;
        _formats.push_back(format);
        mGraphicsDesc.pColorFormats = &_formats[0];
        mGraphicsDesc.mRenderTargetCount++;
        return idx;
    }

    std::vector<TinyImageFormat> _formats;
    std::string _name;
};

enum DescriptorSlotMode {
    DBM_TEXTURES,
    DBM_SAMPLERS
};

class DescriptorDataBuilder {
    //    DescriptorSet *_set;
    //    int _index;

    std::vector<std::string> _names;
    std::vector<std::vector<void *> *> _dataPointers;
    std::vector<DescriptorData> _data;
    std::vector<int> _uavMipSlices;
    std::vector<DescriptorSlotMode> _modes;

   public:
    DescriptorDataBuilder() {
    }

    ~DescriptorDataBuilder() {
        clear();
    }
    void clear() {
        for (int i = 0; i < _dataPointers.size(); i++) {
            ::delete _dataPointers[i];
        }
        _dataPointers.clear();
        _names.clear();
        _data.clear();
        _uavMipSlices.clear();
    }

    void clearSlotData(int slot) {
        _dataPointers[slot]->clear();
    }

    int addSlot(const std::string &name, DescriptorSlotMode type) {
        _names.push_back(name);
        _dataPointers.push_back(::new std::vector<void *>());
        _modes.push_back(type);
        DescriptorData d = {};
        _data.push_back(d);
        _uavMipSlices.push_back(0);
        return _names.size() - 1;
    }
    void addSlotData(int slot, void *data) {
        _dataPointers[slot]->push_back(data);
    }
    void setSlotUAVMipSlice(int slot, int idx) {
        _data[slot].mUAVMipSlice = idx;
    }

    void update(Renderer *pRenderer, int index, DescriptorSet *set) {
        for (auto i = 0; i < _names.size(); i++) {
            _data[i].pName = _names[i].c_str();
            switch (_modes[i]) {
                case DBM_TEXTURES:
                    _data[i].ppTextures = (Texture **)(&(*_dataPointers[i])[0]);
                    break;
                case DBM_SAMPLERS:
                    _data[i].ppSamplers = (Sampler **)(&(*_dataPointers[i])[0]);
                    break;
                default:
                    break;
            }
            _data[i].mCount = _dataPointers[i]->size();
        }

        updateDescriptorSet(pRenderer, index, set, _names.size(), &_data[0]);
    }

    void bind(Cmd *cmd, int index, DescriptorSet *set) {
        cmdBindDescriptorSet(cmd, index, set);
    }
};

class ResourceBarrierBuilder {
   private:
    std::vector<RenderTargetBarrier> _rtBarriers;
    std::vector<BufferBarrier> _buffBarriers;
    std::vector<TextureBarrier> _texBarriers;

   public:
    int addRTBarrier(RenderTarget *rt, ResourceState src, ResourceState dst) {
        _rtBarriers.push_back({rt, src, dst});
        return _rtBarriers.size() - 1;
    }

    void insert(Cmd *cmd) {
        RenderTargetBarrier *rtbp = nullptr;
        BufferBarrier *bbp = nullptr;
        TextureBarrier *tbp = nullptr;
        if (_rtBarriers.size()) rtbp = &_rtBarriers[0];
        if (_buffBarriers.size()) bbp = &_buffBarriers[0];
        if (_texBarriers.size()) tbp = &_texBarriers[0];

        cmdResourceBarrier(cmd, _buffBarriers.size(), bbp, _texBarriers.size(), tbp, _rtBarriers.size(), rtbp);
    }
};

enum AttributeSemantic {
    POSITION,
    NORMAL,
    TANGENT,
    BITANGENT,
    UV0,
    UV1,
    UV2,
    UV3,
    UV4,
    UV5,
    UV6,
    UV7,
    COLOR,
    USER0,
    USER1,
    USER2,
    USER3,
    USER4,
    USER5,
    USER6,
    USER7
};

enum AttributeType {
    FLOAT16,
    FLOAT32,
    FLOAT64,
    INT16,
    INT32,
    INT64
};

int inline getAttributeSize(AttributeType t, int dimensions) {
    switch (t) {
        case FLOAT16:
            return sizeof(u_int16_t) * dimensions;
        case FLOAT32:
            return sizeof(float_t) * dimensions;
        case FLOAT64:
            return sizeof(double) * dimensions;
        case INT16:
            return sizeof(u_int16_t) * dimensions;
        case INT32:
            return sizeof(u_int32_t) * dimensions;
        case INT64:
            return sizeof(u_int64_t) * dimensions;
        default:
            return 0;
    }
}

class PolyMesh {
    struct Attr {
        std::string name;
        AttributeSemantic semantic;
        AttributeType type;
        int dimensions;
        int binarySize;
        bool normalized;
        std::vector<u_int8_t> data;

        void set(int idx, double *v, int count) {
            u_int8_t *d = &data[binarySize * idx];

            if (count > dimensions) count = dimensions;
            for (int i = 0; i < count; i++) {
                switch (type) {
                    case FLOAT16:
                        ((u_int16_t *)d)[i] = fp16_ieee_from_fp32_value(v[i]);
                        break;
                    case FLOAT32:
                        ((float_t *)d)[i] = v[i];
                        break;
                    case FLOAT64:
                        ((double *)d)[i] = v[i];
                        break;
                    case INT16:
                        ((u_int16_t *)d)[i] = normalized ? std::clamp(v[i] * (double)std::numeric_limits<std::uint16_t>::max(), 0., (double)std::numeric_limits<std::uint16_t>::max()) : v[i];
                        break;
                    case INT32:
                        ((u_int32_t *)d)[i] = normalized ? v[i] * (double)std::numeric_limits<std::uint32_t>::max() : v[i];
                        break;
                    case INT64:
                        ((u_int64_t *)d)[i] = normalized ? v[i] * (double)std::numeric_limits<std::uint64_t>::max() : v[i];
                        break;
                }
            }
        }

        void set(int idx, float *v, int count) {
            u_int8_t *d = &data[binarySize * idx];

            if (count > dimensions) count = dimensions;
            for (int i = 0; i < count; i++) {
                switch (type) {
                    case FLOAT16:
                        ((u_int16_t *)d)[i] = fp16_ieee_from_fp32_value(v[i]);
                        break;
                    case FLOAT32:
                        ((float_t *)d)[i] = v[i];
                        break;
                    case FLOAT64:
                        ((double *)d)[i] = v[i];
                        break;
                    case INT16:
                        ((u_int16_t *)d)[i] = normalized ? std::clamp(v[i] * (float)std::numeric_limits<std::uint16_t>::max(), 0.0f, (float)std::numeric_limits<std::uint16_t>::max()) : v[i];
                        break;
                    case INT32:
                        ((u_int32_t *)d)[i] = normalized ? v[i] * (float)std::numeric_limits<std::uint32_t>::max() : v[i];
                        break;
                    case INT64:
                        ((u_int64_t *)d)[i] = normalized ? v[i] * (float)std::numeric_limits<std::uint64_t>::max() : v[i];
                        break;
                }
            }
        }
    };
    std::vector<Attr> _attributes;
    std::vector<uint32_t> _polygonCount;
    std::vector<uint32_t> _indices;
    std::vector<uint32_t> _materials;

    bool _begun;
    int _targetCapacity;
    int _currentPolygonPolyNode;
    int _numVerts;

    Attr *getAttributeBySemantic(AttributeSemantic semantic) {
        for (auto i = 0; i < _attributes.size(); i++) {
            if (_attributes[i].semantic == semantic) return &_attributes[i];
        }

        return nullptr;
    }

   public:
    PolyMesh(int targetCapacity = 100) : _begun(false), _targetCapacity(targetCapacity), _numVerts(0), _currentPolygonPolyNode(0) {
    }
    ~PolyMesh() {}
    void reserve(int polynodes) {
        _targetCapacity = polynodes;
        for (auto i = 0; i < _attributes.size(); i++) {
            _attributes[i].data.reserve(_targetCapacity * _attributes[i].binarySize);
        }
    }
    int addAttribute(const std::string &name, AttributeSemantic semantic, AttributeType type, int dimensions, bool normalized = false) {
        auto x = _attributes.size();
        Attr a;
        a.name = name;
        a.semantic = semantic;
        a.type = type;
        a.dimensions = dimensions;
        a.normalized = normalized;
        a.binarySize = getAttributeSize(type, dimensions);
        a.data.resize(a.binarySize * _targetCapacity);
        _attributes.push_back(a);

        return x;
    }
    int beginPolygon(int count, int matID) {
        auto x = _polygonCount.size();
        _polygonCount.push_back(count);
        _materials.push_back(matID);
        _currentPolygonPolyNode = 0;
        return x;
    }

    int beginPolyNode() {
        _indices.push_back(_numVerts);
        return _currentPolygonPolyNode;
    }
    void setNodeAddtribute1f(int attr, float x) {
        auto &a = _attributes[attr];
        a.set(_numVerts, &x, 1);
    }
    void setNodeAddtribute2f(int attr, float x, float y) {
        auto &a = _attributes[attr];
        float v[2];
        v[0] = x;
        v[1] = y;
        a.set(_numVerts, &v[0], 2);
    }
    void setNodeAddtribute3f(int attr, float x, float y, float z) {
        auto &a = _attributes[attr];
        float v[3];
        v[0] = x;
        v[1] = y;
        v[2] = z;
        a.set(_numVerts, &v[0], 3);
    }
    void setNodeAddtribute4f(int attr, float x, float y, float z, float w) {
        auto &a = _attributes[attr];
        float v[4];
        v[0] = x;
        v[1] = y;
        v[2] = z;
        v[3] = w;
        a.set(_numVerts, &v[0], 4);
    }
    void setNodeAddtribute1d(int attr, double x) {}
    void setNodeAddtribute2d(int attr, double x, double y) {}
    void setNodeAddtribute3d(int attr, double x, double y, double z) {}
    void setNodeAddtribute4d(int attr, double x, double y, double z, double w) {}

    int endPolyNode() {
        _numVerts++;
        return _currentPolygonPolyNode++;
    }
    int endPolygon() {
        return _polygonCount.size() - 1;
    }

    int numPolyNodes() {
        auto count = 0;
        for (auto i = 0; i < _polygonCount.size(); i++) {
            count += _polygonCount[i];
        }
        return count;
    }
    int numVerts() {
        return _numVerts;
    }
    int numPolys() {
        return _polygonCount.size();
    }

    struct PMVert {
        PolyMesh *pm;
        int vert;

        struct KeyHash {
            std::size_t operator()(const PMVert &k) const {
                HashBuilder accum;
                accum.reset(0x1729187);
                auto attrCount = k.pm->_attributes.size();
                for (int i = 0; i < attrCount; i++) {
                    const auto &a = k.pm->_attributes[i];
                    accum.addBytes(&a.data[0], a.binarySize * k.vert, a.binarySize);
                }

                return accum.getValue();
            }
        };

        bool operator==(const PMVert &b) const {
            auto attrCount = pm->_attributes.size();
            for (int i = 0; i < attrCount; i++) {
                const auto &aa = pm->_attributes[i];
                const auto &ba = b.pm->_attributes[i];
                if (memcmp(&aa.data[vert * aa.binarySize], &ba.data[b.vert * aa.binarySize], aa.binarySize) != 0) {
                    return false;
                }
            }
            return true;
        }
    };

    void copyVert(int to, int from) {
        auto attrCount = _attributes.size();
        for (int i = 0; i < attrCount; i++) {
            auto &a = _attributes[i];
            memcpy(&a.data[a.binarySize * to], &a.data[a.binarySize * from], a.binarySize);
        }
    }

    void removeDuplicateVerts() {
        // build map
        std::unordered_map<PMVert, int, PMVert::KeyHash> vertMap;

        std::vector<int> compressedIndices;
        std::vector<int> vertIndexRemap;
        vertIndexRemap.reserve(_numVerts);

        for (auto i = 0; i < _numVerts; i++) {
            PMVert v = {this, i};
            auto x = vertMap.find(v);
            if (x == vertMap.end()) {
                // printf("Addign unique vert %d - %d\n", i, uniqueVerts);
                vertMap[v] = compressedIndices.size();
                vertIndexRemap.push_back(compressedIndices.size());
                compressedIndices.push_back(i);
            } else {
                // printf("Reusing vert %d - %d\n", i, x->second);
                vertIndexRemap.push_back(x->second);
            }
        }

        // write vertices back
        for (auto i = 0; i < compressedIndices.size(); i++) {
            copyVert(i, compressedIndices[i]);
        }
        // substitute verts
        for (auto i = 0; i < _indices.size(); i++) {
            _indices[i] = vertIndexRemap[_indices[i]];
        }

        // reduce the vert count
        _numVerts = compressedIndices.size();
    }

    void convexTriangulate() {
        std::vector<uint32_t> newPolyCount;  // Will be all 3's
        std::vector<uint32_t> triangleIndices;

        auto readHead = 0;

        for (auto i = 0; i < _polygonCount.size(); i++) {
            for (auto t = 2; t < _polygonCount[i]; t++) {
                newPolyCount.push_back(3);

                // printf("intial vert of %d %d is %d - rh %d\n", i, t, _indices[readHead], readHead);
                //  Do a triangle Fan, only works for convex
                triangleIndices.push_back(_indices[readHead]);
                triangleIndices.push_back(_indices[readHead + t - 1]);
                triangleIndices.push_back(_indices[readHead + t]);
            }
            readHead += _polygonCount[i];
        }

        // copy polycount back
        _polygonCount = std::move(newPolyCount);
        _indices = std::move(triangleIndices);
    }

    void optimizeTriangleIndices() {
        /**
         * Vertex transform cache optimizer
         * Reorders indices to reduce the number of GPU vertex shader invocations
         * If index buffer contains multiple ranges for multiple draw calls, this functions needs to be called on each range individually.
         *
         * destination must contain enough space for the resulting index buffer (index_count elements)
         */
        std::vector<u_int32_t> optIndices;
        optIndices.resize(_indices.size());

        meshopt_optimizeVertexCache(&optIndices[0], &_indices[0], _indices.size(), _numVerts);

        _indices = std::move(optIndices);
    }

    bool optimizeTriangleIndicesForOverdraw() {
        std::vector<u_int32_t> optIndices;
        optIndices.resize(_indices.size());

        auto *a = getAttributeBySemantic(POSITION);

        if (a != nullptr) {
            if (a->type == FLOAT32 && a->dimensions == 3) {
                meshopt_optimizeOverdraw(&optIndices[0], &_indices[0], _indices.size(), (float *)&a->data[0], _numVerts, a->binarySize, 1.05f);

                _indices = std::move(optIndices);
                return true;
            }
        }
        return false;
    }

    int getIndices(int *indices) {
        memcpy(indices, &_indices[0], sizeof(uint32_t) * _indices.size());
        return _indices.size();
    }
    void optimizeTriangleMemoryFetch() {
        std::vector<u_int32_t> remapIndices;
        remapIndices.resize(_indices.size());

        meshopt_optimizeVertexFetchRemap(&remapIndices[0], &_indices[0], _indices.size(), _numVerts);

        for (int i = 0; i < _attributes.size(); i++) {
            auto &a = _attributes[i];
            std::vector<uint8_t> tmp;
            tmp.resize( _numVerts * a.binarySize);

            meshopt_remapVertexBuffer(&tmp[0], &a.data[0], _numVerts, a.binarySize, &remapIndices[0]);
            a.data = std::move(tmp);
        }

        std::vector<u_int32_t> updatedIndices;
        updatedIndices.resize(_indices.size());

        meshopt_remapIndexBuffer(&updatedIndices[0], &_indices[0], _indices.size(), &remapIndices[0]);

        _indices = std::move(updatedIndices);
    }
};

// SDL interface
SDL_Window *forge_sdl_get_window(void *ptr);
SDL_MetalView forge_create_metal_view(SDL_Window *);

// Swap Chain
RenderTarget *forge_swap_chain_get_render_target(SwapChain *, int);
inline bool isVSync(SwapChain *sc) {
    return sc->mEnableVsync != 0;
}

// Renderer
Renderer *createRenderer(const char *name);
void destroyRenderer(Renderer *);
Queue *createQueue(Renderer *renderer);
DescriptorSet *forge_renderer_create_descriptor_set(Renderer *, RootSignature *, DescriptorUpdateFrequency updateFrequency, uint maxSets, uint nodeIndex);
RenderTarget *forge_sdl_create_render_target(Renderer *, RenderTargetDesc *);
CmdPool *forge_sdl_renderer_create_cmd_pool(Renderer *, Queue *);
Cmd *forge_sdl_renderer_create_cmd(Renderer *, CmdPool *);
Fence *forge_sdl_renderer_create_fence(Renderer *);
Semaphore *forge_sdl_renderer_create_semaphore(Renderer *);
void forge_init_loader(Renderer *);
void forge_renderer_wait_fence(Renderer *, Fence *);
Shader *forge_renderer_shader_create(Renderer *pRenderer, const char *vertFile, const char *fragFile);
RootSignature *forge_renderer_createRootSignatureSimple(Renderer *pRenderer, Shader *shader);
RootSignature *forge_renderer_createRootSignature(Renderer *pRenderer, RootSignatureFactory *);
Shader *forge_load_compute_shader_file(Renderer *pRenderer, const char *fileName);
Buffer *forge_create_transfer_buffer(Renderer *rp, TinyImageFormat format, int width, int height, int nodeIndex);
bool forge_render_target_capture_2(Renderer *pRenderer, Cmd *pCmd, RenderTarget *pRenderTarget, Queue *pQueue, ResourceState renderTargetCurrentState, uint8_t *alloc, int bufferSize);

// Tools
std::string forge_translate_glsl_metal(const char *source, const char *filepath, bool fragment);
void hl_compile_metal_to_bin(const char *fileName, const char *outFile);

// Queue
void forge_queue_submit_cmd(Queue *queue, Cmd *cmd, Semaphore *signalSemphor, Semaphore *wait, Fence *signalFence);

// Buffer Load
void forge_sdl_buffer_load_desc_set_index_buffer(BufferLoadDesc *bld, int size, void *data, bool shared);
void forge_sdl_buffer_load_desc_set_vertex_buffer(BufferLoadDesc *bld, int size, void *data, bool shared);
Buffer *forge_sdl_buffer_load(BufferLoadDesc *bld, SyncToken *token);
// Buffer
void forge_sdl_buffer_update(Buffer *buffer, void *data);
void forge_sdl_buffer_update_region(Buffer *buffer, void *data, int toffset, int size, int soffset);
inline unsigned char *forge_buffer_get_cpu_address(Buffer *buffer) {
    return (unsigned char *)(buffer->pCpuMappedAddress);
}

// Cmd
void forge_cmd_unbind(Cmd *);
void forge_cmd_push_constant(Cmd *cmd, RootSignature *rs, int index, void *data);
void forge_render_target_bind(Cmd *cmd, RenderTarget *mainRT, RenderTarget *depthStencil, LoadActionType color, LoadActionType depth);
void forge_cmd_wait_for_render(Cmd *cmd, RenderTarget *pRenderTarget);
void forge_cmd_wait_for_present(Cmd *cmd, RenderTarget *pRenderTarget);
void forge_cmd_insert_barrier(Cmd *cmd, ResourceBarrierBuilder *barrier);

// Texture Load
Texture *forge_texture_load(TextureLoadDesc *desc, SyncToken *token);
void forge_texture_set_file_name(TextureLoadDesc *desc, const char *path);
void forge_texture_upload_mip(Texture *tex, int mip, void *data, int dataSize);
void forge_texture_upload_layer_mip(Texture *tex, int layer, int mip, void *data, int dataSize);

// Texture Desc
Texture *forge_texture_load_from_desc(TextureDesc *tdesc, const char *name, SyncToken *token);

// Texture
void forge_sdl_texture_upload(Texture *, void *data, int dataSize);

// Render Target
Texture *forge_render_target_get_texture(RenderTarget *rt);
void forge_render_target_set_clear_colour(RenderTarget *rt, float r, float g, float b, float a);
void forge_render_target_set_clear_depth(RenderTarget *rt, float depth, int stencil);
void forge_render_target_capture(RenderTarget *rt, Buffer *pTransferBuffer, Semaphore *semaphore);
int forge_render_target_capture_size(RenderTarget *pRenderTarget);

// Blend State
void forge_blend_state_desc_set_rt(BlendStateDesc *, BlendStateTargets rt, bool enabled);

// Vertex Layout
VertexAttrib *forge_vertex_layout_get_attrib(VertexLayout *, int idx);

// Vertex Attrib
void forge_vertex_attrib_set_semantic(VertexAttrib *, const char *name);

#endif
