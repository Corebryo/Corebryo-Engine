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

#include "SkyboxPipeline.h"

#include <cstdio>
#include <fstream>
#include <vector>

namespace
{
    /* Read binary shader file. */
    bool ReadFile(const char* path, std::vector<char>& outData)
    {
        std::ifstream file(path, std::ios::ate | std::ios::binary);
        if (!file.is_open())
        {
            std::fprintf(stderr, "SkyboxPipeline::Create: Failed to open %s\n", path);
            return false;
        }

        const std::streamsize fileSize = file.tellg();
        if (fileSize <= 0)
        {
            std::fprintf(stderr, "SkyboxPipeline::Create: Empty shader %s\n", path);
            return false;
        }

        outData.resize(static_cast<size_t>(fileSize));
        file.seekg(0, std::ios::beg);
        if (!file.read(outData.data(), fileSize))
        {
            std::fprintf(stderr, "SkyboxPipeline::Create: Failed to read %s\n", path);
            return false;
        }

        return true;
    }

    /* Create a shader module from SPIR-V bytecode. */
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

SkyboxPipeline::SkyboxPipeline() = default;

SkyboxPipeline::~SkyboxPipeline() = default;

bool SkyboxPipeline::Create(
    VkDevice Device,
    VkRenderPass RenderPass,
    VkDescriptorSetLayout DescriptorSetLayout,
    const char* VertexShaderPath,
    const char* FragmentShaderPath)
{
    Destroy(Device);

    std::vector<char> vertShaderCode;
    std::vector<char> fragShaderCode;
    if (!ReadFile(VertexShaderPath, vertShaderCode) ||
        !ReadFile(FragmentShaderPath, fragShaderCode))
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
        std::fprintf(stderr, "SkyboxPipeline::Create: Failed to create shader modules\n");
        return false;
    }

    VkPipelineShaderStageCreateInfo shaderStages[2]{};
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertShaderModule;
    shaderStages[0].pName = "main";

    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragShaderModule;
    shaderStages[1].pName = "main";

    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(float) * 3;
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attributeDescription{};
    attributeDescription.location = 0;
    attributeDescription.binding = 0;
    attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescription.offset = 0;

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = 1;
    vertexInputInfo.pVertexAttributeDescriptions = &attributeDescription;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_4_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

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

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(float) * 32;

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &DescriptorSetLayout;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(Device, &layoutInfo, nullptr, &PipelineLayout) != VK_SUCCESS)
    {
        std::fprintf(stderr, "SkyboxPipeline::Create: vkCreatePipelineLayout failed\n");
        vkDestroyShaderModule(Device, fragShaderModule, nullptr);
        vkDestroyShaderModule(Device, vertShaderModule, nullptr);
        return false;
    }

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
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = PipelineLayout;
    pipelineInfo.renderPass = RenderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &PipelineHandle) != VK_SUCCESS)
    {
        std::fprintf(stderr, "SkyboxPipeline::Create: vkCreateGraphicsPipelines failed\n");
        vkDestroyPipelineLayout(Device, PipelineLayout, nullptr);
        PipelineLayout = VK_NULL_HANDLE;
        vkDestroyShaderModule(Device, fragShaderModule, nullptr);
        vkDestroyShaderModule(Device, vertShaderModule, nullptr);
        return false;
    }

    vkDestroyShaderModule(Device, fragShaderModule, nullptr);
    vkDestroyShaderModule(Device, vertShaderModule, nullptr);
    return true;
}

void SkyboxPipeline::Destroy(VkDevice Device)
{
    if (PipelineHandle != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(Device, PipelineHandle, nullptr);
        PipelineHandle = VK_NULL_HANDLE;
    }

    if (PipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(Device, PipelineLayout, nullptr);
        PipelineLayout = VK_NULL_HANDLE;
    }
}

VkPipeline SkyboxPipeline::GetPipeline() const
{
    return PipelineHandle;
}

VkPipelineLayout SkyboxPipeline::GetLayout() const
{
    return PipelineLayout;
}
