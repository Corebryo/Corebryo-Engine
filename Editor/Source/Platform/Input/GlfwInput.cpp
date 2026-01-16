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

#include "GlfwInput.h"

#include "Engine/EngineState.h"
#include "Input/InputState.h"
#include "../PlatformIncludes.h"

/* Translate key */
namespace
{
    /* Translate GLFW key codes to input keys. */
    InputKey TranslateKey(int key)
    {
        switch (key)
        {
            case GLFW_KEY_W:
                return InputKey::W;
            case GLFW_KEY_A:
                return InputKey::A;
            case GLFW_KEY_S:
                return InputKey::S;
            case GLFW_KEY_D:
                return InputKey::D;
            case GLFW_KEY_LEFT_SHIFT:
                return InputKey::ShiftLeft;
            case GLFW_KEY_RIGHT_SHIFT:
                return InputKey::ShiftRight;
            default:
                return InputKey::None;
        }
    }

    /* Translate GLFW mouse buttons to input keys. */
    InputKey TranslateMouseButton(int button)
    {
        switch (button)
        {
            case GLFW_MOUSE_BUTTON_LEFT:
                return InputKey::MouseLeft;
            case GLFW_MOUSE_BUTTON_RIGHT:
                return InputKey::MouseRight;
            case GLFW_MOUSE_BUTTON_MIDDLE:
                return InputKey::MouseMiddle;
            default:
                return InputKey::None;
        }
    }
}

/* Toggle raw mouse motion for high precision camera input. */
void GlfwInput::UpdateRawMouseMotion(GLFWwindow* window, bool enable)
{
    if (!glfwRawMouseMotionSupported())
    {
        return;
    }

    glfwSetInputMode(
        window,
        GLFW_RAW_MOUSE_MOTION,
        enable ? GLFW_TRUE : GLFW_FALSE);
}

/* Apply engine input mode and mouse capture state. */
void GlfwInput::ApplyInputMode(GLFWwindow* window)
{
    const bool shouldCapture = ShouldCaptureMouse();
    const int currentMode = glfwGetInputMode(window, GLFW_CURSOR);
    const bool isCaptured = (currentMode == GLFW_CURSOR_DISABLED);

    if (shouldCapture && !isCaptured)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        UpdateRawMouseMotion(window, true);
        g_InputState.FirstMouse = true;
        SetMouseCaptured(true);
    }

    else if (!shouldCapture && isCaptured)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        UpdateRawMouseMotion(window, false);
        g_InputState.FirstMouse = true;
        SetMouseCaptured(false);
    }
}

/* Attach all required GLFW callbacks to the window */
void GlfwInput::Attach(GLFWwindow* window)
{
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetCursorPosCallback(window, MouseCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetScrollCallback(window, ScrollCallback);
    glfwSetWindowFocusCallback(window, WindowFocusCallback);

    /* Ensure raw mouse motion starts disabled. */
    UpdateRawMouseMotion(window, false);

    /* Cache the current focus state for input mode evaluation. */
    SetWindowFocused(glfwGetWindowAttrib(window, GLFW_FOCUSED) == GLFW_TRUE);
}

/* Keyboard input callback */
void GlfwInput::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    (void)scancode;
    (void)mods;

    bool pressed = (action == GLFW_PRESS);
    bool released = (action == GLFW_RELEASE);

    /* Handle key press and release events */
    switch (key)
    {
        case GLFW_KEY_F1:
            if (pressed)
            {
                g_CurrentEngineState = EngineState::Editor;
                RequestMouseCapture(false);
            }
            break;
        case GLFW_KEY_F2:
            if (pressed)
            {
                g_CurrentEngineState = EngineState::Game;
            }
            break;
        case GLFW_KEY_ESCAPE:
            if (pressed &&
                GetInputMode() != InputMode::Game &&
                glfwGetWindowAttrib(window, GLFW_FOCUSED))
            {
                RequestMouseCapture(false);
            }
            break;
    }

    if (pressed || released)
    {
        const InputKey mappedKey = TranslateKey(key);
        if (mappedKey != InputKey::None)
        {
            SetKeyState(mappedKey, pressed);
        }
    }
}

/* Mouse movement callback */
void GlfwInput::MouseCallback(GLFWwindow* window, double xpos, double ypos)     
{
    /* Ignore mouse movement if cursor is not captured or editor look inactive. */
    const int cursorMode = glfwGetInputMode(window, GLFW_CURSOR);
    bool allowLook = (cursorMode == GLFW_CURSOR_DISABLED);

    if (!allowLook && GetInputMode() == InputMode::Editor)
    {
        const std::size_t rightIndex =
            static_cast<std::size_t>(InputKey::MouseRight);
        const std::size_t middleIndex =
            static_cast<std::size_t>(InputKey::MouseMiddle);
        allowLook = g_InputState.Keys[rightIndex] || g_InputState.Keys[middleIndex];
    }

    if (!allowLook)
    {
        return;
    }

    /* Initialize mouse position on first movement. */
    if (g_InputState.FirstMouse)
    {
        g_InputState.MouseX = static_cast<float>(xpos);
        g_InputState.MouseY = static_cast<float>(ypos);
        g_InputState.FirstMouse = false;
        return;
    }

    /* Accumulate mouse deltas for the frame. */
    const float xoffset = static_cast<float>(xpos) - g_InputState.MouseX;
    const float yoffset = g_InputState.MouseY - static_cast<float>(ypos);

    g_InputState.MouseX = static_cast<float>(xpos);
    g_InputState.MouseY = static_cast<float>(ypos);
    AddMouseDelta(xoffset, yoffset);
}

/* Mouse button callback */
void GlfwInput::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    (void)mods;

    /* Capture mouse cursor on left click. */
    if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT &&
        GetInputMode() == InputMode::Game)
    {
        if (glfwGetWindowAttrib(window, GLFW_FOCUSED))
        {
            RequestMouseCapture(true);
        }
    }

    if (action == GLFW_PRESS &&
        GetInputMode() == InputMode::Editor &&
        (button == GLFW_MOUSE_BUTTON_RIGHT ||
            button == GLFW_MOUSE_BUTTON_MIDDLE))
    {
        g_InputState.FirstMouse = true;
    }

    if (action == GLFW_PRESS || action == GLFW_RELEASE)
    {
        const InputKey mappedButton = TranslateMouseButton(button);
        if (mappedButton != InputKey::None)
        {
            SetKeyState(mappedButton, action == GLFW_PRESS);
        }
    }
}

/* Mouse wheel callback. */
void GlfwInput::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    (void)window;
    AddScrollDelta(static_cast<float>(xoffset), static_cast<float>(yoffset));
}

/* Window focus callback */
void GlfwInput::WindowFocusCallback(GLFWwindow* window, int focused)
{
    if (focused == GLFW_TRUE)
    {
        /* Cache focus state for input mode evaluation. */
        SetWindowFocused(true);
    }
    else
    {
        /* Cache focus state and clear capture requests. */
        SetWindowFocused(false);
    }
}
