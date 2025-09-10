#include "vulkan/modern_render_triangle.hpp"
#include <cstring>

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
    auto     glfw_extensions      = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

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