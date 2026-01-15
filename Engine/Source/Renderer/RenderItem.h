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

#include "../Math/MathTypes.h"

struct Mesh;

/* Simple material definition. */
struct Material
{
    /* Base surface color. */
    Vec3 BaseColor = Vec3(1.0f, 1.0f, 1.0f);

    /* Ambient lighting factor. */
    float Ambient = 0.0f;

    /* Base alpha for simple transparency. */
    float Alpha = 1.0f;
};

/* Render submission item. */
struct RenderItem
{
    /* Mesh to render. */
    Mesh* MeshPtr = nullptr;

    /* Material to apply. */
    Material* MaterialPtr = nullptr;

    /* Model transform matrix. */
    Mat4 Model = Mat4::Identity();
};
