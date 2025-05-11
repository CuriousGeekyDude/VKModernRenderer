# VKModernRenderer
 
# How to build the project

Make sure to have visual studio 2022 with a C++20 compiler and vcpkg installed and configured.

- Install requests using pip: pip install requests

- Run build.py found in root directory.

- Run the already present visual studio .sln and compile. Thats it. You can run the renderer now.

# Features
- Indirect rendering
- FXAA
- SSAO
- PBR point light
- Static omnidirectional shadow map
- Tiled deferred shading in compute shader
- CPU Frustum culling
- Bloom
- HDR
- Scene conversion for serializing geometry and material data of models for indirect rendering
- Spirv-v reflection for spirv shader bytecode using Spirv-Reflect for easier pipeline creation

# Render samples

-Tiled deferred shading
![Alt Text](Media/FullScreenTiledDeferredShading.png)
![Alt Text](Media/FullScreenDebugTiledDeferred1.png)
![Alt Text](Media/FullScreenTiledDeferredShading2.png)
![Alt Text](Media/FullScreenDebugTiledDeferred2.png)

-SSAO
![Alt Text](Media/FullScreenSSAO.png)

-Bloom
![Alt Text](Media/FullScreenBloom.png)
![Alt Text](Media/Bloom2.png)

-Frustum culling on cpu
![Demo](Media/FrustumCullingDebugCPU.gif)

-Static Omnidirectional shadow map
![Alt Text](Media/FullScreenOmniDirectional.png)
![Alt Text](Media/FullScreenOmniDirectional2.png)

# TODO
- Volumetric fog
- Replace tiled deferred with clustered deferred
- Frame graph to make transition layout between image layouts automatic
- SSR
