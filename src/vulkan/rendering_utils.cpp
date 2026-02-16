#include <chrono>
#include <thread>

#include "vulkan/rendering_utils.hpp"

std::shared_ptr<SwapChain> SwapChainFactory::createSwapchain(std::shared_ptr<vk::raii::PhysicalDevice> physical_device,
                                                             std::shared_ptr<LogicalDevice>            logical_device,
                                                             std::shared_ptr<vk::raii::SurfaceKHR>     surface,
                                                             std::shared_ptr<GLFWwindow>               window,
                                                             std::shared_ptr<SwapChain>                old_swapchain) {

    // Handle minimized window
    int width = 0, height = 0;
    glfwGetFramebufferSize(window.get(), &width, &height);
    auto  call_time = std::chrono::steady_clock::now();
    while(width == 0 || height == 0) {
        call_time = std::chrono::steady_clock::now();
        glfwGetFramebufferSize(window.get(), &width, &height);
        glfwWaitEvents();
        // wait so that we're actually idle
        std::this_thread::sleep_until(call_time + std::chrono::duration<int, std::milli>(200));
    }

    logical_device->device->waitIdle();

    std::shared_ptr<SwapChain> swapchain = std::make_shared<SwapChain>();

    // get surface capabilites of swapchain
    std::vector<vk::SurfaceFormatKHR> surface_formats = physical_device->getSurfaceFormatsKHR(*surface);
    vk::SurfaceFormatKHR              surface_format  = chooseSwapSurfaceFormat(surface_formats);
    swapchain->format                                 = surface_format.format;

    vk::SurfaceCapabilitiesKHR surface_capabilites        = physical_device->getSurfaceCapabilitiesKHR(*surface);
    swapchain->extent                                     = chooseSwapExtent(surface_capabilites, window);
    std::vector<vk::PresentModeKHR> surface_present_modes = physical_device->getSurfacePresentModesKHR(*surface);
    vk::PresentModeKHR              present_mode          = chooseSwapPresentMode(surface_present_modes);

    vk::SwapchainCreateInfoKHR swapchain_create_info{
        .flags            = vk::SwapchainCreateFlagsKHR(),
        .surface          = *surface,
        .minImageCount    = chooseMinImageCount(surface_capabilites),
        .imageFormat      = swapchain->format,
        .imageColorSpace  = surface_format.colorSpace,
        .imageExtent      = swapchain->extent,
        .imageArrayLayers = 1,  // more than 1 for stereoscopic 3D
        .imageUsage       = vk::ImageUsageFlagBits::eColorAttachment,  // operations we will use image for; color attachment means directly rendering to them; can render to image for post processing and use VK_IMAGE_USAGE_TRANSFER_DST_BIT then do memory transfer to a swapchain
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform     = surface_capabilites.currentTransform,  // e.g. 90 deg clockwise or mirroring
        .compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque,  // whether to use alpha for blending with other system windows
        .presentMode      = present_mode,
        .clipped          = vk::True,  // don't care about color of pixels that are obscured by other windows (best performance)
    };

    // if a swapchain has been specified for replacement
    if (old_swapchain) {
        // use the old swapchain to make the new swapchain
        swapchain_create_info.oldSwapchain = *(old_swapchain->swapchain);
    }

    // handling images across multiple queue families
    uint32_t queue_family_indexes[] = {logical_device->queue_indexes.at(QueueType::GRAPHICS), logical_device->queue_indexes.at(QueueType::PRESENTATION)};
    if (logical_device->queue_indexes.at(QueueType::GRAPHICS) != logical_device->queue_indexes.at(QueueType::PRESENTATION)) {
        swapchain_create_info.imageSharingMode      = vk::SharingMode::eConcurrent;// images can be used across multiple queues without explicit transfer of ownership
        swapchain_create_info.queueFamilyIndexCount = 2;
        swapchain_create_info.pQueueFamilyIndices   = queue_family_indexes; // which queue families will be able to share the images
    } else {
        swapchain_create_info.imageSharingMode      = vk::SharingMode::eExclusive; // explicit image ownership transfer between queues (best performance); do this if present and graphics the same queue
        swapchain_create_info.queueFamilyIndexCount = 0;  // optional
        swapchain_create_info.pQueueFamilyIndices   = nullptr;
    }

    // create the swapchain, swapchain images and associated views
    swapchain->swapchain = std::make_shared<vk::raii::SwapchainKHR>(*logical_device->device, swapchain_create_info);
    swapchain->images    = std::make_shared<std::vector<vk::Image>>(swapchain->swapchain->getImages());
    createImageViews(logical_device, swapchain);

    return swapchain;
}

void SwapChainFactory::createImageViews(std::shared_ptr<LogicalDevice> logical_device,
                                        std::shared_ptr<SwapChain>     swapchain) {

    // setup image view vector
    swapchain->image_views = std::make_shared<std::vector<vk::raii::ImageView>>();

    vk::ImageViewCreateInfo image_view_create_info{ 
        .viewType = vk::ImageViewType::e2D, 
        .format = swapchain->format, 
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        } 
    };

    // for stereographic 3D, create a swapchain with multiple layers, one layer is for each eye
    // VR typically requires a maximum of 4 images; GPUs can typically handle up to 16 image views

    // create an image view for each swap chain image
    for (auto image : *swapchain->images) {
        image_view_create_info.image = image;
        swapchain->image_views->emplace_back(*(logical_device->device), image_view_create_info);
    }
}

uint32_t SwapChainFactory::chooseMinImageCount(const vk::SurfaceCapabilitiesKHR& surface_capabilites) {
    uint32_t min_image_count = std::max(3u, surface_capabilites.minImageCount);
    min_image_count          = (surface_capabilites.maxImageCount > 0 && min_image_count > surface_capabilites.maxImageCount) ? surface_capabilites.maxImageCount : min_image_count;
    min_image_count += 1;  // if if we have the absolute minimum have to wait for driver to complete internal operations before acquiring the next image to render to
    // zero indicates no max image count
    if (surface_capabilites.maxImageCount > 0 && min_image_count > surface_capabilites.maxImageCount) {
        min_image_count = surface_capabilites.maxImageCount;
    }

    return min_image_count;
}

vk::SurfaceFormatKHR SwapChainFactory::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats) {
    // SurfaceFormatKHR contains a format and color, e.g. VK_FORMAT_B8G8R8A8_SRGB is 8 bit RGBA using SRGB format

    for (const auto& format : formats) {
        if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return format;
        }
    }

    return formats.front();
}

vk::PresentModeKHR SwapChainFactory::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& modes) {
    // present mode specifieds condition for showing image to screen; 4 modes available:
    // 1) VK_PRESENT_MODE_IMMEDIATE_KHR - images immediately shown on screen (causes tearing)
    // 2) VK_PRESENT_MODE_FIFO_KHR - swapchain is FIFO queue, display takes image from front of queue when refreshed ( the moment of refresh is known as "vertical blank");
    //                             - program inserts images at the back of the queue
    //                             - if queue is full program waits, similar to VSYNC
    //                             - guaranteed to be available
    // 3) VK_PRESENT_MODE_FIFO_RELAXED_KHR - as above but if queue is empty new image gets transferred right away (causes tearing)
    // 4) VK_PRESENT_MODE_MAILBOX_KHR - as 2) but if queue is full last image is replaced; "triple buffering"

    for (const auto& mode : modes) {
        if (mode == vk::PresentModeKHR::eMailbox) {
            return mode;
        }
    }

    return vk::PresentModeKHR::eFifo;
}   

vk::Extent2D SwapChainFactory::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, std::shared_ptr<GLFWwindow> window) {
    // for some window managers let us have different res to the window, set height and width to uint32_t max but have to specify units correctly
    // screen coordinates don't always correspond to pixels (typically for high res displays)

    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    int width, height;
    glfwGetFramebufferSize(window.get(), &width, &height);
    width  = std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    height = std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
}