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
#include "vulkan/pipeline_utils.hpp"

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
     * 
     * @param window_name the name of the window
     */
    void initWindow(const std::string& window_name);

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
     * @brief create a graphics pipeline for rendering
     */
    void createGraphicsPipeline();

    // should probably be in some kind of command buffer utils files/class; make pipline utils rendering utils, move this and swapchain stuff there

    /**
     * @brief create command pool for use with command buffer
     */
    void createCommandPool();

    /**
     * @brief create a command buffer
     */
    void createCommandBuffer();

    /**
     * @brief record a command buffer
     * 
     * @param image_index index of current swap chain image to write to
     */
    void recordCommandBuffer(uint32_t image_index);

    /**
     * @brief render the current frame
     */
    void drawFrame();

    /**
     * @brief create synchronization objects
     */
    void createSyncObjects();

    /**
     * @brief transition the specified swapchain image layout for different operations
     * 
     * @param image_index index of the swapchain image to be transitioned
     * @param old_layout old image layout
     * @param new_layout new image layout
     * @param src_access_mask source access mask
     * @param dst_access_mask destination access mask
     * @param src_stage_mask source stage mask
     * @param dst_stage_mask destination stage mask
     */
    void transitionImageLayout(
      uint32_t image_index,
      vk::ImageLayout old_layout,
      vk::ImageLayout new_layout,
      vk::AccessFlags2 src_access_mask,
      vk::AccessFlags2 dst_access_mask,
      vk::PipelineStageFlags2 src_stage_mask,
      vk::PipelineStageFlags2 dst_stage_mask
    );

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

    // rendering
    std::unique_ptr<GraphicsPipelineFactory> m_graphics_pipeline_factory;  ///< factory for creating graphics pipelines
    std::unique_ptr<vk::raii::Pipeline>      m_graphics_pipeline;  ///< the graphics pipeline

    // device
    std::unique_ptr<PhysicalDeviceSelector>   m_device_selector;  ///< supporting class for selecting physical device
    std::shared_ptr<vk::raii::PhysicalDevice> m_physical_device;  ///< physical device
    std::unique_ptr<LogicalDeviceFactory>     m_logical_device_factory;  ///< factory for creating logical devices
    std::shared_ptr<LogicalDevice>            m_logical_device;  ///< logical device

    // queues
    std::unique_ptr<vk::raii::Queue> m_graphics_queue; ///< queue for graphics processing
    std::unique_ptr<vk::raii::Queue> m_presentation_queue; ///< queue for presenting (likely same as graphics queue)

    // commands
    std::unique_ptr<vk::raii::CommandPool>   m_command_pool;  ///< manages memory used to store command buffers
    std::unique_ptr<vk::raii::CommandBuffer> m_command_buffer;  ///< buffer of commands to be submitted to a GPU

    // synchronization
    std::unique_ptr<vk::raii::Semaphore> m_present_complete_semaphore;  ///< sempaphore for scheduling presentation
    std::unique_ptr<vk::raii::Semaphore> m_rendering_complete_semaphore;  ///< sempaphore for scheduling rendering
    std::unique_ptr<vk::raii::Fence>     m_draw_fence;  ///< fence to protect frame drawing
};