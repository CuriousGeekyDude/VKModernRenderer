# VKModernRenderer
 
# How to build the project

Make sure to have visual studio 2022 with a C++20 compiler and vcpkg installed and configured.

- Install requests using pip: pip install requests

- Run build.py found in the root directory.

- Run the already present visual studio Renderer.sln and compile. That's it.

# Main Features
- Indirect rendering
- FXAA
- SSAO
- PBR point light
- Static omnidirectional shadow map
- Tiled deferred shading in compute shader
- CPU Frustum culling
- Bloom
- HDR
- Exposure based tone mapping
- Tangent space normal mapping
- Scene conversion for serializing geometry and material data of models for indirect rendering
- Spirv-v reflection for spirv shader bytecode using [Spirv-Reflect](https://github.com/KhronosGroup/SPIRV-Reflect) library for easier pipeline creation


# General project layout

- The effects are all inside source files that end with renderer. The Gbuffer, frustum culling, and indirect rendering are done in IndirectRenderer, the omnidirectional effect happens in 6 renderpasses all of which are named DepthMapLightRenderer, tiled deferred shading is done in TiledDeferredLightningRenderer, and finally the bloom happens in several passes that are done in DownsampleToMipmapsRenderer, UpsampleBlendRenderer, and LinearlyInterpBlurAndSceneRenderer.
- The renderer is defined in VulkanRenderer file but the core rendering loop happens in drawFrame() method of the VulkanApp struct defined in VulkanEngineCore file.
- It is worth mentioning that the file FrameGraph is not a full fledged frame graph yet. It parses the JSON file InitFiles/JSON Files/framegraph.json where we define our input and output resources for that particular renderpass and it generates the vulkan renderpass and vulkan frame graph objects for them. It removes the burden of defining these objects for every renderpass ourselves. Additionally, the FrameGraph generates nodes ,each of which represents a single renderpass in our pipeline. Using the FrameGraph we can access them and do various things like enabling, disabling them, or access the resources that are originally defined in them such as textures etc. This was quite useful while trying to integrate ImGui in the IMGUIRenderer file. Finally, the FrameGraph is responsible for recording the command buffers and submitting them to the vulkan queue in its RenderGraph() method.

# Render samples

- Tiled deferred shading

![Alt Text](Media/FullScreenTiledDeferredShading.png)
![Alt Text](Media/FullScreenDebugTiledDeferred1.png)
![Alt Text](Media/FullScreenTiledDeferredShading2.png)
![Alt Text](Media/FullScreenDebugTiledDeferred2.png)

- SSAO

![Alt Text](Media/FullScreenSSAO.png)

- Bloom

![Alt Text](Media/FullScreenBloom.png)
![Alt Text](Media/Bloom2.png)

- Frustum culling on cpu

![Demo](Media/FrustumCullingDebugCPU.gif)

- Static Omnidirectional shadow map

![Alt Text](Media/FullScreenOmniDirectional.png)
![Alt Text](Media/FullScreenOmniDirectional2.png)

# TODO
- Volumetric fog
- Replace tiled deferred with clustered deferred
- Frame graph to make transition layout between image layouts automatic
- SSR
- Tile based omnidirectional shadow mapping for generating shadows from 100s of point lights in real time
- Visibility buffers
