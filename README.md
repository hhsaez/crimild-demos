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

### <a name="Basics">Basics</a>

#### [Triangle](examples/Triangle/)
Basic and explicit example for rendering a single colored triangle using Crimild. This shows the most important concepts and tools in the engine like geometries, buffers, pipelines and render passes.

#### [Transformations](example/Transformations)
Creates a scene with many objects, each of them with a different transformations. Shows how to specify translations, rotations and scales using the local transformation of nodes.

### <a name="Primitives">Primitives</a>

#### [Points](example/Points)
Shows how to use a POINTS primitive to build a point cloud.

#### [Lines](example/Lines)
Shows how to use a LINES primitive.

#### [Spheres](example/Lines)
Shows how to use a Sphere primitive.

#### [Boxes](example/Lines)
Shows how to use a Box primitive.

#### [Parametric Primitives](example/ParametricPrimitives)
Shows how to use each of the parametric primitives.

### <a name="Textures">Textures</a>

#### [Textures](examples/Textures/)
Loads an image from disk and create an image view, a sampler and a texture to display it on screen. Shows how to use primitives with texture coordinates as well.

#### [Texture Mipmaps Generation](examples/TextureMipmaps/)
Automatically generate mipmaps in runtime for a texture.

#### [Texture Filters](examples/TextureFilters/)
Create textures by using different min/mag filter options in samplers.

#### [Texture Wrap Mode](examples/TextureWrapping/)
Create textures by using different wrap modes options in samplers.

#### [Skybox](examples/Skybox)
Use a cubemap texture to render a skybox around a scene.

#### [Environment Mapping](examples/EnviornmentMapping)
Implements simple reflection and refraction effects using environment mapping.

### <a name="Lighting">Lighting</a>

#### [Lighting Unlit](examples/LightingUnlit)
Uses an unlit material to show how colors are displayed when no lighing is available.

#### [Lighting Basic](examples/LightingBasic)
Implements Phong and Gouraud lighting with simple shaders.

#### [Lighting Material](examples/LightingMaterial)
Renders a scene with several objects using the LitMaterial class.

#### [Lighting Diffuse Map](examples/LightingDiffuseMap)
Renders Planet Earth with diffuse texture and single point light, using Phong shading model.

#### [Lighting Specular Map](examples/LightingSpecularMap)
Renders Planet Earth with diffuse and specular textures, under the influence of a single point light and using Phong shading model.

#### [Lighting Directional](examples/LightingDirectional)
Uses a single directional light to lit a scene.

#### [Lighting Point](examples/LightingPoint)
Uses a single point light to lit a scene.

#### [Lighting Spotlight](examples/LightingSpotlight)
Uses a single spotlight light to lit a scene

#### [Lighting Multiple](examples/LightingMultiple)
Creates a scene with lots of cubes and adds multiple light sources: one directional light, two point lights moving around the scene and a flashlight (using a spot light). Camera can be controlled with WASD and the mouse.

### <a name="Loaders">Loaders</a>

#### [OBJLoader](example/OBJLoader)
Shows how to use the OBJ loader to create a simple scene by loading models from files.

### <a name="Advanced">Advanced</a>

#### [Depth](examples/Depth)
Visualizes the depth buffer by implementing a custom shader.

#### [Depth Functions](examples/DepthFunc)
Visualizes what happens when depth testing is disabled.
