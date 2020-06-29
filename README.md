# Crimild Demos and Examples

A repository for crimild-based demos and code examples

## Table of Contents
+ [Cloning](#Cloning)
+ [Building](#Building)
+ [Examples](#Examples)
    + [Basics](#Basics)
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

### <a name="Textures">Textures</a>

#### [Textures](examples/Textures/)
Loads an image from disk and create an image view, a sampler and a texture to display it on screen. Shows how to use primitives with texture coordinates as well.

#### [Texture Mipmaps Generation](examples/TextureMipmaps/)
Automatically generate mipmaps in runtime for a texture.

#### [Texture Filters](examples/TextureFilters/)
Create textures by using different min/mag filter options in samplers.

#### [Texture Wrap Mode](examples/TextureWrapping/)
Create textures by using different wrap modes options in samplers.

### <a name="Advanced">Advanced</a>

