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

#include "../../../Math/MathTypes.h"
#include "Mesh.h"
#include "../../RenderItem.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <cstddef>
#include <vector>

class VulkanPipeline;
class Camera;
/* Vulkan renderer handling swapchain rendering. */
class VulkanRenderer
{
public:
    /* Initialize empty renderer state. */
    VulkanRenderer();

    /* Destroy renderer resources. */
    ~VulkanRenderer();

    /* Create renderer resources. */
    bool Create(
        VkDevice Device,
        VkPhysicalDevice PhysicalDevice,
        std::uint32_t GraphicsQueueFamily,
        VkQueue GraphicsQueue,
        VkRenderPass RenderPassHandle,
        VkFormat SwapchainFormat,
        const std::vector<VkImageView>& SwapchainImageViews,
        VkExtent2D Extent,
        VkSwapchainKHR SwapchainHandle);

    /* Update camera state. */
    void UpdateCamera(float DeltaTime);

    /* Set render submission list. */
    void SetRenderItems(const std::vector<RenderItem>& Items);

    /* Render a single frame. */
    void DrawFrame(VkDevice Device, VkQueue GraphicsQueue);

    /* Destroy renderer resources. */
    void Destroy(VkDevice Device);

    /* Recreate renderer for resized swapchain. */
    bool Recreate(
        VkDevice Device,
        std::uint32_t GraphicsQueueFamily,
        VkQueue GraphicsQueue,
        VkRenderPass RenderPassHandle,
        VkFormat SwapchainFormat,
        const std::vector<VkImageView>& SwapchainImageViews,
        VkExtent2D Extent,
        VkSwapchainKHR SwapchainHandle);

    /* Built-in test assets. */
    Mesh* GetCubeMesh();
    Material* GetCubeMaterial();

    /* Camera position helpers. */
    Vec3 GetCameraPosition() const;
    void SetCameraPosition(const Vec3& Position);

    /* Per-frame uniform buffer object. */
    struct UniformBufferObject
    {
        Mat4 LightViewProj;
    };

private:
    /* Framebuffer creation. */
    bool CreateFramebuffers(
        VkDevice Device,
        VkRenderPass RenderPassHandle,
        VkFormat SwapchainFormat,
        const std::vector<VkImageView>& SwapchainImageViews,
        VkExtent2D Extent);

    /* Command buffer setup. */
    bool CreateCommandPool(
        VkDevice Device,
        std::uint32_t GraphicsQueueFamily);
    bool CreateCommandBuffers(VkDevice Device);

    /* Synchronization objects. */
    bool CreateSyncObjects(VkDevice Device);

    /* Uniform buffer resources. */
    bool CreateUniformBuffer(
        VkPhysicalDevice PhysicalDevice,
        VkDevice Device);

    /* Descriptor resources. */
    bool CreateDescriptorPool(VkDevice Device);
    bool CreateDescriptorSetLayout(VkDevice Device);
    bool CreateDescriptorSet(VkDevice Device);

    /* Texture resources. */
    bool CreateTextureResources(
        VkPhysicalDevice PhysicalDevice,
        VkDevice Device,
        VkQueue GraphicsQueue,
        std::uint32_t GraphicsQueueFamily);
    void DestroyTextureResources(VkDevice Device);

    /* MSAA color resources. */
    bool CreateColorResources(
        VkPhysicalDevice PhysicalDevice,
        VkDevice Device,
        VkExtent2D Extent,
        VkFormat ColorFormat,
        std::size_t ImageCount);
    void DestroyColorResources(VkDevice Device);

    /* Depth resources. */
    bool CreateDepthResources(
        VkPhysicalDevice PhysicalDevice,
        VkDevice Device,
        VkExtent2D Extent);
    void DestroyDepthResources(VkDevice Device);

    /* Shadow map resources. */
    bool CreateShadowResources(
        VkPhysicalDevice PhysicalDevice,
        VkDevice Device);
    void DestroyShadowResources(VkDevice Device);

    /* Uniform updates. */
    void UpdateUniformBuffer(VkDevice Device);

private:
    /* Swapchain state. */
    VkSwapchainKHR Swapchain = VK_NULL_HANDLE;
    VkRenderPass RenderPass = VK_NULL_HANDLE;
    VkFormat SwapchainFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D SwapchainExtent{};
    VkDevice DeviceHandle = VK_NULL_HANDLE;
    VkPhysicalDevice PhysicalDeviceHandle = VK_NULL_HANDLE;

    /* Command buffers. */
    VkCommandPool CommandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> CommandBuffers;

    /* Framebuffers. */
    std::vector<VkFramebuffer> Framebuffers;

    /* MSAA color buffers. */
    std::vector<VkImage> ColorImages;
    std::vector<VkDeviceMemory> ColorImageMemories;
    std::vector<VkImageView> ColorImageViews;

    /* Synchronization primitives. */
    VkSemaphore ImageAvailable = VK_NULL_HANDLE;
    VkSemaphore RenderFinished = VK_NULL_HANDLE;
    VkFence InFlightFence = VK_NULL_HANDLE;

    /* Graphics pipeline. */
    VulkanPipeline* Pipeline = nullptr;

    /* Uniform buffer. */
    VkBuffer UniformBuffer = VK_NULL_HANDLE;
    VkDeviceMemory UniformMemory = VK_NULL_HANDLE;

    /* Depth buffer. */
    VkImage DepthImage = VK_NULL_HANDLE;
    VkDeviceMemory DepthImageMemory = VK_NULL_HANDLE;
    VkImageView DepthImageView = VK_NULL_HANDLE;

    /* Shadow map. */
    VkRenderPass ShadowRenderPass = VK_NULL_HANDLE;
    VkFramebuffer ShadowFramebuffer = VK_NULL_HANDLE;
    VkImage ShadowImage = VK_NULL_HANDLE;
    VkDeviceMemory ShadowImageMemory = VK_NULL_HANDLE;
    VkImageView ShadowImageView = VK_NULL_HANDLE;
    VkSampler ShadowSampler = VK_NULL_HANDLE;
    VkExtent2D ShadowExtent{ 2048, 2048 };
    bool ShadowLayoutInitialized = false;

    /* Diffuse texture. */
    VkImage DiffuseImage = VK_NULL_HANDLE;
    VkDeviceMemory DiffuseImageMemory = VK_NULL_HANDLE;
    VkImageView DiffuseImageView = VK_NULL_HANDLE;
    VkSampler DiffuseSampler = VK_NULL_HANDLE;

    /* Descriptor sets. */
    VkDescriptorSetLayout DescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool DescriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet DescriptorSet = VK_NULL_HANDLE;

    /* Meshes and render list. */
    Mesh CubeMesh;
    Material CubeMaterial;
    std::vector<RenderItem> RenderItems;

    /* Camera reference. */
    Camera* Camera = nullptr;

    /* Time state. */
    float Time = 0.0f;

    /* Cached light matrix. */
    Mat4 LightViewProj = Mat4::Identity();
};
