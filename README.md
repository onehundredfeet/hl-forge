# hl-forge

A hashlink / heaps driver for The Forge rendering API - https://github.com/ConfettiFX/The-Forge

THIS IS NOT FUNCTIONAL YET AND REQUIRES CHANGES TO BOTH HASHLINK AND HEAPS

IT IS NOT RECOMMENDED TO CLONE THIS REPO AT THIS TIME

Tested Samples
==============
- Adjust Color (Runs)
- Base2D (Runs)
- Base3D (Runs)
- Bounds (Runs)
- Collide Check (Runs)
- Cube Texture (Fails - Needs generateMipMaps)
- Cursor (Runs)
- Filters (Fails - Needs copyTexture)
- GPU Particles (Fails - selectBuffer unsupported path)
- GraphicsDraw (Fails - Needs capturePixels)
- Blur (Runs)
- Input (Runs)
- Interactive (Fails - unknown vertex attribute weights)
- Lights (Fails - Unimplemented mip support)
- Mask (Fails - Unimplemented setRenderZone)
- Pbr (Fails - Unimplemented mip support)
- Quaternion (Fails - Unsupported current buffer)
- Shadows (Runs)
- Skin (Fails - unknown vertex attribute weights)
- Stencil (Fails - Below)
[MTLDebugRenderCommandEncoder validateCommonDrawErrors:]:5252: failed assertion `Draw Errors Validation
MTLDepthStencilDescriptor uses frontFaceStencil but MTLRenderPassDescriptor has a nil stencilAttachment texture
MTLDepthStencilDescriptor uses backFaceStencil but MTLRenderPassDescriptor has a nil stencilAttachment texture

- World (Fails - unknown vertex attribute time)

Incomplete
====
- Auto generate mipmaps
- Capture pixels / copy texture
- Skinned rendereing
- Fix Unsupported texture format RGBA32F
- Viewport resizing
- Check sRGB configuration on render target & texture load
- Support sub render zone
- Stecil support

Completed
========
- Gets a HEAPS SDL window
- Attaches a FORGE render target & depth buffer to it
- Creates vertex and index buffers from HEAPS objects
- Transcodes shaders from HEAPS to GLSL with FORGE markup
- Translates GLSL to native shader language used by FORGE
- Compiles and builds shader program into FORGE object
- Translates shader arguments and uses push constants
- Creates FORGE blend, depth and raster states
- Creates a FORGE render pipeline
- Submits indexed draw calls
- Renders lit cube
- Loads a texture from HEAPS into a texture buffer
- Create texture descriptors & attach to draw
- Fixed Fonts & 2D
- Fixed setRenderTarget
- Fixed Unsupported texture format R16F (Likely a render target)
- Fixed shadows

