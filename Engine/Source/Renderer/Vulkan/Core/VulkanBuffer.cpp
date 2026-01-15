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

#include "VulkanBuffer.h"

#include <vulkan/vulkan.h>

#include <cstdio>
#include <cstring>
#include <stdexcept>

VulkanBuffer::VulkanBuffer()
    : Buffer(VK_NULL_HANDLE)
    , Memory(VK_NULL_HANDLE)
    , CommandPool(VK_NULL_HANDLE)
    , DeviceHandle(VK_NULL_HANDLE)
    , BufferSize(0)
{
    /* Initialize to null state. */
}

VulkanBuffer::~VulkanBuffer()
{
    /* Resources are destroyed externally via Destroy(). */
}

bool VulkanBuffer::CreateVertexBuffer(
    VkPhysicalDevice PhysicalDevice,
    VkDevice Device,
    VkQueue Queue,
    uint32_t QueueFamily,
    const void* Data,
    VkDeviceSize Size)
{
    BufferSize = Size;
    DeviceHandle = Device;

    /* Create command pool for transfer operations if needed. */
    if (CommandPool == VK_NULL_HANDLE)
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        poolInfo.queueFamilyIndex = QueueFamily;

        if (vkCreateCommandPool(Device, &poolInfo, nullptr, &CommandPool) != VK_SUCCESS)
        {
            std::fprintf(stderr, "VulkanBuffer::CreateVertexBuffer: vkCreateCommandPool failed\n");
            return false;
        }
    }

    /* Create staging buffer for CPU-to-GPU transfer. */
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;

    if (!CreateBuffer(
        PhysicalDevice,
        Device,
        Size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingMemory))
    {
        std::fprintf(stderr, "VulkanBuffer::CreateVertexBuffer: CreateBuffer (staging) failed\n");
        return false;
    }

    /* Map staging memory and copy vertex data. */
    void* mappedData;
    vkMapMemory(Device, stagingMemory, 0, Size, 0, &mappedData);
    memcpy(mappedData, Data, static_cast<size_t>(Size));
    vkUnmapMemory(Device, stagingMemory);

    /* Create device-local vertex buffer. */
    if (!CreateBuffer(
        PhysicalDevice,
        Device,
        Size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        Buffer,
        Memory))
    {
        std::fprintf(stderr, "VulkanBuffer::CreateVertexBuffer: CreateBuffer (vertex) failed\n");
        vkDestroyBuffer(Device, stagingBuffer, nullptr);
        vkFreeMemory(Device, stagingMemory, nullptr);
        return false;
    }

    /* Transfer data from staging to vertex buffer. */
    CopyBuffer(Device, Queue, QueueFamily, stagingBuffer, Buffer, Size);

    /* Clean up staging resources. */
    vkDestroyBuffer(Device, stagingBuffer, nullptr);
    vkFreeMemory(Device, stagingMemory, nullptr);

    return true;
}

bool VulkanBuffer::CreateIndexBuffer(
    VkPhysicalDevice PhysicalDevice,
    VkDevice Device,
    VkQueue Queue,
    uint32_t QueueFamily,
    const uint32_t* Indices,
    uint32_t IndexCount)
{
    VkDeviceSize size = IndexCount * sizeof(uint32_t);
    BufferSize = size;
    DeviceHandle = Device;

    /* Create command pool for transfer operations if needed. */
    if (CommandPool == VK_NULL_HANDLE)
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        poolInfo.queueFamilyIndex = QueueFamily;

        if (vkCreateCommandPool(Device, &poolInfo, nullptr, &CommandPool) != VK_SUCCESS)
        {
            std::fprintf(stderr, "VulkanBuffer::CreateIndexBuffer: vkCreateCommandPool failed\n");
            return false;
        }
    }

    /* Create staging buffer for CPU-to-GPU transfer. */
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;

    if (!CreateBuffer(
        PhysicalDevice,
        Device,
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingMemory))
    {
        std::fprintf(stderr, "VulkanBuffer::CreateIndexBuffer: CreateBuffer (staging) failed\n");
        return false;
    }

    /* Map staging memory and copy index data. */
    void* mappedData;
    vkMapMemory(Device, stagingMemory, 0, size, 0, &mappedData);
    memcpy(mappedData, Indices, static_cast<size_t>(size));
    vkUnmapMemory(Device, stagingMemory);

    /* Create device-local index buffer. */
    if (!CreateBuffer(
        PhysicalDevice,
        Device,
        size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        Buffer,
        Memory))
    {
        std::fprintf(stderr, "VulkanBuffer::CreateIndexBuffer: CreateBuffer (index) failed\n");
        vkDestroyBuffer(Device, stagingBuffer, nullptr);
        vkFreeMemory(Device, stagingMemory, nullptr);
        return false;
    }

    /* Transfer data from staging to index buffer. */
    CopyBuffer(Device, Queue, QueueFamily, stagingBuffer, Buffer, size);

    /* Clean up staging resources. */
    vkDestroyBuffer(Device, stagingBuffer, nullptr);
    vkFreeMemory(Device, stagingMemory, nullptr);

    return true;
}

void VulkanBuffer::Destroy(VkDevice Device)
{
    /* Destroy buffer if allocated. */
    if (Buffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(Device, Buffer, nullptr);
        Buffer = VK_NULL_HANDLE;
    }

    /* Free memory if allocated. */
    if (Memory != VK_NULL_HANDLE)
    {
        vkFreeMemory(Device, Memory, nullptr);
        Memory = VK_NULL_HANDLE;
    }

    /* Destroy command pool if created. */
    if (CommandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(Device, CommandPool, nullptr);
        CommandPool = VK_NULL_HANDLE;
    }

    /* Reset state. */
    DeviceHandle = VK_NULL_HANDLE;
    BufferSize = 0;
}

VkBuffer VulkanBuffer::GetBuffer() const
{
    /* Return buffer handle. */
    return Buffer;
}

VkDeviceMemory VulkanBuffer::GetMemory() const
{
    /* Return memory handle. */
    return Memory;
}

bool VulkanBuffer::CreateBuffer(
    VkPhysicalDevice PhysicalDevice,
    VkDevice Device,
    VkDeviceSize Size,
    VkBufferUsageFlags Usage,
    VkMemoryPropertyFlags Properties,
    VkBuffer& OutBuffer,
    VkDeviceMemory& OutMemory)
{
    /* Create buffer object. */
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = Size;
    bufferInfo.usage = Usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(Device, &bufferInfo, nullptr, &OutBuffer) != VK_SUCCESS)
    {
        std::fprintf(stderr, "VulkanBuffer::CreateBuffer: vkCreateBuffer failed\n");
        return false;
    }

    /* Query memory requirements. */
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(Device, OutBuffer, &memRequirements);

    /* Allocate device memory. */
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(PhysicalDevice, memRequirements.memoryTypeBits, Properties);

    if (vkAllocateMemory(Device, &allocInfo, nullptr, &OutMemory) != VK_SUCCESS)
    {
        std::fprintf(stderr, "VulkanBuffer::CreateBuffer: vkAllocateMemory failed\n");
        vkDestroyBuffer(Device, OutBuffer, nullptr);
        return false;
    }

    /* Bind buffer to memory. */
    vkBindBufferMemory(Device, OutBuffer, OutMemory, 0);
    return true;
}

uint32_t VulkanBuffer::FindMemoryType(
    VkPhysicalDevice PhysicalDevice,
    uint32_t TypeFilter,
    VkMemoryPropertyFlags Properties)
{
    /* Query available memory types. */
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &memProperties);

    /* Find suitable memory type. */
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((TypeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & Properties) == Properties)
        {
            return i;
        }
    }

    std::fprintf(stderr, "VulkanBuffer::FindMemoryType: no suitable memory type found\n");
    return UINT32_MAX;
}

void VulkanBuffer::CopyBuffer(
    VkDevice Device,
    VkQueue Queue,
    uint32_t QueueFamily,
    VkBuffer SrcBuffer,
    VkBuffer DstBuffer,
    VkDeviceSize Size)
{
    (void)QueueFamily;

    /* Allocate one-time command buffer. */
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = CommandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(Device, &allocInfo, &commandBuffer);

    /* Begin one-time command buffer. */
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    /* Record copy command. */
    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = Size;

    vkCmdCopyBuffer(commandBuffer, SrcBuffer, DstBuffer, 1, &copyRegion);

    /* Submit and wait for completion. */
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkEndCommandBuffer(commandBuffer);
    vkQueueSubmit(Queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(Queue);

    /* Clean up command buffer. */
    vkFreeCommandBuffers(Device, CommandPool, 1, &commandBuffer);
}
