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

#include "MathVector.h"
#include <cmath>

/* 4x4 column-major matrix with float components. */
struct Mat4
{
    float m[16];

    /* Returns an identity matrix. */
    static Mat4 Identity()
    {
        Mat4 Result{};
        Result.m[0] = 1.0f;
        Result.m[5] = 1.0f;
        Result.m[10] = 1.0f;
        Result.m[15] = 1.0f;
        return Result;
    }

    /* Returns a perspective projection matrix. */
    static Mat4 Perspective(float FovRadians, float Aspect, float Near, float Far)
    {
        Mat4 Result{};
        float TanHalfFov = std::tan(FovRadians * 0.5f);

        Result.m[0] = 1.0f / (Aspect * TanHalfFov);
        Result.m[5] = -1.0f / TanHalfFov;
        Result.m[10] = Far / (Near - Far);
        Result.m[11] = -1.0f;
        Result.m[14] = (Near * Far) / (Near - Far);

        return Result;
    }

    /* Returns an orthographic projection matrix. */
    static Mat4 Orthographic(float Left, float Right, float Bottom, float Top, float Near, float Far)
    {
        Mat4 Result = Identity();

        Result.m[0] = 2.0f / (Right - Left);
        Result.m[5] = -2.0f / (Top - Bottom);
        Result.m[10] = 1.0f / (Near - Far);
        Result.m[12] = -(Right + Left) / (Right - Left);
        Result.m[13] = (Top + Bottom) / (Top - Bottom);
        Result.m[14] = Near / (Near - Far);

        return Result;
    }

    /* Returns a look-at view matrix. */
    static Mat4 LookAt(const Vec3& Eye, const Vec3& Center, const Vec3& Up)
    {
        Vec3 F = (Center - Eye).Normalized();
        Vec3 S = Vec3::Cross(F, Up).Normalized();
        Vec3 U = Vec3::Cross(S, F);

        Mat4 Result = Identity();

        Result.m[0] = S.x;
        Result.m[4] = S.y;
        Result.m[8] = S.z;

        Result.m[1] = U.x;
        Result.m[5] = U.y;
        Result.m[9] = U.z;

        Result.m[2] = -F.x;
        Result.m[6] = -F.y;
        Result.m[10] = -F.z;

        Result.m[12] = -Vec3::Dot(S, Eye);
        Result.m[13] = -Vec3::Dot(U, Eye);
        Result.m[14] = Vec3::Dot(F, Eye);

        return Result;
    }

    /* Matrix multiplication. */
    Mat4 operator*(const Mat4& Other) const
    {
        Mat4 Result{};

        for (int Col = 0; Col < 4; ++Col)
        {
            for (int Row = 0; Row < 4; ++Row)
            {
                Result.m[Col * 4 + Row] =
                    m[0 * 4 + Row] * Other.m[Col * 4 + 0] +
                    m[1 * 4 + Row] * Other.m[Col * 4 + 1] +
                    m[2 * 4 + Row] * Other.m[Col * 4 + 2] +
                    m[3 * 4 + Row] * Other.m[Col * 4 + 3];
            }
        }

        return Result;
    }
};
