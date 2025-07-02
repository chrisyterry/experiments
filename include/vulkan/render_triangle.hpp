
// #include <vulkan/vulkan.h> // all the vulkan functions, we use glfw header instead as it configures its own vulkan stuff in the background
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>
#include <optional>

/**
 * @brief helper function to look up vkCreateDebugUtilsMessenger function to create a debug messenger
 * @note this is due to the function being extension, so it's not loaded by default
 * 
 * @param instance the vulkan instance
 * @param message_settings settings for debug messenger pointer
 * @param allocater callback allocater pointer
 * @param debug_messenger debug messenger pointer
 * 
 * @retunr whether the operation was successful
 */
VkResult createDebugUtilsMessengerEXT(VkInstance instance,
                                      const VkDebugUtilsMessengerCreateInfoEXT *message_settings,
                                      const VkAllocationCallbacks *allocater,
                                      VkDebugUtilsMessengerEXT *debug_messenger)
{
    // load the create debug messenegr function
    auto create_debug_function = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    // if we found the debug function
    if (create_debug_function != nullptr) {
        // call the function
        return create_debug_function(instance, message_settings, allocater, debug_messenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

/**
 * @brief helper function to destriy a debug messenger
 * 
 * @param instance the vulkan instance
 * @param debug_messenger the debug messenger to destroy
 * @param allocator allocator pointer
 */
void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger, const VkAllocationCallbacks* allocator) {
    // get the address of the debug messenger destruction function
    auto destroy_debug_function = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    // if we found the debug function
    if (destroy_debug_function != nullptr) {
        destroy_debug_function(instance, debug_messenger, allocator);
    }
}

class TriangleRenderer
{
public:
    /**
     * @brief run the renderer
     */
    void run();

private:

    /**
     * @brief structure to hold queue family indices
     */
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphics_family; ///< index for graphics queues

        /**
         * @brief whether the available queue families are complete
         * 
         * @return true if qeue families are complete
         */
        bool isComplete() {
            return graphics_family.has_value();
        }
    };

    /**
     * @brief Initialize a gflw window for vulkan to use
     */
    void initWindow();

    /**
     * @brief Initialize all the various vulkan components we need
     */
    void initVulkan();

    /**
     * @brief create a vulkan instance
     */
    void createInstance();

    /**
     * @brief main run loop
     */
    void mainLoop();

    /**
     * @brief get available extensions
     */
    void getAvailableExtensions();

    /**
     * @brief get the extensions required by by glfw
     * 
     * @return list of required extensions
     */
    std::vector<const char*> getRequiredExtensions();

    /**
     * @brief select physical devices to use
     */
    void selectPhysicalDevice();

    /**
     * @brief create a logical device to use
     */
    void createLogicalDevice();

    /**
     * @brief assign a score to the physical device based on its suitability for our purposes
     * 
     * @param device the physical device to check
     * 
     * @return integer suitability score for the device
     */
    uint32_t ratePhysicalDevice(VkPhysicalDevice device);

    /**
     * @brief find available queue families for the specified physical device
     * 
     * @param device the physical device to analyse
     * 
     * @return avialable queue families
     */
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

    /**
     * @brief deallocate resources
     * @note we use manual deletion just to be clear about lifetimes of stuff here
     */
    void cleanup();

    /**
     * @brief check which validation layers are available
     * 
     * @param validation_layers the validation layers to be tested 
     * 
     * @return if all the requested validation layers are supported
     */
    bool checkValidationLayerSupport(const std::vector<const char*>& validation_layers);

    /**
     * @brief setup the debug messenger
     */
    void setupDebugMessenger();

    /**
     * @brief populate debug messenger creation info structure
     */
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debug_settings);

    /**
     * @brief Vulkan debug callback
     * 
     * @param message_severity severity of the message, four levels of severity:
     * VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT - Diagnostic
     * VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT - Informational message (e.g. resource creation)
     * VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT - Not nessecarily an error but likely a bug
     * VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT - Invalid behavior that may cause crashes
     * 
     * Setup as enumeration so you can do message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
     *
     * @param message_type the type of the message, three types:
     * VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT - event unrelated to specification or performance
     * VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT - event violates spec or is a mistake
     * VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT - potential unoptimal Vulkan use
     * 
     * @param callback_data struct contianing data for the message, most useful components are:
     * pMessage - Debug message as null terminated string
     * pObjects - Array oVulkan handles related to message
     * objectCount - NUmber of objects in array
     * 
     * @param user_data data set during setup of callback, lets you pass your own data to it
     * 
     * @return boolean inidicating whether Vulkan call triggering message should be aborted; if true, call returns with
     *  VK_ERROR_VALIDATION_FAILED_EXT error (generally for testing validation layer so return VK_FALSE in most cases)
     */
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
        VkDebugUtilsMessageTypeFlagsEXT message_type,
        const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
        void* user_data) {

        // get the debug message
        std::cerr << "validation layer: " << callback_data->pMessage << std::endl;

        return VK_FALSE;
    }
    
    // Vulkan components
    VkInstance m_vulkan_instance;                        ///< instance of vulkan
    VkPhysicalDevice m_physical_device = VK_NULL_HANDLE; ///< physical device to use
    VkDevice m_logical_device;                           ///< logical device to use

    // window components
    GLFWwindow *m_window;                                ///< glfw window for rendering

    // queues
    VkQueue m_graphics_queue; ///< queue for graphics presentation

    // Debugging
    #define NDEBUG

    #ifdef NDEBUG
        const bool m_enable_validation_layers = true;
    #else
        const bool m_enable_validation_layers = false;
    #endif

    VkDebugUtilsMessengerEXT m_debug_messenger; ///< manages debug callback, can have multiple of these
    // This has to be global/live beyond the scope of create instance for it to work
    const std::vector<const char*> m_validation_layers = {
        "VK_LAYER_KHRONOS_validation" // standard validation layer
    };
};