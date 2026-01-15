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
#include <vector>

struct GLFWwindow;

/* Owns VkInstance lifecycle and validation setup. */

class VulkanInstance
{
public:
    /* Initialize instance state. */
    VulkanInstance();

    /* Release instance resources. */
    ~VulkanInstance();

    /* Create VkInstance and optional debug messenger. */
    bool Create(const char* AppName, GLFWwindow* WindowHandle);

    /* Destroy debug messenger and instance. */
    void Destroy();

    /* Get instance handle. */
    VkInstance GetHandle() const;

private:
    /* Create Vulkan instance with required extensions. */
    bool CreateInstance(const char* AppName, GLFWwindow* WindowHandle);

    /* Setup debug messenger in debug builds. */
    bool SetupDebugMessenger();

    /* Check if validation is enabled. */
    bool IsValidationEnabled() const;

    /* Verify validation layer availability. */
    bool CheckValidationLayerSupport() const;

    /* Get required instance extensions. */
    std::vector<const char*> GetRequiredExtensions(GLFWwindow* WindowHandle) const;

private:
    /* Vulkan instance handle. */
    VkInstance Instance;

    /* Debug messenger handle. */
    VkDebugUtilsMessengerEXT DebugMessenger;

    /* Validation layer state. */
    bool ValidationEnabled;
};
