#pragma once

// Vulkan
//#ifdef __INTELLISENSE__
#include <vulkan/vulkan_raii.hpp>
//#else
//import vulkan_hpp; // using the vulkan module gives super weird redefinition errors for thing sfor the stl; probably have to use import std; or wrap the includes/this stuff in a module
//#endif

// GLFW
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <unordered_set>
#include <unordered_map>
#include <optional>
#include <vector>
#include <memory>
#include <cstdint>

// This is required for the version header apparently
//#include <vulkan/vulkan_core.h>

enum class QueueType : uint8_t {
  GRAPHICS = 0,
  PRESENTATION,
  NUM_ENTRIES
};

struct DestroyGLFWWindow {
  void operator()(GLFWwindow* ptr) {
    if (ptr) {
      glfwDestroyWindow(ptr);
    }
  }
};

/**
 * @brief selects physical devices for rendering
 */
class PhysicalDeviceSelector {

  public:

    /**
     * @brief construct physical device selector with specified requirements
     */
    PhysicalDeviceSelector(const std::vector<const char*>& required_extensions);

    /**
     * @brief get a score for the specified physical device
     * 
     * @param device the device to analyze
     * 
     * @return the integer score of the device based on the configured criteria; if the device is unsuitable; return nullopt
     */
    std::optional<uint32_t> scoreDevice(const vk::raii::PhysicalDevice device);

  private:

    class SelectionCriteria {
        public:
        /**
         * @brief virtual destructor to make the interface work
         */
        virtual ~SelectionCriteria() {}; 
        /**
         * @brief method to get weight based on criteria
         * 
         * @param device the device to weight
         */
        virtual std::optional<uint32_t> getScore(const vk::raii::PhysicalDevice device) = 0;
    };

    class QueueCriteria : public SelectionCriteria {
        /**
         * @brief method to get weight based on criteria
         * 
         * @param device the device to weight
         */
        virtual std::optional<uint32_t> getScore(const vk::raii::PhysicalDevice device);
    };

    class ExtensionsCriteria : public SelectionCriteria {
      public:
        /**
         * @brief constructor for extension criteria
         *
         * @param required_extensions vector of required extensions
         */
        ExtensionsCriteria(const std::vector<const char*>& required_extensions) {
            m_required_extensions = required_extensions;
        }

        /**
         * @brief method to get weight based on criteria
         *
         * @param device the device to weight
         */
        virtual std::optional<uint32_t> getScore(const vk::raii::PhysicalDevice device);

      private:
        std::vector<const char*> m_required_extensions;  // required extensions for physical devices
    };

    class PropertiesCriteria : public SelectionCriteria {
      public:
        /**
         * @brief constructor for properties criteria
         */
        PropertiesCriteria();

        /**
         * @brief method to get weight based on criteria
         *
         * @param device the device to weight
         */
        virtual std::optional<uint32_t> getScore(const vk::raii::PhysicalDevice device);

      private:
        std::unordered_set<vk::PhysicalDeviceType> m_permitted_devices;  ///< permitted types of physical device
    };

    std::vector<std::unique_ptr<SelectionCriteria>> m_selection_criteria; ///< criteria for selecting a physical device
};

struct LogicalDevice {
    std::shared_ptr<vk::raii::Device>       device = nullptr;  ///< the logical device
    std::unordered_map<QueueType, uint32_t> queue_indexes;  ///< indexes for the requested queues
};

/**
 * @brief creates a logical device with the specified settings
 */
class LogicalDeviceFactory {
  public:

    /**
     * @brief constructor for logical device factory
     * 
     * @param required_extensions extensions required
     */
    LogicalDeviceFactory(const std::vector<const char*>& required_extensions);

    /**
     * @brief create a logical device for the specified physical device and surface
     *
     * @param required_queues physical device queues required
     * @param device the physical device to use
     * @param surface the surface to be rendered to (if required)
     *
     * @return struct containing the logical device and the indexes of the queues specified
     */
    std::shared_ptr<LogicalDevice> createLogicalDevice(std::unordered_set<QueueType>             required_queues,
                                                       std::shared_ptr<vk::raii::PhysicalDevice> device,
                                                       std::shared_ptr<vk::raii::SurfaceKHR>     surface = nullptr);

  private:
    /**
     * @brief get the requested queue indexes that were found for the specified physical device
     *
     * @param physical_device the physical device to use
     * @param surface the surface to be rendered to (if required)
     *
     * @return unordered map containing the queue type and associated index for the device
     */
    std::unordered_map<QueueType, std::uint32_t> getQueueIndexes(std::shared_ptr<vk::raii::PhysicalDevice> physical_device,
                                                                 std::shared_ptr<vk::raii::SurfaceKHR>     surface);
    std::vector<const char*> m_required_device_extensions;  ///< required device extensions
};

/**
 * @brief struct to hold swaphchain and associated data
 */
struct SwapChain {
    std::shared_ptr<vk::raii::SwapchainKHR> swapchain;  ///< the swapchain
    std::shared_ptr<std::vector<vk::Image>> swapchain_images;  ///< images in the swapchain
    vk::SurfaceFormatKHR                    surface_format;  ///< format of surface associated with swapchain
    vk::Extent2D                            surface_extent;  ///< extent of swapchain surface
};

/**
 * @brief class for creating swapchains
 */
class SwapChainFactory {
  public:
    /**
     * @brief swapchain factory constructor
     */
    SwapChainFactory(){};

    /**
     * @brief create a swapchain with the specified settings
     *
     * @param physical_device the physical device to use
     * @param logical_device the logical device to use
     * @param surface the surface to render to
     * @param window the window to be rendered to
     *
     * @return the swapchain and associated data
     */
    std::shared_ptr<SwapChain> createSwapchain(std::shared_ptr<vk::raii::PhysicalDevice> physical_device,
                                               std::shared_ptr<LogicalDevice>            logical_device,
                                               std::shared_ptr<vk::raii::SurfaceKHR>     surface,
                                               std::shared_ptr<GLFWwindow>               window);
  private:
    /**
     * @brief choose the minimum image count for the swapchain
     * 
     * @param surface_capabilites the capabilites for the surface to be rendered to
     * 
     * @return the minimum number of images in the swapchain
     */
    uint32_t chooseMinImageCount(const vk::SurfaceCapabilitiesKHR& surface_capabilites);

    /**
     * @brief select a swapchain surface format from the provided list
     *
     * @param formats a list of available swapchain surface formats
     *
     * @return the selected swapchain surface format
     */
    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats);

    /**
     * @brief select a swapchain presentation mode from the provided list
     *
     * @param modes list of available swapchain presentation modes
     *
     * @return the selected presentation mode
     */
    vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& modes);

    /**
     * @brief select a swapchain extent
     *
     * @param capabilities struct of surface capabilities
     * @param window the window the swapchain will render to
     *
     * @return exten of surface
     */
    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, std::shared_ptr<GLFWwindow> window);
};