#include "vulkan/device_utils.hpp"
#include "algorithm"
#include <iostream>

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