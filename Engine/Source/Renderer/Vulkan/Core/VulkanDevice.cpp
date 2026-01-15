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

#include "VulkanDevice.h"

#include <cstdio>
#include <vector>

VulkanDevice::VulkanDevice()
    : PhysicalDevice(VK_NULL_HANDLE),
    Device(VK_NULL_HANDLE),
    GraphicsQueue(VK_NULL_HANDLE),
    GraphicsQueueFamily(UINT32_MAX)
{
    /* Initialize to null state. */
}

VulkanDevice::~VulkanDevice()
{
    /* Destroy device resources. */
    Destroy();
}

bool VulkanDevice::Create(VkInstance Instance)
{
    /* Select suitable physical device. */
    if (!PickPhysicalDevice(Instance))
    {
        std::fprintf(stderr, "VulkanDevice::Create: PickPhysicalDevice failed\n");
        return false;
    }

    /* Find graphics queue family. */
    if (!FindGraphicsQueueFamily(PhysicalDevice))
    {
        std::fprintf(stderr, "VulkanDevice::Create: FindGraphicsQueueFamily failed\n");
        return false;
    }

    /* Create logical device and queues. */
    if (!CreateLogicalDevice())
    {
        std::fprintf(stderr, "VulkanDevice::Create: CreateLogicalDevice failed\n");
        return false;
    }

    return true;
}

void VulkanDevice::Destroy()
{
    /* Destroy logical device. */
    if (Device != VK_NULL_HANDLE)
    {
        vkDestroyDevice(Device, nullptr);
        Device = VK_NULL_HANDLE;
    }

    /* Reset state. */
    PhysicalDevice = VK_NULL_HANDLE;
    GraphicsQueue = VK_NULL_HANDLE;
    GraphicsQueueFamily = UINT32_MAX;
}

VkPhysicalDevice VulkanDevice::GetPhysicalDevice() const
{
    /* Return physical device handle. */
    return PhysicalDevice;
}

VkDevice VulkanDevice::GetDevice() const
{
    /* Return logical device handle. */
    return Device;
}

VkQueue VulkanDevice::GetGraphicsQueue() const
{
    /* Return graphics queue handle. */
    return GraphicsQueue;
}

uint32_t VulkanDevice::GetGraphicsQueueFamily() const
{
    /* Return graphics queue family index. */
    return GraphicsQueueFamily;
}

bool VulkanDevice::PickPhysicalDevice(VkInstance Instance)
{
    /* Enumerate available physical devices. */
    uint32_t DeviceCount = 0;
    vkEnumeratePhysicalDevices(Instance, &DeviceCount, nullptr);

    if (DeviceCount == 0)
    {
        std::fprintf(stderr, "VulkanDevice::PickPhysicalDevice: no devices found\n");
        return false;
    }

    std::vector<VkPhysicalDevice> Devices(DeviceCount);
    vkEnumeratePhysicalDevices(Instance, &DeviceCount, Devices.data());

    /* Prefer discrete GPU over integrated. */
    for (VkPhysicalDevice Candidate : Devices)
    {
        VkPhysicalDeviceProperties Properties{};
        vkGetPhysicalDeviceProperties(Candidate, &Properties);

        if (Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            PhysicalDevice = Candidate;
            return true;
        }
    }

    /* Fallback to first available device. */
    PhysicalDevice = Devices[0];
    return true;
}

bool VulkanDevice::FindGraphicsQueueFamily(VkPhysicalDevice InPhysicalDevice)
{
    /* Query available queue families. */
    uint32_t QueueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(InPhysicalDevice, &QueueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> Families(QueueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(InPhysicalDevice, &QueueFamilyCount, Families.data());

    /* Find queue family with graphics support. */
    for (uint32_t Index = 0; Index < QueueFamilyCount; ++Index)
    {
        if (Families[Index].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            GraphicsQueueFamily = Index;
            return true;
        }
    }

    std::fprintf(stderr, "VulkanDevice::FindGraphicsQueueFamily: no graphics queue family found\n");
    return false;
}

bool VulkanDevice::CreateLogicalDevice()
{
    /* Configure graphics queue creation. */
    float QueuePriority = 1.0f;

    VkDeviceQueueCreateInfo QueueInfo{};
    QueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    QueueInfo.queueFamilyIndex = GraphicsQueueFamily;
    QueueInfo.queueCount = 1;
    QueueInfo.pQueuePriorities = &QueuePriority;

    /* Enable basic device features. */
    VkPhysicalDeviceFeatures Features{};

    /* Required device extensions for swapchain support. */
    const char* DeviceExtensions[] =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    VkDeviceCreateInfo CreateInfo{};
    CreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    CreateInfo.queueCreateInfoCount = 1;
    CreateInfo.pQueueCreateInfos = &QueueInfo;
    CreateInfo.pEnabledFeatures = &Features;

    /* Enable required extensions. */
    CreateInfo.enabledExtensionCount = 1;
    CreateInfo.ppEnabledExtensionNames = DeviceExtensions;

#if defined(_DEBUG)
    /* Enable validation layer on device for debugging. */
    const char* ValidationLayer = "VK_LAYER_KHRONOS_validation";
    CreateInfo.enabledLayerCount = 1;
    CreateInfo.ppEnabledLayerNames = &ValidationLayer;
#else
    CreateInfo.enabledLayerCount = 0;
    CreateInfo.ppEnabledLayerNames = nullptr;
#endif

    /* Create logical device. */
    if (vkCreateDevice(PhysicalDevice, &CreateInfo, nullptr, &Device) != VK_SUCCESS)
    {
        std::fprintf(stderr, "VulkanDevice::CreateLogicalDevice: vkCreateDevice failed\n");
        return false;
    }

    /* Retrieve graphics queue handle. */
    vkGetDeviceQueue(Device, GraphicsQueueFamily, 0, &GraphicsQueue);

    return true;
}
