# Crimild Demos and Examples

A repository for crimild-based demos and code examples

## Table of Contents
+ [Cloning](#Cloning)
+ [Building](#Building)
+ [Examples](#Examples)
    + [Basics](#Basics)
    + [Primitives](#Primitives)
	+ [Textures](#Textures)
    + [Advanced](#Advanced)

## <a name="Cloning">Cloning</a>
```
git clone --recursive https://github.com/hhsaez/crimild-demos.git
```

### Updating existing code
```
git pull origin <BRANCH_NAME>
git submodule update --init --recursive
cmake .
```

## <a name="Building">Building</a>

### Unix Makefiles
```
cmake .
make clean all test -j4
```

### Xcode
```
cmake . -G Xcode
```

### Visual Studio
```
cmake . -G "Visual Studio 2014"
```

### Web (Wasm/WebGL)
```
mkdir build-web
cd build-web
cmake .. -DCMAKE_TOOLCHAIN_FILE=#{EMSCRIPTEN_HOME}/cmake/Modules/Platform/Emscripten.cmake
make clean all -j8
```
It's safe to ignore SDL_Mixer2 warnings, if any.

## <a name="Examples">Examples</a>

&nbsp; | &nbsp; | &nbsp;
-- | -- | --
<a name="Basics">Basics</a> | |
<img src="examples/Triangle/screenshot.png" width=200 />|[Triangle](examples/Triangle/) | Basic (and very explicit) example for rendering a single colored triangle using Crimild. This shows the most important concepts and tools in the engine like geometries, buffers, pipelines and render passes. 
<img src="examples/Transformations/screenshot.png" width=200 /> | [Transformations](examples/Transformations) | Creates a scene with many objects, each of them with a different transformations. Shows how to specify translations, rotations and scales using the local transformation of nodes.
<a name="Primitives">Primitives</a> | |
<img src="examples/Points/screenshot.png" width=200 /> | [Points](examples/Points) |Shows how to use a POINTS primitive to build a point cloud.
<img src="examples/Lines/screenshot.png" width=200 /> | [Lines](examples/Lines) | Shows how to use a LINES primitive.
<img src="examples/Spheres/screenshot.png" width=200 /> | [Spheres](examples/Lines) | Shows how to use a Sphere primitive.
<img src="examples/Boxes/screenshot.png" width=200 /> | [Boxes](examples/Lines) |Shows how to use a Box primitive.
<img src="examples/ParametricPrimitives/screenshot.png" width=200 /> | [Parametric Primitives](examples/ParametricPrimitives) | Shows how to use each of the parametric primitives, using different generator params, to construct several objects objects.
<a name="Textures">Textures</a> | |
<img src="examples/Textures/screenshot.png" width=200 /> | [Textures](examples/Textures/) | Loads an image from disk and create an image view, a sampler and a texture to display it on screen. Shows how to use primitives with texture coordinates as well.
<img src="examples/TextureMipmaps/screenshot.png" width=200 /> | [Texture Mipmaps Generation](examples/TextureMipmaps/) | Automatically generate mipmaps in runtime for a texture.
<img src="examples/TextureFilters/screenshot.png" width=200 /> | [Texture Filters](examples/TextureFilters/) | Create textures by using different min/mag filter options in samplers.
<img src="examples/TextureWrapping/screenshot.png" width="200" /> | [Texture Wrap Mode](examples/TextureWrapping/) | Create textures by using different wrap modes options in samplers.
<a name="Environment">Environment</a> | | 
<img src="examples/Skybox/screenshot.png" width="200" /> | [Skybox](examples/Skybox) | Use a cubemap texture to render a skybox around a scene.
<img src="examples/EnvironmentMapping/screenshot.png" width="200" /> | [Environment Mapping](examples/EnviornmentMapping) | Implements simple reflection and refraction effects using environment mapping.
<a name="Lighting">Lighting</a> | |
<img src="examples/LightingUnlit/screenshot.png" width="200" /> | [Lighting Unlit](examples/LightingUnlit) | Uses an unlit material to show how colors are displayed when no lighing is available.
<img src="examples/LightingBasic/screenshot.png" width="200" /> | [Lighting Basic](examples/LightingBasic) | Implements Phong and Gouraud lighting with simple shaders.
<img src="examples/LightingMaterial/screenshot.png" width="200" /> | [Lighting Material](examples/LightingMaterial) | Renders a scene with several objects using the LitMaterial class.
<img src="examples/LightingDiffuseMap/screenshot.png" width="200" /> | [Lighting Diffuse Map](examples/LightingDiffuseMap) | Renders Planet Earth with diffuse texture and single point light, using Phong shading model.
<img src="examples/LightingSpecularMap/screenshot.png" width="200" /> | [Lighting Specular Map](examples/LightingSpecularMap) | Renders Planet Earth with diffuse and specular textures, under the influence of a single point light and using Phong shading model.
<img src="examples/LightingDirectional/screenshot.png" width="200" /> | [Lighting Directional](examples/LightingDirectional) | Uses a single directional light to lit a scene.
<img src="examples/LightingPoint/screenshot.png" width="200" /> | [Lighting Point](examples/LightingPoint) | Uses a single point light to lit a scene.
<img src="examples/LightingSpotlight/screenshot.png" width="200" /> | [Lighting Spotlight](examples/LightingSpotlight) | Uses a single spotlight light to lit a scene
<img src="examples/LightingMultiple/screenshot.png" width="200" /> | [Lighting Multiple](examples/LightingMultiple) | Creates a scene with lots of cubes and adds multiple light sources: one directional light, two point lights moving around the scene and a flashlight (using a spot light). Camera can be controlled with WASD and the mouse.
<img src="examples/LightingNormalMapping/screenshot.png" width="200" /> | [Normal Mapping](examples/NormalMapping) | Uses a normal texture to provide more detail to models
<a name="Loaders">Loaders</a> | |
<img src="examples/OBJLoader/screenshot.png" width="200" /> | [OBJLoader](example/OBJLoader) | Shows how to use the OBJ loader to create a simple scene by loading models from files.
<a name="DepthStencil">Depth/Stencil</a> | |
<img src="examples/Unavailable/screenshot.png" width="200" /> | [Depth](examples/Depth) | Visualizes the depth buffer by implementing a custom shader.
<img src="examples/Unavailable/screenshot.png" width="200" /> | [Depth Functions](examples/DepthFunc) | Visualizes what happens when depth testing is disabled.
<img src="examples/Unavailable/screenshot.png" width="200" /> | [Stencil Outline](examples/StencilOutline) | Renders an outline around some objects using the stencil buffer and multiple passes
<a name="Alpha">Alpha</a> | |
<img src="examples/Unavailable/screenshot.png" width="200" /> | [AlphaDiscard](examples/AlphaDiscard) | Displays a scene with several objects, discarding fragments if the alpha value is lower than some threshold.
<a name="Pipelines">Pipelines</a> | |
<img src="examples/Unavailable/screenshot.png" width="200" /> | [Pipeline Cull mode](examples/PipelineCullMode) | Modify a pipeline to disable back-face culling for a rotating quad.
<a name="Compositions">Compositions</a> | |
<img src="examples/Unavailable/screenshot.png" width="200" /> | [Postprocessing Negative](examples/PostprocessingNegative) | Applies a "negative" post processing effect by using frame compositions
<img src="examples/Unavailable/screenshot.png" width="200" /> | [Postprocessing Grayscale](examples/PostprocessingGrayscale) |  Converts a color scene into grayscale by using frame compositions
<img src="examples/Unavailable/screenshot.png" width="200" /> | [Postprocessing Sharpen](examples/PostprocessingSharpen) | Applies a sharpen convolution to a rendered scene by using frame compositions
<img src="examples/Unavailable/screenshot.png" width="200" /> | [Postprocessing Blur](examples/PostprocessingBlur) | Applies a blur convolution to a rendered scene by using frame compositions
<img src="examples/Unavailable/screenshot.png" width="200" /> | [Postprocessing Edges](examples/PostprocessingEdges) | Process a rendered scene, highlighting edges by using frame compositions
<a name="Shadows">Shadows</a> | |
<img src="examples/Unavailable/screenshot.png" width="200" /> | [Directional](examples/Shadows) | A simple scene is rendered using a directional light that cast shadows on both dynamic and static objects.
<img src="examples/Unavailable/screenshot.png" width="200" /> | [Spot](examples/ShadowsSpot) | A simple scene is rendered using a spot light that cast shadows on both dynamic and static objects
<img src="examples/Unavailable/screenshot.png" width="200" /> | [Point](examples/ShadowsPoint) | A scene is rendered using a point light that casting shadows for all objects in all directions
<img src="examples/Unavailable/screenshot.png" width="200" /> | [Directional (many lights)](examples/ShadowsDirectionalMultiple) | A simple scene is rendered using two directional lights, each one casting shadows on both dynamic and static objects.
<img src="examples/Unavailable/screenshot.png" width="200" /> | [Spot (many lights)](examples/ShadowsSpotMultiple) | A simple scene is rendered using three spot lights with different colors, each one casting shadows on both dynamic and static objects
<img src="examples/Unavailable/screenshot.png" width="200" /> | [Point (many lights)](examples/ShadowsPointMultiple) | A scene is rendered using three point lights with different colors, each one casting shadows for all objects in all directions
<a name="Image Effects">Image Effects</a> | |
<img src="examples/Unavailable/screenshot.png" width="200" /> | [HDR](examples/HDR) | Renders a scene with HDR enabled, providing more control over bright and dark colors
<img src="examples/Unavailable/screenshot.png" width="200" /> | [Bloom](examples/Bloom) | Filters brigth areas, applying blur to the result, to generate a light bleeding effect
<a name="PBR">PBR</a> | |
<img src="examples/PBRBasic/screenshot.png" width="200" /> | [PBR Basic](examples/PBRBasic) | TBD
<img src="examples/PBRTexture/screenshot.png" width="200" /> | [PBR Texture](examples/PBRTexture) | TBD
<img src="examples/PBRIBL/screenshot.png" width="200" /> | [PBR Image Based Lighting](examples/PBRIBL) | TBD
<img src="examples/PBRModel/screenshot.png" width="200" /> | [PBR 3D Model](examples/PBRModel) | TBD 
 