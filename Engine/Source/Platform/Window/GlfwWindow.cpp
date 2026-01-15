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

#include "GlfwWindow.h"

#include "../../Input/InputState.h"
#include "../PlatformIncludes.h"

#if defined(_WIN32)
#include "../Windows/Win32WindowUtils.h"
#endif

#include <cstdio>

/* Global GLFW reference counter. */
static std::int32_t G_GlfwRefCount = 0;

/* Initialize window state. */
GlfwWindow::GlfwWindow()
    : Handle(nullptr)
    , WindowWidth(0)
    , WindowHeight(0)
    , ResizedFlag(false)
    , IsWindowFocused(false)
{
}

/* Destroy window resources. */
GlfwWindow::~GlfwWindow()
{
    Destroy();
}

/* Create GLFW window. */
bool GlfwWindow::Create(std::int32_t Width, std::int32_t Height, const char* Title)
{
    if (G_GlfwRefCount == 0)
    {
        if (!glfwInit())
        {
            std::fprintf(stderr, "GlfwWindow::Create: glfwInit failed\n");
            return false;
        }
    }
    ++G_GlfwRefCount;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    WindowWidth = Width;
    WindowHeight = Height;
    ResizedFlag = false;
    IsWindowFocused = false;

    Handle = glfwCreateWindow(Width, Height, Title, nullptr, nullptr);
    if (!Handle)
    {
        std::fprintf(stderr, "GlfwWindow::Create: glfwCreateWindow failed\n");
        Destroy();
        return false;
    }

    glfwSetWindowUserPointer(Handle, this);

    GLFWmonitor* PrimaryMonitor = glfwGetPrimaryMonitor();
    if (PrimaryMonitor)
    {
        const GLFWvidmode* VideoMode = glfwGetVideoMode(PrimaryMonitor);
        if (VideoMode)
        {
            const int CenterX = (VideoMode->width - Width) / 2;
            const int CenterY = (VideoMode->height - Height) / 2;
            glfwSetWindowPos(Handle, CenterX, CenterY);
        }
    }

    glfwSetWindowFocusCallback(
        Handle,
        [](GLFWwindow* Window, int Focused)
        {
            GlfwWindow* Self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(Window));
            if (Self)
            {
                Self->SetFocusState(Focused == GLFW_TRUE);
            }
        });

    glfwSetFramebufferSizeCallback(
        Handle,
        [](GLFWwindow* Window, int Width, int Height)
        {
            GlfwWindow* Self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(Window));
            if (!Self)
                return;

            Self->WindowWidth = Width;
            Self->WindowHeight = Height;
            Self->ResizedFlag = true;
        });

#if defined(_WIN32)
    Win32WindowUtils::ApplyDarkTitlebar(glfwGetWin32Window(Handle));
#endif

    glfwShowWindow(Handle);

    return true;
}

/* Destroy GLFW window. */
void GlfwWindow::Destroy()
{
    if (Handle)
    {
        glfwDestroyWindow(Handle);
        Handle = nullptr;
    }

    if (G_GlfwRefCount > 0)
    {
        --G_GlfwRefCount;
        if (G_GlfwRefCount == 0)
        {
            glfwTerminate();
        }
    }

    ResizedFlag = false;
    IsWindowFocused = false;
    WindowWidth = 0;
    WindowHeight = 0;
}

/* Poll window events. */
void GlfwWindow::PollEvents() const
{
    glfwPollEvents();
}

/* Check if window should close. */
bool GlfwWindow::ShouldClose() const
{
    if (!Handle)
        return true;

    return glfwWindowShouldClose(Handle);
}

/* Get GLFW window handle. */
GLFWwindow* GlfwWindow::GetHandle() const
{
    return Handle;
}

/* Get cached window size. */
void GlfwWindow::GetSize(std::int32_t& Width, std::int32_t& Height) const
{
    Width = WindowWidth;
    Height = WindowHeight;
}

/* Check if window was resized. */
bool GlfwWindow::WasResized()
{
    if (!ResizedFlag)
        return false;

    ResizedFlag = false;
    return true;
}

/* Reset resize flag. */
void GlfwWindow::ResetResizeFlag()
{
    ResizedFlag = false;
}

/* Update focus state. */
void GlfwWindow::SetFocusState(bool Focused)
{
    IsWindowFocused = Focused;
}

/* Check if window is focused. */
bool GlfwWindow::IsFocused() const
{
    return IsWindowFocused;
}

/* Bring window to front. */
void GlfwWindow::BringToFront()
{
    if (!Handle)
        return;

#if defined(_WIN32)
    Win32WindowUtils::BringWindowToFront(glfwGetWin32Window(Handle));
#endif

    IsWindowFocused = true;
}
