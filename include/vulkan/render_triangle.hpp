
// #include <vulkan/vulkan.h> // all the vulkan functions, we use glfw header instead as it configures its own vulkan stuff in the background
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>
#include <optional>
#include <fstream>

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

static std::vector<char> readBinaryFile(const std::string& file_path) {
    // create a binary file stream, starting at the end of the file
    std::ifstream file(file_path, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error(("Could not open " + file_path + "!"));
    }

    // get the file size using the fact we are at the end of the file
    size_t file_size = (size_t) file.tellg();
    std::vector<char> byte_buffer(file_size);

    // read file from start
    file.seekg(0);
    file.read(byte_buffer.data(), file_size);
    
    // return the file
    file.close();
    return byte_buffer;
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
        std::optional<uint32_t> present_family; ///< index for presentation queues (graphics and presentation queues may not overlap)

        /**
         * @brief whether the available queue families are complete
         * 
         * @return true if qeue families are complete
         */
        bool isComplete() {
            return graphics_family.has_value() && present_family.has_value();
        }
    };

    /**
     * @brief structure to hold swap properties
     */
    struct SwapChainSupport {
        VkSurfaceCapabilitiesKHR capabilites; // Swap chain capabilities
        std::vector<VkSurfaceFormatKHR> formats; // supported surface formats
        std::vector<VkPresentModeKHR> modes; // supported presentation modes

        /**
         * @brief whether the swap chain support is adequate
         * 
         * @return true if the swap chain is adequate (has a compatible presentation mode and format)
         */
        bool isAdequate() {
            return !formats.empty() && !modes.empty();
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
     * @brief render the frame
     * 
     * @note rendering a frame involves a few common steps"
     *      1) wait for previous frame to finish
     *      2) acquire image from swapchain
     *      3) record command buffer to draw scene to image
     *      4) submit command buffer
     *      5) present image
     * 
     * @note a lot of command are asynchronous, have to manually enforce order
     */
    void drawFrame();

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
     * @brief create a surface to render to
     */
    void createSurface();

    /**
     * @brief create a logical device to use
     */
    void createLogicalDevice();

    /**
     * @brief select physical devices to use
     */
    void selectPhysicalDevice();

    /**
     * @brief assign a score to the physical device based on its suitability for our purposes
     * 
     * @param device the physical device to check
     * 
     * @return integer suitability score for the device
     */
    uint32_t ratePhysicalDevice(VkPhysicalDevice device);

    /**
     * @brief check if the speicifed physical device supports the required extensions
     * @param device the physical device to check
     * 
     * @return true if the physical device supports all the required extensions
     */
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);

    /**
     * @brief determine the capabilities of swap chains for the specified physical device
     */
    SwapChainSupport getSwapChainSupport(VkPhysicalDevice device);

    /**
     * @brief select a swapchain surface format from the available options
     * @note currently just selects VK_FORMAT_B8G8R8A8_SRGB if it's available
     * 
     * @return the selected swap chain format
     */
    VkSurfaceFormatKHR selectSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats);

    /**
     * @brief select the presentation mode for the swapchain from the available options
     * @note currently selects VK_PRESENT_MODE_MAILBOX_KHR if it's available
     * 
     * @return the selected swap chain presentation mode
     */
    VkPresentModeKHR selectSwapPresentationMode(const std::vector<VkPresentModeKHR>& available_modes);

    /**
     * @brief select a swap chain surface exten based on the swap chain capabilities
     * @param capabilities the swapchain capabilities
     * 
     * @return the selected Surface extent
     */
    VkExtent2D selectSwapExtent(const VkSurfaceCapabilitiesKHR& capabilties);

    /**
     * @brief create a swapchain with the logical device
     */
    void createSwapChain();

    /**
     * @brief create swapchain image views
     */
    void createImageViews();

    /**
     * @brief create graphics pipeline for rendering
     */
    void createGraphicsPipeline();

    /**
     * @brief create render pass object
     */
    void createRenderPass();

    /**
     * @brief create frame buffers
     */
    void createFrameBuffers();

    /**
     * @brief create a command pool
     */
    void createCommandPool();

    /**
     * @brief create a command buffer
     */
    void createCommandBuffer();

    /**
     * @brief record commands for a command buffer
     * @note calling this function on an existing buffer will reset it; can't append to command buffers
     * 
     * @param command_buffer the command buffer to populate
     * @param image_index index of current swapchain image to write to
     */
    void recordCommandBuffer(VkCommandBuffer command_buffer, uint32_t image_index);

    /**
     * @brief create a VK shader module from parsed SPV binary
     * 
     * @param shader_code byte code for shader
     * 
     * @return VK shader module for the shader
     */
    VkShaderModule createShaderModule(const std::vector<char>& shader_code);

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
     * @brief create synchronization objects
     */
    void createSyncObjects();

    /**
     * @brief cleanup synchronization objects
     */
    void cleanupSyncObjects();

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
    GLFWwindow* m_window; ///< glfw window for rendering
    VkSurfaceKHR m_surface; ///< surface to render to

    // swapchain
    std::vector<VkFramebuffer> m_swapchain_framebuffer; ///< frame buffer for the swapchain
    VkSwapchainKHR m_swapchain; ///< swap chain for images to render to the screen
    std::vector<VkImage> m_swapchain_images; ///< images in the swapchain
    std::vector<VkImageView> m_swapchain_image_views; ///< image views for swapchain images
    VkFormat m_swapchain_format; ///< swapchain image format
    VkExtent2D m_swapchain_extent; ///< swapchain image extent (Add setting of these)

    // queues and graphics
    VkQueue m_graphics_queue;           ///< queue for graphics presentation
    VkQueue m_presentation_queue;       ///< queue for presenting graphics to screen
    VkPipelineLayout m_pipeline_layout; ///< graphics pipeline layout
    VkRenderPass m_render_pass;         ///< render pass
    VkPipeline m_graphics_pipeline;     ///< graphics pipeline

    // command pool
    VkCommandPool m_command_pool; ///< command pool for execution
    VkCommandBuffer m_command_buffer; ///< command buffer (freed when associated command pool is destroyed)

    // synchronization
    VkSemaphore m_image_avialble_sempahore; ///< semaphore to signal image availability
    VkSemaphore m_render_finished_semaphore; ///< semaphore to signal rendering has finished
    VkFence m_inflight_fence; ///< fence to ensure single frame is rendered

    // NDEBUG is set if its a debug build, including RelWithDebInfo
    #ifdef NDEBUG
        const bool m_enable_validation_layers = true;
    #else
        const bool m_enable_validation_layers = false;
    #endif

    VkDebugUtilsMessengerEXT m_debug_messenger; ///< manages debug callback, can have multiple of these
    // These have to be global/live beyond the scope of create instance for it to work
    const std::vector<const char*> m_validation_layers = {
        "VK_LAYER_KHRONOS_validation" // standard validation layer
    };
    const std::vector<const char*> m_device_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME // swap chains
    };
};