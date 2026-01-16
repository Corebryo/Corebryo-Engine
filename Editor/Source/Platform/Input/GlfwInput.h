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

struct GLFWwindow;

/* Handles GLFW input callbacks and state management. */

class GlfwInput
{
public:
    /* Attach input callbacks to GLFW window. */
    static void Attach(GLFWwindow* WindowHandle);

    /* Apply engine input mode and mouse capture state. */
    static void ApplyInputMode(GLFWwindow* WindowHandle);

private:
    /* Handle key events. */
    static void KeyCallback(GLFWwindow* Window, int Key, int Scancode, int Action, int Mods);

    /* Handle mouse position events. */
    static void MouseCallback(GLFWwindow* Window, double XPos, double YPos);    

    /* Handle mouse button events. */
    static void MouseButtonCallback(GLFWwindow* Window, int Button, int Action, int Mods);

    /* Handle mouse scroll events. */
    static void ScrollCallback(GLFWwindow* Window, double XOffset, double YOffset);

    /* Handle window focus events. */
    static void WindowFocusCallback(GLFWwindow* Window, int Focused);

    /* Enable or disable raw mouse motion when supported. */
    static void UpdateRawMouseMotion(GLFWwindow* Window, bool Enable);
};
