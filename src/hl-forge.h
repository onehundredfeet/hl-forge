#ifndef __HL_FORGE_H_
#define __HL_FORGE_H_
#pragma once
#include <map>
#include <vector>
#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
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

class RootSignatureFactory {
    public:
        RootSignatureFactory();
        ~RootSignatureFactory();
        RootSignature *create(Renderer *pRenderer);
        void addShader( Shader *);
        void addSampler( Sampler * sampler );
        std::vector<Shader *> _shaders;
        std::vector<Sampler *> _samplers;
};

class StateBuilder {
    public:
        StateBuilder() {reset(); }
        ~StateBuilder() {}
        void reset() {
            memset( this, 0, sizeof(StateBuilder));
        }
        
        uint64_t getSignature(int shaderID);

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
        }

        int add(Buffer *b, int stride, int offset) {
            _buffers.push_back(b);
            _strides.push_back(stride);
            _offsets.push_back(offset);
            return _buffers.size() - 1;
        }

    static void bind(Cmd *cmd, BufferBinder *binder ) {
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
    inline GraphicsPipelineDesc *graphicsPipeline(  ) {
        this->mType = PIPELINE_TYPE_GRAPHICS;
        return &this->mGraphicsDesc;
    }
    void setName( const char *name) {
        _name = name;
        this->pName = _name.c_str();
    }

    int addGraphicsRenderTarget(  RenderTarget *rt ) {
        auto idx = mGraphicsDesc.mRenderTargetCount;

        _formats.push_back(rt->mFormat);
        mGraphicsDesc.pColorFormats = &_formats[0];
        mGraphicsDesc.mSampleCount = rt->mSampleCount;
        mGraphicsDesc.mSampleQuality = rt->mSampleQuality;
        mGraphicsDesc.mRenderTargetCount++;
        return idx;
    }

    std::vector<TinyImageFormat> _formats;
    std::string _name;
};



Renderer *createRenderer(const char *name);
void destroyRenderer( Renderer * );
Queue* createQueue(Renderer *renderer);
SDL_Window *forge_sdl_get_window(void *ptr);
void forge_sdl_buffer_load_desc_set_index_buffer( BufferLoadDesc *bld, int size, void *data, bool shared);
void forge_sdl_buffer_load_desc_set_vertex_buffer( BufferLoadDesc *bld, int size, void *data, bool shared);
void forge_cmd_unbind(Cmd *);

Buffer*forge_sdl_buffer_load( BufferLoadDesc *bld, SyncToken *token);
Texture*forge_texture_load(TextureLoadDesc *desc, SyncToken *token);
Texture *forge_texture_load_from_desc(TextureDesc *tdesc, const char *name, SyncToken *token );
void forge_sdl_texture_upload(Texture *, void *data, int dataSize);
void forge_cmd_push_constant(Cmd *cmd, RootSignature *rs, int index, void *data);
void forge_blend_state_desc_set_rt( BlendStateDesc *, BlendStateTargets rt, bool enabled);
VertexAttrib *forge_vertex_layout_get_attrib( VertexLayout *, int idx);
void forge_vertex_attrib_set_semantic( VertexAttrib *, const char *name );
void forge_texture_set_file_name(TextureLoadDesc *desc, const char *path);
void forge_render_target_clear(Cmd *cmd, RenderTarget *mainRT, RenderTarget *depthStencil);
void forge_sdl_buffer_update(Buffer *buffer, void *data);
void forge_sdl_buffer_update_region(Buffer *buffer, void *data, int toffset, int size, int soffset);
RenderTarget *forge_sdl_create_render_target(Renderer *, RenderTargetDesc *);
CmdPool * forge_sdl_renderer_create_cmd_pool( Renderer *, Queue * );
Cmd * forge_sdl_renderer_create_cmd( Renderer *, CmdPool * );
Fence * forge_sdl_renderer_create_fence( Renderer * );
Semaphore * forge_sdl_renderer_create_semaphore( Renderer * );
SDL_MetalView forge_create_metal_view(SDL_Window *);
void forge_init_loader( Renderer * );
void forge_renderer_wait_fence( Renderer *, Fence *);
RenderTarget *forge_swap_chain_get_render_target(SwapChain *, int );
void forge_queue_submit_cmd(Queue *queue, Cmd *cmd, Semaphore *signalSemphor, Semaphore *wait, Fence *signalFence);
Shader *forge_renderer_shader_create(Renderer *pRenderer, const char *vertFile, const char *fragFile);
RootSignature *forge_renderer_createRootSignatureSimple(Renderer *pRenderer, Shader *shader);
RootSignature *forge_renderer_createRootSignature(Renderer *pRenderer, RootSignatureFactory *);
#endif
