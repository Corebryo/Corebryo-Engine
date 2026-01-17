/*
 * Corebryo
 * Copyright (c) 2026 Jonathan Den Haerynck
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <Windows.h>

#include "EditorConfig.h"
#include "Platform/Input/GlfwInput.h"
#include "Platform/Window/GlfwWindow.h"
#include "Platform/Windows/Win32PowerPerformance.h"
#include "Engine/EngineRuntime.h"
#include "Engine/EngineState.h"
#include "Input/InputState.h"

#include <GLFW/glfw3.h>

#include <cstdio>
#include <cstdint>
#include <filesystem>

/* Working directory root*/
static void SetWorkingDirectoryToEngineRoot()
{
    /* Retrieve the full path of the current executable */
    char modulePath[MAX_PATH]{};
    const DWORD length = GetModuleFileNameA(nullptr, modulePath, MAX_PATH);

    /* Abort if the executable path could not be resolved */
    if (length == 0 || length == MAX_PATH)
    {
        return;
    }

    /* Convert executable path to a filesystem path */
    std::filesystem::path exePath(modulePath);

    /* Resolve the solution root by walking up two directory levels */
    std::filesystem::path solutionDir = exePath.parent_path().parent_path();

    /* Construct the expected Engine directory path */
    std::filesystem::path engineDir = solutionDir / "Engine";

    /* Switch the current working directory if the Engine directory exists */
    if (std::filesystem::exists(engineDir))
    {
        SetCurrentDirectoryA(engineDir.string().c_str());
    }
}

/* Simple scope-exit helper for reliable cleanup paths. */
template <typename TFunc>
class ScopeExit
{
public:
    /* Store cleanup callback and arm the scope guard. */
    explicit ScopeExit(TFunc func)
        : Func(func)
        , Active(true)
    {
        /* Default state is active. */
    }

    /* Run the cleanup callback if the guard is still active. */
    ~ScopeExit()
    {
        /* Only execute when not released. */
        if (Active)
        {
            /* Execute deferred cleanup. */
            Func();
        }
    }

    /* Disarm the guard so the destructor does nothing. */
    void Release()
    {
        /* Disable deferred cleanup. */
        Active = false;
    }

private:
    /* Stored cleanup function invoked on scope exit. */
    TFunc Func;

    /* Tracks whether the cleanup function should run. */
    bool Active;
};

int main()
{
    std::printf("Starting editor initialization...\n");

    const EditorConfig editorConfig{};
    EngineConfig engineConfig{};
    engineConfig.FallbackWidth = editorConfig.WindowWidth;
    engineConfig.FallbackHeight = editorConfig.WindowHeight;

    if (editorConfig.UseHighPerformancePowerMode)
    {
        SetHighPerformancePowerMode();
    }
    SetWorkingDirectoryToEngineRoot();

    /* Create the native window first because Vulkan needs a surface provider. */
    GlfwWindow window;
    bool windowCreated = false;

    if (!window.Create(editorConfig.WindowWidth, editorConfig.WindowHeight, editorConfig.WindowTitle))
    {
        std::fprintf(stderr, "Failed to create window\n");
        return 1;
    }

    windowCreated = true;

    /* Ensure input is connected to the active GLFW window. */
    GlfwInput::Attach(window.GetHandle());

    /* Bring window to front early, but keep it hidden until the first frame is ready. */
    if (editorConfig.HideWindowUntilReady)
    {
        window.BringToFront();
    }

    /* Initialize the engine runtime with the editor window. */
    EngineRuntime engine;
    bool engineCreated = false;

    if (!engine.Initialize(window.GetHandle(), engineConfig))
    {
        std::fprintf(stderr, "Failed to initialize engine runtime\n");

        if (windowCreated)
        {
            window.Destroy();
        }

        return 1;
    }

    engineCreated = true;

    /* Ensure we always shut down cleanly, even if we add new early returns later. */
    ScopeExit cleanup([&]()
        {
            if (engineCreated)
            {
                engine.Shutdown();
            }

            if (windowCreated)
            {
                window.Destroy();
            }
        });

    if (editorConfig.HideWindowUntilReady)
    {
        glfwShowWindow(window.GetHandle());
    }

    /* Time tracking based on GLFW monotonic timer. */
    double lastTime = glfwGetTime();

    /* Main loop runs until the window requests close. */
    while (!window.ShouldClose())
    {
        /* Start a new input frame. */
        InputBeginFrame();

        /* Poll input and window events. */
        window.PollEvents();

        /* Sync engine and input modes before applying cursor state. */
        SetInputMode(
            g_CurrentEngineState == EngineState::Game
            ? InputMode::Game
            : InputMode::Editor);

        /* Apply engine input mode and mouse capture state. */
        GlfwInput::ApplyInputMode(window.GetHandle());

        /* Snapshot input for deterministic updates. */
        InputEndFrame();

        /* Compute delta time for simulation and rendering. */
        const double nowTime = glfwGetTime();
        const float deltaTime = static_cast<float>(nowTime - lastTime);
        lastTime = nowTime;
        const float clampedDeltaTime =
            deltaTime > editorConfig.MaxDeltaTime
            ? editorConfig.MaxDeltaTime
            : deltaTime;

        /* Handle window resize by recreating swapchain dependent resources. */
        if (window.WasResized())
        {
            std::int32_t width = 0;
            std::int32_t height = 0;
            window.GetSize(width, height);

            if (width > 0 && height > 0)
            {
                engine.OnResize(width, height);
            }

            window.ResetResizeFlag();
        }

        /* Update and render a single engine frame. */
        engine.Tick(clampedDeltaTime);
    }

    /* Cleanup is performed by the scope guard. */
    cleanup.Release();

    return 0;
}
