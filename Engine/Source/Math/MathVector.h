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

#include <cmath>

/* 3D vector with float components. */
struct Vec3
{
    float x;
    float y;
    float z;

    /* Default constructor. */
    Vec3() : x(0.0f), y(0.0f), z(0.0f) {}

    /* Constructor with components. */
    Vec3(float X, float Y, float Z) : x(X), y(Y), z(Z) {}

    /* Vector addition. */
    Vec3 operator+(const Vec3& Other) const
    {
        return Vec3(x + Other.x, y + Other.y, z + Other.z);
    }

    /* Vector subtraction. */
    Vec3 operator-(const Vec3& Other) const
    {
        return Vec3(x - Other.x, y - Other.y, z - Other.z);
    }

    /* Scalar multiplication. */
    Vec3 operator*(float Scalar) const
    {
        return Vec3(x * Scalar, y * Scalar, z * Scalar);
    }

    /* Returns the vector length. */
    float Length() const
    {
        return std::sqrt(x * x + y * y + z * z);
    }

    /* Returns the normalized vector. */
    Vec3 Normalized() const
    {
        float Len = Length();
        if (Len == 0.0f)
            return Vec3();
        return Vec3(x / Len, y / Len, z / Len);
    }

    /* Returns the cross product of two vectors. */
    static Vec3 Cross(const Vec3& A, const Vec3& B)
    {
        return Vec3(
            A.y * B.z - A.z * B.y,
            A.z * B.x - A.x * B.z,
            A.x * B.y - A.y * B.x
        );
    }

    /* Returns the dot product of two vectors. */
    static float Dot(const Vec3& A, const Vec3& B)
    {
        return A.x * B.x + A.y * B.y + A.z * B.z;
    }
};
