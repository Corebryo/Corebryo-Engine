# Corebryo Engine

Corebryo is a low level C++ game engine project with a clean, explicit architecture and a focus on learning, performance, and control.

## Status

This project is under active development and is not production ready.
Expect breaking changes.

## Supported Platforms

- Windows 10/11, x64
- Visual Studio 2022 toolchain

## Goals

- Keep the engine code understandable and explicit.
- Prefer simple systems over “magic”.
- Make Vulkan usage clean and predictable.
- Maintain a tidy repository with a clear folder layout.

## Why Corebryo

Corebryo exists for developers who want to understand how a modern renderer and engine systems are built, without fighting a large proprietary toolchain. It prioritizes clarity, control, and small, inspectable systems over one-click features.

### When it is a good fit
- Indie developers who want to learn engine architecture by reading and changing the code
- Graphics programmers experimenting with Vulkan and render pipelines
- Small teams that want a lightweight base they can own and extend
- Students or hobbyists who care more about understanding than shipping fast

### Why not just Unity or Unreal
- Those engines optimize for broad usability and rapid content creation, not transparency
- Corebryo keeps the stack small so you can reason about every system and modify it safely
- It is intentionally minimal, so you can build exactly the tools and workflows you need

## Current Features

- Entity + component scene storage (Transform, Mesh, Material)
- Render submission list generation (RenderItem build step)
- Basic engine project structure (Assets, Shaders, Source, etc.)
- Win32 windowing layer (plus GLFW integration)
- Vulkan renderer foundation
- Main render stages (skybox, opaque, transparent) executed within a single render pass
- Render list grouping by material alpha with stage-appropriate sorting (opaque front-to-back, transparent back-to-front)
- Opaque batching by mesh/material to reduce redundant binds while preserving draw order
- GPU instancing for opaque batches using per-instance transforms to cut draw calls
- Nuklear performance overlay with FPS, frame time, draw calls, triangle count, and vertex count
- Editor entities panel to list and select scene entities for inspection and editing
- Inspector and selection info panels for viewing and editing Transform data plus read-only component/bounds details
- Clamped editor delta time to avoid large simulation jumps after window interaction

## Planned Features

- Asset pipeline and loaders (textures, meshes)
- JSON serialization foundation (Vec3, Mat4, scene save/load)
- Improved camera and input systems
- Debug rendering utilities
- Basic physics and collision (AABB broad-phase)
- Editor and debug tooling
- Scene graph / ECS direction decisions

## Repository Layout

- `Engine/` Core runtime and renderer
- `Engine/Source/` Engine C++ source
- `Editor/` Editor application
- `Editor/Source/Main.cpp` Editor entry point
- `Engine/Assets/` (or `Assets/`) Engine assets and runtime data
- `Engine/Shaders/` Shader source files
- `Assets/Ready/` Compiled shader outputs (if present)
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

### Run
1. Set **Editor** as the startup project
2. Run (F5) to launch the editor and validate runtime behavior

## Dependencies

- Vulkan SDK installed and configured
- GLFW linked locally (DLL present for runtime)
- Windows SDK that matches your Visual Studio installation

## Who This Is For

- Developers who want to learn how a renderer and engine systems fit together
- Teams that want a lightweight base they can fully own and extend
- Graphics programmers experimenting with Vulkan and render pipelines

## What This Is Not

- A turn-key, production-ready engine
- A full content-creation suite like Unity or Unreal
- A drop-in replacement for large toolchains

## Contributing

- Keep code style consistent with the existing project
- Prefer small, focused commits with clear messages
- Avoid committing build outputs or IDE cache files

## License

MIT License. See `LICENSE`.
