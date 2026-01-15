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

#pragma once

#include <cstdint>

struct GLFWwindow;

/* Manages GLFW window lifecycle and state. */

class GlfwWindow
{
public:
    /* Initialize window state. */
    GlfwWindow();

    /* Destroy window resources. */
    ~GlfwWindow();

    /* Create GLFW window. */
    bool Create(std::int32_t Width, std::int32_t Height, const char* Title);

    /* Destroy GLFW window. */
    void Destroy();

    /* Poll window events. */
    void PollEvents() const;

    /* Check if window should close. */
    bool ShouldClose() const;

    /* Get GLFW window handle. */
    GLFWwindow* GetHandle() const;

    /* Get cached window size. */
    void GetSize(std::int32_t& Width, std::int32_t& Height) const;

    /* Check if window was resized. */
    bool WasResized();

    /* Reset resize flag. */
    void ResetResizeFlag();

    /* Check if window is focused. */
    bool IsFocused() const;

    /* Bring window to front. */
    void BringToFront();

private:
    /* Update focus state. */
    void SetFocusState(bool Focused);

private:
    GLFWwindow* Handle;
    std::int32_t WindowWidth;
    std::int32_t WindowHeight;
    bool ResizedFlag;
    bool IsWindowFocused;
};
