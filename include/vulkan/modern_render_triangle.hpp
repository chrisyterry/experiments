#pragma once

// Standard library
#include <stdexcept>
#include <algorithm>
#include <memory>
#include <iostream>
#include <vector>

// Vulkan
//#ifdef __INTELLISENSE__
#include <vulkan/vulkan_raii.hpp>
//#else
//import vulkan_hpp;
//#endif

// GLFW
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// Validation layers
#ifdef NDEBUG
constexpr bool VALIDATION_LAYERS = true;
#else
constexpr bool VALIDATION_LAYERS = false;
#endif

// homebrew utilities
#include "vulkan/device_utils.hpp"

class ModernRenderTriangle {
  public:

    /**
     * @brief construct an instance of the renderer
     */
    ModernRenderTriangle();

    /**
     * @brief destructor for renderer
     */
    ~ModernRenderTriangle();

    /**
     * @brief run the render triangle application
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
     * @brief get list of required extensions based on whether validation layers are enabled
     * 
     * @return vector of required extensions
     */
    std::vector<char const*> getRequiredExtensions();

    /**
     * @brief create surface to render to
     */
    void createSurface();

    /**
     * @brief select a physical device/GPU to use
     */
    void pickPhysicalDevice();

    /**
     * @brief create a logical device for the selected physical device
     */
    void createLogicalDevice();

    /**
     * @brief create a swapchain and supporting structures for showing images to the screen
     */
    void createSwapchain();

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
     * @brief cleanup vulkan and its dependencies
     */
    void cleanup();

    // instance
    vk::raii::Context                m_context;  ///< vulkan context
    vk::raii::Instance               m_instance          = nullptr;  ///< vulkan instance
    vk::raii::DebugUtilsMessengerEXT m_debug_messenger   = nullptr;  ///< can have multiple of these
    const std::vector<const char*>   m_validation_layers = { "VK_LAYER_KHRONOS_validation" };

    // windowing
    const std::pair<uint32_t, uint32_t>   m_window_size = { 800, 800 };
    std::shared_ptr<GLFWwindow>           m_window;  // GLFW window to render to
    std::shared_ptr<vk::raii::SurfaceKHR> m_surface;  ///< surface to render to

    // presentation
    std::unique_ptr<SwapChainFactory> m_swapchain_factory;  ///< factory for creating swapchains
    std::shared_ptr<SwapChain>        m_swapchain;  ///< swapchain for image presentation
    std::vector<vk::ImageView>        m_image_views;  ///< image views to be rendered to

    // device
    std::unique_ptr<PhysicalDeviceSelector>   m_device_selector;  ///< supporting class for selecting physical device
    std::shared_ptr<vk::raii::PhysicalDevice> m_physical_device;  ///< physical device
    std::unique_ptr<LogicalDeviceFactory>     m_logical_device_factory;  ///< factory for creating logical devices
    std::shared_ptr<LogicalDevice>            m_logical_device;  ///< logical device

    // queues
    std::unique_ptr<vk::raii::Queue> m_graphics_queue; ///< queue for graphics processing
    std::unique_ptr<vk::raii::Queue> m_presentation_queue; ///< queue for presenting (likely same as graphics queue)
};