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

#include "../Core/VulkanBuffer.h"

#include <vulkan/vulkan.h>
#include <cstdint>

/* GPU mesh data container. */
struct Mesh
{
    /* Vertex and index buffers */
    VulkanBuffer VertexBuffer;
    VulkanBuffer IndexBuffer;

    /* Geometry counts */
    std::uint32_t VertexCount = 0;
    std::uint32_t IndexCount = 0;
    bool HasIndex = false;

    /* Release GPU buffers and reset state. */
    void Destroy(VkDevice Device)
    {
        VertexBuffer.Destroy(Device);
        IndexBuffer.Destroy(Device);
        VertexCount = 0;
        IndexCount = 0;
        HasIndex = false;
    }
};
