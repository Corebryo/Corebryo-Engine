# Repository Guidelines

## Project Structure & Module Organization
- `Source/` holds all C++ code, with modules grouped by domain (e.g., `Renderer/Vulkan`, `Scene`, `Platform`).
- `Assets/` stores textures and skybox resources used at runtime.
- `Shaders/` contains GLSL shader sources and compiled `.spv` outputs.
- `Binary/` is the build output directory; `Temp/` is the intermediate build directory.
- `Engine.sln` and `Engine.vcxproj` define the Visual Studio build.

## Build, Test, and Development Commands
- `msbuild Engine.sln /p:Configuration=Debug /p:Platform=x64` builds the debug x64 binary into `Binary/`.
- `msbuild Engine.sln /p:Configuration=Release /p:Platform=x64` builds an optimized release binary.
- Open `Engine.sln` in Visual Studio 2022 for local debugging and asset iteration.

## Coding Style & Naming Conventions
- C++20 is enabled (`LanguageStandard=stdcpp20`). Keep code standard-conforming.
- Use `.h`/`.cpp` pairs and follow existing naming (e.g., `VulkanRenderer`, `EngineCamera`).
- Headers in `Source/` are included via project include paths; prefer project-relative includes.
- Keep indentation consistent with existing files (tabs/spaces as already used in nearby code).

## Testing Guidelines
- No automated test framework is currently configured in this repository.
- If you add tests, document the framework and how to run it in this file.

## Commit & Pull Request Guidelines
- Commit messages use short, sentence-style summaries (e.g., "Refactor math headers for consistency and clarity").
- PRs should describe changes, include build/run notes, and link relevant issues if applicable.
- Include screenshots or GIFs for visual or rendering changes.

## Security & Configuration Notes
- The build expects Vulkan SDK and GLFW libraries to be available; the project references `$(VULKAN_SDK)` and local GLFW include/lib paths in `Engine.vcxproj`.
- Keep local machine-specific paths out of source changes when possible.