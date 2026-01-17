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
- Main render stages (skybox, opaque, transparent) executed within a single render pass

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

This repository uses a Visual Studio Solution Extension file (`.slnx`).

### Requirements
- Visual Studio 2022 (or newer)
- Desktop development with C++
- Vulkan SDK installed
- GLFW (prebuilt or source, linked locally)

### Build Steps
1. Open `Corebryo.slnx` in Visual Studio
2. Select configuration: `Debug` or `Release`
3. Select platform: `x64`
4. Build the solution

The solution contains:
- **Engine** – Corebryo engine library
- **Editor** – Development editor built on top of the engine
- **Nuklear** – External library for the UI-layer of the editor

## Contributing

- Keep code style consistent with the existing project
- Prefer small, focused commits with clear messages
- Avoid committing build outputs or IDE cache files

## License

MIT License. See `LICENSE`.
