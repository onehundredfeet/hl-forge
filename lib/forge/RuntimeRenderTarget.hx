package forge;

class RuntimeRenderTarget {
    public function new() {

    }
    public var texture : h3d.mat.Texture;
    public var nativeDesc : forge.Native.RenderTargetDesc;
    public var heapsDepthBuffer : h3d.mat.DepthBuffer; // IS a depth buffer, not HAS
    public var layer : Int;
    public var mip : Int;
    public var created  = false;
    public var attached = false;
    public var nativeRT : forge.Native.RenderTarget;
    public var inBarrier : forge.Native.ResourceBarrierBuilder;
    public var outBarrier: forge.Native.ResourceBarrierBuilder;

    // reserved for swap chain targets
    public var swapChain : forge.Native.SwapChain;
    public var begin: forge.Native.ResourceBarrierBuilder;
    public var present: forge.Native.ResourceBarrierBuilder;
    public var captureBuffer : forge.Native.Buffer;

    //Time
    public var createdFrame : Int;
    public var lastClearedFrame : Int;
    public var lastBoundFrame : Int;
}