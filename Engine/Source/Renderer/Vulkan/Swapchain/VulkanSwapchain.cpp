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

#include "VulkanSwapchain.h"

#include <algorithm>
#include <cstdio>

/* Initialize empty swapchain state. */
VulkanSwapchain::VulkanSwapchain()
    : Swapchain(VK_NULL_HANDLE)
    , ImageFormat(VK_FORMAT_UNDEFINED)
    , SwapchainExtent{}
{
}

/* Destroy swapchain object. */
VulkanSwapchain::~VulkanSwapchain()
{
}

/* Create swapchain and image views. */
bool VulkanSwapchain::Create(
    VkPhysicalDevice PhysicalDevice,
    VkDevice Device,
    VkSurfaceKHR Surface,
    uint32_t GraphicsQueueFamily,
    uint32_t Width,
    uint32_t Height,
    bool EnableVsync)
{
    (void)GraphicsQueueFamily;

    /* Query surface capabilities. */
    VkSurfaceCapabilitiesKHR capabilities{};
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        PhysicalDevice, Surface, &capabilities) != VK_SUCCESS)
    {
        std::fprintf(stderr,
            "VulkanSwapchain::Create: failed to query surface capabilities\n");
        return false;
    }

    /* Query surface formats. */
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        PhysicalDevice, Surface, &formatCount, nullptr);

    if (formatCount == 0)
    {
        std::fprintf(stderr,
            "VulkanSwapchain::Create: no surface formats available\n");
        return false;
    }

    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        PhysicalDevice, Surface, &formatCount, formats.data());

    /* Select surface format. */
    VkSurfaceFormatKHR surfaceFormat = formats[0];
    if (formats.size() == 1 &&
        formats[0].format == VK_FORMAT_UNDEFINED)
    {
        surfaceFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
        surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    }

    /* Query present modes. */
    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        PhysicalDevice, Surface, &presentModeCount, nullptr);

    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    if (presentModeCount > 0)
    {
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            PhysicalDevice, Surface, &presentModeCount, presentModes.data());
    }

    /* Select present mode. */
    VkPresentModeKHR presentMode = EnableVsync
        ? VK_PRESENT_MODE_FIFO_KHR
        : VK_PRESENT_MODE_IMMEDIATE_KHR;
    for (VkPresentModeKHR mode : presentModes)
    {
        if (!EnableVsync && mode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            presentMode = mode;
            break;
        }
        if (!EnableVsync && mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
        {
            presentMode = mode;
        }
    }

    /* Determine swapchain extent. */
    VkExtent2D extent{};
    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        extent = capabilities.currentExtent;
    }
    else
    {
        extent.width = std::max(
            capabilities.minImageExtent.width,
            std::min(capabilities.maxImageExtent.width, Width));

        extent.height = std::max(
            capabilities.minImageExtent.height,
            std::min(capabilities.maxImageExtent.height, Height));
    }

    /* Determine image count. */
    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 &&
        imageCount > capabilities.maxImageCount)
    {
        imageCount = capabilities.maxImageCount;
    }

    /* Fill swapchain create info. */
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = Surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage =
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
        VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    /* Create swapchain. */
    if (vkCreateSwapchainKHR(
        Device, &createInfo, nullptr, &Swapchain) != VK_SUCCESS)
    {
        std::fprintf(stderr,
            "VulkanSwapchain::Create: failed to create swapchain\n");
        return false;
    }

    /* Retrieve swapchain images. */
    uint32_t swapchainImageCount = 0;
    vkGetSwapchainImagesKHR(
        Device, Swapchain, &swapchainImageCount, nullptr);

    std::vector<VkImage> swapchainImages(swapchainImageCount);
    vkGetSwapchainImagesKHR(
        Device, Swapchain, &swapchainImageCount, swapchainImages.data());

    /* Create image views. */
    SwapchainImageViews.clear();
    SwapchainImageViews.reserve(swapchainImages.size());

    for (VkImage image : swapchainImages)
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = surfaceFormat.format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView = VK_NULL_HANDLE;
        if (vkCreateImageView(
            Device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
        {
            std::fprintf(stderr,
                "VulkanSwapchain::Create: failed to create image view\n");
            Destroy(Device);
            return false;
        }

        SwapchainImageViews.push_back(imageView);
    }

    /* Store swapchain state. */
    ImageFormat = surfaceFormat.format;
    SwapchainExtent = extent;

    return true;
}

/* Recreate swapchain for resized surface. */
bool VulkanSwapchain::Recreate(
    VkPhysicalDevice PhysicalDevice,
    VkDevice Device,
    VkSurfaceKHR Surface,
    uint32_t GraphicsQueueFamily,
    uint32_t Width,
    uint32_t Height,
    bool EnableVsync)
{
    Destroy(Device);
    return Create(
        PhysicalDevice, Device, Surface,
        GraphicsQueueFamily, Width, Height, EnableVsync);
}

/* Destroy swapchain and image views. */
void VulkanSwapchain::Destroy(VkDevice Device)
{
    for (VkImageView view : SwapchainImageViews)
    {
        if (view != VK_NULL_HANDLE)
        {
            vkDestroyImageView(Device, view, nullptr);
        }
    }

    SwapchainImageViews.clear();

    if (Swapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(Device, Swapchain, nullptr);
        Swapchain = VK_NULL_HANDLE;
    }
}

/* Get swapchain handle. */
VkSwapchainKHR VulkanSwapchain::GetHandle() const
{
    return Swapchain;
}

/* Get swapchain image format. */
VkFormat VulkanSwapchain::GetImageFormat() const
{
    return ImageFormat;
}

/* Get swapchain extent. */
VkExtent2D VulkanSwapchain::GetExtent() const
{
    return SwapchainExtent;
}

/* Get swapchain image views. */
std::vector<VkImageView> VulkanSwapchain::GetImageViews() const
{
    return SwapchainImageViews;
}
