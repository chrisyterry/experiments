#include "vulkan/modern_render_triangle.hpp"
#include <cstring>
#include <map>
#include <cassert>

ModernRenderTriangle::ModernRenderTriangle() {
    m_required_device_extensions = {
        vk::KHRSwapchainExtensionName,
        vk::KHRSpirv14ExtensionName,
        vk::KHRSynchronization2ExtensionName,
        vk::KHRCreateRenderpass2ExtensionName
    };

    m_device_selector = std::make_unique<PhysicalDeviceSelector>(m_required_device_extensions);
}

void ModernRenderTriangle::initWindow() {
    // initialize GLFW without openGL stuff
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // create the GLFW window (fourth param is monitor to open window on, last is openGL only)
    m_window = glfwCreateWindow(m_window_size.first, m_window_size.second, "vulkan", nullptr, nullptr);
}

void ModernRenderTriangle::initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
}

void ModernRenderTriangle::createSurface() {
    VkSurfaceKHR surface; // from C API
    if (glfwCreateWindowSurface(*m_instance, m_window, nullptr, &surface) != 0) {
        throw std::runtime_error("failed to create window surface!");
    }
    m_surface = std::make_unique<vk::raii::SurfaceKHR>(m_instance, surface);
}

void ModernRenderTriangle::pickPhysicalDevice() {
    auto devices = m_instance.enumeratePhysicalDevices();

    std::multimap<uint32_t, std::unique_ptr<vk::raii::PhysicalDevice>> candidate_devices;

    for (const auto& device : devices) {
        std::optional<uint32_t> weight = m_device_selector->scoreDevice(device);
        if (weight.has_value()) {
            candidate_devices.emplace(weight.value(), std::make_unique<vk::raii::PhysicalDevice>(device));
        }
    }

    // if we obtained a valid physical device
    if (candidate_devices.rbegin()->first > 0) {
        std::cout << "Selected " << candidate_devices.rbegin()->second->getProperties().deviceName << " with score " << candidate_devices.rbegin()->first << std::endl;
        m_physical_device = std::move(candidate_devices.rbegin()->second);
    } else {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }
}

void ModernRenderTriangle::createLogicalDevice() {
    std::vector<vk::QueueFamilyProperties> queue_family_properties = m_physical_device->getQueueFamilyProperties();

    // get the first index into queueFamilyProperties which supports graphics
    auto graphics_queue_property = std::ranges::find_if(queue_family_properties, [](auto const& qfp) { return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0); });
    assert(graphics_queue_property != queue_family_properties.end() && "No graphics queue family found!");
    auto graphics_index = static_cast<uint32_t>(std::distance(queue_family_properties.begin(), graphics_queue_property));

    // check if the graphics queue supports presentation
    auto present_index  = m_physical_device->getSurfaceSupportKHR(graphics_index, *m_surface) ? graphics_index : static_cast<uint32_t>(queue_family_properties.size());

    // if the graphics queue we found does not support presentation
    if (present_index == queue_family_properties.size()) {
        for (size_t i = 0; i < queue_family_properties.size(); ++i) {
            if ((queue_family_properties[i].queueFlags & vk::QueueFlagBits::eGraphics) &&
                m_physical_device->getSurfaceSupportKHR(static_cast<uint32_t>(i), *m_surface)) {
                graphics_index = static_cast<uint32_t>(i);
                present_index  = graphics_index;
            }
        }
    }

    // if we can't find a queue that does graphics and presentation
    if (present_index == queue_family_properties.size()) {
        for (size_t i = 0; i < queue_family_properties.size(); ++i) {
            if (m_physical_device->getSurfaceSupportKHR(static_cast<uint32_t>(i), *m_surface)) {
                present_index = static_cast<uint32_t>(i);
                break;
            }
        }
    }

    if (graphics_index == queue_family_properties.size() || present_index == queue_family_properties.size()) {
        throw std::runtime_error( "Could not find a queue for graphics or presentation -> terminating" );
    }

    float queue_priority = 0.0;

    // can only create small number of queues for each family but can have multiple command buffers and submit them all to the same queue
    vk::DeviceQueueCreateInfo queue_create_info {
         .queueFamilyIndex = graphics_index,
         .queueCount = 1,
         .pQueuePriorities = &queue_priority // can be a number between 0 and 1, influences command buffer execution scheduling
    };

    // to enable multiple features have to link structure elements together with pNext; 
    // structure chain does this for you
    // Each template argument is a structure in the chain
    vk::StructureChain<vk::PhysicalDeviceFeatures2, 
                       vk::PhysicalDeviceVulkan13Features, // if we needed something added in other versions of vulkan, add the structs for those versions to the chains
                       vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
    feature_chain = {
        {}, // empty struct
        {.dynamicRendering = true}, // feature was added in vulkan 1.3 so we have to use the vulkan 1.3 struct to set it
        {.extendedDynamicState = true}
    };

    // can only create small number of queues for each family but can have multiple command buffers and submit them all to the same queue
    vk::DeviceCreateInfo device_create_info {
         .pNext = &feature_chain.get<vk::PhysicalDeviceFeatures2>(), // should be same type as first element of structure chain
         .queueCreateInfoCount = 1,
         .pQueueCreateInfos = &queue_create_info,
         .enabledExtensionCount = static_cast<uint32_t>(m_required_device_extensions.size()),
         .ppEnabledExtensionNames = m_required_device_extensions.data()
    };

    // create the logical device
    m_logical_device = std::make_unique<vk::raii::Device>(*m_physical_device, device_create_info);

    // get graphics queue handle
    m_graphics_queue = std::make_unique<vk::raii::Queue>(*m_logical_device, graphics_index, 0);
    m_presentation_queue = std::make_unique<vk::raii::Queue>(*m_logical_device, present_index, 0);
}

void ModernRenderTriangle::setupDebugMessenger() {
    if (!VALIDATION_LAYERS) {
        return;
    }

    // severitys to listen for
    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
    // types to listen for
    vk::DebugUtilsMessageTypeFlagsEXT    messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
        .messageSeverity = severityFlags,
        .messageType     = messageTypeFlags,
        .pfnUserCallback = &debugCallback,  // debug callback function
        .pUserData       = nullptr
    };

    m_debug_messenger = m_instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
}

std::vector<char const*> ModernRenderTriangle::getRequiredLayers() {

    std::vector<char const*> required_layers;
    if (VALIDATION_LAYERS) {
        required_layers.assign(m_validation_layers.begin(), m_validation_layers.end());
    }

    // check if layers are supported
    auto layer_properties = m_context.enumerateInstanceLayerProperties();
    if (std::ranges::any_of(required_layers, [&layer_properties](auto const& required_layer) {
        return std::ranges::none_of(layer_properties, [required_layer](auto const& layer_property) {
            return std::strcmp(layer_property.layerName, required_layer) == 0; });
        })

    ) {
        throw std::runtime_error("One or more required layers are not supported!");
    }

    return std::move(required_layers);
}

std::vector<char const*> ModernRenderTriangle::getRequiredExtensions() {
    // get GLFW extensions
    uint32_t glfw_extension_count = 0;
    auto     glfw_extensions      = glfwGetRequiredInstanceExtensions(&glfw_extension_count); // this will get platform-specific windowing extensions for us

    // check if extensions are supported by Vulkan
    auto extension_properties = m_context.enumerateInstanceExtensionProperties();
    for (uint32_t i = 0; i < glfw_extension_count; ++i) {
        if (std::ranges::none_of(extension_properties, [glfw_extension = glfw_extensions[i]](auto const& extension_property) { return std::strcmp(extension_property.extensionName, glfw_extension) == 0; })) {
            throw std::runtime_error("Required GLFW extension " + std::string(glfw_extensions[i]) + " not supported!");
        }
    }

    std::vector<char const*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);
    if (VALIDATION_LAYERS) {
        extensions.push_back(vk::EXTDebugUtilsExtensionName);
    }

    return std::move(extensions);
}

void ModernRenderTriangle::createInstance() {
    // vulkan application config
    constexpr vk::ApplicationInfo app_info{
        .pApplicationName   = "Modern Triangle Renderer",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName        = "No Engine",
        .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion         = vk::ApiVersion14
    };

    // get required layers
    std::vector<char const*> required_layers = getRequiredLayers();
    // get required extensions
    std::vector<char const*> required_extensions = getRequiredExtensions();

    // instance creation config
    vk::InstanceCreateInfo create_info {
        .pApplicationInfo        = &app_info,
        .enabledLayerCount       = static_cast<uint32_t>(required_layers.size()),
        .ppEnabledLayerNames     = required_layers.data(),
        .enabledExtensionCount   = static_cast<uint32_t>(required_extensions.size()),
        .ppEnabledExtensionNames = required_extensions.data() 
    };

    // create the instance
    m_instance = vk::raii::Instance(m_context, create_info);
}

void ModernRenderTriangle::mainLoop() {
    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();
    }
}

void ModernRenderTriangle::cleanup() {
    // cleanup glfw
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void ModernRenderTriangle::run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

int main() {
    ModernRenderTriangle renderer;

    try { 
        renderer.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}