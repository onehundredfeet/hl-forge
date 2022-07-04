# hl-forge

A hashlink / heaps driver for The Forge rendering API - https://github.com/ConfettiFX/The-Forge

THIS IS NOT FUNCTIONAL YET AND REQUIRES CHANGES TO BOTH HASHLINK AND HEAPS

IT IS NOT RECOMMENDED TO CLONE THIS REPO AT THIS TIME

Tested Samples
==============
- Adjust Color (Has some blinking on mouse move, likely resource premature deleting)
- Base2D
- Base3D
- Collide Check
- Cube Texture (Fails - Needs generateMipMaps)
- Cursor
- Filters (Fails - Needs setRenderTarget)
- GPU Particles (Fails - selectBuffer unsupported path)
- GraphicsDraw (Fails - Needs setRenderTarget)
- Blur (Fails - Needs setRenderTarget)
- Input 
- Interactive (Fails - Unsupported texture format R16F)
- Lights (Fails - Unsupported texture format RGBA32F)
- Mask (Fails - Unimplemented setRenderZone)
- Pbr (Fails - Unsupported texture format RGBA32F)
- Quaternion (Fails - Unsupported texture format R16F)
- Shadows (Fails - Unsupported texture format R16F)
- Skin (Fails - Unsupported texture format R16F)
- Stencil (Runs - Missing reflection)
- World (Fails - Unsupported texture format R16F)

Progress
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
- Fix Fonts & 2D

Next
====
- Fix Unsupported texture format R16F (Likely a render target)
- Fix Unsupported texture format RGBA32F
- Fix setRenderTarget
- Viewport resizing
- Check sRGB configuration on render target & texture load