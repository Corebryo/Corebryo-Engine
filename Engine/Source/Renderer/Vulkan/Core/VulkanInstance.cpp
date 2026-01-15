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

#include "VulkanInstance.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdio>
#include <cstring>
#include <vector>

/* Validation layer debug callback. */
static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT MessageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* CallbackData,
    void* UserData)
{
    /* Suppress unused parameters - messages are ignored. */
    (void)MessageSeverity;
    (void)MessageTypes;
    (void)CallbackData;
    (void)UserData;

    return VK_FALSE;
}

/* Load debug messenger creation function. */
static VkResult CreateDebugUtilsMessengerEXT(
    VkInstance Instance,
    const VkDebugUtilsMessengerCreateInfoEXT* CreateInfo,
    const VkAllocationCallbacks* Allocator,
    VkDebugUtilsMessengerEXT* Messenger)
{
    /* Dynamically load extension function. */
    auto Func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        Instance, "vkCreateDebugUtilsMessengerEXT");

    if (Func)
    {
        return Func(Instance, CreateInfo, Allocator, Messenger);
    }

    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

/* Load debug messenger destruction function. */
static void DestroyDebugUtilsMessengerEXT(
    VkInstance Instance,
    VkDebugUtilsMessengerEXT Messenger,
    const VkAllocationCallbacks* Allocator)
{
    /* Dynamically load extension function. */
    auto Func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        Instance, "vkDestroyDebugUtilsMessengerEXT");

    if (Func)
    {
        Func(Instance, Messenger, Allocator);
    }
}

VulkanInstance::VulkanInstance()
    : Instance(VK_NULL_HANDLE)
    , DebugMessenger(VK_NULL_HANDLE)
    , ValidationEnabled(false)
{
#if defined(_DEBUG)
    ValidationEnabled = true;
#else
    ValidationEnabled = false;
#endif
}

VulkanInstance::~VulkanInstance()
{
    /* Destroy instance resources. */
    Destroy();
}

bool VulkanInstance::Create(const char* AppName, GLFWwindow* WindowHandle)
{
    /* Create Vulkan instance. */
    if (!CreateInstance(AppName, WindowHandle))
    {
        std::fprintf(stderr, "VulkanInstance::Create: CreateInstance failed\n");
        return false;
    }

    /* Setup debug messenger when validation enabled. */
    if (IsValidationEnabled())
    {
        if (!SetupDebugMessenger())
        {
            std::fprintf(stderr, "VulkanInstance::Create: SetupDebugMessenger failed\n");
            return false;
        }
    }

    return true;
}

void VulkanInstance::Destroy()
{
    /* Destroy debug messenger first. */
    if (DebugMessenger != VK_NULL_HANDLE)
    {
        DestroyDebugUtilsMessengerEXT(Instance, DebugMessenger, nullptr);
        DebugMessenger = VK_NULL_HANDLE;
    }

    /* Destroy Vulkan instance. */
    if (Instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(Instance, nullptr);
        Instance = VK_NULL_HANDLE;
    }
}

VkInstance VulkanInstance::GetHandle() const
{
    /* Return instance handle. */
    return Instance;
}

bool VulkanInstance::IsValidationEnabled() const
{
    /* Return validation state. */
    return ValidationEnabled;
}

bool VulkanInstance::CheckValidationLayerSupport() const
{
    /* Check for Khronos validation layer. */
    const char* RequestedLayer = "VK_LAYER_KHRONOS_validation";

    uint32_t LayerCount = 0;
    vkEnumerateInstanceLayerProperties(&LayerCount, nullptr);

    std::vector<VkLayerProperties> AvailableLayers(LayerCount);
    vkEnumerateInstanceLayerProperties(&LayerCount, AvailableLayers.data());

    /* Search for requested layer. */
    for (const auto& Layer : AvailableLayers)
    {
        if (std::strcmp(Layer.layerName, RequestedLayer) == 0)
        {
            return true;
        }
    }

    std::fprintf(stderr, "VulkanInstance::CheckValidationLayerSupport: VK_LAYER_KHRONOS_validation not found\n");
    return false;
}

std::vector<const char*> VulkanInstance::GetRequiredExtensions(GLFWwindow* WindowHandle) const
{
    /* Get GLFW-required extensions for windowing. */
    (void)WindowHandle;

    uint32_t Count = 0;
    const char** GlfwExtensions = glfwGetRequiredInstanceExtensions(&Count);

    std::vector<const char*> Extensions;
    Extensions.reserve(Count + 2);

    /* Add GLFW extensions. */
    for (uint32_t i = 0; i < Count; ++i)
    {
        Extensions.push_back(GlfwExtensions[i]);
    }

    /* Add debug utils extension when validation enabled. */
    if (IsValidationEnabled())
    {
        Extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return Extensions;
}

bool VulkanInstance::CreateInstance(const char* AppName, GLFWwindow* WindowHandle)
{
    /* Disable validation if layer unavailable. */
    if (IsValidationEnabled() && !CheckValidationLayerSupport())
    {
        std::fprintf(stderr, "VulkanInstance::CreateInstance: validation layer missing, disabling validation\n");
        ValidationEnabled = false;
    }

    /* Configure application info. */
    VkApplicationInfo AppInfo{};
    AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    AppInfo.pApplicationName = AppName;
    AppInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    AppInfo.pEngineName = "Engine";
    AppInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    AppInfo.apiVersion = VK_API_VERSION_1_4;

    /* Get required extensions. */
    auto Extensions = GetRequiredExtensions(WindowHandle);

    /* Configure instance creation. */
    VkInstanceCreateInfo CreateInfo{};
    CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    CreateInfo.pApplicationInfo = &AppInfo;
    CreateInfo.enabledExtensionCount = static_cast<uint32_t>(Extensions.size());
    CreateInfo.ppEnabledExtensionNames = Extensions.data();

    const char* ValidationLayer = "VK_LAYER_KHRONOS_validation";

    /* Enable validation layer if requested. */
    if (IsValidationEnabled())
    {
        CreateInfo.enabledLayerCount = 1;
        CreateInfo.ppEnabledLayerNames = &ValidationLayer;
    }
    else
    {
        CreateInfo.enabledLayerCount = 0;
        CreateInfo.ppEnabledLayerNames = nullptr;
    }

    /* Create Vulkan instance. */
    if (vkCreateInstance(&CreateInfo, nullptr, &Instance) != VK_SUCCESS)
    {
        std::fprintf(stderr, "VulkanInstance::CreateInstance: vkCreateInstance failed\n");
        return false;
    }

    return true;
}

bool VulkanInstance::SetupDebugMessenger()
{
    /* Configure debug messenger. */
    VkDebugUtilsMessengerCreateInfoEXT CreateInfo{};
    CreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    CreateInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    CreateInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    CreateInfo.pfnUserCallback = DebugCallback;

    /* Create debug messenger. */
    if (CreateDebugUtilsMessengerEXT(Instance, &CreateInfo, nullptr, &DebugMessenger) != VK_SUCCESS)
    {
        std::fprintf(stderr, "VulkanInstance::SetupDebugMessenger: vkCreateDebugUtilsMessengerEXT failed\n");
        return false;
    }

    return true;
}
