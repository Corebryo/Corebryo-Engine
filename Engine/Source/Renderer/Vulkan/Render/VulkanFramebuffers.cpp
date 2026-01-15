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

#include "VulkanFramebuffers.h"

#include <cstdio>

/* Initialize empty framebuffer state. */
VulkanFramebuffers::VulkanFramebuffers()
    : Device(VK_NULL_HANDLE)
{
}

/* Destroy framebuffer object. */
VulkanFramebuffers::~VulkanFramebuffers()
{
}

/* Create framebuffers for each swapchain image view. */
bool VulkanFramebuffers::Create(
    VkDevice Device,
    VkRenderPass RenderPass,
    VkExtent2D Extent,
    const std::vector<VkImageView>& SwapchainImageViews)
{
    this->Device = Device;
    
    Framebuffers.resize(SwapchainImageViews.size());

    for (size_t i = 0; i < SwapchainImageViews.size(); ++i)
    {
        VkImageView attachments[] = { SwapchainImageViews[i] };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = RenderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = Extent.width;
        framebufferInfo.height = Extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(Device, &framebufferInfo, nullptr, &Framebuffers[i]) != VK_SUCCESS)
        {
            std::fprintf(stderr, "VulkanFramebuffers::Create: failed to create framebuffer %zu\n", i);
            Destroy(Device);
            return false;
        }
    }

    return true;
}

/* Destroy all framebuffers. */
void VulkanFramebuffers::Destroy(VkDevice Device)
{
    for (VkFramebuffer framebuffer : Framebuffers)
    {
        if (framebuffer != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(Device, framebuffer, nullptr);
        }
    }
    Framebuffers.clear();
    this->Device = VK_NULL_HANDLE;
}

/* Get framebuffer handles. */
const std::vector<VkFramebuffer>& VulkanFramebuffers::GetFramebuffers() const
{
    return Framebuffers;
}

/* Get framebuffer at specific index. */
VkFramebuffer VulkanFramebuffers::GetFramebuffer(uint32_t Index) const
{
    if (Index < Framebuffers.size())
    {
        return Framebuffers[Index];
    }
    return VK_NULL_HANDLE;
}

/* Get framebuffer count. */
uint32_t VulkanFramebuffers::GetCount() const
{
    return static_cast<uint32_t>(Framebuffers.size());
}