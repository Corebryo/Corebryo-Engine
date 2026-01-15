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

#include <vulkan/vulkan.h>
#include <cstdint>

/* Wraps buffer creation and memory ownership. */

class VulkanBuffer
{
public:
    /* Initialize buffer state. */
    VulkanBuffer();

    /* Release buffer resources. */
    ~VulkanBuffer();

    /* Create vertex buffer with staging transfer. */
    bool CreateVertexBuffer(
        VkPhysicalDevice PhysicalDevice,
        VkDevice Device,
        VkQueue Queue,
        uint32_t QueueFamily,
        const void* Data,
        VkDeviceSize Size);

    /* Create index buffer with staging transfer. */
    bool CreateIndexBuffer(
        VkPhysicalDevice PhysicalDevice,
        VkDevice Device,
        VkQueue Queue,
        uint32_t QueueFamily,
        const uint32_t* Indices,
        uint32_t IndexCount);

    /* Destroy buffer resources. */
    void Destroy(VkDevice Device);

    /* Get buffer handle. */
    VkBuffer GetBuffer() const;

    /* Get memory handle. */
    VkDeviceMemory GetMemory() const;

    /* Create buffer with memory allocation. */
    bool CreateBuffer(
        VkPhysicalDevice PhysicalDevice,
        VkDevice Device,
        VkDeviceSize Size,
        VkBufferUsageFlags Usage,
        VkMemoryPropertyFlags Properties,
        VkBuffer& OutBuffer,
        VkDeviceMemory& OutMemory);

private:
    /* Find suitable memory type for requirements. */
    uint32_t FindMemoryType(
        VkPhysicalDevice PhysicalDevice,
        uint32_t TypeFilter,
        VkMemoryPropertyFlags Properties);

    /* Copy buffer contents using command buffer. */
    void CopyBuffer(
        VkDevice Device,
        VkQueue Queue,
        uint32_t QueueFamily,
        VkBuffer SrcBuffer,
        VkBuffer DstBuffer,
        VkDeviceSize Size);

private:
    VkBuffer Buffer;
    VkDeviceMemory Memory;
    VkCommandPool CommandPool;
    VkDevice DeviceHandle;
    VkDeviceSize BufferSize;
};
