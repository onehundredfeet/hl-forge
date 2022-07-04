# hl-forge

A hashlink / heaps driver for The Forge rendering API - https://github.com/ConfettiFX/The-Forge

THIS IS NOT FUNCTIONAL YET AND REQUIRES CHANGES TO BOTH HASHLINK AND HEAPS

IT IS NOT RECOMMENDED TO CLONE THIS REPO AT THIS TIME

Progress
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

Next
- Fix Fonts & 2D
