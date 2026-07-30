# Runa

Runa is a set of tools planned to be an open-source, real-time, cross-platform 2D and 3D engine.

## Suported Platforms

---

- [x] Windows

- [x] Linux(Help Wanted)

- [ ] Mac(Help Wanted)

## Download and Install

---

- [Git](https://git-scm.com)

- [CMake 3.28 or above](https://cmake.org/download/)

- [vcpkg](https://github.com/microsoft/vcpkg)
  - Add `VCPKG_ROOT=/path/to/vcpkg` to enviroment variables

- Windows only
  - Build With Visual Studio 2026
    - [Visual Studio 2026](https://visualstudio.microsoft.com/downloads/)
    - 👇 Install the following workloads:
    - Game Development with C++
    - MSVC v142 or above | x64/x86
    - C++ 2015/2022 redistributable update

- Linux only
  - Build With GNU ```install with package manager```
    - GCC 9.x or above
    - make 
    - m4 
    - autoconf 
    - automake 
    - libtool
    - perl
    - zlib
  
  - Python packages
    - jinja2 ``` pip install Jinja2 ```
    - passlib ``` pip install passlib ```
  
## Setup Repository

---

```shell
git clone https://github.com/Cesio137/Runa.git
```

## Building the Engine

---

#### Any OS

* Setup Project.
  * Create a `Build` folder and open terminal inside.
  * Commands to generate project
    * ```bash
      cmake .. --preset=debug
      cmake .. --preset=release
      ``` 