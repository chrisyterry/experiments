#include "vulkan/device_utils.hpp"
#include "algorithm"
#include <iostream>
#include <cassert>

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

LogicalDevice LogicalDeviceFactory::createLogicalDevice(std::unordered_set<QueueType>             required_queues,
                                                        std::shared_ptr<vk::raii::PhysicalDevice> physical_device,
                                                        std::shared_ptr<vk::raii::SurfaceKHR>     surface) {

    LogicalDevice logical_device;

    // get the required queue indexes
    logical_device.queue_indexes = getQueueIndexes(physical_device, surface);
    for (const auto& queue : required_queues) {
        if (logical_device.queue_indexes.count(queue) < 1) {
            return logical_device;
        }
    }

    float queue_priority = 0.0;

    // can only create small number of queues for each family but can have multiple command buffers and submit them all to the same queue
    vk::DeviceQueueCreateInfo queue_create_info{
        .queueFamilyIndex = logical_device.queue_indexes.at(QueueType::GRAPHICS),
        .queueCount       = 1,
        .pQueuePriorities = &queue_priority // can be a number between 0 and 1, influences command buffer execution scheduling
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
    logical_device.device = std::make_unique<vk::raii::Device>(*physical_device, device_create_info);

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