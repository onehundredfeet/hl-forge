#ifndef __HL_FORGE_H_
#define __HL_FORGE_H_
#pragma once
#if __APPLE__
#include <Foundation/Foundation.h>
#include <IGraphics.h>
#include <IResourceLoader.h>







void heuristicTest(void (*fn)());
void heuristicTest2(float (*fn)(int));


#endif // APPLE


#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#ifndef SDL_MAJOR_VERSION
#error "SDL2 SDK not found"
#endif

#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include <fp16.h>
#include <algorithm>

#include <meshoptimizer/src/meshoptimizer.h>

//#include <Renderer/IRenderer.h>
//#include <Renderer/IResourceLoader.h>

//#define XXHASH_EXPOSE_STATE
#define XXH_STATIC_LINKING_ONLY
#include <xxhash.h>

#define TWIN _ABSTRACT(sdl_window)

#define DEBUG_PRINT(...) printf(__VA_ARGS__)
//#define DEBUG_PRINT(...)

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

bool hlForgeInitialize(const char *name);
class ForgeSDLWindow {
   public:
    ForgeSDLWindow(SDL_Window *window);
    SwapChain *createSwapChain(Renderer *renderer, Queue *queue, int width, int height, int chainCount, bool hdr10);
    SDL_Window *window;
    SDL_SysWMinfo wmInfo;
    Renderer *renderer() { return _renderer; }
    Renderer *_renderer;
    #if __APPLE__
    SDL_MetalView _view;
    CAMetalLayer *_layer;
    #endif
    void present(Queue *pGraphicsQueue, SwapChain *pSwapChain, int swapchainImageIndex, Semaphore *pRenderCompleteSemaphore);
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

class BufferExt {
   public:
    inline BufferExt() : _idx(0) {}
    inline ~BufferExt() {}

    int _idx;
    std::vector<Buffer *> _buffers;

    inline Buffer *current() {
        return _buffers[_idx];
    }

    inline int next() {
        _idx = (_idx + 1) % _buffers.size();
        return _idx;
    }

    inline void setCurrent(int idx) {
        _idx = idx % _buffers.size();
    }

    inline void updateRegion(void *data, int toffset, int size, int soffset) {
        BufferUpdateDesc desc = {};
        desc.pBuffer = current();
        desc.mDstOffset = toffset;
        desc.mSize = size;
        beginUpdateResource(&desc);
        memcpy(desc.pMappedData, &((char *)data)[soffset], size);
        endUpdateResource(&desc, NULL);
    }

    inline void update(void *data) {
        BufferUpdateDesc desc = {};
        desc.pBuffer = current();
        beginUpdateResource(&desc);
        memcpy(desc.pMappedData, data, desc.pBuffer->mSize);
        endUpdateResource(&desc, NULL);
    }

    void dispose();

    inline unsigned char *getCpuAddress() {
        return (unsigned char *)(current()->pCpuMappedAddress);
    }

    inline 	int getSize() {
        return current()->mSize;
    }


    inline int currentIdx() {
        return _idx;
    }

    static void bindAsIndex(Cmd *cmd, BufferExt *b, IndexType it, int offset);
};

enum DescriptorSlotMode {
    DBM_TEXTURES,
    DBM_SAMPLERS,
    DBM_UNIFORMS
};

class BufferLoadDescExt : public BufferLoadDesc {
   public:
    BufferLoadDescExt();
    ~BufferLoadDescExt();

    void setIndexBuffer(int size, void *data) {
        mDesc.mDescriptors = DESCRIPTOR_TYPE_INDEX_BUFFER;
        mDesc.mSize = size;
        pData = data;
    }
    void setVertexBuffer(int size, void *data) {
        mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
        mDesc.mSize = size;
        pData = data;
        //
    }
    void setUniformBuffer(int size, void *data) {
        mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        mDesc.mSize = size;
        pData = data;
    }

    void setUsage(bool shared) {
        if (shared) {
            mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
        } else {
            mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
        }
    }

    int _depth;

    void setDynamic(int depth) {
        _depth = depth;
    }

    BufferExt *load(SyncToken *token) {
        BufferExt *ext = new BufferExt();
        for (int i = 0; i < _depth; i++) {
            Buffer *tmp = nullptr;
            ppBuffer = &tmp;

            if (i == _depth - 1)
                addResource(this, token); // this may not work, but I don't want to make multiple sync tokens
            else
                addResource(this, nullptr);

            ext->_buffers.push_back(tmp);
        }
        ext->setCurrent(0);
        return ext;
    }
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
    std::vector<BufferExt *> _buffers;
    std::vector<Buffer *> _buffersTmp;
    std::vector<uint32_t> _strides;
    std::vector<uint64_t> _offsets;

   public:
    BufferBinder() {}
    ~BufferBinder() {}

    void reset() {
        _buffers.clear();
        _strides.clear();
        _offsets.clear();
        _buffersTmp.clear();
    }

    int add(BufferExt *b, int stride, int offset) {
        _buffers.push_back(b);
        _buffersTmp.push_back(nullptr);
        _strides.push_back(stride);
        _offsets.push_back(offset);
        return (int)(_buffers.size() - 1);
    }

    static void bindAsVertex(Cmd *cmd, BufferBinder *This) {
        if (This->_buffers.size() == 0) {
            printf("Warning: binding 0 size buffer\n");
        }
        for (int i = 0; i < This->_buffers.size(); i++) {
            This->_buffersTmp[i] = This->_buffers[i]->current();
        }
        // these strides don't seem to matter on Metal ENABLE_DRAW_INDEX_BASE_VERTEX_FALLBACK
        cmdBindVertexBuffer(cmd, (int)This->_buffers.size(), &This->_buffersTmp[0], &This->_strides[0], &This->_offsets[0]);
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
        *static_cast<PipelineDesc *>(this) = {};


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

    int addSlot(DescriptorSlotMode type) {
        _names.push_back("");
        _dataPointers.push_back(::new std::vector<void *>());
        _modes.push_back(type);
        DescriptorData d = {};
        _data.push_back(d);
        _uavMipSlices.push_back(0);
        return _names.size() - 1;
    }

    void setSlotBindName(int slot, const std::string &name) {
        _names[slot] = name;
        _data[slot].mBindByIndex = false;
    }
    void setSlotBindIndex(int slot, int index) {
        _data[slot].mIndex = index;
        _data[slot].mBindByIndex = true;
    }


    void addSlotData(int slot, Texture *data) {
        _dataPointers[slot]->push_back(data);
    }
    void addSlotData(int slot, Sampler *data) {
        _dataPointers[slot]->push_back(data);
    }
    void addSlotData(int slot, BufferExt *bp) {
        _dataPointers[slot]->push_back(bp->current());
    }
    void addSlotData(int slot, Buffer *bp) {
        _dataPointers[slot]->push_back(bp);
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
                case DBM_UNIFORMS:
                    _data[i].ppBuffers = (Buffer **)(&(*_dataPointers[i])[0]);
                    break;
                default:
                    break;
            }
            _data[i].mCount = (unsigned int)_dataPointers[i]->size();
        }

        updateDescriptorSet(pRenderer, index, set, (unsigned int)_names.size(), &_data[0]);
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
        if (rt != nullptr) {
            printf("render target %p - texture %p\n", rt, rt->pTexture);
            if (rt->pTexture ==(void *)(0xdeadbeefdeadbeef)) {
                printf("Texture is invalid on reder target");
                exit(-1);
            }
        }
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

        if (_rtBarriers.size()) {
            RenderTarget *rt = rtbp[0].pRenderTarget;
            if (rt->pTexture ==(void *)(0xdeadbeefdeadbeef)) {
                 printf("render target %p - texture %p\n", rt, rt->pTexture);
                printf("Texture is invalid on reder target");
                exit(-1);
            }
        }
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
    ATTR_FLOAT16,
    ATTR_FLOAT32,
    ATTR_FLOAT64,
    ATTR_UINT8,
    ATTR_UINT16,
    ATTR_UINT32,
    ATTR_UINT64
};

int inline getAttributeSize(AttributeType t, int dimensions) {
    switch (t) {
        case ATTR_FLOAT16:
            return sizeof(uint16_t) * dimensions;
        case ATTR_FLOAT32:
            return sizeof(float_t) * dimensions;
        case ATTR_FLOAT64:
            return sizeof(double) * dimensions;
        case ATTR_UINT8:
            return sizeof(uint8_t) * dimensions;
        case ATTR_UINT16:
            return sizeof(uint16_t) * dimensions;
        case ATTR_UINT32:
            return sizeof(uint32_t) * dimensions;
        case ATTR_UINT64:
            return sizeof(uint64_t) * dimensions;
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
        std::vector<uint8_t> data;

        void set(int idx, double *v, int count) {
            auto offset = binarySize * idx;

            if (offset > data.size()) {
                printf("ERROR: Index %d is out of bounds\n", idx);
            }
            uint8_t *d = &data[offset];

            if (count > dimensions) count = dimensions;
            for (int i = 0; i < count; i++) {
                switch (type) {
                    case ATTR_FLOAT16:
                        ((uint16_t *)d)[i] = fp16_ieee_from_fp32_value(v[i]);
                        break;
                    case ATTR_FLOAT32:
                        ((float_t *)d)[i] = v[i];
                        break;
                    case ATTR_FLOAT64:
                        ((double *)d)[i] = v[i];
                        break;
                    case ATTR_UINT8:
                        ((uint8_t *)d)[i] = normalized ? std::clamp(v[i] * (double)std::numeric_limits<std::uint8_t>::max(), 0., (double)std::numeric_limits<std::uint8_t>::max()) : v[i];
                        break;
                    case ATTR_UINT16:
                        ((uint16_t *)d)[i] = normalized ? std::clamp(v[i] * (double)std::numeric_limits<std::uint16_t>::max(), 0., (double)std::numeric_limits<std::uint16_t>::max()) : v[i];
                        break;
                    case ATTR_UINT32:
                        ((uint32_t *)d)[i] = (uint32_t)(normalized ? v[i] * (double)std::numeric_limits<std::uint32_t>::max() : v[i]);
                        break;
                    case ATTR_UINT64:
                        ((uint64_t *)d)[i] = normalized ? v[i] * (double)std::numeric_limits<std::uint64_t>::max() : v[i];
                        break;
                }
            }
        }

        void set(int idx, float *v, int count) {
            uint8_t *d = &data[binarySize * idx];

            if (count > dimensions) count = dimensions;
            for (int i = 0; i < count; i++) {
                switch (type) {
                    case ATTR_FLOAT16:
                        ((uint16_t *)d)[i] = fp16_ieee_from_fp32_value(v[i]);
                        break;
                    case ATTR_FLOAT32:
                        ((float_t *)d)[i] = v[i];
                        break;
                    case ATTR_FLOAT64:
                        ((double *)d)[i] = v[i];
                        break;
                    case ATTR_UINT8:
                        ((uint8_t *)d)[i] = normalized ? std::clamp(v[i] * (float)std::numeric_limits<std::uint8_t>::max(), 0.0f, (float)std::numeric_limits<std::uint8_t>::max()) : v[i];
                        break;
                    case ATTR_UINT16:
                        ((uint16_t *)d)[i] = normalized ? std::clamp(v[i] * (float)std::numeric_limits<std::uint16_t>::max(), 0.0f, (float)std::numeric_limits<std::uint16_t>::max()) : v[i];
                        break;
                    case ATTR_UINT32:
                        ((uint32_t *)d)[i] = normalized ? v[i] * (float)std::numeric_limits<std::uint32_t>::max() : v[i];
                        break;
                    case ATTR_UINT64:
                        ((uint64_t *)d)[i] = normalized ? v[i] * (float)std::numeric_limits<std::uint64_t>::max() : v[i];
                        break;
                }
            }
        }
    };
    std::vector<Attr> _attributes;
    std::vector<uint32_t> _polygonCount;
    std::vector<uint32_t> _indices;

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
        // printf("CREATING POLYMESH\n");
    }
    ~PolyMesh() {
        // printf("DELETING POLY MESH\n");
    }
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
    int beginPolygon(int count) {
        auto x = _polygonCount.size();
        _polygonCount.push_back(count);
        _currentPolygonPolyNode = 0;
        return x;
    }

    int beginPolyNode() {
        _indices.push_back(_numVerts);

        for (int i = 0; i < _attributes.size(); i++) {
            auto &a = _attributes[i];
            a.data.resize(a.data.size() + a.binarySize);
        }

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
        std::vector<uint32_t> optIndices;
        optIndices.resize(_indices.size());

        meshopt_optimizeVertexCache(&optIndices[0], &_indices[0], _indices.size(), _numVerts);

        _indices = std::move(optIndices);
    }

    bool optimizeTriangleIndicesForOverdraw() {
        std::vector<uint32_t> optIndices;
        optIndices.resize(_indices.size());

        auto *a = getAttributeBySemantic(POSITION);

        if (a != nullptr) {
            if (a->type == ATTR_FLOAT32 && a->dimensions == 3) {
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
        std::vector<uint32_t> remapIndices;
        remapIndices.resize(_indices.size());

        meshopt_optimizeVertexFetchRemap(&remapIndices[0], &_indices[0], _indices.size(), _numVerts);

        for (int i = 0; i < _attributes.size(); i++) {
            auto &a = _attributes[i];
            std::vector<uint8_t> tmp;
            tmp.resize(_numVerts * a.binarySize);

            meshopt_remapVertexBuffer(&tmp[0], &a.data[0], _numVerts, a.binarySize, &remapIndices[0]);
            a.data = std::move(tmp);
        }

        std::vector<uint32_t> updatedIndices;
        updatedIndices.resize(_indices.size());

        meshopt_remapIndexBuffer(&updatedIndices[0], &_indices[0], _indices.size(), &remapIndices[0]);

        _indices = std::move(updatedIndices);
    }

    int getStride() {
        auto accum = 0;
        for (int i = 0; i < _attributes.size(); i++) {
            accum += _attributes[i].binarySize;
        }
        return accum;
    }

    int getAttributeIndexBySemantic(AttributeSemantic semantic) {
        for (int i = 0; i < _attributes.size(); i++) {
            auto &a = _attributes[i];
            if (a.semantic == semantic) return i;
        }

        return -1;
    }

    int getAttributeOffset(int index) {
        auto accum = 0;
        for (int i = 0; i < index; i++) {
            auto &a = _attributes[i];
            accum += a.binarySize;
        }

        return accum;
    }

    AttributeSemantic getAttributeSemantic(int index) {
        return _attributes[index].semantic;
    }
    AttributeType getAttributeType(int index) {
        return _attributes[index].type;
    }
    int getAttributeDimensions(int index) {
        return _attributes[index].dimensions;
    }

    int numAttributes() {
        return _attributes.size();
    }

    const std::string &getAttributeName(int index) {
        return _attributes[index].name;
    }

    void getInterleavedVertices(void *ptr) {
        uint8_t *data = (uint8_t *)ptr;

        for (int v = 0; v < _numVerts; v++) {
            for (int i = 0; i < _attributes.size(); i++) {
                auto &a = _attributes[i];
                memcpy(data, &a.data[v * a.binarySize], a.binarySize);
                data += a.binarySize;
            }
        }
    }
};

#if __APPLE__
// SDL interface
SDL_Window *forge_sdl_get_window(void *ptr);
SDL_MetalView forge_create_metal_view(SDL_Window *);
#endif

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
void forge_renderer_destroySwapChain(Renderer *pRenderer, SwapChain *swapChain);
void forge_renderer_destroyRenderTarget(Renderer *pRenderer, RenderTarget *rt);
void forge_renderer_fill_descriptor_set(Renderer *pRenderer, BufferExt *buf, DescriptorSet *pDS, DescriptorSlotMode mode, int slotIndex);


// Tools
#ifdef __APPLE__
std::string forge_translate_glsl_metal(const char *source, const char *filepath, bool fragment);
void hl_compile_metal_to_bin(const char *fileName, const char *outFile);
#elif defined (WIN32)

#endif

// Queue
void forge_queue_submit_cmd(Queue *queue, Cmd *cmd, Semaphore *signalSemphor, Semaphore *wait, Fence *signalFence);

// Buffer Load
/*
void forge_sdl_buffer_load_desc_set_index_buffer(BufferLoadDesc *bld, int size, void *data, bool shared);
void forge_sdl_buffer_load_desc_set_vertex_buffer(BufferLoadDesc *bld, int size, void *data, bool shared);
void forge_sdl_buffer_load_desc_set_uniform_buffer(BufferLoadDesc *bld, int size, void *data, bool shared);
void forge_sdl_buffer_load_desc_set_dynamic(BufferLoadDesc *bold, bool isDynamic);

Buffer *forge_sdl_buffer_load(BufferLoadDesc *bld, SyncToken *token);
// Buffer
void forge_sdl_buffer_update(Buffer *buffer, void *data);
void forge_sdl_buffer_dispose(Buffer *buffer);
void forge_sdl_buffer_update_region(Buffer *buffer, void *data, int toffset, int size, int soffset);
*/

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
