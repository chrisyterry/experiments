#pragma once
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/device_utils.hpp>

/**
 * @brief struct to hold swaphchain and associated data
 */
struct SwapChain {

    /**
     * @brief constructor for swapchain
     */
    SwapChain() {
        swapchain   = nullptr;
        images      = nullptr;
        image_views = nullptr;
        format      = vk::Format::eUndefined;
    }

    bool outdated = false; ///< whether the swapchain is out date and needs removing

    std::shared_ptr<vk::raii::SwapchainKHR>           swapchain;  ///< the swapchain
    std::shared_ptr<std::vector<vk::Image>>           images;  ///< images in the swapchain
    std::shared_ptr<std::vector<vk::raii::ImageView>> image_views;  ///< image views for the images in the swapchain
    vk::Format                                        format;  ///< format of swapchain images
    vk::Extent2D                                      extent;  ///< extent of swapchain surface
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
     * @param old_swapchain [optional] old swapchain to use for swapchain recreation
     *        (note) if you use this, you should destroy the old swapchain as it is not in use
     *
     * @return the swapchain and associated data
     */
    std::shared_ptr<SwapChain> createSwapchain(std::shared_ptr<vk::raii::PhysicalDevice> physical_device,
                                               std::shared_ptr<LogicalDevice>            logical_device,
                                               std::shared_ptr<vk::raii::SurfaceKHR>     surface,
                                               std::shared_ptr<GLFWwindow>               window,
                                               std::shared_ptr<SwapChain>                swapchain = nullptr
                                              );
  private:
    /**
     * @brief create image views for the images in the swapchain
     *
     * @param logical_device logical device use for image view creation
     * @param swapchain pointer to the swapchain to update with image views
     */
    void createImageViews(std::shared_ptr<LogicalDevice> logical_device,
                          std::shared_ptr<SwapChain>     swapchain);

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