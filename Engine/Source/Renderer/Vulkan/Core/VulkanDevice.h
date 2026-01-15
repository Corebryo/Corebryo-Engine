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

/* Owns physical and logical device handles and queues. */

class VulkanDevice
{
public:
    /* Initialize device wrapper state. */
    VulkanDevice();

    /* Release device resources. */
    ~VulkanDevice();

    /* Select physical device and create logical device. */
    bool Create(VkInstance Instance);

    /* Destroy logical device. */
    void Destroy();

    /* Get physical device handle. */
    VkPhysicalDevice GetPhysicalDevice() const;

    /* Get logical device handle. */
    VkDevice GetDevice() const;

    /* Get graphics queue handle. */
    VkQueue GetGraphicsQueue() const;

    /* Get graphics queue family index. */
    uint32_t GetGraphicsQueueFamily() const;

private:
    /* Select suitable physical device. */
    bool PickPhysicalDevice(VkInstance Instance);

    /* Find graphics-capable queue family. */
    bool FindGraphicsQueueFamily(VkPhysicalDevice PhysicalDevice);

    /* Create logical device and queues. */
    bool CreateLogicalDevice();

private:
    /* Selected GPU handle. */
    VkPhysicalDevice PhysicalDevice;

    /* Logical device handle. */
    VkDevice Device;

    /* Graphics queue handle. */
    VkQueue GraphicsQueue;

    /* Graphics queue family index. */
    uint32_t GraphicsQueueFamily;
};
