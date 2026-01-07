#include "vulkan/device_utils.hpp"
#include <algorithm>
#include <iostream>
#include <cassert>
#include <limits>

PhysicalDeviceSelector::PhysicalDeviceSelector(const std::vector<const char*>& required_extensions) {
    
    // add the selection criteria
    m_selection_criteria.emplace_back(std::make_unique<QueueCriteria>());
    m_selection_criteria.emplace_back(std::make_unique<ExtensionsCriteria>(required_extensions));
    m_selection_criteria.emplace_back(std::make_unique<PropertiesCriteria>());
}

std::optional<uint32_t> PhysicalDeviceSelector::scoreDevice(const vk::raii::PhysicalDevice device) {
    
    uint32_t score = 0;

    // require vulkan version 1.3 or above
    if (device.getProperties().apiVersion < VK_API_VERSION_1_3) {
        return std::nullopt;
    }

    for (const auto& criteria : m_selection_criteria) {
        std::optional<uint32_t> criteria_score = criteria->getScore(device);
        if (!criteria_score.has_value()) {
            return std::nullopt;
        } else {
            score += criteria_score.value();
        }

    }

    return score;
}

// Queue criteria

std::optional<uint32_t> PhysicalDeviceSelector::QueueCriteria::getScore(const vk::raii::PhysicalDevice device) {
    uint32_t score = 0;
    
    auto queue_families = device.getQueueFamilyProperties();
    // check that there is at least one queue that supports graphics
    const auto queue_fam_it = std::ranges::find_if(queue_families, [](vk::QueueFamilyProperties const & queue_fam_properties) {
        return (queue_fam_properties.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0);
    });

    if (queue_fam_it == queue_families.end()) {
        return std::nullopt;
    }

    return score;
}

// Extensions criteria

std::optional<uint32_t> PhysicalDeviceSelector::ExtensionsCriteria::getScore(const vk::raii::PhysicalDevice device) {
    uint32_t score = 0;

    bool extensions_found = true;
    auto extensions = device.enumerateDeviceExtensionProperties();
    // check that all the extensions are present
    for (const auto& extension : m_required_extensions) {
        auto extension_it = std::ranges::find_if(extensions, [extension](auto const & ext) {return strcmp(ext.extensionName, extension) == 0;});
        extensions_found = extensions_found && extension_it != extensions.end();
    }

    if (!extensions_found) {
        return std::nullopt;
    }

    return score;
}

// Properties criteria

PhysicalDeviceSelector::PropertiesCriteria::PropertiesCriteria() {
    m_permitted_devices = { vk::PhysicalDeviceType::eDiscreteGpu };
}

std::optional<uint32_t> PhysicalDeviceSelector::PropertiesCriteria::getScore(const vk::raii::PhysicalDevice device) {
    uint32_t score = 0;
    
    auto device_properties = device.getProperties();
    // if the device type is permitted
    if (m_permitted_devices.count(device_properties.deviceType) >= 1) {
        score += 1000;
    }

    return score;
}

LogicalDeviceFactory::LogicalDeviceFactory(const std::vector<const char*>& required_extensions) {
    // TODO - this might be better passed to the creation function if we might want different extensions for different devices
    m_required_device_extensions = required_extensions;
}

std::shared_ptr<LogicalDevice> LogicalDeviceFactory::createLogicalDevice(std::unordered_set<QueueType>             required_queues,
                                                                         std::shared_ptr<vk::raii::PhysicalDevice> physical_device,
                                                                         std::shared_ptr<vk::raii::SurfaceKHR>     surface) {

    std::shared_ptr<LogicalDevice> logical_device = std::make_shared<LogicalDevice>();

    // get the required queue indexes
    logical_device->queue_indexes = getQueueIndexes(physical_device, surface);
    for (const auto& queue : required_queues) {
        if (logical_device->queue_indexes.count(queue) < 1) {
            return logical_device;
        }
    }

    float queue_priority = 0.0;

    // can only create small number of queues for each family but can have multiple command buffers and submit them all to the same queue
    vk::DeviceQueueCreateInfo queue_create_info{
        .queueFamilyIndex = logical_device->queue_indexes.at(QueueType::GRAPHICS),
        .queueCount       = 1,
        .pQueuePriorities = &queue_priority  // can be a number between 0 and 1, influences command buffer execution scheduling
    };

    // to enable multiple features have to link structure elements together with pNext;
    // structure chain does this for you
    // Each template argument is a structure in the chain
    vk::StructureChain<vk::PhysicalDeviceFeatures2,
                       vk::PhysicalDeviceVulkan13Features,  // if we needed something added in other versions of vulkan, add the structs for those versions to the chains
                       vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
        feature_chain = {
            {},  // empty struct
            { .dynamicRendering = true },  // feature was added in vulkan 1.3 so we have to use the vulkan 1.3 struct to set it
            { .extendedDynamicState = true }
        };

    // can only create small number of queues for each family but can have multiple command buffers and submit them all to the same queue
    vk::DeviceCreateInfo device_create_info{
        .pNext                   = &feature_chain.get<vk::PhysicalDeviceFeatures2>(),  // should be same type as first element of structure chain
        .queueCreateInfoCount    = 1,
        .pQueueCreateInfos       = &queue_create_info,
        .enabledExtensionCount   = static_cast<uint32_t>(m_required_device_extensions.size()),
        .ppEnabledExtensionNames = m_required_device_extensions.data()
    };

    // create the logical device
    logical_device->device = std::make_shared<vk::raii::Device>(*physical_device, device_create_info);

    return std::move(logical_device);
}

std::unordered_map<QueueType, uint32_t> LogicalDeviceFactory::getQueueIndexes(std::shared_ptr<vk::raii::PhysicalDevice> physical_device,
                                                                              std::shared_ptr<vk::raii::SurfaceKHR>     surface) {

    std::unordered_map<QueueType, uint32_t> queue_indexes;
    std::vector<vk::QueueFamilyProperties> queue_family_properties = physical_device->getQueueFamilyProperties();

    // get the first index into queueFamilyProperties which supports graphics
    auto graphics_queue_property = std::ranges::find_if(queue_family_properties, [](auto const& qfp) { return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0); });
    assert(graphics_queue_property != queue_family_properties.end() && "No graphics queue family found!");
    uint32_t graphics_index = static_cast<uint32_t>(std::distance(queue_family_properties.begin(), graphics_queue_property));

    // check if the graphics queue supports presentation
    uint32_t present_index = physical_device->getSurfaceSupportKHR(graphics_index, *surface) ? graphics_index : static_cast<uint32_t>(queue_family_properties.size());

    // if the graphics queue we found does not support presentation
    if (present_index == queue_family_properties.size()) {
        for (size_t i = 0; i < queue_family_properties.size(); ++i) {
            if ((queue_family_properties[i].queueFlags & vk::QueueFlagBits::eGraphics) &&
                physical_device->getSurfaceSupportKHR(static_cast<uint32_t>(i), *surface)) {
                graphics_index = static_cast<uint32_t>(i);
                present_index  = graphics_index;
            }
        }
    }

    // if we can't find a queue that does graphics and presentation
    if (present_index == queue_family_properties.size()) {
        for (size_t i = 0; i < queue_family_properties.size(); ++i) {
            if (physical_device->getSurfaceSupportKHR(static_cast<uint32_t>(i), *surface)) {
                present_index = static_cast<uint32_t>(i);
                break;
            }
        }
    }

    // if a graphics queue was found
    if (graphics_index != queue_family_properties.size()) {
        queue_indexes.emplace(QueueType::GRAPHICS, graphics_index);
    }

    // if a presentation queue was found
    if (present_index != queue_family_properties.size()) {
        queue_indexes.emplace(QueueType::PRESENTATION, present_index);
    }

    return queue_indexes;
}

std::shared_ptr<SwapChain> SwapChainFactory::createSwapchain(std::shared_ptr<vk::raii::PhysicalDevice> physical_device,
                                                             std::shared_ptr<LogicalDevice>            logical_device,
                                                             std::shared_ptr<vk::raii::SurfaceKHR>     surface,
                                                             std::shared_ptr<GLFWwindow>               window) {

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
        .oldSwapchain     = nullptr
    };

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