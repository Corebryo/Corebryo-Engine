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

/* Manages graphics pipelines and layouts. */

class VulkanPipeline
{
public:
    /* Initialize pipeline state. */
    VulkanPipeline();

    /* Release pipeline resources. */
    ~VulkanPipeline();

    /* Create graphics pipelines for main and shadow passes. */
    bool Create(
        VkDevice Device,
        VkRenderPass RenderPass,
        VkRenderPass ShadowRenderPass,
        VkDescriptorSetLayout DescriptorSetLayout);

    /* Destroy pipeline handles and layouts. */
    void Destroy(VkDevice Device);

    /* Sky rendering pipeline. */
    VkPipeline GetSkyHandle() const;

    /* World rendering pipeline. */
    VkPipeline GetWorldHandle() const;

    /* Shared pipeline layout. */
    VkPipelineLayout GetLayout() const;

    /* Shadow map pipeline. */
    VkPipeline GetShadowHandle() const;

    /* Shadow pipeline layout. */
    VkPipelineLayout GetShadowLayout() const;

private:
    /* Main render pipelines. */
    VkPipeline SkyPipeline = VK_NULL_HANDLE;
    VkPipeline WorldPipeline = VK_NULL_HANDLE;

    /* Shadow render pipeline. */
    VkPipeline ShadowPipeline = VK_NULL_HANDLE;

    /* Pipeline layouts. */
    VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;
    VkPipelineLayout ShadowPipelineLayout = VK_NULL_HANDLE;
};
