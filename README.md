# bg-generation-mandelbrot

- Github : [https://github.com/edmBernard/bg-generation-mandelbrot](https://github.com/edmBernard/bg-generation-mandelbrot)

[Mandelbrot](https://en.wikipedia.org/wiki/Mandelbrot_set) and [Julia](https://en.wikipedia.org/wiki/Julia_set) set implementation using OpenGL shader.

## Build and Dependencies

### Dependencies

We use [vcpkg](https://github.com/Microsoft/vcpkg) to manage dependencies

This project depends on:
- [cxxopts](https://github.com/jarro2783/cxxopts): Command line argument parsing
- [fmt](https://fmt.dev/latest/index.html): A modern formatting library
- [spdlog](https://github.com/gabime/spdlog): Very fast, header-only/compiled, C++ logging library
- [glfw](https://www.glfw.org/): GLFW is an Open Source, multi-platform library for OpenGL, OpenGL ES and Vulkan development on the desktop.
- [glad](https://glad.dav1d.de/): Multi-Language GL/GLES/EGL/GLX/WGL Loader-Generator based on the official specs.
- [stb](https://github.com/nothings/stb): stb single-file public domain libraries for C/C++


```
./vcpkg install spdlog cxxopts fmt stb glfw glad
```

### Build

The recommended way to obtain the source code is to clone the entire repository from GitHub:

```
git clone git@github.com:edmBernard/bg-generation-mandelbrot
```

Building the main executable is done by the following command :

```bash
mkdir build
cd build
# configure cmake with vcpkg toolchain
cmake .. -DCMAKE_TOOLCHAIN_FILE=${VCPKG_DIR}/scripts/buildsystems/vcpkg.cmake
# on Windows : cmake .. -DCMAKE_TOOLCHAIN_FILE=${env:VCPKG_DIR}/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

The executable is named `bg-generation-mandelbrot`

## Disclaimer

It's a toy project. So if you spot error, improvement comments are welcome.
