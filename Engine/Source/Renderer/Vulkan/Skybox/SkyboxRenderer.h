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

#include "SkyboxPipeline.h"
#include "../Core/VulkanBuffer.h"
#include "../../../Math/MathTypes.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

class Camera;

/* Vulkan skybox renderer using a cubemap. */
class SkyboxRenderer
{
public:
    SkyboxRenderer();
    ~SkyboxRenderer();

    /* Create skybox resources. */
    bool Create(
        VkDevice Device,
        VkPhysicalDevice PhysicalDevice,
        VkRenderPass RenderPass,
        VkQueue GraphicsQueue,
        std::uint32_t GraphicsQueueFamily);

    /* Destroy skybox resources. */
    void Destroy(VkDevice Device);

    /* Recreate pipeline for a new render pass. */
    bool Recreate(VkDevice Device, VkRenderPass RenderPass);

    /* Record skybox draw commands. */
    void Record(
        VkCommandBuffer CommandBuffer,
        VkExtent2D Extent,
        const Camera* Camera);

    /* Set active skybox by name. */
    bool SetActiveSkybox(
        const std::string& Name,
        VkDevice Device,
        VkPhysicalDevice PhysicalDevice,
        VkQueue GraphicsQueue,
        std::uint32_t GraphicsQueueFamily);

    /* Get active skybox name. */
    const std::string& GetActiveSkyboxName() const;

    /* Check readiness for rendering. */
    bool IsReady() const;

private:
    struct SkyboxDefinition
    {
        std::string Name;
        std::string HdrPath;
        std::uint32_t Size = 512;
    };

    /* Locate the assets ready root directory. */
    bool LocateAssetsReadyRoot();

    /* Load skybox catalog entries. */
    bool LoadCatalog();

    /* Load and build cubemap resources. */
    bool LoadSkyboxResources(
        const SkyboxDefinition& Definition,
        VkDevice Device,
        VkPhysicalDevice PhysicalDevice,
        VkQueue GraphicsQueue,
        std::uint32_t GraphicsQueueFamily);

    /* Destroy cubemap resources. */
    void DestroySkyboxResources(VkDevice Device);

    /* Create descriptor set layout and pool. */
    bool CreateDescriptorResources(VkDevice Device);

    /* Destroy descriptor resources. */
    void DestroyDescriptorResources(VkDevice Device);

    /* Create skybox vertex buffer. */
    bool CreateVertexBuffer(
        VkPhysicalDevice PhysicalDevice,
        VkDevice Device,
        VkQueue GraphicsQueue,
        std::uint32_t GraphicsQueueFamily);

private:
    std::string AssetsReadyRoot;
    std::string VertexShaderPath;
    std::string FragmentShaderPath;

    std::unordered_map<std::string, SkyboxDefinition> Skyboxes;
    std::string DefaultSkyboxName;
    std::string ActiveSkyboxName;

    SkyboxPipeline Pipeline;
    VulkanBuffer VertexBuffer;

    VkDescriptorSetLayout DescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool DescriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet DescriptorSet = VK_NULL_HANDLE;

    VkImage CubemapImage = VK_NULL_HANDLE;
    VkDeviceMemory CubemapMemory = VK_NULL_HANDLE;
    VkImageView CubemapView = VK_NULL_HANDLE;
    VkSampler CubemapSampler = VK_NULL_HANDLE;
};
