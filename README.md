# Crimild Demos and Examples

A repository for crimild-based demos and code examples

## Table of Contents
+ [Cloning](#Cloning)
+ [Building](#Building)

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
(Ignore SDL_Mixer2 warning)

