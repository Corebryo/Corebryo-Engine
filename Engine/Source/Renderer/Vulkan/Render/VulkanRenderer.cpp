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

#include "VulkanRenderer.h"

#include "Renderer/Vulkan/Pipeline/VulkanPipeline.h"
#include "Renderer/Vulkan/Skybox/SkyboxRenderer.h"
#include "Scene/EngineCamera.h"
#include "Renderer/Vulkan/Render/WicTextureLoader.h"

#include <vulkan/vulkan.h>

#include <cstdio>
#include <cstring>

namespace
{
    /* Push constants shared between pipelines. */
    struct PushConstants
    {
        Mat4 MVP;
        Mat4 Model;
        Vec3 BaseColor;
        float Ambient;
        float Alpha;
        int Mode;
        float Padding[2];
    };

    /* Simple vertex layout. */
    struct Vertex
    {
        float Position[3];
        float UV[2];
    };

    const VkSampleCountFlagBits kMsaaSamples = VK_SAMPLE_COUNT_4_BIT;

    uint32_t FindMemoryType(
        VkPhysicalDevice physicalDevice,
        uint32_t typeFilter,
        VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties{};
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
        {
            if ((typeFilter & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        return UINT32_MAX;
    }

    bool CreateBuffer(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkBuffer& outBuffer,
        VkDeviceMemory& outMemory)
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &outBuffer) != VK_SUCCESS)
        {
            std::fprintf(stderr, "VulkanRenderer: vkCreateBuffer failed\n");
            return false;
        }

        VkMemoryRequirements memRequirements{};
        vkGetBufferMemoryRequirements(device, outBuffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(
            physicalDevice,
            memRequirements.memoryTypeBits,
            properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &outMemory) != VK_SUCCESS)
        {
            std::fprintf(stderr, "VulkanRenderer: vkAllocateMemory (buffer) failed\n");
            vkDestroyBuffer(device, outBuffer, nullptr);
            outBuffer = VK_NULL_HANDLE;
            return false;
        }

        vkBindBufferMemory(device, outBuffer, outMemory, 0);
        return true;
    }

    bool CreateImage(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        uint32_t width,
        uint32_t height,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkImage& outImage,
        VkDeviceMemory& outMemory)
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(device, &imageInfo, nullptr, &outImage) != VK_SUCCESS)
        {
            std::fprintf(stderr, "VulkanRenderer: vkCreateImage failed\n");
            return false;
        }

        VkMemoryRequirements memRequirements{};
        vkGetImageMemoryRequirements(device, outImage, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(
            physicalDevice,
            memRequirements.memoryTypeBits,
            properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &outMemory) != VK_SUCCESS)
        {
            std::fprintf(stderr, "VulkanRenderer: vkAllocateMemory (image) failed\n");
            vkDestroyImage(device, outImage, nullptr);
            outImage = VK_NULL_HANDLE;
            return false;
        }

        vkBindImageMemory(device, outImage, outMemory, 0);
        return true;
    }

    /* One-shot command helpers intentionally unchanged semantically. */
    bool BeginSingleTimeCommands(
        VkDevice device,
        uint32_t queueFamily,
        VkCommandPool& outPool,
        VkCommandBuffer& outCommandBuffer)
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        poolInfo.queueFamilyIndex = queueFamily;

        if (vkCreateCommandPool(device, &poolInfo, nullptr, &outPool) != VK_SUCCESS)
        {
            std::fprintf(stderr, "VulkanRenderer: vkCreateCommandPool failed\n");
            return false;
        }

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = outPool;
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(device, &allocInfo, &outCommandBuffer) != VK_SUCCESS)
        {
            std::fprintf(stderr, "VulkanRenderer: vkAllocateCommandBuffers failed\n");
            vkDestroyCommandPool(device, outPool, nullptr);
            outPool = VK_NULL_HANDLE;
            return false;
        }

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        if (vkBeginCommandBuffer(outCommandBuffer, &beginInfo) != VK_SUCCESS)
        {
            std::fprintf(stderr, "VulkanRenderer: vkBeginCommandBuffer failed\n");
            vkFreeCommandBuffers(device, outPool, 1, &outCommandBuffer);
            vkDestroyCommandPool(device, outPool, nullptr);
            outPool = VK_NULL_HANDLE;
            outCommandBuffer = VK_NULL_HANDLE;
            return false;
        }

        return true;
    }

    void EndSingleTimeCommands(
        VkDevice device,
        VkQueue queue,
        VkCommandPool pool,
        VkCommandBuffer commandBuffer)
    {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);

        vkFreeCommandBuffers(device, pool, 1, &commandBuffer);
        vkDestroyCommandPool(device, pool, nullptr);
    }

    void TransitionImageLayout(
        VkDevice device,
        VkQueue queue,
        uint32_t queueFamily,
        VkImage image,
        VkFormat format,
        VkImageLayout oldLayout,
        VkImageLayout newLayout)
    {
        (void)format;

        VkCommandPool pool = VK_NULL_HANDLE;
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        if (!BeginSingleTimeCommands(device, queueFamily, pool, commandBuffer))
        {
            return;
        }

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
            newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
            newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage,
            destinationStage,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier);

        EndSingleTimeCommands(device, queue, pool, commandBuffer);
    }

    void CopyBufferToImage(
        VkDevice device,
        VkQueue queue,
        uint32_t queueFamily,
        VkBuffer buffer,
        VkImage image,
        uint32_t width,
        uint32_t height)
    {
        VkCommandPool pool = VK_NULL_HANDLE;
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        if (!BeginSingleTimeCommands(device, queueFamily, pool, commandBuffer))
        {
            return;
        }

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { width, height, 1 };

        vkCmdCopyBufferToImage(
            commandBuffer,
            buffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region);

        EndSingleTimeCommands(device, queue, pool, commandBuffer);
    }
}

VulkanRenderer::VulkanRenderer()
    : Swapchain(VK_NULL_HANDLE)
    , RenderPass(VK_NULL_HANDLE)
    , SwapchainFormat(VK_FORMAT_UNDEFINED)
    , SwapchainExtent{}
    , DeviceHandle(VK_NULL_HANDLE)
    , PhysicalDeviceHandle(VK_NULL_HANDLE)
    , CommandPool(VK_NULL_HANDLE)
    , ImageAvailable(VK_NULL_HANDLE)
    , RenderFinished(VK_NULL_HANDLE)
    , InFlightFence(VK_NULL_HANDLE)
    , Pipeline(nullptr)
    , UniformBuffer(VK_NULL_HANDLE)
    , UniformMemory(VK_NULL_HANDLE)
    , DepthImage(VK_NULL_HANDLE)
    , DepthImageMemory(VK_NULL_HANDLE)
    , DepthImageView(VK_NULL_HANDLE)
    , ShadowRenderPass(VK_NULL_HANDLE)
    , ShadowFramebuffer(VK_NULL_HANDLE)
    , ShadowImage(VK_NULL_HANDLE)
    , ShadowImageMemory(VK_NULL_HANDLE)
    , ShadowImageView(VK_NULL_HANDLE)
    , ShadowSampler(VK_NULL_HANDLE)
    , DiffuseImage(VK_NULL_HANDLE)
    , DiffuseImageMemory(VK_NULL_HANDLE)
    , DiffuseImageView(VK_NULL_HANDLE)
    , DiffuseSampler(VK_NULL_HANDLE)
    , DescriptorSetLayout(VK_NULL_HANDLE)
    , DescriptorPool(VK_NULL_HANDLE)
    , DescriptorSet(VK_NULL_HANDLE)
    , Camera(nullptr)
    , Time(0.0f)
{
}

VulkanRenderer::~VulkanRenderer()
{
    Destroy(VK_NULL_HANDLE);
}

bool VulkanRenderer::Create(
    VkDevice Device,
    VkPhysicalDevice PhysicalDevice,
    std::uint32_t GraphicsQueueFamily,
    VkQueue GraphicsQueue,
    VkRenderPass RenderPassHandle,
    VkFormat SwapchainFormat,
    const std::vector<VkImageView>& SwapchainImageViews,
    VkExtent2D Extent,
    VkSwapchainKHR SwapchainHandle)
{
    /* Cache swapchain state and build renderer resources. */

    DeviceHandle = Device;
    PhysicalDeviceHandle = PhysicalDevice;
    Swapchain = SwapchainHandle;
    RenderPass = RenderPassHandle;
    this->SwapchainFormat = SwapchainFormat;
    SwapchainExtent = Extent;

    if (!Camera)
    {
        Camera = new ::Camera();
    }

    if (!CreateDescriptorSetLayout(Device))
    {
        std::fprintf(stderr, "VulkanRenderer::Create: CreateDescriptorSetLayout failed\n");
        return false;
    }
    if (!CreateUniformBuffer(PhysicalDevice, Device))
    {
        std::fprintf(stderr, "VulkanRenderer::Create: CreateUniformBuffer failed\n");
        return false;
    }
    if (!CreateDescriptorPool(Device))
    {
        std::fprintf(stderr, "VulkanRenderer::Create: CreateDescriptorPool failed\n");
        return false;
    }
    if (!CreateShadowResources(PhysicalDevice, Device))
    {
        std::fprintf(stderr, "VulkanRenderer::Create: CreateShadowResources failed\n");
        return false;
    }
    if (!CreateTextureResources(PhysicalDevice, Device, GraphicsQueue, GraphicsQueueFamily))
    {
        std::fprintf(stderr, "VulkanRenderer::Create: CreateTextureResources failed\n");
        return false;
    }
    if (!CreateDescriptorSet(Device))
    {
        std::fprintf(stderr, "VulkanRenderer::Create: CreateDescriptorSet failed\n");
        return false;
    }
    if (!Pipeline)
    {
        Pipeline = new VulkanPipeline();
    }
    if (!Pipeline->Create(Device, RenderPass, ShadowRenderPass, DescriptorSetLayout))
    {
        std::fprintf(stderr, "VulkanRenderer::Create: Pipeline::Create failed\n");
        return false;
    }
    /* Create skybox resources. */
    if (!Skybox.Create(Device, PhysicalDevice, RenderPass, GraphicsQueue, GraphicsQueueFamily))
    {
        std::fprintf(stderr, "VulkanRenderer::Create: Skybox::Create failed\n");
        return false;
    }
    static const Vertex kCubeVertices[] = {
        { { -0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f } },
        { {  0.5f, -0.5f,  0.5f }, { 1.0f, 0.0f } },
        { {  0.5f,  0.5f,  0.5f }, { 1.0f, 1.0f } },
        { {  0.5f,  0.5f,  0.5f }, { 1.0f, 1.0f } },
        { { -0.5f,  0.5f,  0.5f }, { 0.0f, 1.0f } },
        { { -0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f } },

        { {  0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f } },
        { { -0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f } },
        { { -0.5f,  0.5f, -0.5f }, { 1.0f, 1.0f } },
        { { -0.5f,  0.5f, -0.5f }, { 1.0f, 1.0f } },
        { {  0.5f,  0.5f, -0.5f }, { 0.0f, 1.0f } },
        { {  0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f } },

        { { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f } },
        { { -0.5f, -0.5f,  0.5f }, { 1.0f, 0.0f } },
        { { -0.5f,  0.5f,  0.5f }, { 1.0f, 1.0f } },
        { { -0.5f,  0.5f,  0.5f }, { 1.0f, 1.0f } },
        { { -0.5f,  0.5f, -0.5f }, { 0.0f, 1.0f } },
        { { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f } },

        { {  0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f } },
        { {  0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f } },
        { {  0.5f,  0.5f, -0.5f }, { 1.0f, 1.0f } },
        { {  0.5f,  0.5f, -0.5f }, { 1.0f, 1.0f } },
        { {  0.5f,  0.5f,  0.5f }, { 0.0f, 1.0f } },
        { {  0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f } },

        { { -0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f } },
        { {  0.5f,  0.5f,  0.5f }, { 1.0f, 0.0f } },
        { {  0.5f,  0.5f, -0.5f }, { 1.0f, 1.0f } },
        { {  0.5f,  0.5f, -0.5f }, { 1.0f, 1.0f } },
        { { -0.5f,  0.5f, -0.5f }, { 0.0f, 1.0f } },
        { { -0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f } },

        { { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f } },
        { {  0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f } },
        { {  0.5f, -0.5f,  0.5f }, { 1.0f, 1.0f } },
        { {  0.5f, -0.5f,  0.5f }, { 1.0f, 1.0f } },
        { { -0.5f, -0.5f,  0.5f }, { 0.0f, 1.0f } },
        { { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f } }
    };

    CubeMesh.Destroy(Device);
    if (!CubeMesh.VertexBuffer.CreateVertexBuffer(
        PhysicalDevice,
        Device,
        GraphicsQueue,
        GraphicsQueueFamily,
        kCubeVertices,
        sizeof(kCubeVertices)))
    {
        std::fprintf(stderr, "VulkanRenderer::Create: Failed to upload cube mesh\n");
        return false;
    }
    CubeMesh.VertexCount = static_cast<uint32_t>(
        sizeof(kCubeVertices) / sizeof(kCubeVertices[0]));
    CubeMesh.IndexCount = 0;
    CubeMesh.HasIndex = false;
    CubeMaterial.BaseColor = Vec3(1.0f, 1.0f, 1.0f);
    CubeMaterial.Ambient = 0.0f;
    CubeMaterial.Alpha = 1.0f;
    if (!CreateDepthResources(PhysicalDevice, Device, Extent))
    {
        std::fprintf(stderr, "VulkanRenderer::Create: CreateDepthResources failed\n");
        return false;
    }
    if (!CreateColorResources(PhysicalDeviceHandle, Device, Extent, SwapchainFormat, SwapchainImageViews.size()))
    {
        std::fprintf(stderr, "VulkanRenderer::Create: CreateColorResources failed\n");
        return false;
    }
    if (!CreateFramebuffers(Device, RenderPass, SwapchainFormat, SwapchainImageViews, Extent))
    {
        std::fprintf(stderr, "VulkanRenderer::Create: CreateFramebuffers failed\n");
        return false;
    }
    if (!CreateCommandPool(Device, GraphicsQueueFamily))
    {
        std::fprintf(stderr, "VulkanRenderer::Create: CreateCommandPool failed\n");
        return false;
    }
    if (!CreateCommandBuffers(Device))
    {
        std::fprintf(stderr, "VulkanRenderer::Create: CreateCommandBuffers failed\n");
        return false;
    }
    if (!CreateSyncObjects(Device))
    {
        std::fprintf(stderr, "VulkanRenderer::Create: CreateSyncObjects failed\n");
        return false;
    }

    (void)GraphicsQueue;
    return true;
}

void VulkanRenderer::UpdateCamera(float DeltaTime)
{
    if (Camera)
    {
        Camera->Update(DeltaTime);
    }
}

/* Get current camera position. */
Vec3 VulkanRenderer::GetCameraPosition() const
{
    if (!Camera)
    {
        return Vec3(0.0f, 0.0f, 0.0f);
    }

    return Camera->GetPosition();
}

/* Set camera position. */
void VulkanRenderer::SetCameraPosition(const Vec3& Position)
{
    if (!Camera)
    {
        return;
    }

    Camera->SetPosition(Position);
}

void VulkanRenderer::SetRenderItems(const std::vector<RenderItem>& Items)       
{
    RenderItems = Items;
}

void VulkanRenderer::RecordSkyboxStage(VkCommandBuffer CommandBuffer, VkExtent2D Extent)
{
    /* Render skybox before scene geometry. */
    Skybox.Record(CommandBuffer, Extent, Camera);
}

void VulkanRenderer::RecordOpaqueStage(VkCommandBuffer CommandBuffer, VkExtent2D Extent)
{
    vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline->GetWorldHandle());
    vkCmdBindDescriptorSets(
        CommandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        Pipeline->GetLayout(),
        0,
        1,
        &DescriptorSet,
        0,
        nullptr);
    float aspect = 1.0f;
    if (Extent.height > 0)
    {
        aspect = static_cast<float>(Extent.width) /
            static_cast<float>(Extent.height);
    }

    for (const RenderItem& item : RenderItems)
    {
        if (!item.MeshPtr || item.MeshPtr->VertexCount == 0)
        {
            continue;
        }

        VkBuffer vertexBuffer = item.MeshPtr->VertexBuffer.GetBuffer();
        if (vertexBuffer != VK_NULL_HANDLE)
        {
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &vertexBuffer, offsets);
        }

        PushConstants worldPush{};
        worldPush.MVP = Camera ? Camera->GetMVPMatrix(aspect, item.Model) : Mat4::Identity();
        worldPush.Model = item.Model;
        if (item.MaterialPtr)
        {
            worldPush.BaseColor = item.MaterialPtr->BaseColor;
            worldPush.Ambient = item.MaterialPtr->Ambient;
            worldPush.Alpha = item.MaterialPtr->Alpha;
        }
        else
        {
            worldPush.BaseColor = Vec3(1.0f, 1.0f, 1.0f);
            worldPush.Ambient = 0.0f;
            worldPush.Alpha = 1.0f;
        }
        worldPush.Mode = 1;
        vkCmdPushConstants(
            CommandBuffer,
            Pipeline->GetLayout(),
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(PushConstants),
            &worldPush);
        if (item.MeshPtr->HasIndex && item.MeshPtr->IndexCount > 0)
        {
            VkBuffer indexBuffer = item.MeshPtr->IndexBuffer.GetBuffer();
            if (indexBuffer != VK_NULL_HANDLE)
            {
                vkCmdBindIndexBuffer(CommandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(CommandBuffer, item.MeshPtr->IndexCount, 1, 0, 0, 0);
            }
        }
        else
        {
            vkCmdDraw(CommandBuffer, item.MeshPtr->VertexCount, 1, 0, 0);
        }
    }
}

void VulkanRenderer::RecordTransparentStage(VkCommandBuffer CommandBuffer, VkExtent2D Extent)
{
    /* Transparent stage reserved for future passes. */
    (void)CommandBuffer;
    (void)Extent;
}

void VulkanRenderer::DrawFrame(VkDevice Device, VkQueue GraphicsQueue)
{
    /* Record a clear-only pass and present the swapchain image. */

    if (Swapchain == VK_NULL_HANDLE || RenderPass == VK_NULL_HANDLE || CommandBuffers.empty())
    {
        return;
    }

    vkWaitForFences(Device, 1, &InFlightFence, VK_TRUE, UINT64_MAX);

    uint32_t imageIndex = 0;
    VkResult acquireResult = vkAcquireNextImageKHR(
        Device,
        Swapchain,
        UINT64_MAX,
        ImageAvailable,
        VK_NULL_HANDLE,
        &imageIndex);

    if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR)
    {
        return;
    }

    vkResetFences(Device, 1, &InFlightFence);

    VkCommandBuffer commandBuffer = CommandBuffers[imageIndex];
    vkResetCommandBuffer(commandBuffer, 0);

    UpdateUniformBuffer(Device);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkImageMemoryBarrier shadowToDepth{};
    shadowToDepth.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    shadowToDepth.srcAccessMask = ShadowLayoutInitialized ? VK_ACCESS_SHADER_READ_BIT : 0;
    shadowToDepth.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    shadowToDepth.oldLayout = ShadowLayoutInitialized
        ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
        : VK_IMAGE_LAYOUT_UNDEFINED;
    shadowToDepth.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    shadowToDepth.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    shadowToDepth.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    shadowToDepth.image = ShadowImage;
    shadowToDepth.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    shadowToDepth.subresourceRange.baseMipLevel = 0;
    shadowToDepth.subresourceRange.levelCount = 1;
    shadowToDepth.subresourceRange.baseArrayLayer = 0;
    shadowToDepth.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(
        commandBuffer,
        ShadowLayoutInitialized ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &shadowToDepth);

    VkClearValue shadowClear{};
    shadowClear.depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo shadowPassInfo{};
    shadowPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    shadowPassInfo.renderPass = ShadowRenderPass;
    shadowPassInfo.framebuffer = ShadowFramebuffer;
    shadowPassInfo.renderArea.offset = { 0, 0 };
    shadowPassInfo.renderArea.extent = ShadowExtent;
    shadowPassInfo.clearValueCount = 1;
    shadowPassInfo.pClearValues = &shadowClear;

    vkCmdBeginRenderPass(commandBuffer, &shadowPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport shadowViewport{};
    shadowViewport.x = 0.0f;
    shadowViewport.y = 0.0f;
    shadowViewport.width = static_cast<float>(ShadowExtent.width);
    shadowViewport.height = static_cast<float>(ShadowExtent.height);
    shadowViewport.minDepth = 0.0f;
    shadowViewport.maxDepth = 1.0f;

    VkRect2D shadowScissor{};
    shadowScissor.offset = { 0, 0 };
    shadowScissor.extent = ShadowExtent;

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline->GetShadowHandle());
    vkCmdSetViewport(commandBuffer, 0, 1, &shadowViewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &shadowScissor);

    struct ShadowPush
    {
        Mat4 LightViewProj;
        Mat4 Model;
    };

    for (const RenderItem& item : RenderItems)
    {
        if (!item.MeshPtr || item.MeshPtr->VertexCount == 0)
        {
            continue;
        }

        VkBuffer vertexBuffer = item.MeshPtr->VertexBuffer.GetBuffer();
        if (vertexBuffer != VK_NULL_HANDLE)
        {
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
        }

        ShadowPush shadowPush{};
        shadowPush.LightViewProj = LightViewProj;
        shadowPush.Model = item.Model;

        vkCmdPushConstants(
            commandBuffer,
            Pipeline->GetShadowLayout(),
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(ShadowPush),
            &shadowPush);

        if (item.MeshPtr->HasIndex && item.MeshPtr->IndexCount > 0)
        {
            VkBuffer indexBuffer = item.MeshPtr->IndexBuffer.GetBuffer();
            if (indexBuffer != VK_NULL_HANDLE)
            {
                vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(commandBuffer, item.MeshPtr->IndexCount, 1, 0, 0, 0);
            }
        }
        else
        {
            vkCmdDraw(commandBuffer, item.MeshPtr->VertexCount, 1, 0, 0);
        }
    }

    vkCmdEndRenderPass(commandBuffer);

    VkImageMemoryBarrier shadowToRead{};
    shadowToRead.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    shadowToRead.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    shadowToRead.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    shadowToRead.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    shadowToRead.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    shadowToRead.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    shadowToRead.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    shadowToRead.image = ShadowImage;
    shadowToRead.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    shadowToRead.subresourceRange.baseMipLevel = 0;
    shadowToRead.subresourceRange.levelCount = 1;
    shadowToRead.subresourceRange.baseArrayLayer = 0;
    shadowToRead.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &shadowToRead);
    ShadowLayoutInitialized = true;

    VkClearValue clearValues[3]{};
    clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
    clearValues[1].depthStencil = { 1.0f, 0 };
    clearValues[2].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = RenderPass;
    renderPassInfo.framebuffer = Framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = SwapchainExtent;
    renderPassInfo.clearValueCount = 3;
    renderPassInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(SwapchainExtent.width);
    viewport.height = static_cast<float>(SwapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = SwapchainExtent;

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    /* Main render pass stages: skybox -> opaque -> transparent. */
    RecordSkyboxStage(commandBuffer, SwapchainExtent);
    RecordOpaqueStage(commandBuffer, SwapchainExtent);
    RecordTransparentStage(commandBuffer, SwapchainExtent);
    vkCmdEndRenderPass(commandBuffer);

    vkEndCommandBuffer(commandBuffer);

    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &ImageAvailable;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &RenderFinished;

    if (vkQueueSubmit(GraphicsQueue, 1, &submitInfo, InFlightFence) != VK_SUCCESS)
    {
        return;
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &RenderFinished;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &Swapchain;
    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(GraphicsQueue, &presentInfo);
}

void VulkanRenderer::Destroy(VkDevice Device)
{
    /* Release GPU resources in a safe teardown order. */

    VkDevice activeDevice = Device != VK_NULL_HANDLE ? Device : DeviceHandle;

    if (activeDevice != VK_NULL_HANDLE)
    {
        if (!CommandBuffers.empty() && CommandPool != VK_NULL_HANDLE)
        {
            vkFreeCommandBuffers(activeDevice, CommandPool,
                static_cast<uint32_t>(CommandBuffers.size()), CommandBuffers.data());
        }
        CommandBuffers.clear();

        for (VkFramebuffer framebuffer : Framebuffers)
        {
            if (framebuffer != VK_NULL_HANDLE)
            {
                vkDestroyFramebuffer(activeDevice, framebuffer, nullptr);
            }
        }
        Framebuffers.clear();

        DestroyDepthResources(activeDevice);
        DestroyColorResources(activeDevice);
        DestroyTextureResources(activeDevice);
        /* Release skybox resources. */
        Skybox.Destroy(activeDevice);

        if (CommandPool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(activeDevice, CommandPool, nullptr);
            CommandPool = VK_NULL_HANDLE;
        }

        if (ImageAvailable != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(activeDevice, ImageAvailable, nullptr);
            ImageAvailable = VK_NULL_HANDLE;
        }

        if (RenderFinished != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(activeDevice, RenderFinished, nullptr);
            RenderFinished = VK_NULL_HANDLE;
        }

        if (InFlightFence != VK_NULL_HANDLE)
        {
            vkDestroyFence(activeDevice, InFlightFence, nullptr);
            InFlightFence = VK_NULL_HANDLE;
        }

        if (DescriptorPool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(activeDevice, DescriptorPool, nullptr);
            DescriptorPool = VK_NULL_HANDLE;
        }

        if (DescriptorSetLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(activeDevice, DescriptorSetLayout, nullptr);
            DescriptorSetLayout = VK_NULL_HANDLE;
        }

        if (UniformBuffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(activeDevice, UniformBuffer, nullptr);
            UniformBuffer = VK_NULL_HANDLE;
        }

        if (UniformMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(activeDevice, UniformMemory, nullptr);
            UniformMemory = VK_NULL_HANDLE;
        }
    }

    if (activeDevice != VK_NULL_HANDLE)
    {
        CubeMesh.Destroy(activeDevice);
        RenderItems.clear();
    }

    if (Pipeline)
    {
        if (activeDevice != VK_NULL_HANDLE)
        {
            Pipeline->Destroy(activeDevice);
        }
        delete Pipeline;
        Pipeline = nullptr;
    }

    if (activeDevice != VK_NULL_HANDLE)
    {
        DestroyShadowResources(activeDevice);
    }

    if (Camera)
    {
        delete Camera;
        Camera = nullptr;
    }

    DeviceHandle = VK_NULL_HANDLE;
    PhysicalDeviceHandle = VK_NULL_HANDLE;
}

bool VulkanRenderer::Recreate(
    VkDevice Device,
    std::uint32_t GraphicsQueueFamily,
    VkQueue GraphicsQueue,
    VkRenderPass RenderPassHandle,
    VkFormat SwapchainFormat,
    const std::vector<VkImageView>& SwapchainImageViews,
    VkExtent2D Extent,
    VkSwapchainKHR SwapchainHandle)
{
    DeviceHandle = Device;
    Swapchain = SwapchainHandle;
    RenderPass = RenderPassHandle;
    this->SwapchainFormat = SwapchainFormat;
    SwapchainExtent = Extent;

    for (VkFramebuffer framebuffer : Framebuffers)
    {
        if (framebuffer != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(Device, framebuffer, nullptr);
        }
    }
    Framebuffers.clear();

    DestroyDepthResources(Device);
    DestroyColorResources(Device);

    if (!CreateDepthResources(PhysicalDeviceHandle, Device, Extent))
    {
        std::fprintf(stderr, "VulkanRenderer::Recreate: CreateDepthResources failed\n");
        return false;
    }
    if (!CreateColorResources(PhysicalDeviceHandle, Device, Extent, SwapchainFormat, SwapchainImageViews.size()))
    {
        std::fprintf(stderr, "VulkanRenderer::Recreate: CreateColorResources failed\n");
        return false;
    }
    if (!CreateFramebuffers(Device, RenderPass, SwapchainFormat, SwapchainImageViews, Extent))
    {
        std::fprintf(stderr, "VulkanRenderer::Recreate: CreateFramebuffers failed\n");
        return false;
    }
    /* Recreate skybox pipeline for the new render pass. */
    if (!Skybox.Recreate(Device, RenderPass))
    {
        std::fprintf(stderr, "VulkanRenderer::Recreate: Skybox::Recreate failed\n");
        return false;
    }

    if (CommandPool != VK_NULL_HANDLE && !CommandBuffers.empty())
    {
        vkFreeCommandBuffers(Device, CommandPool,
            static_cast<uint32_t>(CommandBuffers.size()), CommandBuffers.data());
        CommandBuffers.clear();
    }

    if (!CreateCommandPool(Device, GraphicsQueueFamily))
    {
        std::fprintf(stderr, "VulkanRenderer::Recreate: CreateCommandPool failed\n");
        return false;
    }
    if (!CreateCommandBuffers(Device))
    {
        std::fprintf(stderr, "VulkanRenderer::Recreate: CreateCommandBuffers failed\n");
        return false;
    }

    (void)GraphicsQueue;
    return true;
}

Mesh* VulkanRenderer::GetCubeMesh()
{
    return &CubeMesh;
}

Material* VulkanRenderer::GetCubeMaterial()
{
    return &CubeMaterial;
}

bool VulkanRenderer::CreateFramebuffers(
    VkDevice Device,
    VkRenderPass RenderPassHandle,
    VkFormat SwapchainFormat,
    const std::vector<VkImageView>& SwapchainImageViews,
    VkExtent2D Extent)
{
    (void)SwapchainFormat;

    Framebuffers.clear();
    Framebuffers.reserve(SwapchainImageViews.size());

    if (ColorImageViews.size() != SwapchainImageViews.size())
    {
        std::fprintf(stderr, "VulkanRenderer::CreateFramebuffers: color resources mismatch\n");
        return false;
    }

    for (std::size_t i = 0; i < SwapchainImageViews.size(); ++i)
    {
        VkImageView attachments[3] = {
            ColorImageViews[i],
            DepthImageView,
            SwapchainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = RenderPassHandle;
        framebufferInfo.attachmentCount = 3;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = Extent.width;
        framebufferInfo.height = Extent.height;
        framebufferInfo.layers = 1;

        VkFramebuffer framebuffer = VK_NULL_HANDLE;
        if (vkCreateFramebuffer(Device, &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS)
        {
            std::fprintf(stderr, "VulkanRenderer::CreateFramebuffers: vkCreateFramebuffer failed\n");
            return false;
        }

        Framebuffers.push_back(framebuffer);
    }

    return true;
}

bool VulkanRenderer::CreateCommandPool(VkDevice Device, std::uint32_t GraphicsQueueFamily)
{
    if (CommandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(Device, CommandPool, nullptr);
        CommandPool = VK_NULL_HANDLE;
    }

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = GraphicsQueueFamily;

    if (vkCreateCommandPool(Device, &poolInfo, nullptr, &CommandPool) != VK_SUCCESS)
    {
        std::fprintf(stderr, "VulkanRenderer::CreateCommandPool: vkCreateCommandPool failed\n");
        return false;
    }

    return true;
}

bool VulkanRenderer::CreateCommandBuffers(VkDevice Device)
{
    CommandBuffers.clear();

    if (Framebuffers.empty() || CommandPool == VK_NULL_HANDLE)
    {
        return true;
    }

    CommandBuffers.resize(Framebuffers.size());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = CommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(CommandBuffers.size());

    if (vkAllocateCommandBuffers(Device, &allocInfo, CommandBuffers.data()) != VK_SUCCESS)
    {
        std::fprintf(stderr, "VulkanRenderer::CreateCommandBuffers: vkAllocateCommandBuffers failed\n");
        CommandBuffers.clear();
        return false;
    }

    return true;
}

bool VulkanRenderer::CreateSyncObjects(VkDevice Device)
{
    if (ImageAvailable != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(Device, ImageAvailable, nullptr);
        ImageAvailable = VK_NULL_HANDLE;
    }
    if (RenderFinished != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(Device, RenderFinished, nullptr);
        RenderFinished = VK_NULL_HANDLE;
    }
    if (InFlightFence != VK_NULL_HANDLE)
    {
        vkDestroyFence(Device, InFlightFence, nullptr);
        InFlightFence = VK_NULL_HANDLE;
    }

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if (vkCreateSemaphore(Device, &semaphoreInfo, nullptr, &ImageAvailable) != VK_SUCCESS)
    {
        std::fprintf(stderr, "VulkanRenderer::CreateSyncObjects: vkCreateSemaphore (ImageAvailable) failed\n");
        return false;
    }
    if (vkCreateSemaphore(Device, &semaphoreInfo, nullptr, &RenderFinished) != VK_SUCCESS)
    {
        std::fprintf(stderr, "VulkanRenderer::CreateSyncObjects: vkCreateSemaphore (RenderFinished) failed\n");
        return false;
    }

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateFence(Device, &fenceInfo, nullptr, &InFlightFence) != VK_SUCCESS)
    {
        std::fprintf(stderr, "VulkanRenderer::CreateSyncObjects: vkCreateFence failed\n");
        return false;
    }

    return true;
}

bool VulkanRenderer::CreateUniformBuffer(VkPhysicalDevice PhysicalDevice, VkDevice Device)
{
    if (UniformBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(Device, UniformBuffer, nullptr);
        UniformBuffer = VK_NULL_HANDLE;
    }
    if (UniformMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(Device, UniformMemory, nullptr);
        UniformMemory = VK_NULL_HANDLE;
    }

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof(UniformBufferObject);
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(Device, &bufferInfo, nullptr, &UniformBuffer) != VK_SUCCESS)
    {
        std::fprintf(stderr, "VulkanRenderer::CreateUniformBuffer: vkCreateBuffer failed\n");
        return false;
    }

    VkMemoryRequirements memRequirements{};
    vkGetBufferMemoryRequirements(Device, UniformBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(
        PhysicalDevice,
        memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(Device, &allocInfo, nullptr, &UniformMemory) != VK_SUCCESS)
    {
        std::fprintf(stderr, "VulkanRenderer::CreateUniformBuffer: vkAllocateMemory failed\n");
        vkDestroyBuffer(Device, UniformBuffer, nullptr);
        UniformBuffer = VK_NULL_HANDLE;
        return false;
    }

    vkBindBufferMemory(Device, UniformBuffer, UniformMemory, 0);
    return true;
}

bool VulkanRenderer::CreateDescriptorPool(VkDevice Device)
{
    if (DescriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(Device, DescriptorPool, nullptr);
        DescriptorPool = VK_NULL_HANDLE;
    }

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = 1;

    VkDescriptorPoolSize samplerPoolSize{};
    samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerPoolSize.descriptorCount = 1;

    VkDescriptorPoolSize poolSizes[] = { poolSize, samplerPoolSize };

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 2;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = 1;

    if (vkCreateDescriptorPool(Device, &poolInfo, nullptr, &DescriptorPool) != VK_SUCCESS)
    {
        std::fprintf(stderr, "VulkanRenderer::CreateDescriptorPool: vkCreateDescriptorPool failed\n");
        return false;
    }

    return true;
}

bool VulkanRenderer::CreateDescriptorSetLayout(VkDevice Device)
{
    if (DescriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(Device, DescriptorSetLayout, nullptr);
        DescriptorSetLayout = VK_NULL_HANDLE;
    }

    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding diffuseLayoutBinding{};
    diffuseLayoutBinding.binding = 1;
    diffuseLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    diffuseLayoutBinding.descriptorCount = 1;
    diffuseLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    diffuseLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding bindings[] = { uboLayoutBinding, diffuseLayoutBinding };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 2;
    layoutInfo.pBindings = bindings;

    if (vkCreateDescriptorSetLayout(Device, &layoutInfo, nullptr, &DescriptorSetLayout) != VK_SUCCESS)
    {
        std::fprintf(stderr, "VulkanRenderer::CreateDescriptorSetLayout: vkCreateDescriptorSetLayout failed\n");
        return false;
    }

    return true;
}

bool VulkanRenderer::CreateDescriptorSet(VkDevice Device)
{
    if (DescriptorPool == VK_NULL_HANDLE || DescriptorSetLayout == VK_NULL_HANDLE)
    {
        std::fprintf(stderr, "VulkanRenderer::CreateDescriptorSet: descriptor pool or layout missing\n");
        return false;
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = DescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &DescriptorSetLayout;

    if (vkAllocateDescriptorSets(Device, &allocInfo, &DescriptorSet) != VK_SUCCESS)
    {
        std::fprintf(stderr, "VulkanRenderer::CreateDescriptorSet: vkAllocateDescriptorSets failed\n");
        return false;
    }

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = UniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = DescriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;

    VkDescriptorImageInfo diffuseInfo{};
    diffuseInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    diffuseInfo.imageView = DiffuseImageView;
    diffuseInfo.sampler = DiffuseSampler;

    VkWriteDescriptorSet diffuseWrite{};
    diffuseWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    diffuseWrite.dstSet = DescriptorSet;
    diffuseWrite.dstBinding = 1;
    diffuseWrite.dstArrayElement = 0;
    diffuseWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    diffuseWrite.descriptorCount = 1;
    diffuseWrite.pImageInfo = &diffuseInfo;

    VkWriteDescriptorSet writes[] = { descriptorWrite, diffuseWrite };

    vkUpdateDescriptorSets(Device, 2, writes, 0, nullptr);
    return true;
}

bool VulkanRenderer::CreateTextureResources(
    VkPhysicalDevice PhysicalDevice,
    VkDevice Device,
    VkQueue GraphicsQueue,
    std::uint32_t GraphicsQueueFamily)
{
    DestroyTextureResources(Device);

    TextureData texture;

    if (!LoadPngWic("../Assets/Textures/Base.png", texture))
    {
        std::fprintf(stderr, "VulkanRenderer::CreateTextureResources: Failed to load Base.png\n");
        return false;
    }

    VkDeviceSize imageSize = static_cast<VkDeviceSize>(texture.Pixels.size());

    if (imageSize == 0)
    {
        std::fprintf(stderr, "VulkanRenderer::CreateTextureResources: Empty texture\n");
        return false;
    }

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;

    if (!CreateBuffer(
        PhysicalDevice,
        Device,
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingMemory))
    {
        std::fprintf(stderr, "VulkanRenderer::CreateTextureResources: Staging buffer failed\n");
        return false;
    }

    void* mappedData = nullptr;

    if (vkMapMemory(Device, stagingMemory, 0, imageSize, 0, &mappedData) != VK_SUCCESS)
    {
        std::fprintf(stderr, "VulkanRenderer::CreateTextureResources: vkMapMemory failed\n");
        vkDestroyBuffer(Device, stagingBuffer, nullptr);
        vkFreeMemory(Device, stagingMemory, nullptr);
        return false;
    }

    std::memcpy(mappedData, texture.Pixels.data(), static_cast<size_t>(imageSize));
    vkUnmapMemory(Device, stagingMemory);

    VkFormat textureFormat = VK_FORMAT_R8G8B8A8_SRGB;
    if (!CreateImage(
        PhysicalDevice,
        Device,
        texture.Width,
        texture.Height,
        textureFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        DiffuseImage,
        DiffuseImageMemory))
    {
        std::fprintf(stderr, "VulkanRenderer::CreateTextureResources: CreateImage failed\n");
        vkDestroyBuffer(Device, stagingBuffer, nullptr);
        vkFreeMemory(Device, stagingMemory, nullptr);
        return false;
    }

    TransitionImageLayout(
        Device,
        GraphicsQueue,
        GraphicsQueueFamily,
        DiffuseImage,
        textureFormat,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    CopyBufferToImage(
        Device,
        GraphicsQueue,
        GraphicsQueueFamily,
        stagingBuffer,
        DiffuseImage,
        texture.Width,
        texture.Height);

    TransitionImageLayout(
        Device,
        GraphicsQueue,
        GraphicsQueueFamily,
        DiffuseImage,
        textureFormat,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(Device, stagingBuffer, nullptr);
    vkFreeMemory(Device, stagingMemory, nullptr);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = DiffuseImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = textureFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(Device, &viewInfo, nullptr, &DiffuseImageView) != VK_SUCCESS)
    {
        std::fprintf(stderr, "VulkanRenderer::CreateTextureResources: vkCreateImageView failed\n");
        DestroyTextureResources(Device);
        return false;
    }

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

    if (vkCreateSampler(Device, &samplerInfo, nullptr, &DiffuseSampler) != VK_SUCCESS)
    {
        std::fprintf(stderr, "VulkanRenderer::CreateTextureResources: vkCreateSampler failed\n");
        DestroyTextureResources(Device);
        return false;
    }

    return true;
}

void VulkanRenderer::DestroyTextureResources(VkDevice Device)
{
    if (DiffuseSampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(Device, DiffuseSampler, nullptr);
        DiffuseSampler = VK_NULL_HANDLE;
    }

    if (DiffuseImageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(Device, DiffuseImageView, nullptr);
        DiffuseImageView = VK_NULL_HANDLE;
    }

    if (DiffuseImage != VK_NULL_HANDLE)
    {
        vkDestroyImage(Device, DiffuseImage, nullptr);
        DiffuseImage = VK_NULL_HANDLE;
    }

    if (DiffuseImageMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(Device, DiffuseImageMemory, nullptr);
        DiffuseImageMemory = VK_NULL_HANDLE;
    }
}


bool VulkanRenderer::CreateColorResources(
    VkPhysicalDevice PhysicalDevice,
    VkDevice Device,
    VkExtent2D Extent,
    VkFormat ColorFormat,
    std::size_t ImageCount)
{
    DestroyColorResources(Device);

    if (ImageCount == 0)
    {
        return true;
    }

    ColorImages.assign(ImageCount, VK_NULL_HANDLE);
    ColorImageMemories.assign(ImageCount, VK_NULL_HANDLE);
    ColorImageViews.assign(ImageCount, VK_NULL_HANDLE);

    for (std::size_t i = 0; i < ImageCount; ++i)
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = Extent.width;
        imageInfo.extent.height = Extent.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = ColorFormat;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        imageInfo.samples = kMsaaSamples;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(Device, &imageInfo, nullptr, &ColorImages[i]) != VK_SUCCESS)
        {
            std::fprintf(stderr, "VulkanRenderer::CreateColorResources: vkCreateImage failed\n");
            DestroyColorResources(Device);
            return false;
        }

        VkMemoryRequirements memRequirements{};
        vkGetImageMemoryRequirements(Device, ColorImages[i], &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(
            PhysicalDevice,
            memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(Device, &allocInfo, nullptr, &ColorImageMemories[i]) != VK_SUCCESS)
        {
            std::fprintf(stderr, "VulkanRenderer::CreateColorResources: vkAllocateMemory failed\n");
            DestroyColorResources(Device);
            return false;
        }

        vkBindImageMemory(Device, ColorImages[i], ColorImageMemories[i], 0);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = ColorImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = ColorFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(Device, &viewInfo, nullptr, &ColorImageViews[i]) != VK_SUCCESS)
        {
            std::fprintf(stderr, "VulkanRenderer::CreateColorResources: vkCreateImageView failed\n");
            DestroyColorResources(Device);
            return false;
        }
    }

    return true;
}

void VulkanRenderer::DestroyColorResources(VkDevice Device)
{
    for (VkImageView view : ColorImageViews)
    {
        if (view != VK_NULL_HANDLE)
        {
            vkDestroyImageView(Device, view, nullptr);
        }
    }

    ColorImageViews.clear();

    for (VkImage image : ColorImages)
    {
        if (image != VK_NULL_HANDLE)
        {
            vkDestroyImage(Device, image, nullptr);
        }
    }

    ColorImages.clear();

    for (VkDeviceMemory memory : ColorImageMemories)
    {
        if (memory != VK_NULL_HANDLE)
        {
            vkFreeMemory(Device, memory, nullptr);
        }
    }

    ColorImageMemories.clear();
}
bool VulkanRenderer::CreateDepthResources(
    VkPhysicalDevice PhysicalDevice,
    VkDevice Device,
    VkExtent2D Extent)
{
    DestroyDepthResources(Device);
    DestroyColorResources(Device);

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = Extent.width;
    imageInfo.extent.height = Extent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_D32_SFLOAT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.samples = kMsaaSamples;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(Device, &imageInfo, nullptr, &DepthImage) != VK_SUCCESS)
    {
        std::fprintf(stderr, "VulkanRenderer::CreateDepthResources: vkCreateImage failed\n");
        return false;
    }

    VkMemoryRequirements memRequirements{};
    vkGetImageMemoryRequirements(Device, DepthImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(
        PhysicalDevice,
        memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(Device, &allocInfo, nullptr, &DepthImageMemory) != VK_SUCCESS)
    {
        std::fprintf(stderr, "VulkanRenderer::CreateDepthResources: vkAllocateMemory failed\n");
        vkDestroyImage(Device, DepthImage, nullptr);
        DepthImage = VK_NULL_HANDLE;
        return false;
    }

    vkBindImageMemory(Device, DepthImage, DepthImageMemory, 0);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = DepthImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_D32_SFLOAT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(Device, &viewInfo, nullptr, &DepthImageView) != VK_SUCCESS)
    {
        std::fprintf(stderr, "VulkanRenderer::CreateDepthResources: vkCreateImageView failed\n");
        vkDestroyImage(Device, DepthImage, nullptr);
        vkFreeMemory(Device, DepthImageMemory, nullptr);
        DepthImage = VK_NULL_HANDLE;
        DepthImageMemory = VK_NULL_HANDLE;
        return false;
    }

    return true;
}

void VulkanRenderer::DestroyDepthResources(VkDevice Device)
{
    if (DepthImageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(Device, DepthImageView, nullptr);
        DepthImageView = VK_NULL_HANDLE;
    }

    if (DepthImage != VK_NULL_HANDLE)
    {
        vkDestroyImage(Device, DepthImage, nullptr);
        DepthImage = VK_NULL_HANDLE;
    }

    if (DepthImageMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(Device, DepthImageMemory, nullptr);
        DepthImageMemory = VK_NULL_HANDLE;
    }
}

bool VulkanRenderer::CreateShadowResources(VkPhysicalDevice PhysicalDevice, VkDevice Device)
{
    DestroyShadowResources(Device);

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = ShadowExtent.width;
    imageInfo.extent.height = ShadowExtent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_D32_SFLOAT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(Device, &imageInfo, nullptr, &ShadowImage) != VK_SUCCESS)
    {
        std::fprintf(stderr, "VulkanRenderer::CreateShadowResources: vkCreateImage failed\n");
        return false;
    }

    VkMemoryRequirements memRequirements{};
    vkGetImageMemoryRequirements(Device, ShadowImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(
        PhysicalDevice,
        memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(Device, &allocInfo, nullptr, &ShadowImageMemory) != VK_SUCCESS)
    {
        std::fprintf(stderr, "VulkanRenderer::CreateShadowResources: vkAllocateMemory failed\n");
        vkDestroyImage(Device, ShadowImage, nullptr);
        ShadowImage = VK_NULL_HANDLE;
        return false;
    }

    vkBindImageMemory(Device, ShadowImage, ShadowImageMemory, 0);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = ShadowImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_D32_SFLOAT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(Device, &viewInfo, nullptr, &ShadowImageView) != VK_SUCCESS)
    {
        std::fprintf(stderr, "VulkanRenderer::CreateShadowResources: vkCreateImageView failed\n");
        vkDestroyImage(Device, ShadowImage, nullptr);
        vkFreeMemory(Device, ShadowImageMemory, nullptr);
        ShadowImage = VK_NULL_HANDLE;
        ShadowImageMemory = VK_NULL_HANDLE;
        return false;
    }

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;

    if (vkCreateSampler(Device, &samplerInfo, nullptr, &ShadowSampler) != VK_SUCCESS)
    {
        std::fprintf(stderr, "VulkanRenderer::CreateShadowResources: vkCreateSampler failed\n");
        vkDestroyImageView(Device, ShadowImageView, nullptr);
        vkDestroyImage(Device, ShadowImage, nullptr);
        vkFreeMemory(Device, ShadowImageMemory, nullptr);
        ShadowImageView = VK_NULL_HANDLE;
        ShadowImage = VK_NULL_HANDLE;
        ShadowImageMemory = VK_NULL_HANDLE;
        return false;
    }

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = VK_FORMAT_D32_SFLOAT;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthRef{};
    depthRef.attachment = 0;
    depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 0;
    subpass.pDepthStencilAttachment = &depthRef;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &depthAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    if (vkCreateRenderPass(Device, &renderPassInfo, nullptr, &ShadowRenderPass) != VK_SUCCESS)
    {
        std::fprintf(stderr, "VulkanRenderer::CreateShadowResources: vkCreateRenderPass failed\n");
        return false;
    }

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = ShadowRenderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = &ShadowImageView;
    framebufferInfo.width = ShadowExtent.width;
    framebufferInfo.height = ShadowExtent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(Device, &framebufferInfo, nullptr, &ShadowFramebuffer) != VK_SUCCESS)
    {
        std::fprintf(stderr, "VulkanRenderer::CreateShadowResources: vkCreateFramebuffer failed\n");
        return false;
    }

    ShadowLayoutInitialized = false;
    return true;
}

void VulkanRenderer::DestroyShadowResources(VkDevice Device)
{
    if (ShadowFramebuffer != VK_NULL_HANDLE)
    {
        vkDestroyFramebuffer(Device, ShadowFramebuffer, nullptr);
        ShadowFramebuffer = VK_NULL_HANDLE;
    }

    if (ShadowRenderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(Device, ShadowRenderPass, nullptr);
        ShadowRenderPass = VK_NULL_HANDLE;
    }

    if (ShadowSampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(Device, ShadowSampler, nullptr);
        ShadowSampler = VK_NULL_HANDLE;
    }

    if (ShadowImageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(Device, ShadowImageView, nullptr);
        ShadowImageView = VK_NULL_HANDLE;
    }

    if (ShadowImage != VK_NULL_HANDLE)
    {
        vkDestroyImage(Device, ShadowImage, nullptr);
        ShadowImage = VK_NULL_HANDLE;
    }

    if (ShadowImageMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(Device, ShadowImageMemory, nullptr);
        ShadowImageMemory = VK_NULL_HANDLE;
    }

    ShadowLayoutInitialized = false;
}

void VulkanRenderer::UpdateUniformBuffer(VkDevice Device)
{
    if (UniformMemory == VK_NULL_HANDLE || !Camera)
    {
        return;
    }

    Vec3 lightDir(0.0f, 0.0f, -1.0f);
    Vec3 target(0.0f, 0.0f, -5.0f);
    Vec3 lightPos = target - lightDir * 10.0f;

    Mat4 lightView = Mat4::LookAt(lightPos, target, Vec3(0.0f, 1.0f, 0.0f));
    Mat4 lightProj = Mat4::Orthographic(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 30.0f);

    UniformBufferObject ubo{};
    ubo.LightViewProj = lightProj * lightView;
    LightViewProj = ubo.LightViewProj;

    void* data = nullptr;

    if (vkMapMemory(Device, UniformMemory, 0, sizeof(UniformBufferObject), 0, &data) == VK_SUCCESS)
    {
        std::memcpy(data, &ubo, sizeof(UniformBufferObject));
        vkUnmapMemory(Device, UniformMemory);
    }
}
