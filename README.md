# Corebryo Engine

Corebryo is a low level C++ game engine project with a clean, explicit architecture and a focus on learning, performance, and control.

## Status

This project is under active development and is not production ready.
Expect breaking changes.

## Goals

- Keep the engine code understandable and explicit.
- Prefer simple systems over “magic”.
- Make Vulkan usage clean and predictable.
- Maintain a tidy repository with a clear folder layout.

## Current Features

- Entity + component scene storage (Transform, Mesh, Material)
- Render submission list generation (RenderItem build step)
- Basic engine project structure (Assets, Shaders, Source, etc.)
- Win32 windowing layer (plus GLFW integration)
- Vulkan renderer foundation

## Planned Features

- Asset pipeline and loaders (textures, meshes)
- JSON serialization foundation (Vec3, Mat4, scene save/load)
- Improved camera and input systems
- Debug rendering utilities
- Basic physics and collision (AABB broad-phase)
- Editor and debug tooling
- Scene graph / ECS direction decisions

## Repository Layout

- `Assets/` Engine assets and runtime data
- `Shaders/` Shader source files
- `Source/` Engine source code
- `Binary/` Local build output (ignored by git)
- `Temp/` Local temporary files (ignored by git)

## Build (Visual Studio)

This repository does not include a `.sln` file.

You can build it in Visual Studio by creating a solution locally:

1. Open Visual Studio
2. Create a new **Empty Project** or **Console App (C++)**
3. Add the files under `Source/` to the project
4. Set include directories to match the engine layout (if required)
5. Select `x64` and `Debug` or `Release`
6. Build

## Contributing

- Keep code style consistent with the existing project
- Prefer small, focused commits with clear messages
- Avoid committing build outputs or IDE cache files

## License

MIT License. See `LICENSE`.
