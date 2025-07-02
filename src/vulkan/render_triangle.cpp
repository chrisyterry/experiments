#include "vulkan/render_triangle.hpp"

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <memory>
#include <stdint.h>
#include <cstring>
#include <map>

void TriangleRenderer::run() {
    initWindow(); // setup window
    initVulkan();  // setup Vulkan
    mainLoop(); // run loop
    cleanup(); // cleanup
}

void TriangleRenderer::initWindow() {
    glfwInit(); // initialize GLFW
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // disable OpenGL context
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // don't make window resizable (for now)

    const uint16_t WINDOW_HEIGHT = 600;
    const uint16_t WINDOW_WIDTH = 800;

    m_window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan", nullptr, nullptr); // create a window named "vulkan", 4th param is monitor to render on, last parameter is OpenGL only
}

void TriangleRenderer::initVulkan() {
    createInstance(); // create vulkan interface
    setupDebugMessenger(); // setup debug layer messenger
    selectPhysicalDevice(); // select physical device(s)
    createLogicalDevice(); // create logical device
}

void TriangleRenderer::createLogicalDevice() {
    QueueFamilyIndices indices = findQueueFamilies(m_physical_device);

    // configure queue creation
    VkDeviceQueueCreateInfo queue_creation_config{};
    queue_creation_config.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_creation_config.queueFamilyIndex = indices.graphics_family.value();
    queue_creation_config.queueCount = 1;
    float queue_priority = 1.0f; // influences scheduling priority, value between 0 and 1
    queue_creation_config.pQueuePriorities = &queue_priority;

    // set physical device features to use
    VkPhysicalDeviceFeatures device_features{};

    VkDeviceCreateInfo logical_device_config{};
    logical_device_config.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    logical_device_config.pQueueCreateInfos = &queue_creation_config;
    logical_device_config.queueCreateInfoCount = 1;
    logical_device_config.pEnabledFeatures = &device_features;
    logical_device_config.enabledExtensionCount = 0;

    // if validation layers are enabled
    if (m_enable_validation_layers) {
        logical_device_config.enabledLayerCount = static_cast<uint32_t>(m_validation_layers.size());
        logical_device_config.ppEnabledLayerNames = m_validation_layers.data();
    } else {
        logical_device_config.enabledLayerCount = 0;
    }

    // attempt to create the logical device
    if (vkCreateDevice(m_physical_device, &logical_device_config, nullptr, &m_logical_device) != VK_SUCCESS) {
        throw std::runtime_error("Could not create logical device!");
    }

    // get queues for device
    vkGetDeviceQueue(m_logical_device, indices.graphics_family.value(), 0, &m_graphics_queue);
}

void TriangleRenderer::selectPhysicalDevice() {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(m_vulkan_instance, &device_count, nullptr);
    if (device_count == 0) {
        throw std::runtime_error("Could not find GPU with Vulkan support on system!");
    }
    // get the physical devices present on the system
    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(m_vulkan_instance, &device_count, devices.data());

    std::multimap<uint32_t, VkPhysicalDevice> available_devices;

    // for each vulkan-compatible physical device
    for (const auto& device : devices) {
        int suitability_score = ratePhysicalDevice(device);
        available_devices.insert({suitability_score, device});
    }
    
    if (available_devices.rbegin()->first > 0) {
        // select the highest scoring device
        m_physical_device = available_devices.rbegin()->second;
    // throw an error if we can't find a device
    } else {
        throw std::runtime_error("Failed to find a suitable GPU");
    }
}

uint32_t TriangleRenderer::ratePhysicalDevice(VkPhysicalDevice device) {
    // basic device properties (e.g. name, supported Vulkan versions, type etc.)
    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(device, &device_properties);
    // optional device features
    VkPhysicalDeviceFeatures device_features;
    vkGetPhysicalDeviceFeatures(device, &device_features);

    std::cout << "Device: " << std::string(device_properties.deviceName) << std::endl;
    uint32_t device_score = 0;
    device_score = device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? 1000 : 0;
    device_score += device_properties.limits.maxImageDimension2D;

    // demand a geometry shader
    if (!device_features.geometryShader) {
        device_score = 0;
    }
    // get the queue family indices
    QueueFamilyIndices queue_families = findQueueFamilies(device);
    // demand that we have a complete set of queue families
    if (!queue_families.isComplete()) {
        device_score = 0;
    }
    std::cout <<"\tscore: " << device_score << std::endl;
    return device_score;
}

TriangleRenderer::QueueFamilyIndices TriangleRenderer::findQueueFamilies(VkPhysicalDevice device) {
    
    // get the available queue families for the physical device
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

    QueueFamilyIndices queue_indices;
    uint32_t i = 0;
    // for each queue family
    for (const auto& queue_family : queue_families) {
        // if it's a graphics queue family
        if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            queue_indices.graphics_family = i;
        }
        // if we've got all the queues we need
        if (queue_indices.isComplete()) {
            break;
        }
        ++i;
    }

    return queue_indices;
}

bool TriangleRenderer::checkValidationLayerSupport(const std::vector<const char*>& validation_layers) {
    // get a list of all the available validation layers
    uint32_t layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
    std::vector<VkLayerProperties> available_layers(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

    // for each layer name
    for (const char* layer_name : validation_layers) {
        bool layer_found = false;

        // for each available layer name
        for (const auto& layerProperties : available_layers) {
            if (std::strcmp(layer_name, layerProperties.layerName) == 0) {
                layer_found = true;
                break;
            }
        }

        if (!layer_found) {
            return false;
        }
    }

    return true;
}

void TriangleRenderer::setupDebugMessenger() {
    if (!m_enable_validation_layers) return;

    // setup the debug messenger config structure
    VkDebugUtilsMessengerCreateInfoEXT messenger_config;
    populateDebugMessengerCreateInfo(messenger_config);
    
    if (createDebugUtilsMessengerEXT(m_vulkan_instance, &messenger_config, nullptr, &m_debug_messenger) != VK_SUCCESS) {
        throw std::runtime_error("Could not setup debug messenger");
    }

}

void TriangleRenderer::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& messenger_config) {
    messenger_config = {};
    messenger_config.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT; // debug messenger creation
    messenger_config.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT; // set message severities to trigger callback on
    messenger_config.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT; // set message types to tirgger callback on
    messenger_config.pfnUserCallback = TriangleRenderer::debugCallback;             // set the callback function
    messenger_config.pUserData = nullptr;
}

void TriangleRenderer::getAvailableExtensions() {
    // get available extensions
    uint32_t extension_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
    std::vector<VkExtensionProperties> extensions(extension_count);
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());

    std::cout << "available extensions:\n";
    for (const auto& extension : extensions) {
        std::cout << '\t' << extension.extensionName << '\n';
    }
    std::cout << std::endl;
}

void TriangleRenderer::createInstance() {
    // vulkan settings are typically passed using structs

    // create vulkan application config
    VkApplicationInfo application_config{}; // defaults other parameters such as pNext to nullptr (this can point to extension info)
    application_config.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO; // set structure type to vulkan app/instance info
    application_config.pApplicationName = "Triangle Renderer";
    application_config.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    application_config.pEngineName = "No Engine";
    application_config.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    application_config.apiVersion = VK_API_VERSION_1_0;

    // create vulkan instance config
    VkInstanceCreateInfo instance_config{}; // default parameters
    instance_config.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_config.pApplicationInfo = &application_config;


    // get GLFW extension to allow Vulkan to work with this machine
    std::vector<const char*> extensions = getRequiredExtensions(); 
    instance_config.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    instance_config.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debug_creation_config{};
    // check for validation layers
    if (m_enable_validation_layers) {

        if (!checkValidationLayerSupport(m_validation_layers)) {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        // set validation layers
        instance_config.enabledLayerCount = static_cast<uint32_t>(m_validation_layers.size());
        instance_config.ppEnabledLayerNames = m_validation_layers.data();

        // setup debug creation config and add debug layers for instance creation and debug
        populateDebugMessengerCreateInfo(debug_creation_config);
        instance_config.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debug_creation_config;
    } else {
        // no validation layers
        instance_config.enabledLayerCount = 0; 
        instance_config.pNext = nullptr;
    }

    // get available extensions
    getAvailableExtensions();

    // initialize the vulkan instance (general vulkan pattern is config, custom callbacks pointer, handle to new object)
    // and check if intialization was successful
    VkResult result = vkCreateInstance(&instance_config, nullptr, &m_vulkan_instance);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("could not create vulkan instance!");
    }
}

std::vector<const char*> TriangleRenderer::getRequiredExtensions() {
    // get the required extensions for Vulkan
    uint32_t glfw_extension_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

    if (m_enable_validation_layers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); // add debug extension
    }

    return extensions;
}

void TriangleRenderer::mainLoop() {
    // while the window is not closed
    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents(); // check for window events (e.g. pressing the x button)
    }
}

void TriangleRenderer::cleanup() {
    if (m_enable_validation_layers) {
        // destroy the debug messenger
        destroyDebugUtilsMessengerEXT(m_vulkan_instance, m_debug_messenger, nullptr);
    }
    vkDestroyDevice(m_logical_device, nullptr);
    vkDestroyInstance(m_vulkan_instance, nullptr); // destory vulkan instance, nullptr is for callback
    glfwDestroyWindow(m_window); // destroy the window
    glfwTerminate(); // terminate glfw
}

int main () {
    std::unique_ptr<TriangleRenderer> renderer = std::make_unique<TriangleRenderer>();

    try {
        renderer->run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE; // from cstdlib
    }

    return EXIT_SUCCESS; // from cstdlib
}