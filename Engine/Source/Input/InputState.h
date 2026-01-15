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

#include <cstddef>
#include <cstdint>

/* Supported physical keys. */
enum class InputKey : std::uint8_t
{
    None = 0,
    W,
    A,
    S,
    D,
    ShiftLeft,
    ShiftRight,
    MouseLeft,
    MouseRight,
    MouseMiddle,
    Count
};

/* Engine input modes. */
enum class InputMode : std::uint8_t
{
    Game = 0,
    Editor,
    UI
};

/* Logical actions triggered by input. */
enum class InputAction : std::uint8_t
{
    MoveForward = 0,
    MoveLeft,
    MoveBackward,
    MoveRight,
    FastMove,
    MousePrimary,
    MouseSecondary,
    MouseMiddle,
    Count
};

/* Logical axes produced by input. */
enum class InputAxis : std::uint8_t
{
    LookX = 0,
    LookY,
    ScrollX,
    ScrollY,
    Count
};

constexpr std::size_t InputKeyCount = static_cast<std::size_t>(InputKey::Count);
constexpr std::size_t InputActionCount =
    static_cast<std::size_t>(InputAction::Count);
constexpr std::size_t InputAxisCount =
    static_cast<std::size_t>(InputAxis::Count);

/* Action binding maps keys to a named action. */
struct ActionBinding
{
    InputKey Primary = InputKey::None;
    InputKey Secondary = InputKey::None;
};

/* Axis binding maps keys or mouse motion to a named axis. */
struct AxisBinding
{
    InputKey Negative = InputKey::None;
    InputKey Positive = InputKey::None;
    bool UseMouseDelta = false;
    bool UseScrollDelta = false;
    float Scale = 1.0f;
};

/* Runtime configurable input bindings. */
struct InputMapping
{
    ActionBinding Actions[InputActionCount]{};
    AxisBinding Axes[InputAxisCount]{};
};

/* Frame snapshot consumed by deterministic update loops. */
struct InputFrame
{
    bool Actions[InputActionCount]{};
    bool ActionsPressed[InputActionCount]{};
    bool ActionsReleased[InputActionCount]{};
    float Axes[InputAxisCount]{};
    float Yaw = 90.0f;
    float Pitch = 0.0f;
};

/* Global input state updated by the platform layer. */
struct InputState
{
    /* Keyboard */
    bool Keys[InputKeyCount]{};
    bool PrevActions[InputActionCount]{};

    /* Mouse */
    bool FirstMouse = true;
    float MouseX = 0.0f;
    float MouseY = 0.0f;
    float MouseDeltaX = 0.0f;
    float MouseDeltaY = 0.0f;
    float ScrollDeltaX = 0.0f;
    float ScrollDeltaY = 0.0f;
    bool WindowFocused = false;
    bool WantsMouseCapture = false;
    bool MouseCaptured = false;

    /* Camera */
    float Yaw = 90.0f;
    float Pitch = 0.0f;
};

/* Update raw key state from the platform layer. */
void SetKeyState(InputKey Key, bool Pressed);

/* Accumulate mouse deltas for the current frame. */
void AddMouseDelta(float DeltaX, float DeltaY);

/* Accumulate scroll deltas for the current frame. */
void AddScrollDelta(float DeltaX, float DeltaY);

/* Update focus state from the platform layer. */
void SetWindowFocused(bool Focused);

/* Set the active engine input mode. */
void SetInputMode(InputMode Mode);

/* Get the active engine input mode. */
InputMode GetInputMode();

/* Request or clear mouse capture for the next frame. */
void RequestMouseCapture(bool Capture);

/* Query whether mouse capture is desired for the current mode. */
bool ShouldCaptureMouse();

/* Update the cached mouse capture state. */
void SetMouseCaptured(bool Captured);

/* Begin a new input frame by clearing transient state. */
void InputBeginFrame();

/* End input processing and snapshot the frame state. */
void InputEndFrame();

/* Access the immutable snapshot for the current frame. */
const InputFrame& GetFrameInput();

/* Configure or query the active input mapping. */
void SetInputMapping(const InputMapping& Mapping);
const InputMapping& GetInputMapping();

/* Convenience accessors for the current frame snapshot. */
bool GetActionState(InputAction Action);
bool GetActionPressed(InputAction Action);
bool GetActionReleased(InputAction Action);
float GetAxisValue(InputAxis Axis);

/* Global input state instance. */
extern InputState g_InputState;
