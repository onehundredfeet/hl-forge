# hl-forge

A hashlink / heaps driver for The Forge rendering API - https://github.com/ConfettiFX/The-Forge

THIS IS NOT FUNCTIONAL YET AND REQUIRES CHANGES TO BOTH HASHLINK AND HEAPS

IT IS NOT RECOMMENDED TO CLONE THIS REPO AT THIS TIME

Tested Samples
==============
- Adjust Color (Runs)
- Base2D (Runs)
- Base3D (Runs)
- Bounds (Runs with some flickering when mouse is moving)
- Collide Check (Runs)
- Cube Texture (Runs)
- Cursor (Runs)
- Filters (Runs)
- GPU Particles (Runs)
- GraphicsDraw (Fails - Needs capturePixels)
- Blur (Runs - Some UI is in the wrong spot)
- Input (Runs)
- Interactive (Runs)
- Lights (Fails - Can't capture pixels on this platform)
- Mask (Fails - Unimplemented setRenderZone)
- Pbr (Fails - Multiple render targets are unsupported)
- Quaternion (Runs)
- Shadows (Runs)
- Skin (Runs)
- Stencil (Fails - Below)
[MTLDebugRenderCommandEncoder validateCommonDrawErrors:]:5252: failed assertion `Draw Errors Validation
MTLDepthStencilDescriptor uses frontFaceStencil but MTLRenderPassDescriptor has a nil stencilAttachment texture
MTLDepthStencilDescriptor uses backFaceStencil but MTLRenderPassDescriptor has a nil stencilAttachment texture

- World (Runs)

Incomplete
====
- Auto generate mipmaps
- Capture pixels / copy texture
- Viewport resizing
- Check sRGB configuration on render target & texture load
- Support sub render zone
- Stecil support
- Fix Z-fighting
- Texture Filtering check

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
- Skinned rendereing
- Fix Unsupported texture format RGBA32F

