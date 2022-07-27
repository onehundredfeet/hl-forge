#ifndef __HL_FORGE_H_
#define __HL_FORGE_H_
#pragma once
#include <map>
#include <vector>
#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <xxhash.h>
#ifndef SDL_MAJOR_VERSION
#	error "SDL2 SDK not found in hl/include/sdl/"
#endif

#define TWIN _ABSTRACT(sdl_window)

void heuristicTest(void (*fn)());
void heuristicTest2(float (*fn)(int));

bool hlForgeInitialize(const char *name);


#include <Renderer/IRenderer.h>
#include <Renderer/IResourceLoader.h>

class ForgeSDLWindow {
    public:
        ForgeSDLWindow(SDL_Window *window);
        SwapChain* createSwapChain(Renderer *renderer, Queue *queue, int width, int height, int chainCount, bool hdr10);
        SDL_Window *window;
        SDL_SysWMinfo wmInfo;
        Renderer *renderer() { return _renderer;}
        Renderer *_renderer;
        SDL_MetalView _view;
        CAMetalLayer *_layer;
        void present(Queue *pGraphicsQueue, SwapChain *pSwapChain, int swapchainImageIndex, Semaphore * pRenderCompleteSemaphore);
};

class HashBuilder {
    private:
        XXH64_state_t *_state;
    public:
        HashBuilder() {
            _state = XXH64_createState();
        }
        ~HashBuilder() {
            XXH64_freeState(_state);
        }

        void reset(int64_t seed) {
            XXH64_reset(_state, seed);
        }
        void addInt8( int v ) {
            int8_t x = (int8_t)v;
            XXH64_update(_state, &x, sizeof(int8_t));
        }
        void addInt16( int v ){
            int16_t x = (int16_t)v;
            XXH64_update(_state, &x, sizeof(int16_t));
        }
        void addInt32( int v ){
            int32_t x = (int32_t)v;
            XXH64_update(_state, &x, sizeof(int32_t));
        }
        void addInt64( int64_t v ){
            XXH64_update(_state, &v, sizeof(int64_t));
        }
        void addBytes( uint8_t *b, int offset, int length ) {
            XXH64_update(_state, &b[offset], length );
        }
        int64 getValue() {
            return XXH64_digest(_state);
        }
};

class RootSignatureFactory {
    public:
        RootSignatureFactory();
        ~RootSignatureFactory();
        RootSignature *create(Renderer *pRenderer);
        void addShader( Shader *);
        void addSampler( Sampler * sampler, const char *name );
        std::vector<Shader *> _shaders;
        std::vector<Sampler *> _samplers;
        std::vector<std::string> _names;
};

class StateBuilder {
    public:
        StateBuilder() {reset(); }
        ~StateBuilder() {}
        void reset() {
            memset( this, 0, sizeof(StateBuilder));
        }
        
        void addToHash(HashBuilder *hb) {
            hb->addBytes((uint8_t *)this, 0, sizeof(StateBuilder));
        }
        uint64_t getSignature(int shaderID, RenderTarget *rt, RenderTarget *depth);

        DepthStateDesc _depth;
        RasterizerStateDesc _raster;
        BlendStateDesc _blend;

        inline DepthStateDesc *depth() {return &_depth;}
        inline RasterizerStateDesc *raster() {return &_raster;}
        inline BlendStateDesc *blend() {return &_blend;}

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

    static void bind(Cmd *cmd, BufferBinder *binder ) {
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

    inline bool exists(  int64 key   ) {
        return _map.find(key) != _map.end();
    }
	inline void set(  int64 key, int value) {
//        printf("Signature : Setting key %lld, unsigned %llu n", key, (* ((uint64 *) &key)));
        
        _map[key] = value;
    }
	inline int get( int64 key) {
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
    

    void setName( const char *name) {
        _name = name;
        this->pName = _name.c_str();
    }

    void reset() {
        mGraphicsDesc.mRenderTargetCount = 0;
        _formats.clear();
    }
    void setRenderTargetGlobals(  SampleCount sampleCount, int sampleQuality ) {
        mGraphicsDesc.mSampleCount = sampleCount;
        mGraphicsDesc.mSampleQuality = sampleQuality;
    }

    int addGraphicsRenderTarget(  TinyImageFormat format ) {
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
        DescriptorDataBuilder( ) {
        }

        ~DescriptorDataBuilder() {
            clear();
        }
        void clear() {
            for(int i = 0; i < _dataPointers.size(); i++) {
                ::delete _dataPointers[i];
            }
            _dataPointers.clear();
            _names.clear();
            _data.clear();
            _uavMipSlices.clear();
        }

        void clearSlotData( int slot ) {
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
        void addSlotData( int slot, void * data) {
            _dataPointers[slot]->push_back(data);
        }
        void setSlotUAVMipSlice( int slot, int idx ) {
            _data[slot].mUAVMipSlice = idx;
        }

        void update(Renderer *pRenderer, int index, DescriptorSet *set) {
            for(auto i = 0; i < _names.size(); i++) {
                _data[i].pName = _names[i].c_str();
                switch(_modes[i] ) {
                    case DBM_TEXTURES:
                    _data[i].ppTextures = (Texture **)(&(*_dataPointers[i])[0]);
                    break;
                    case DBM_SAMPLERS:
                    _data[i].ppSamplers = (Sampler **)(&(*_dataPointers[i])[0]);
                    break;
                    default:break;
                }
                _data[i].mCount = _dataPointers[i]->size();
            }
            
            updateDescriptorSet(pRenderer, index, set, _names.size(), &_data[0]);
        }

        void bind(Cmd *cmd, int index, DescriptorSet *set ) {
            cmdBindDescriptorSet(cmd, index, set  );
        }
};

class ResourceBarrierBuilder {
    private:
        std::vector<RenderTargetBarrier> _rtBarriers;
        std::vector<BufferBarrier> _buffBarriers;
        std::vector<TextureBarrier> _texBarriers;

    public:
        int addRTBarrier( RenderTarget * rt, ResourceState src, ResourceState dst) {
            _rtBarriers.push_back( { rt, src, dst } );
            return _rtBarriers.size() - 1;
        }
        
        void insert( Cmd *cmd) {
            RenderTargetBarrier *rtbp= nullptr;
            BufferBarrier *bbp= nullptr;
            TextureBarrier *tbp= nullptr;
            if (_rtBarriers.size()) rtbp = &_rtBarriers[0];
            if (_buffBarriers.size()) bbp = &_buffBarriers[0];
            if (_texBarriers.size()) tbp = &_texBarriers[0];

            cmdResourceBarrier(cmd, _buffBarriers.size(), bbp, _texBarriers.size(), tbp, _rtBarriers.size(), rtbp);
        }


};


// SDL interface
SDL_Window *forge_sdl_get_window(void *ptr);
SDL_MetalView forge_create_metal_view(SDL_Window *);

// Swap Chain
RenderTarget *forge_swap_chain_get_render_target(SwapChain *, int );
inline bool isVSync( SwapChain *sc) {
    return sc->mEnableVsync != 0;
}

// Renderer
Renderer *createRenderer(const char *name);
void destroyRenderer( Renderer * );
Queue* createQueue(Renderer *renderer);
DescriptorSet *forge_renderer_create_descriptor_set( Renderer *, RootSignature *, DescriptorUpdateFrequency updateFrequency, uint maxSets, uint nodeIndex);
RenderTarget *forge_sdl_create_render_target(Renderer *, RenderTargetDesc *);
CmdPool * forge_sdl_renderer_create_cmd_pool( Renderer *, Queue * );
Cmd * forge_sdl_renderer_create_cmd( Renderer *, CmdPool * );
Fence * forge_sdl_renderer_create_fence( Renderer * );
Semaphore * forge_sdl_renderer_create_semaphore( Renderer * );
void forge_init_loader( Renderer * );
void forge_renderer_wait_fence( Renderer *, Fence *);
Shader *forge_renderer_shader_create(Renderer *pRenderer, const char *vertFile, const char *fragFile);
RootSignature *forge_renderer_createRootSignatureSimple(Renderer *pRenderer, Shader *shader);
RootSignature *forge_renderer_createRootSignature(Renderer *pRenderer, RootSignatureFactory *);
Shader *forge_load_compute_shader_file(Renderer *pRenderer, const char *fileName );
Buffer *forge_create_transfer_buffer(Renderer *rp, TinyImageFormat format, int width, int height, int nodeIndex);
bool forge_render_target_capture_2(Renderer *pRenderer, Cmd *pCmd, RenderTarget*pRenderTarget, Queue *pQueue,  ResourceState renderTargetCurrentState, uint8_t *alloc, int bufferSize);

// Queue
void forge_queue_submit_cmd(Queue *queue, Cmd *cmd, Semaphore *signalSemphor, Semaphore *wait, Fence *signalFence);

//Buffer Load
void forge_sdl_buffer_load_desc_set_index_buffer( BufferLoadDesc *bld, int size, void *data, bool shared);
void forge_sdl_buffer_load_desc_set_vertex_buffer( BufferLoadDesc *bld, int size, void *data, bool shared);
Buffer*forge_sdl_buffer_load( BufferLoadDesc *bld, SyncToken *token);
//Buffer
void forge_sdl_buffer_update(Buffer *buffer, void *data);
void forge_sdl_buffer_update_region(Buffer *buffer, void *data, int toffset, int size, int soffset);
inline unsigned char *forge_buffer_get_cpu_address( Buffer *buffer) {
    return (unsigned char *)(buffer->pCpuMappedAddress);
}

// Cmd
void forge_cmd_unbind(Cmd *);
void forge_cmd_push_constant(Cmd *cmd, RootSignature *rs, int index, void *data);
void forge_render_target_bind(Cmd *cmd, RenderTarget *mainRT, RenderTarget *depthStencil, LoadActionType color, LoadActionType depth);
void forge_cmd_wait_for_render(Cmd *cmd, RenderTarget *pRenderTarget);
void forge_cmd_wait_for_present(Cmd *cmd, RenderTarget *pRenderTarget);
void forge_cmd_insert_barrier(Cmd *cmd, ResourceBarrierBuilder *barrier);

//Texture Load
Texture*forge_texture_load(TextureLoadDesc *desc, SyncToken *token);
void forge_texture_set_file_name(TextureLoadDesc *desc, const char *path);
void forge_texture_upload_mip(Texture *tex, int mip, void *data, int dataSize);
void forge_texture_upload_layer_mip(Texture *tex, int layer, int mip, void *data, int dataSize);

//Texture Desc
Texture *forge_texture_load_from_desc(TextureDesc *tdesc, const char *name, SyncToken *token );

//Texture
void forge_sdl_texture_upload(Texture *, void *data, int dataSize);

// Render Target
Texture *forge_render_target_get_texture( RenderTarget *rt);
void forge_render_target_set_clear_colour( RenderTarget *rt, float r, float g, float b,float a);
void forge_render_target_set_clear_depth( RenderTarget *rt, float depth, int stencil);
void forge_render_target_capture(RenderTarget*rt,  Buffer *pTransferBuffer, Semaphore *semaphore);
int forge_render_target_capture_size(RenderTarget*pRenderTarget);

//Blend State
void forge_blend_state_desc_set_rt( BlendStateDesc *, BlendStateTargets rt, bool enabled);

//Vertex Layout
VertexAttrib *forge_vertex_layout_get_attrib( VertexLayout *, int idx);

// Vertex Attrib
void forge_vertex_attrib_set_semantic( VertexAttrib *, const char *name );



#endif
