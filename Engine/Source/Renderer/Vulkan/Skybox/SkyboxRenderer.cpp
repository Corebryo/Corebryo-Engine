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

#include "SkyboxRenderer.h"

#include "Scene/EngineCamera.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace
{
    /* HDR image container. */
    struct HdrImage
    {
        int Width = 0;
        int Height = 0;
        std::vector<float> Data;
    };

    /* Skybox push constants. */
    struct SkyboxPushConstants
    {
        Mat4 ViewProjection;
        Mat4 ViewInverse;
    };

    const float kPi = 3.1415926535f;
    const uint32_t kSkyboxVertexCount = 36;

    /* Trim whitespace from both ends. */
    std::string Trim(const std::string& Text)
    {
        std::size_t start = 0;
        while (start < Text.size() && std::isspace(static_cast<unsigned char>(Text[start])) != 0)
        {
            ++start;
        }

        std::size_t end = Text.size();
        while (end > start && std::isspace(static_cast<unsigned char>(Text[end - 1])) != 0)
        {
            --end;
        }

        return Text.substr(start, end - start);
    }

    /* Find a file by name within a directory tree. */
    std::string FindFileInTree(const std::filesystem::path& Root, const std::string& Name)
    {
        std::error_code error;
        if (!std::filesystem::exists(Root, error))
        {
            return std::string();
        }

        for (const auto& entry : std::filesystem::recursive_directory_iterator(Root, error))
        {
            if (error)
            {
                break;
            }

            if (!entry.is_regular_file())
            {
                continue;
            }

            if (entry.path().filename().string() == Name)
            {
                return entry.path().string();
            }
        }

        return std::string();
    }

    /* Convert float to 16-bit float. */
    std::uint16_t FloatToHalf(float Value)
    {
        std::uint32_t bits = 0;
        std::memcpy(&bits, &Value, sizeof(bits));

        std::uint32_t sign = (bits >> 16) & 0x8000u;
        std::int32_t exponent = static_cast<std::int32_t>((bits >> 23) & 0xFFu) - 127 + 15;
        std::uint32_t mantissa = bits & 0x7FFFFFu;

        if (exponent <= 0)
        {
            if (exponent < -10)
            {
                return static_cast<std::uint16_t>(sign);
            }

            mantissa |= 0x800000u;
            std::uint32_t shift = static_cast<std::uint32_t>(1 - exponent);
            std::uint32_t halfMantissa = mantissa >> (shift + 13);
            return static_cast<std::uint16_t>(sign | halfMantissa);
        }

        if (exponent >= 31)
        {
            return static_cast<std::uint16_t>(sign | 0x7C00u);
        }

        std::uint16_t halfExponent = static_cast<std::uint16_t>(exponent << 10);
        std::uint16_t halfMantissa = static_cast<std::uint16_t>(mantissa >> 13);
        return static_cast<std::uint16_t>(sign | halfExponent | halfMantissa);
    }

    /* Decode an RLE scanline into RGBe components. */
    bool DecodeHdrScanline(std::ifstream& File, int Width, std::vector<std::uint8_t>& OutScanline)
    {
        OutScanline.resize(static_cast<std::size_t>(Width) * 4);

        for (int channel = 0; channel < 4; ++channel)
        {
            int x = 0;
            while (x < Width)
            {
                unsigned char count = 0;
                if (!File.read(reinterpret_cast<char*>(&count), 1))
                {
                    return false;
                }

                if (count > 128)
                {
                    count = static_cast<unsigned char>(count - 128);
                    unsigned char value = 0;
                    if (!File.read(reinterpret_cast<char*>(&value), 1))
                    {
                        return false;
                    }

                    for (int i = 0; i < count; ++i)
                    {
                        OutScanline[static_cast<std::size_t>(4 * x + channel)] = value;
                        ++x;
                    }
                }
                else
                {
                    for (int i = 0; i < count; ++i)
                    {
                        unsigned char value = 0;
                        if (!File.read(reinterpret_cast<char*>(&value), 1))
                        {
                            return false;
                        }

                        OutScanline[static_cast<std::size_t>(4 * x + channel)] = value;
                        ++x;
                    }
                }
            }
        }

        return true;
    }

    /* Load Radiance HDR file to linear floats. */
    bool LoadHdrImage(const std::string& Path, HdrImage& OutImage)
    {
        std::ifstream file(Path, std::ios::binary);
        if (!file.is_open())
        {
            std::fprintf(stderr, "SkyboxRenderer: Failed to open %s\n", Path.c_str());
            return false;
        }

        std::string line;
        bool formatOk = false;
        while (std::getline(file, line))
        {
            if (line.empty())
            {
                break;
            }

            if (line.rfind("FORMAT=", 0) == 0 && line.find("32-bit_rle_rgbe") != std::string::npos)
            {
                formatOk = true;
            }
        }

        if (!formatOk)
        {
            std::fprintf(stderr, "SkyboxRenderer: Unsupported HDR format %s\n", Path.c_str());
            return false;
        }

        std::getline(file, line);
        int width = 0;
        int height = 0;
        if (::sscanf_s(line.c_str(), "-Y %d +X %d", &height, &width) != 2)
        {
            if (::sscanf_s(line.c_str(), "+X %d -Y %d", &width, &height) != 2)
            {
                std::fprintf(stderr, "SkyboxRenderer: Invalid HDR resolution %s\n", Path.c_str());
                return false;
            }
        }

        if (width <= 0 || height <= 0)
        {
            std::fprintf(stderr, "SkyboxRenderer: Invalid HDR dimensions %s\n", Path.c_str());
            return false;
        }

        OutImage.Width = width;
        OutImage.Height = height;
        OutImage.Data.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 3);

        std::vector<std::uint8_t> scanline;
        std::vector<std::uint8_t> rgbe(4);

        for (int y = 0; y < height; ++y)
        {
            if (!file.read(reinterpret_cast<char*>(rgbe.data()), 4))
            {
                return false;
            }

            if (rgbe[0] != 2 || rgbe[1] != 2 || (rgbe[2] & 0x80) != 0)
            {
                std::fprintf(stderr, "SkyboxRenderer: Unsupported HDR scanline %s\n", Path.c_str());
                return false;
            }

            int scanlineWidth = (static_cast<int>(rgbe[2]) << 8) | rgbe[3];
            if (scanlineWidth != width)
            {
                std::fprintf(stderr, "SkyboxRenderer: HDR scanline width mismatch %s\n", Path.c_str());
                return false;
            }

            if (!DecodeHdrScanline(file, width, scanline))
            {
                return false;
            }

            for (int x = 0; x < width; ++x)
            {
                std::uint8_t r = scanline[static_cast<std::size_t>(4 * x + 0)];
                std::uint8_t g = scanline[static_cast<std::size_t>(4 * x + 1)];
                std::uint8_t b = scanline[static_cast<std::size_t>(4 * x + 2)];
                std::uint8_t e = scanline[static_cast<std::size_t>(4 * x + 3)];

                float scale = 0.0f;
                if (e != 0)
                {
                    scale = std::ldexp(1.0f, static_cast<int>(e) - (128 + 8));
                }

                std::size_t index = (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + x) * 3;
                OutImage.Data[index + 0] = (static_cast<float>(r) + 0.5f) * scale;
                OutImage.Data[index + 1] = (static_cast<float>(g) + 0.5f) * scale;
                OutImage.Data[index + 2] = (static_cast<float>(b) + 0.5f) * scale;
            }
        }

        return true;
    }

    /* Sample equirectangular HDR with bilinear filtering. */
    Vec3 SampleEquirectangular(const HdrImage& Image, float U, float V)
    {
        float uWrapped = U - std::floor(U);
        float vClamped = std::clamp(V, 0.0f, 1.0f);

        float x = uWrapped * static_cast<float>(Image.Width - 1);
        float y = vClamped * static_cast<float>(Image.Height - 1);

        int x0 = static_cast<int>(std::floor(x));
        int y0 = static_cast<int>(std::floor(y));
        int x1 = (x0 + 1) % Image.Width;
        int y1 = std::min(y0 + 1, Image.Height - 1);

        float tx = x - static_cast<float>(x0);
        float ty = y - static_cast<float>(y0);

        auto SampleAt = [&](int sx, int sy) -> Vec3
        {
            std::size_t index = (static_cast<std::size_t>(sy) * static_cast<std::size_t>(Image.Width) + sx) * 3;
            return Vec3(
                Image.Data[index + 0],
                Image.Data[index + 1],
                Image.Data[index + 2]);
        };

        Vec3 c00 = SampleAt(x0, y0);
        Vec3 c10 = SampleAt(x1, y0);
        Vec3 c01 = SampleAt(x0, y1);
        Vec3 c11 = SampleAt(x1, y1);

        Vec3 c0 = c00 + (c10 - c00) * tx;
        Vec3 c1 = c01 + (c11 - c01) * tx;
        return c0 + (c1 - c0) * ty;
    }

    /* Convert equirectangular HDR to cubemap faces. */
    bool ConvertToCubemap(
        const HdrImage& Image,
        std::uint32_t FaceSize,
        std::vector<std::uint16_t>& OutData)
    {
        if (Image.Width == 0 || Image.Height == 0)
        {
            return false;
        }

        std::size_t facePixels = static_cast<std::size_t>(FaceSize) * static_cast<std::size_t>(FaceSize);
        OutData.resize(facePixels * 6 * 4);

        for (int face = 0; face < 6; ++face)
        {
            for (std::uint32_t y = 0; y < FaceSize; ++y)
            {
                for (std::uint32_t x = 0; x < FaceSize; ++x)
                {
                    float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(FaceSize);
                    float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(FaceSize);
                    float a = 2.0f * u - 1.0f;
                    float b = 2.0f * v - 1.0f;

                    Vec3 dir;
                    switch (face)
                    {
                    case 0:
                        /* +X face */
                        dir = Vec3(1.0f, -b, -a);
                        break;
                    case 1:
                        /* -X face */
                        dir = Vec3(-1.0f, -b, a);
                        break;
                    case 2:
                        /* +Y face */
                        dir = Vec3(a, 1.0f, b);
                        break;
                    case 3:
                        /* -Y face */
                        dir = Vec3(a, -1.0f, -b);
                        break;
                    case 4:
                        /* +Z face */
                        dir = Vec3(a, -b, 1.0f);
                        break;
                    default:
                        /* -Z face */
                        dir = Vec3(-a, -b, -1.0f);
                        break;
                    }

                    dir = dir.Normalized();

                    float theta = std::atan2(dir.z, dir.x);
                    float phi = std::acos(std::clamp(dir.y, -1.0f, 1.0f));
                    float uEq = (theta + kPi) / (2.0f * kPi);
                    float vEq = phi / kPi;

                    Vec3 color = SampleEquirectangular(Image, uEq, vEq);

                    std::size_t pixelIndex =
                        (static_cast<std::size_t>(face) * facePixels +
                            static_cast<std::size_t>(y) * FaceSize + x) * 4;

                    OutData[pixelIndex + 0] = FloatToHalf(color.x);
                    OutData[pixelIndex + 1] = FloatToHalf(color.y);
                    OutData[pixelIndex + 2] = FloatToHalf(color.z);
                    OutData[pixelIndex + 3] = FloatToHalf(1.0f);
                }
            }
        }

        return true;
    }

    /* Find Vulkan memory type. */
    uint32_t FindMemoryType(
        VkPhysicalDevice PhysicalDevice,
        uint32_t TypeFilter,
        VkMemoryPropertyFlags Properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties{};
        vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
        {
            if ((TypeFilter & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & Properties) == Properties)
            {
                return i;
            }
        }

        return UINT32_MAX;
    }

    /* Create a Vulkan buffer with memory. */
    bool CreateBuffer(
        VkPhysicalDevice PhysicalDevice,
        VkDevice Device,
        VkDeviceSize Size,
        VkBufferUsageFlags Usage,
        VkMemoryPropertyFlags Properties,
        VkBuffer& OutBuffer,
        VkDeviceMemory& OutMemory)
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = Size;
        bufferInfo.usage = Usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(Device, &bufferInfo, nullptr, &OutBuffer) != VK_SUCCESS)
        {
            std::fprintf(stderr, "SkyboxRenderer: vkCreateBuffer failed\n");
            return false;
        }

        VkMemoryRequirements memRequirements{};
        vkGetBufferMemoryRequirements(Device, OutBuffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(
            PhysicalDevice,
            memRequirements.memoryTypeBits,
            Properties);

        if (vkAllocateMemory(Device, &allocInfo, nullptr, &OutMemory) != VK_SUCCESS)
        {
            std::fprintf(stderr, "SkyboxRenderer: vkAllocateMemory failed\n");
            vkDestroyBuffer(Device, OutBuffer, nullptr);
            OutBuffer = VK_NULL_HANDLE;
            return false;
        }

        vkBindBufferMemory(Device, OutBuffer, OutMemory, 0);
        return true;
    }

    /* Begin one-shot command buffer. */
    bool BeginSingleTimeCommands(
        VkDevice Device,
        uint32_t QueueFamily,
        VkCommandPool& OutPool,
        VkCommandBuffer& OutCommandBuffer)
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        poolInfo.queueFamilyIndex = QueueFamily;

        if (vkCreateCommandPool(Device, &poolInfo, nullptr, &OutPool) != VK_SUCCESS)
        {
            std::fprintf(stderr, "SkyboxRenderer: vkCreateCommandPool failed\n");
            return false;
        }

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = OutPool;
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(Device, &allocInfo, &OutCommandBuffer) != VK_SUCCESS)
        {
            std::fprintf(stderr, "SkyboxRenderer: vkAllocateCommandBuffers failed\n");
            vkDestroyCommandPool(Device, OutPool, nullptr);
            OutPool = VK_NULL_HANDLE;
            return false;
        }

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        if (vkBeginCommandBuffer(OutCommandBuffer, &beginInfo) != VK_SUCCESS)
        {
            std::fprintf(stderr, "SkyboxRenderer: vkBeginCommandBuffer failed\n");
            vkFreeCommandBuffers(Device, OutPool, 1, &OutCommandBuffer);
            vkDestroyCommandPool(Device, OutPool, nullptr);
            OutPool = VK_NULL_HANDLE;
            return false;
        }

        return true;
    }

    /* End and submit one-shot command buffer. */
    void EndSingleTimeCommands(
        VkDevice Device,
        VkQueue Queue,
        VkCommandPool CommandPool,
        VkCommandBuffer CommandBuffer)
    {
        vkEndCommandBuffer(CommandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &CommandBuffer;

        vkQueueSubmit(Queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(Queue);

        vkFreeCommandBuffers(Device, CommandPool, 1, &CommandBuffer);
        vkDestroyCommandPool(Device, CommandPool, nullptr);
    }

    /* Transition cubemap image layout. */
    void TransitionImageLayout(
        VkCommandBuffer CommandBuffer,
        VkImage Image,
        VkImageLayout OldLayout,
        VkImageLayout NewLayout,
        uint32_t LayerCount)
    {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = OldLayout;
        barrier.newLayout = NewLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = Image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = LayerCount;

        VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

        if (OldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
            NewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
            NewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }

        vkCmdPipelineBarrier(
            CommandBuffer,
            srcStage,
            dstStage,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier);
    }

    /* Copy a buffer into a cubemap image. */
    void CopyBufferToImage(
        VkCommandBuffer CommandBuffer,
        VkBuffer Buffer,
        VkImage Image,
        uint32_t FaceSize)
    {
        std::array<VkBufferImageCopy, 6> regions{};
        VkDeviceSize faceSizeBytes = static_cast<VkDeviceSize>(FaceSize) *
            static_cast<VkDeviceSize>(FaceSize) *
            sizeof(std::uint16_t) * 4;

        for (uint32_t face = 0; face < 6; ++face)
        {
            VkBufferImageCopy& region = regions[face];
            region.bufferOffset = faceSizeBytes * face;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = face;
            region.imageSubresource.layerCount = 1;
            region.imageOffset = { 0, 0, 0 };
            region.imageExtent = { FaceSize, FaceSize, 1 };
        }

        vkCmdCopyBufferToImage(
            CommandBuffer,
            Buffer,
            Image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            static_cast<uint32_t>(regions.size()),
            regions.data());
    }
}

SkyboxRenderer::SkyboxRenderer() = default;

SkyboxRenderer::~SkyboxRenderer() = default;

bool SkyboxRenderer::Create(
    VkDevice Device,
    VkPhysicalDevice PhysicalDevice,
    VkRenderPass RenderPass,
    VkQueue GraphicsQueue,
    std::uint32_t GraphicsQueueFamily)
{
    Destroy(Device);

    if (!LocateAssetsReadyRoot())
    {
        std::fprintf(stderr, "SkyboxRenderer::Create: Assets/Ready root not found\n");
        return false;
    }

    if (!LoadCatalog())
    {
        std::fprintf(stderr, "SkyboxRenderer::Create: Failed to load skybox catalog\n");
        return false;
    }

    if (!CreateDescriptorResources(Device))
    {
        std::fprintf(stderr, "SkyboxRenderer::Create: Descriptor resources failed\n");
        return false;
    }

    if (!CreateVertexBuffer(PhysicalDevice, Device, GraphicsQueue, GraphicsQueueFamily))
    {
        std::fprintf(stderr, "SkyboxRenderer::Create: Vertex buffer failed\n");
        return false;
    }

    VertexShaderPath = FindFileInTree(AssetsReadyRoot, "Skybox.vert.spv");
    FragmentShaderPath = FindFileInTree(AssetsReadyRoot, "Skybox.frag.spv");

    if (VertexShaderPath.empty() || FragmentShaderPath.empty())
    {
        std::fprintf(stderr, "SkyboxRenderer::Create: Skybox shaders not found in Assets/Ready\n");
        return false;
    }

    ActiveSkyboxName = DefaultSkyboxName;
    auto it = Skyboxes.find(ActiveSkyboxName);
    if (it == Skyboxes.end())
    {
        std::fprintf(stderr, "SkyboxRenderer::Create: Default skybox not found\n");
        return false;
    }

    if (!LoadSkyboxResources(it->second, Device, PhysicalDevice, GraphicsQueue, GraphicsQueueFamily))
    {
        std::fprintf(stderr, "SkyboxRenderer::Create: Failed to load skybox resources\n");
        return false;
    }

    if (!Pipeline.Create(
        Device,
        RenderPass,
        DescriptorSetLayout,
        VertexShaderPath.c_str(),
        FragmentShaderPath.c_str()))
    {
        std::fprintf(stderr, "SkyboxRenderer::Create: Failed to create pipeline\n");
        return false;
    }

    return true;
}

void SkyboxRenderer::Destroy(VkDevice Device)
{
    Pipeline.Destroy(Device);
    DestroySkyboxResources(Device);
    DestroyDescriptorResources(Device);
    VertexBuffer.Destroy(Device);

    AssetsReadyRoot.clear();
    VertexShaderPath.clear();
    FragmentShaderPath.clear();
    Skyboxes.clear();
    DefaultSkyboxName.clear();
    ActiveSkyboxName.clear();
}

bool SkyboxRenderer::Recreate(VkDevice Device, VkRenderPass RenderPass)
{
    if (DescriptorSetLayout == VK_NULL_HANDLE ||
        VertexShaderPath.empty() ||
        FragmentShaderPath.empty())
    {
        return false;
    }

    Pipeline.Destroy(Device);
    return Pipeline.Create(
        Device,
        RenderPass,
        DescriptorSetLayout,
        VertexShaderPath.c_str(),
        FragmentShaderPath.c_str());
}

void SkyboxRenderer::Record(
    VkCommandBuffer CommandBuffer,
    VkExtent2D Extent,
    const Camera* Camera)
{
    if (!Camera || Pipeline.GetPipeline() == VK_NULL_HANDLE || DescriptorSet == VK_NULL_HANDLE)
    {
        return;
    }

    if (Extent.width == 0 || Extent.height == 0)
    {
        return;
    }

    float fovRadians = Camera->GetZoom() * kPi / 180.0f;
    Mat4 projection = Mat4::Perspective(fovRadians, static_cast<float>(Extent.width) /
        static_cast<float>(Extent.height), 0.1f, 1000.0f);

    Vec3 front = Camera->GetFront();
    Vec3 up = Camera->GetUp();

    /* Build a rotation-only view matrix from the camera orientation. */
    Mat4 view = Mat4::LookAt(Vec3(0.0f, 0.0f, 0.0f), front, up);
    Mat4 viewRotation = Mat4::Identity();
    viewRotation.m[0] = view.m[0];
    viewRotation.m[1] = view.m[1];
    viewRotation.m[2] = view.m[2];
    viewRotation.m[4] = view.m[4];
    viewRotation.m[5] = view.m[5];
    viewRotation.m[6] = view.m[6];
    viewRotation.m[8] = view.m[8];
    viewRotation.m[9] = view.m[9];
    viewRotation.m[10] = view.m[10];
    /* Combine projection with rotation-only view. */
    Mat4 viewProjection = projection * viewRotation;

    /* Use object-space directions for cubemap sampling; view rotation already aligns the cube. */
    Mat4 viewInv = Mat4::Identity();

    SkyboxPushConstants pushConstants{};
    pushConstants.ViewProjection = viewProjection;
    pushConstants.ViewInverse = viewInv;

    /* Bind viewport and scissor for skybox draw. */
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(Extent.width);
    viewport.height = static_cast<float>(Extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = Extent;

    vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.GetPipeline());
    vkCmdBindDescriptorSets(
        CommandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        Pipeline.GetLayout(),
        0,
        1,
        &DescriptorSet,
        0,
        nullptr);
    vkCmdSetViewport(CommandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(CommandBuffer, 0, 1, &scissor);

    VkBuffer vertexBuffer = VertexBuffer.GetBuffer();
    if (vertexBuffer == VK_NULL_HANDLE)
    {
        return;
    }

    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &vertexBuffer, offsets);

    vkCmdPushConstants(
        CommandBuffer,
        Pipeline.GetLayout(),
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(SkyboxPushConstants),
        &pushConstants);

    vkCmdDraw(CommandBuffer, kSkyboxVertexCount, 1, 0, 0);
}

bool SkyboxRenderer::SetActiveSkybox(
    const std::string& Name,
    VkDevice Device,
    VkPhysicalDevice PhysicalDevice,
    VkQueue GraphicsQueue,
    std::uint32_t GraphicsQueueFamily)
{
    auto it = Skyboxes.find(Name);
    if (it == Skyboxes.end())
    {
        return false;
    }

    if (!LoadSkyboxResources(it->second, Device, PhysicalDevice, GraphicsQueue, GraphicsQueueFamily))
    {
        return false;
    }

    ActiveSkyboxName = Name;
    return true;
}

const std::string& SkyboxRenderer::GetActiveSkyboxName() const
{
    return ActiveSkyboxName;
}

bool SkyboxRenderer::IsReady() const
{
    return Pipeline.GetPipeline() != VK_NULL_HANDLE &&
        DescriptorSet != VK_NULL_HANDLE &&
        CubemapView != VK_NULL_HANDLE;
}

bool SkyboxRenderer::LocateAssetsReadyRoot()
{
    std::error_code error;
    std::filesystem::path current = std::filesystem::current_path(error);
    if (error)
    {
        return false;
    }

    while (!current.empty())
    {
        std::filesystem::path candidate = current / "Assets" / "Ready";
        if (std::filesystem::exists(candidate, error))
        {
            AssetsReadyRoot = candidate.string();
            return true;
        }

        std::filesystem::path parent = current.parent_path();
        if (parent == current)
        {
            break;
        }

        current = parent;
    }

    return false;
}

bool SkyboxRenderer::LoadCatalog()
{
    Skyboxes.clear();
    DefaultSkyboxName.clear();

    std::string catalogPath = FindFileInTree(AssetsReadyRoot, "skyboxes.txt");
    if (catalogPath.empty())
    {
        std::fprintf(stderr, "SkyboxRenderer::LoadCatalog: skyboxes.txt not found\n");
        return false;
    }

    std::ifstream file(catalogPath);
    if (!file.is_open())
    {
        std::fprintf(stderr, "SkyboxRenderer::LoadCatalog: Failed to open %s\n", catalogPath.c_str());
        return false;
    }

    std::string line;
    while (std::getline(file, line))
    {
        std::string trimmed = Trim(line);
        if (trimmed.empty())
        {
            continue;
        }

        if (trimmed[0] == '#')
        {
            continue;
        }

        std::size_t equalPos = trimmed.find('=');
        if (equalPos == std::string::npos)
        {
            continue;
        }

        std::string key = Trim(trimmed.substr(0, equalPos));
        std::string value = Trim(trimmed.substr(equalPos + 1));

        if (key.empty() || value.empty())
        {
            continue;
        }

        if (key == "default")
        {
            DefaultSkyboxName = value;
            continue;
        }

        SkyboxDefinition definition{};
        definition.Name = key;

        std::stringstream tokenizer(value);
        std::string token;
        bool firstToken = true;
        while (std::getline(tokenizer, token, ';'))
        {
            std::string part = Trim(token);
            if (part.empty())
            {
                continue;
            }

            if (firstToken)
            {
                std::filesystem::path hdrPath = part;
                if (hdrPath.is_relative())
                {
                    hdrPath = std::filesystem::path(AssetsReadyRoot) / hdrPath;
                }

                definition.HdrPath = hdrPath.string();
                firstToken = false;
            }
            else if (part.rfind("size=", 0) == 0)
            {
                std::string sizeStr = Trim(part.substr(5));
                if (!sizeStr.empty())
                {
                    char* endPtr = nullptr;
                    unsigned long sizeValue = std::strtoul(sizeStr.c_str(), &endPtr, 10);
                    if (endPtr != sizeStr.c_str())
                    {
                        definition.Size = static_cast<std::uint32_t>(sizeValue);
                    }
                }
            }
        }

        if (!definition.HdrPath.empty())
        {
            Skyboxes[definition.Name] = definition;
        }
    }

    if (Skyboxes.empty())
    {
        std::fprintf(stderr, "SkyboxRenderer::LoadCatalog: No skyboxes defined\n");
        return false;
    }

    if (DefaultSkyboxName.empty() || Skyboxes.find(DefaultSkyboxName) == Skyboxes.end())
    {
        DefaultSkyboxName = Skyboxes.begin()->first;
    }

    return true;
}

bool SkyboxRenderer::LoadSkyboxResources(
    const SkyboxDefinition& Definition,
    VkDevice Device,
    VkPhysicalDevice PhysicalDevice,
    VkQueue GraphicsQueue,
    std::uint32_t GraphicsQueueFamily)
{
    DestroySkyboxResources(Device);

    HdrImage hdrImage;
    if (!LoadHdrImage(Definition.HdrPath, hdrImage))
    {
        std::fprintf(stderr, "SkyboxRenderer: Failed to load HDR %s\n", Definition.HdrPath.c_str());
        return false;
    }

    std::vector<std::uint16_t> cubemapData;
    if (!ConvertToCubemap(hdrImage, Definition.Size, cubemapData))
    {
        std::fprintf(stderr, "SkyboxRenderer: Failed to build cubemap %s\n", Definition.HdrPath.c_str());
        return false;
    }

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    VkDeviceSize dataSize = static_cast<VkDeviceSize>(cubemapData.size()) * sizeof(std::uint16_t);

    if (!CreateBuffer(
        PhysicalDevice,
        Device,
        dataSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingMemory))
    {
        return false;
    }

    void* mapped = nullptr;
    if (vkMapMemory(Device, stagingMemory, 0, dataSize, 0, &mapped) != VK_SUCCESS)
    {
        std::fprintf(stderr, "SkyboxRenderer: vkMapMemory failed\n");
        vkDestroyBuffer(Device, stagingBuffer, nullptr);
        vkFreeMemory(Device, stagingMemory, nullptr);
        return false;
    }

    std::memcpy(mapped, cubemapData.data(), static_cast<std::size_t>(dataSize));
    vkUnmapMemory(Device, stagingMemory);

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = Definition.Size;
    imageInfo.extent.height = Definition.Size;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 6;
    imageInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    if (vkCreateImage(Device, &imageInfo, nullptr, &CubemapImage) != VK_SUCCESS)
    {
        std::fprintf(stderr, "SkyboxRenderer: vkCreateImage failed\n");
        vkDestroyBuffer(Device, stagingBuffer, nullptr);
        vkFreeMemory(Device, stagingMemory, nullptr);
        return false;
    }

    VkMemoryRequirements memRequirements{};
    vkGetImageMemoryRequirements(Device, CubemapImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(
        PhysicalDevice,
        memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(Device, &allocInfo, nullptr, &CubemapMemory) != VK_SUCCESS)
    {
        std::fprintf(stderr, "SkyboxRenderer: vkAllocateMemory failed\n");
        vkDestroyImage(Device, CubemapImage, nullptr);
        vkDestroyBuffer(Device, stagingBuffer, nullptr);
        vkFreeMemory(Device, stagingMemory, nullptr);
        CubemapImage = VK_NULL_HANDLE;
        return false;
    }

    vkBindImageMemory(Device, CubemapImage, CubemapMemory, 0);

    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    if (!BeginSingleTimeCommands(Device, GraphicsQueueFamily, commandPool, commandBuffer))
    {
        vkDestroyImage(Device, CubemapImage, nullptr);
        vkFreeMemory(Device, CubemapMemory, nullptr);
        vkDestroyBuffer(Device, stagingBuffer, nullptr);
        vkFreeMemory(Device, stagingMemory, nullptr);
        CubemapImage = VK_NULL_HANDLE;
        CubemapMemory = VK_NULL_HANDLE;
        return false;
    }

    TransitionImageLayout(
        commandBuffer,
        CubemapImage,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        6);

    CopyBufferToImage(commandBuffer, stagingBuffer, CubemapImage, Definition.Size);

    TransitionImageLayout(
        commandBuffer,
        CubemapImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        6);

    EndSingleTimeCommands(Device, GraphicsQueue, commandPool, commandBuffer);

    vkDestroyBuffer(Device, stagingBuffer, nullptr);
    vkFreeMemory(Device, stagingMemory, nullptr);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = CubemapImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    viewInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 6;

    if (vkCreateImageView(Device, &viewInfo, nullptr, &CubemapView) != VK_SUCCESS)
    {
        std::fprintf(stderr, "SkyboxRenderer: vkCreateImageView failed\n");
        DestroySkyboxResources(Device);
        return false;
    }

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;

    if (vkCreateSampler(Device, &samplerInfo, nullptr, &CubemapSampler) != VK_SUCCESS)
    {
        std::fprintf(stderr, "SkyboxRenderer: vkCreateSampler failed\n");
        DestroySkyboxResources(Device);
        return false;
    }

    VkDescriptorImageInfo imageInfoDesc{};
    imageInfoDesc.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfoDesc.imageView = CubemapView;
    imageInfoDesc.sampler = CubemapSampler;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = DescriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfoDesc;

    vkUpdateDescriptorSets(Device, 1, &descriptorWrite, 0, nullptr);
    return true;
}

void SkyboxRenderer::DestroySkyboxResources(VkDevice Device)
{
    if (CubemapSampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(Device, CubemapSampler, nullptr);
        CubemapSampler = VK_NULL_HANDLE;
    }

    if (CubemapView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(Device, CubemapView, nullptr);
        CubemapView = VK_NULL_HANDLE;
    }

    if (CubemapImage != VK_NULL_HANDLE)
    {
        vkDestroyImage(Device, CubemapImage, nullptr);
        CubemapImage = VK_NULL_HANDLE;
    }

    if (CubemapMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(Device, CubemapMemory, nullptr);
        CubemapMemory = VK_NULL_HANDLE;
    }
}

bool SkyboxRenderer::CreateDescriptorResources(VkDevice Device)
{
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerLayoutBinding;

    if (vkCreateDescriptorSetLayout(Device, &layoutInfo, nullptr, &DescriptorSetLayout) != VK_SUCCESS)
    {
        std::fprintf(stderr, "SkyboxRenderer: vkCreateDescriptorSetLayout failed\n");
        return false;
    }

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;

    if (vkCreateDescriptorPool(Device, &poolInfo, nullptr, &DescriptorPool) != VK_SUCCESS)
    {
        std::fprintf(stderr, "SkyboxRenderer: vkCreateDescriptorPool failed\n");
        vkDestroyDescriptorSetLayout(Device, DescriptorSetLayout, nullptr);
        DescriptorSetLayout = VK_NULL_HANDLE;
        return false;
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = DescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &DescriptorSetLayout;

    if (vkAllocateDescriptorSets(Device, &allocInfo, &DescriptorSet) != VK_SUCCESS)
    {
        std::fprintf(stderr, "SkyboxRenderer: vkAllocateDescriptorSets failed\n");
        vkDestroyDescriptorPool(Device, DescriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(Device, DescriptorSetLayout, nullptr);
        DescriptorPool = VK_NULL_HANDLE;
        DescriptorSetLayout = VK_NULL_HANDLE;
        return false;
    }

    return true;
}

void SkyboxRenderer::DestroyDescriptorResources(VkDevice Device)
{
    if (DescriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(Device, DescriptorPool, nullptr);
        DescriptorPool = VK_NULL_HANDLE;
        DescriptorSet = VK_NULL_HANDLE;
    }

    if (DescriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(Device, DescriptorSetLayout, nullptr);
        DescriptorSetLayout = VK_NULL_HANDLE;
    }
}

bool SkyboxRenderer::CreateVertexBuffer(
    VkPhysicalDevice PhysicalDevice,
    VkDevice Device,
    VkQueue GraphicsQueue,
    std::uint32_t GraphicsQueueFamily)
{
    const float vertices[] = {
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    VkDeviceSize size = sizeof(vertices);
    if (!VertexBuffer.CreateVertexBuffer(
        PhysicalDevice,
        Device,
        GraphicsQueue,
        GraphicsQueueFamily,
        vertices,
        size))
    {
        std::fprintf(stderr, "SkyboxRenderer: Vertex buffer upload failed\n");
        return false;
    }

    return true;
}
