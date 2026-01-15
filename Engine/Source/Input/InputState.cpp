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

#include "InputState.h"

/* Active input mode managed by the engine layer. */
static InputMode g_InputMode = InputMode::Game;

namespace
{
    /* Tuning constants for default look input. */
    constexpr float MouseSensitivity = 0.1f;
    constexpr float MaxPitch = 89.0f;

    /* Build the default action/axis mappings. */
    InputMapping CreateDefaultMapping()
    {
        InputMapping mapping{};
        mapping.Actions[static_cast<std::size_t>(InputAction::MoveForward)] =
            { InputKey::W, InputKey::None };
        mapping.Actions[static_cast<std::size_t>(InputAction::MoveLeft)] =
            { InputKey::A, InputKey::None };
        mapping.Actions[static_cast<std::size_t>(InputAction::MoveBackward)] =
            { InputKey::S, InputKey::None };
        mapping.Actions[static_cast<std::size_t>(InputAction::MoveRight)] =
            { InputKey::D, InputKey::None };
        mapping.Actions[static_cast<std::size_t>(InputAction::FastMove)] =
            { InputKey::ShiftLeft, InputKey::ShiftRight };
        mapping.Actions[static_cast<std::size_t>(InputAction::MousePrimary)] =
            { InputKey::MouseLeft, InputKey::None };
        mapping.Actions[static_cast<std::size_t>(InputAction::MouseSecondary)] =
            { InputKey::MouseRight, InputKey::None };
        mapping.Actions[static_cast<std::size_t>(InputAction::MouseMiddle)] =
            { InputKey::MouseMiddle, InputKey::None };

        AxisBinding lookX{};
        lookX.UseMouseDelta = true;
        lookX.Scale = MouseSensitivity;
        mapping.Axes[static_cast<std::size_t>(InputAxis::LookX)] = lookX;

        AxisBinding lookY{};
        lookY.UseMouseDelta = true;
        lookY.Scale = MouseSensitivity;
        mapping.Axes[static_cast<std::size_t>(InputAxis::LookY)] = lookY;

        AxisBinding scrollX{};
        scrollX.UseScrollDelta = true;
        scrollX.Scale = 1.0f;
        mapping.Axes[static_cast<std::size_t>(InputAxis::ScrollX)] = scrollX;

        AxisBinding scrollY{};
        scrollY.UseScrollDelta = true;
        scrollY.Scale = 1.0f;
        mapping.Axes[static_cast<std::size_t>(InputAxis::ScrollY)] = scrollY;

        return mapping;
    }

    /* Resolve a key state with bounds checking. */
    bool IsKeyDown(InputKey key)
    {
        const std::size_t index = static_cast<std::size_t>(key);

        if (index >= InputKeyCount)
        {
            return false;
        }

        return g_InputState.Keys[index];
    }

    /* Resolve an action from the current key state. */
    bool ResolveAction(const ActionBinding& binding)
    {
        return IsKeyDown(binding.Primary) || IsKeyDown(binding.Secondary);
    }

    /* Resolve an axis value from the current key or mouse state. */
    float ResolveAxis(const AxisBinding& binding, InputAxis axis)
    {
        float value = 0.0f;

    if (binding.UseMouseDelta)
    {
        if (g_InputMode == InputMode::Editor)
        {
            const bool wantsLook =
                IsKeyDown(InputKey::MouseRight) ||
                IsKeyDown(InputKey::MouseMiddle);

            if (!wantsLook)
            {
                return 0.0f;
            }
        }
        else if (g_InputMode != InputMode::Game)
        {
            return 0.0f;
        }

        value = (axis == InputAxis::LookX)
            ? g_InputState.MouseDeltaX
            : g_InputState.MouseDeltaY;
        }
        else if (binding.UseScrollDelta)
        {
            value = (axis == InputAxis::ScrollX)
                ? g_InputState.ScrollDeltaX
                : g_InputState.ScrollDeltaY;
        }
        else
        {
            if (IsKeyDown(binding.Positive))
            {
                value += 1.0f;
            }

            if (IsKeyDown(binding.Negative))
            {
                value -= 1.0f;
            }
        }

        return value * binding.Scale;
    }
}

/* Global input state instance. */
InputState g_InputState;
static InputFrame g_FrameInput;
static InputMapping g_InputMapping = CreateDefaultMapping();

/* Update raw key state from the platform layer. */
void SetKeyState(InputKey Key, bool Pressed)
{
    const std::size_t index = static_cast<std::size_t>(Key);

    if (index >= InputKeyCount)
    {
        return;
    }

    g_InputState.Keys[index] = Pressed;
}

/* Accumulate raw mouse deltas for the current frame. */
void AddMouseDelta(float DeltaX, float DeltaY)
{
    g_InputState.MouseDeltaX += DeltaX;
    g_InputState.MouseDeltaY += DeltaY;
}

/* Accumulate raw scroll deltas for the current frame. */
void AddScrollDelta(float DeltaX, float DeltaY)
{
    g_InputState.ScrollDeltaX += DeltaX;
    g_InputState.ScrollDeltaY += DeltaY;
}

/* Update focus state from the platform layer. */
void SetWindowFocused(bool Focused)
{
    g_InputState.WindowFocused = Focused;

    if (!Focused)
    {
        g_InputState.WantsMouseCapture = false;
    }
}

/* Set the active engine input mode. */
void SetInputMode(InputMode Mode)
{
    g_InputMode = Mode;
    RequestMouseCapture(Mode == InputMode::Game);
}

/* Get the active engine input mode. */
InputMode GetInputMode()
{
    return g_InputMode;
}

/* Request or clear mouse capture for the next frame. */
void RequestMouseCapture(bool Capture)
{
    g_InputState.WantsMouseCapture = Capture;
}

/* Query whether mouse capture is desired for the current mode. */
bool ShouldCaptureMouse()
{
    if (g_InputMode != InputMode::Game)
    {
        return false;
    }

    return g_InputState.WindowFocused && g_InputState.WantsMouseCapture;
}

/* Update the cached mouse capture state. */
void SetMouseCaptured(bool Captured)
{
    g_InputState.MouseCaptured = Captured;
}

/* Clear transient input values at frame start. */
void InputBeginFrame()
{
    /* Reset transient mouse deltas at the start of each frame. */
    g_InputState.MouseDeltaX = 0.0f;
    g_InputState.MouseDeltaY = 0.0f;
    g_InputState.ScrollDeltaX = 0.0f;
    g_InputState.ScrollDeltaY = 0.0f;
}

/* Resolve mappings and snapshot deterministic input for the frame. */
void InputEndFrame()
{
    /* Evaluate axis bindings first so camera rotation uses frame inputs. */
    for (std::size_t i = 0; i < InputAxisCount; ++i)
    {
        const InputAxis axis = static_cast<InputAxis>(i);
        g_FrameInput.Axes[i] = ResolveAxis(
            g_InputMapping.Axes[i],
            axis);
    }

    /* Apply accumulated axes to camera rotation. */
    g_InputState.Yaw += g_FrameInput.Axes[static_cast<std::size_t>(
        InputAxis::LookX)];
    g_InputState.Pitch += g_FrameInput.Axes[static_cast<std::size_t>(
        InputAxis::LookY)];

    if (g_InputState.Pitch > MaxPitch)
        g_InputState.Pitch = MaxPitch;

    if (g_InputState.Pitch < -MaxPitch)
        g_InputState.Pitch = -MaxPitch;

    /* Resolve action states and edge transitions. */
    for (std::size_t i = 0; i < InputActionCount; ++i)
    {
        const bool active = ResolveAction(g_InputMapping.Actions[i]);
        g_FrameInput.Actions[i] = active;
        g_FrameInput.ActionsPressed[i] = active && !g_InputState.PrevActions[i];
        g_FrameInput.ActionsReleased[i] = !active && g_InputState.PrevActions[i];
        g_InputState.PrevActions[i] = active;
    }

    /* Cache camera angles for the frame snapshot. */
    g_FrameInput.Yaw = g_InputState.Yaw;
    g_FrameInput.Pitch = g_InputState.Pitch;
}

/* Access the immutable input snapshot for the current frame. */
const InputFrame& GetFrameInput()
{
    return g_FrameInput;
}

/* Replace the active input mapping. */
void SetInputMapping(const InputMapping& Mapping)
{
    g_InputMapping = Mapping;
}

/* Query the active input mapping. */
const InputMapping& GetInputMapping()
{
    return g_InputMapping;
}

/* Query whether an action is active for the current frame. */
bool GetActionState(InputAction Action)
{
    const std::size_t index = static_cast<std::size_t>(Action);

    if (index >= InputActionCount)
    {
        return false;
    }

    return g_FrameInput.Actions[index];
}

/* Query whether an action was pressed this frame. */
bool GetActionPressed(InputAction Action)
{
    const std::size_t index = static_cast<std::size_t>(Action);

    if (index >= InputActionCount)
    {
        return false;
    }

    return g_FrameInput.ActionsPressed[index];
}

/* Query whether an action was released this frame. */
bool GetActionReleased(InputAction Action)
{
    const std::size_t index = static_cast<std::size_t>(Action);

    if (index >= InputActionCount)
    {
        return false;
    }

    return g_FrameInput.ActionsReleased[index];
}

/* Query an axis value for the current frame. */
float GetAxisValue(InputAxis Axis)
{
    const std::size_t index = static_cast<std::size_t>(Axis);

    if (index >= InputAxisCount)
    {
        return 0.0f;
    }

    return g_FrameInput.Axes[index];
}
