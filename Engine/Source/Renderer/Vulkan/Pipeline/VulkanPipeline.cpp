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

#include "VulkanPipeline.h"

#include <cstdio>
#include <fstream>
#include <vector>

namespace
{
    struct PushConstants
    {
        float BaseColorAmbient[4];
        float AlphaModePadding[4];
    };

    struct ShadowPushConstants
    {
        float LightViewProj[16];
        float Model[16];
    };

    bool ReadFile(const char* path, std::vector<char>& outData)
    {
        std::ifstream file(path, std::ios::ate | std::ios::binary);
        if (!file.is_open())
        {
            std::fprintf(stderr, "VulkanPipeline::Create: Failed to open %s\n", path);
            return false;
        }

        std::streamsize fileSize = file.tellg();
        if (fileSize <= 0)
        {
            std::fprintf(stderr, "VulkanPipeline::Create: Empty shader %s\n", path);
            return false;
        }

        outData.resize(static_cast<size_t>(fileSize));
        file.seekg(0, std::ios::beg);
        if (!file.read(outData.data(), fileSize))
        {
            std::fprintf(stderr, "VulkanPipeline::Create: Failed to read %s\n", path);
            return false;
        }
        return true;
    }

    VkShaderModule CreateShaderModule(VkDevice device, const std::vector<char>& code)
    {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule = VK_NULL_HANDLE;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        {
            return VK_NULL_HANDLE;
        }
        return shaderModule;
    }
}

VulkanPipeline::VulkanPipeline()
    : SkyPipeline(VK_NULL_HANDLE)
    , WorldPipeline(VK_NULL_HANDLE)
    , ShadowPipeline(VK_NULL_HANDLE)
    , PipelineLayout(VK_NULL_HANDLE)
    , ShadowPipelineLayout(VK_NULL_HANDLE)
{
}

VulkanPipeline::~VulkanPipeline()
{
}

bool VulkanPipeline::Create(
    VkDevice Device,
    VkRenderPass RenderPass,
    VkRenderPass ShadowRenderPass,
    VkDescriptorSetLayout DescriptorSetLayout)
{
    if (SkyPipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(Device, SkyPipeline, nullptr);
        SkyPipeline = VK_NULL_HANDLE;
    }
    if (WorldPipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(Device, WorldPipeline, nullptr);
        WorldPipeline = VK_NULL_HANDLE;
    }
    if (ShadowPipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(Device, ShadowPipeline, nullptr);
        ShadowPipeline = VK_NULL_HANDLE;
    }
    if (PipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(Device, PipelineLayout, nullptr);
        PipelineLayout = VK_NULL_HANDLE;
    }
    if (ShadowPipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(Device, ShadowPipelineLayout, nullptr);
        ShadowPipelineLayout = VK_NULL_HANDLE;
    }

    std::vector<char> vertShaderCode;
    std::vector<char> fragShaderCode;
    if (!ReadFile("../Assets/Ready/Triangle.vert.spv", vertShaderCode) ||
        !ReadFile("../Assets/Ready/Triangle.frag.spv", fragShaderCode))
    {
        return false;
    }

    VkShaderModule vertShaderModule = CreateShaderModule(Device, vertShaderCode);
    VkShaderModule fragShaderModule = CreateShaderModule(Device, fragShaderCode);
    if (vertShaderModule == VK_NULL_HANDLE || fragShaderModule == VK_NULL_HANDLE)
    {
        if (vertShaderModule != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(Device, vertShaderModule, nullptr);
        }
        if (fragShaderModule != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(Device, fragShaderModule, nullptr);
        }
        std::fprintf(stderr, "VulkanPipeline::Create: Failed to create shader modules\n");
        return false;
    }

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        vertShaderStageInfo,
        fragShaderStageInfo
    };

    VkVertexInputBindingDescription bindings[2]{};
    bindings[0].binding = 0;
    bindings[0].stride = sizeof(float) * 5;
    bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    bindings[1].binding = 1;
    bindings[1].stride = sizeof(float) * 16;
    bindings[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

    VkVertexInputAttributeDescription attributeDescriptions[6]{};
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = 0;

    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[1].offset = sizeof(float) * 3;

    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].binding = 1;
    attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[2].offset = 0;

    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].binding = 1;
    attributeDescriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[3].offset = sizeof(float) * 4;

    attributeDescriptions[4].location = 4;
    attributeDescriptions[4].binding = 1;
    attributeDescriptions[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[4].offset = sizeof(float) * 8;

    attributeDescriptions[5].location = 5;
    attributeDescriptions[5].binding = 1;
    attributeDescriptions[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[5].offset = sizeof(float) * 12;

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 2;
    vertexInputInfo.pVertexBindingDescriptions = bindings;
    vertexInputInfo.vertexAttributeDescriptionCount = 6;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

    VkVertexInputBindingDescription shadowBinding{};
    shadowBinding.binding = 0;
    shadowBinding.stride = sizeof(float) * 5;
    shadowBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription shadowAttribute{};
    shadowAttribute.location = 0;
    shadowAttribute.binding = 0;
    shadowAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    shadowAttribute.offset = 0;

    VkPipelineVertexInputStateCreateInfo shadowVertexInputInfo{};
    shadowVertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    shadowVertexInputInfo.vertexBindingDescriptionCount = 1;
    shadowVertexInputInfo.pVertexBindingDescriptions = &shadowBinding;
    shadowVertexInputInfo.vertexAttributeDescriptionCount = 1;
    shadowVertexInputInfo.pVertexAttributeDescriptions = &shadowAttribute;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = 1.0f;
    viewport.height = 1.0f;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = { 1, 1 };

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_4_BIT;

    VkPipelineMultisampleStateCreateInfo shadowMultisampling = multisampling;
    shadowMultisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPipelineColorBlendAttachmentState alphaBlendAttachment = colorBlendAttachment;
    alphaBlendAttachment.blendEnable = VK_TRUE;
    alphaBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    alphaBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    alphaBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    alphaBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    alphaBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    alphaBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo alphaBlending = colorBlending;
    alphaBlending.pAttachments = &alphaBlendAttachment;

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &DescriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstants);

    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(Device, &pipelineLayoutInfo, nullptr, &PipelineLayout) != VK_SUCCESS)
    {
        std::fprintf(stderr, "VulkanPipeline::Create: vkCreatePipelineLayout failed\n");
        vkDestroyShaderModule(Device, fragShaderModule, nullptr);
        vkDestroyShaderModule(Device, vertShaderModule, nullptr);
        return false;
    }

    VkPipelineLayoutCreateInfo shadowLayoutInfo{};
    shadowLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    shadowLayoutInfo.setLayoutCount = 0;

    VkPushConstantRange shadowPushRange{};
    shadowPushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    shadowPushRange.offset = 0;
    shadowPushRange.size = sizeof(ShadowPushConstants);

    shadowLayoutInfo.pushConstantRangeCount = 1;
    shadowLayoutInfo.pPushConstantRanges = &shadowPushRange;

    if (vkCreatePipelineLayout(Device, &shadowLayoutInfo, nullptr, &ShadowPipelineLayout) != VK_SUCCESS)
    {
        std::fprintf(stderr, "VulkanPipeline::Create: vkCreatePipelineLayout (shadow) failed\n");
        vkDestroyPipelineLayout(Device, PipelineLayout, nullptr);
        PipelineLayout = VK_NULL_HANDLE;
        vkDestroyShaderModule(Device, fragShaderModule, nullptr);
        vkDestroyShaderModule(Device, vertShaderModule, nullptr);
        return false;
    }

    auto CreateGraphicsPipeline = [&](VkBool32 depthTest,
        VkBool32 depthWrite,
        VkCompareOp depthCompare,
        const VkPipelineColorBlendStateCreateInfo& blendState,
        VkPipeline& outPipeline) -> bool
    {
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = depthTest;
        depthStencil.depthWriteEnable = depthWrite;
        depthStencil.depthCompareOp = depthCompare;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &blendState;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = PipelineLayout;
        pipelineInfo.renderPass = RenderPass;
        pipelineInfo.subpass = 0;

        if (vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &outPipeline) != VK_SUCCESS)
        {
            return false;
        }

        return true;
    };

    if (!CreateGraphicsPipeline(VK_TRUE, VK_FALSE, VK_COMPARE_OP_ALWAYS, colorBlending, SkyPipeline))
    {
        std::fprintf(stderr, "VulkanPipeline::Create: vkCreateGraphicsPipelines (sky) failed\n");
        vkDestroyPipelineLayout(Device, PipelineLayout, nullptr);
        PipelineLayout = VK_NULL_HANDLE;
        vkDestroyShaderModule(Device, fragShaderModule, nullptr);
        vkDestroyShaderModule(Device, vertShaderModule, nullptr);
        return false;
    }

    if (!CreateGraphicsPipeline(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS, alphaBlending, WorldPipeline))
    {
        std::fprintf(stderr, "VulkanPipeline::Create: vkCreateGraphicsPipelines (world) failed\n");
        vkDestroyPipeline(Device, SkyPipeline, nullptr);
        SkyPipeline = VK_NULL_HANDLE;
        vkDestroyPipelineLayout(Device, PipelineLayout, nullptr);
        PipelineLayout = VK_NULL_HANDLE;
        vkDestroyShaderModule(Device, fragShaderModule, nullptr);
        vkDestroyShaderModule(Device, vertShaderModule, nullptr);
        return false;
    }

    std::vector<char> shadowVertCode;
    if (!ReadFile("../Assets/Ready/Shadow.vert.spv", shadowVertCode))
    {
        std::fprintf(stderr, "VulkanPipeline::Create: Failed to load shadow vertex shader\n");
        vkDestroyPipeline(Device, SkyPipeline, nullptr);
        vkDestroyPipeline(Device, WorldPipeline, nullptr);
        SkyPipeline = VK_NULL_HANDLE;
        WorldPipeline = VK_NULL_HANDLE;
        vkDestroyPipelineLayout(Device, ShadowPipelineLayout, nullptr);
        ShadowPipelineLayout = VK_NULL_HANDLE;
        vkDestroyPipelineLayout(Device, PipelineLayout, nullptr);
        PipelineLayout = VK_NULL_HANDLE;
        vkDestroyShaderModule(Device, fragShaderModule, nullptr);
        vkDestroyShaderModule(Device, vertShaderModule, nullptr);
        return false;
    }

    VkShaderModule shadowVertModule = CreateShaderModule(Device, shadowVertCode);
    if (shadowVertModule == VK_NULL_HANDLE)
    {
        std::fprintf(stderr, "VulkanPipeline::Create: Failed to create shadow shader module\n");
        vkDestroyPipeline(Device, SkyPipeline, nullptr);
        vkDestroyPipeline(Device, WorldPipeline, nullptr);
        SkyPipeline = VK_NULL_HANDLE;
        WorldPipeline = VK_NULL_HANDLE;
        vkDestroyPipelineLayout(Device, ShadowPipelineLayout, nullptr);
        ShadowPipelineLayout = VK_NULL_HANDLE;
        vkDestroyPipelineLayout(Device, PipelineLayout, nullptr);
        PipelineLayout = VK_NULL_HANDLE;
        vkDestroyShaderModule(Device, fragShaderModule, nullptr);
        vkDestroyShaderModule(Device, vertShaderModule, nullptr);
        return false;
    }

    VkPipelineShaderStageCreateInfo shadowStage{};
    shadowStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shadowStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    shadowStage.module = shadowVertModule;
    shadowStage.pName = "main";

    VkPipelineShaderStageCreateInfo shadowStages[] = { shadowStage };

    VkPipelineDepthStencilStateCreateInfo shadowDepth{};
    shadowDepth.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    shadowDepth.depthTestEnable = VK_TRUE;
    shadowDepth.depthWriteEnable = VK_TRUE;
    shadowDepth.depthCompareOp = VK_COMPARE_OP_LESS;
    shadowDepth.depthBoundsTestEnable = VK_FALSE;
    shadowDepth.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo shadowBlend{};
    shadowBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    shadowBlend.logicOpEnable = VK_FALSE;
    shadowBlend.attachmentCount = 0;
    shadowBlend.pAttachments = nullptr;

    VkGraphicsPipelineCreateInfo shadowInfo{};
    shadowInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    shadowInfo.stageCount = 1;
    shadowInfo.pStages = shadowStages;
    shadowInfo.pVertexInputState = &shadowVertexInputInfo;
    shadowInfo.pInputAssemblyState = &inputAssembly;
    shadowInfo.pViewportState = &viewportState;
    shadowInfo.pRasterizationState = &rasterizer;
    shadowInfo.pMultisampleState = &shadowMultisampling;
    shadowInfo.pDepthStencilState = &shadowDepth;
    shadowInfo.pColorBlendState = &shadowBlend;
    shadowInfo.pDynamicState = &dynamicState;
    shadowInfo.layout = ShadowPipelineLayout;
    shadowInfo.renderPass = ShadowRenderPass;
    shadowInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &shadowInfo, nullptr, &ShadowPipeline) != VK_SUCCESS)
    {
        std::fprintf(stderr, "VulkanPipeline::Create: vkCreateGraphicsPipelines (shadow) failed\n");
        vkDestroyShaderModule(Device, shadowVertModule, nullptr);
        vkDestroyPipeline(Device, SkyPipeline, nullptr);
        vkDestroyPipeline(Device, WorldPipeline, nullptr);
        SkyPipeline = VK_NULL_HANDLE;
        WorldPipeline = VK_NULL_HANDLE;
        vkDestroyPipelineLayout(Device, ShadowPipelineLayout, nullptr);
        ShadowPipelineLayout = VK_NULL_HANDLE;
        vkDestroyPipelineLayout(Device, PipelineLayout, nullptr);
        PipelineLayout = VK_NULL_HANDLE;
        vkDestroyShaderModule(Device, fragShaderModule, nullptr);
        vkDestroyShaderModule(Device, vertShaderModule, nullptr);
        return false;
    }

    vkDestroyShaderModule(Device, shadowVertModule, nullptr);

    vkDestroyShaderModule(Device, fragShaderModule, nullptr);
    vkDestroyShaderModule(Device, vertShaderModule, nullptr);
    return true;
}

void VulkanPipeline::Destroy(VkDevice Device)
{
    if (SkyPipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(Device, SkyPipeline, nullptr);
        SkyPipeline = VK_NULL_HANDLE;
    }
    if (WorldPipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(Device, WorldPipeline, nullptr);
        WorldPipeline = VK_NULL_HANDLE;
    }
    if (ShadowPipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(Device, ShadowPipeline, nullptr);
        ShadowPipeline = VK_NULL_HANDLE;
    }
    if (PipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(Device, PipelineLayout, nullptr);
        PipelineLayout = VK_NULL_HANDLE;
    }
    if (ShadowPipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(Device, ShadowPipelineLayout, nullptr);
        ShadowPipelineLayout = VK_NULL_HANDLE;
    }
}

VkPipeline VulkanPipeline::GetSkyHandle() const
{
    return SkyPipeline;
}

VkPipeline VulkanPipeline::GetWorldHandle() const
{
    return WorldPipeline;
}

VkPipelineLayout VulkanPipeline::GetLayout() const
{
    return PipelineLayout;
}

VkPipeline VulkanPipeline::GetShadowHandle() const
{
    return ShadowPipeline;
}

VkPipelineLayout VulkanPipeline::GetShadowLayout() const
{
    return ShadowPipelineLayout;
}
