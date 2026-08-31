# Build

MSVC, Windows 10 SDK, CMake 3.20, Ninja.

```
cmake --preset windows-release
cmake --build --preset windows-release
```

`UR_VULKAN=ON` needs the Vulkan SDK. Off by default.

Outputs:

- `build/windows-release/Showcase.exe` — full desk
- `build/windows-release/Hello.exe` — 20-line hello

Backends: Direct3D 11, Direct3D 12, OpenGL. Vulkan is off unless `UR_VULKAN=ON`.

Add an app:

```cmake
ur_add_app( myapp Main.cpp )
```

Working directory should be the project root, or keep `assets/` next to the exe.
