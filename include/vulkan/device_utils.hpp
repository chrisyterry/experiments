#pragma once

// Vulkan
#ifdef __INTELLISENSE__
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif
#include <vulkan/vulkan.h>
#include <vulkan/vk_platform.h>
#include <unordered_set>
#include <optional>
#include <vector>
#include <memory>

// This is required for the version header apparently
#include <vulkan/vulkan_core.h>

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
        std::vector<const char*> m_required_extensions;  // required extensions for phsyical devices
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