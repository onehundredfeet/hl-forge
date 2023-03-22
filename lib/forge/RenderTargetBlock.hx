package forge;

import h3d.mat.DepthBuffer;

class RenderTargetBlock {
    public function new() {

    }
//    public var inputTextures : Array<h3d.mat.Texture>;
    public var depthBuffer : DepthBuffer;
    public var colorTargets = new Array<RuntimeRenderTarget>();
    public var depthTarget : RuntimeRenderTarget;

    public var sampleCount = forge.Native.SampleCount.SAMPLE_COUNT_1;
    public var sampleQuality = 0;

}