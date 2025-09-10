#pragma once

// Standard library
#include <stdexcept>
#include <cstdlib>
#include <algorithm>
#include <memory>
#include <iostream>
#include <vector>

// Vulkan
#ifdef __INTELLISENSE__
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif
#include <vulkan/vk_platform.h>

// GLFW
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// Validation layers
#ifdef NDEBUG
constexpr bool VALIDATION_LAYERS = true;
#else
constexpr bool VALIDATION_LAYERS = false;
#endif

class ModernRenderTriangle {
  public:
    /**
     * @brief run the render triangle applciation
     */
    void run();

  private:
    /**
     * @brief initialize vulkan and its dependencies
     */
    void initVulkan();

    /**
     * @brief initialize the window to render to
     */
    void initWindow();

    /**
     * @brief create vulkan instance
     */
    void createInstance();

    /**
     * @brief get the required layers
     * 
     * @return vector of required layers
     */
    std::vector<char const*> getRequiredLayers();

    /**
     * @brief get list of required ecxtensions based on whether validation layers are enabled
     * 
     * @return vector of required extensions
     */
    std::vector<char const*> getRequiredExtensions();

    /**
     * @brief debug callback
     */
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void*) {
        std::cerr << "validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;

        return vk::False;
    }

    /**
     * @brief setup the debug messenger
     */
    void setupDebugMessenger();

    /**
     * @brief main run loop
     */
    void mainLoop();

    /**
     * @brief cleansup vulkan and its dependencies
     */
    void cleanup();

    vk::raii::Context                m_context;  ///< vulkan context
    vk::raii::Instance               m_instance        = nullptr;  ///< vulkan instance
    vk::raii::DebugUtilsMessengerEXT m_debug_messenger = nullptr;  ///< can have multiple of these

    const std::pair<uint32_t, uint32_t> m_window_size = { 800, 800 };
    GLFWwindow*                         m_window;  // GLFW window to render to

    const std::vector<const char*> m_validation_layers = { "VK_LAYER_KHRONOS_validation" };
};