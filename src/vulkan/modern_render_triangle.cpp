#include "vulkan/modern_render_triangle.hpp"
#include <cstring>
#include <map>
#include <cassert>

#include "utils/utils.hpp"
#include <filesystem>

#include <iostream>

ModernRenderTriangle::ModernRenderTriangle() {
    std::vector<const char*> required_device_extensions = {
        vk::KHRSwapchainExtensionName,
        vk::KHRSpirv14ExtensionName,
        vk::KHRSynchronization2ExtensionName,
        vk::KHRCreateRenderpass2ExtensionName
    };

    m_device_selector           = std::make_unique<PhysicalDeviceSelector>(required_device_extensions);
    m_logical_device_factory    = std::make_unique<LogicalDeviceFactory>(required_device_extensions);
    m_swapchain_factory         = std::make_unique<SwapChainFactory>();
    m_graphics_pipeline_factory = std::make_unique<GraphicsPipelineFactory>();
}

ModernRenderTriangle::~ModernRenderTriangle() {
    cleanup();
}


void ModernRenderTriangle::initWindow(const std::string& window_name) {
    // initialize GLFW without openGL stuff
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    // create the GLFW window (fourth param is monitor to open window on, last is openGL only)
    GLFWwindow* window = glfwCreateWindow(m_window_size.first, m_window_size.second, window_name.c_str(), nullptr, nullptr);
    // set pointer to class to use for callback
    glfwSetWindowUserPointer(window, this);
    // set the resize callback for the window
    glfwSetFramebufferSizeCallback(window, ModernRenderTriangle::framebufferResizeCallback);
    m_window           = std::shared_ptr<GLFWwindow>(window, DestroyGLFWWindow{});
}

void ModernRenderTriangle::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<ModernRenderTriangle*>(glfwGetWindowUserPointer(window));
    app->framebufferResized();
}

void ModernRenderTriangle::initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapchain();
    createGraphicsPipeline();
    createCommandBuffers();
    createSyncObjects();
}

void ModernRenderTriangle::createSurface() {
    VkSurfaceKHR surface; // from C API
    if (glfwCreateWindowSurface(*m_instance, m_window.get(), nullptr, &surface) != 0) {
        throw std::runtime_error("failed to create window surface!");
    }
    m_surface = std::make_unique<vk::raii::SurfaceKHR>(m_instance, surface);
}

void ModernRenderTriangle::pickPhysicalDevice() {
    auto devices = m_instance.enumeratePhysicalDevices();

    std::multimap<uint32_t, std::shared_ptr<vk::raii::PhysicalDevice>> candidate_devices;

    for (const auto& device : devices) {
        std::optional<uint32_t> weight = m_device_selector->scoreDevice(device);
        if (weight.has_value()) {
            candidate_devices.emplace(weight.value(), std::make_shared<vk::raii::PhysicalDevice>(device));
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
    
    // attempt to create the logical device
    m_logical_device = m_logical_device_factory->createLogicalDevice({QueueType::GRAPHICS, QueueType::PRESENTATION}, m_physical_device, m_surface);

    if (!m_logical_device->device) {
        throw std::runtime_error( "Could not find a queue for graphics or presentation" );
    }

    // get handles for the required queues
    m_graphics_queue     = std::make_unique<vk::raii::Queue>(*m_logical_device->device, m_logical_device->queue_indexes.at(QueueType::GRAPHICS), 0);
    m_presentation_queue = std::make_unique<vk::raii::Queue>(*m_logical_device->device, m_logical_device->queue_indexes.at(QueueType::PRESENTATION), 0);
}

void ModernRenderTriangle::createSwapchain () {
    m_swapchain = m_swapchain_factory->createSwapchain(m_physical_device, m_logical_device, m_surface, m_window, m_swapchain);
}   

void ModernRenderTriangle::createGraphicsPipeline() {

    // get the path to the shader file
    std::filesystem::path executable_path = experiments::utils::getExecutablePath();
    std::string           shader_path     = executable_path.parent_path().string() + "/shaders/nu_triangle_shaders.spv";

    // create the graphics pipeline
    m_graphics_pipeline = m_graphics_pipeline_factory->createGraphicsPipeline(m_logical_device, m_swapchain, shader_path);
}

void ModernRenderTriangle::createCommandPool() {
    /*
    Two options for command pool flags:
    VK_COMMAND_POOL_CREATE_TRANSIENT_BIT - hint that command buffers are re-recorded with new commands frequently
    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT - allow command buffers to be recorded individually, otherwise all have to be reset together
     */
    vk::CommandPoolCreateInfo command_pool_info {
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = m_logical_device->queue_indexes.at(QueueType::GRAPHICS)
    };

    m_command_pool = std::make_unique<vk::raii::CommandPool>(*(m_logical_device->device), command_pool_info);
}

void ModernRenderTriangle::createCommandBuffers() {
    // create a command pool
    createCommandPool();

    // clear any existing command buffers
    m_command_buffers.clear();

    /*
    Command buffer level has two options:
    VK_COMMAND_BUFFER_LEVEL_PRIMARY - can be submitted for execution but not called form other command buffers
    VK_COMMAND_BUFFER_LEVEL_SECONDARY - cannot be directly submitted but can be called from primary command buffers (i.e. reuse common operations)
    */
    vk::CommandBufferAllocateInfo buffer_allocation_info {
        .commandPool = *m_command_pool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = m_max_frames_in_flight, // can allocate multiple buffers in one buffer allocation call
    };

    // create a command buffer
    std::vector<vk::raii::CommandBuffer> command_buffers = vk::raii::CommandBuffers(*(m_logical_device->device), buffer_allocation_info);
    for (uint8_t buffer_index = 0; buffer_index < m_max_frames_in_flight; ++buffer_index) {
        m_command_buffers.emplace_back(std::make_unique<vk::raii::CommandBuffer>(std::move(command_buffers.front())));
    }
}

void ModernRenderTriangle::recordCommandBuffer(uint32_t image_index) {
    // always start command buffer recording with a begin; can't append to command buffer after recording
    /*
    options for struct given to begin:
        
    flags:
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT - buffer will be rerecorded after executing it
        VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT - secondary command buffer that will be entirely in single render pass
        VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT - can be resubmitted while pending execution
    pInheritanceInfo - specified state to inherit from calling primary command buffers
    */
    m_command_buffers.at(m_frame_index)->begin({});

    // transition image to color attachment
    transitionImageLayout(
        image_index,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        {},  // no need to wait for previous operations
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput);

    // setup for color attachment
    vk::ClearValue              clear_color     = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);  // black
    vk::RenderingAttachmentInfo attachment_info = {
        .imageView   = m_swapchain->image_views->at(image_index),  // image to render to
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,  // image format
        .loadOp      = vk::AttachmentLoadOp::eClear,  // clear image
        .storeOp     = vk::AttachmentStoreOp::eStore,  // store new colors for cleared image
        .clearValue  = clear_color,
    };

    // setup rendering info
    vk::RenderingInfo rendering_info = {
        .renderArea = {
            .offset = { 0, 0 },
            .extent = m_swapchain->extent,
        },
        .layerCount           = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments    = &attachment_info,
    };

    // begin rendering
    m_command_buffers.at(m_frame_index)->beginRendering(rendering_info);

    // bind graphics pipeline
    m_command_buffers.at(m_frame_index)->bindPipeline(vk::PipelineBindPoint::eGraphics, *m_graphics_pipeline); // first param specifies compute vs graphics

    // set dynamic state
    m_command_buffers.at(m_frame_index)->setViewport(
        0, 
        vk::Viewport(0.0f, 0.0f, static_cast<float>(m_swapchain->extent.height),
        static_cast<float>(m_swapchain->extent.width), 
        0.0f, 
        1.0f));
    m_command_buffers.at(m_frame_index)->setScissor(0, vk::Rect2D(vk::Offset2D(0,0), m_swapchain->extent));

    /* draw command
    vertexCount - number of vertices to draw
    instanceCount - number of instances to draw; 1 if not using instanced rendering
    firstVertex - offset in vertex buffer; lowest value of SV_VertexId
    firstInstance - offset for instanced rendering, lowest value of SV_Instance_ID
    */
    m_command_buffers.at(m_frame_index)->draw(3, 1, 0, 0); 

    // end rendering
    m_command_buffers.at(m_frame_index)->endRendering();

    // return image to presentation layout
    transitionImageLayout(
        image_index,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        {},
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eBottomOfPipe
    );

    // end command buffer recording
    m_command_buffers.at(m_frame_index)->end();
}

void ModernRenderTriangle::transitionImageLayout(
    uint32_t image_index,
    vk::ImageLayout old_layout,
    vk::ImageLayout new_layout,
    vk::AccessFlags2 src_access_mask,
    vk::AccessFlags2 dst_access_mask,
    vk::PipelineStageFlags2 src_stage_mask,
    vk::PipelineStageFlags2 dst_stage_mask
) {

    // create barrier for the transition operation
    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask = src_stage_mask,
        .srcAccessMask = src_access_mask,
        .dstStageMask = dst_stage_mask,
        .dstAccessMask = dst_access_mask,
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = m_swapchain->images->at(image_index),
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    vk::DependencyInfo dependencyInfo = {
        .dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier
    };
    m_command_buffers.at(m_frame_index)->pipelineBarrier2(dependencyInfo);
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

void ModernRenderTriangle::drawFrame() {
    /**
     * steps in rendering a frame:
     * 1) wait for previous frame to finish
     * 2) acquire swapchain image
     * 3) record command buffer for rendering scene
     * 4) submit command buffer
     * 5) present swapchain image
     *
     * have to perform synchronization explicitly; all operations are asynchronous - they will return immediately in CPU but the GPU will wait
     */

    // wait till preceding frame has finished rendering (takes array of fences)
    vk::Result fence_result = m_logical_device->device->waitForFences(*(*m_draw_fences.at(m_frame_index)), vk::True, UINT64_MAX);  // true means wait for all fences, final val is timeout [ns]
    if (fence_result != vk::Result::eSuccess) {
        throw std::runtime_error(("failed to wait for fence for frame index " + std::to_string(int(m_frame_index)) + "!"));
    }

    // acquire the next swapchain image
    vk::Result image_acquisition_result;
    uint32_t   swapchain_image_index;
    std::tie(image_acquisition_result, swapchain_image_index) = m_swapchain->swapchain->acquireNextImage(UINT64_MAX, *(*m_present_complete_semaphores.at(m_frame_index)), nullptr);  // first val is timeout, last is variable to write index of swapchain image that has become available

    // if the current swapchain is no longer valid
    if (image_acquisition_result == vk::Result::eErrorOutOfDateKHR) {
        // recreate the swapchain (we can longer render to the old swapchain)
        createSwapchain();
        return;
    }

    // we've failed to acquire a swapchain image for an unexpected reason
    if (image_acquisition_result != vk::Result::eSuccess &&
        image_acquisition_result != vk::Result::eSuboptimalKHR) {
        assert(result == vk::Result::eTimeout || result == vk::Result::eNotReady);
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    // reset the fence that we were waiting for, only doing this if the swapchain has not been reset
    m_logical_device->device->resetFences(*(*m_draw_fences.at(m_frame_index)));

    m_command_buffers.at(m_frame_index)->reset();
    recordCommandBuffer(swapchain_image_index);

    vk::PipelineStageFlags wait_destination_stage_mask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    const vk::SubmitInfo submit_info {
        // sempahores to wait on before execution, stages of pipeline to wait
        // - want to wait on image being available before writing colors to image 
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*(*m_present_complete_semaphores.at(m_frame_index)), // each index corresponds to index in waitStages array
        .pWaitDstStageMask = &wait_destination_stage_mask,
        //command buffer to submit for execution
        .commandBufferCount = 1,
        .pCommandBuffers = &*(*m_command_buffers.at(m_frame_index)),
        // semaphores to signal on completion
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &*(*m_rendering_complete_semaphores.at(m_frame_index)),
    };

    // submit command buffer to graphics queue (takes array of submit info for larger loads)
    m_graphics_queue->submit(submit_info, *(*m_draw_fences.at(m_frame_index)));

    const vk::PresentInfoKHR presentation_info {
        // semaphores to wait on before presentation
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*(*m_rendering_complete_semaphores.at(m_frame_index)),
        // swaphchains to present to
        .swapchainCount = 1,
        .pSwapchains = &*(*(m_swapchain->swapchain)),
        .pImageIndices = &swapchain_image_index,
        .pResults = nullptr, // optional, can specify an array of vk::Result for each swapchain to verify presentation is successful
    };

    vk::Result presentation_result = m_presentation_queue->presentKHR(presentation_info);

    // if there were issues with presentation, recreate the swapchain
    if ((presentation_result == vk::Result::eSuboptimalKHR) ||
        (presentation_result == vk::Result::eErrorOutOfDateKHR) ||
        m_frame_buffer_resized) {
        createSwapchain();
        m_frame_buffer_resized = false;
    } else {
        assert(presentation_result == vk::Result::eSuccess);
    }

    // advance to the next frame index
    m_frame_index = (m_frame_index + 1) & m_max_frames_in_flight;
}

void ModernRenderTriangle::createSyncObjects() {
    // semaphores order execution on the GPU (queue operations; work submitted to a queue or within a function)
    // semaphores can be binary or timeline; binary are signaled or unsignaled
    // supply semaphore as signal sempahore for first operation, ait for second operation

    // fences order execution on the CPU - when we need to know when the GPU has finished something
    // fences signaled or unsignaled; attach fence to GPU work, will be signaled when it's complete
    // fences have to be reset manually

    assert(m_present_complete_semaphores.empty() && m_rendering_complete_semaphores.empty() && m_draw_fences.empty());

    // for each swapchain image
    for (size_t i = 0; i < m_swapchain->images->size(); ++i) {
        m_rendering_complete_semaphores.emplace_back(std::make_unique<vk::raii::Semaphore>(*(m_logical_device->device), vk::SemaphoreCreateInfo()));
    }

    vk::FenceCreateInfo fence_info = {
    .flags = vk::FenceCreateFlagBits::eSignaled
    };

    // for each frame in flight
    for (size_t i = 0; i < m_max_frames_in_flight; ++i) {
        m_present_complete_semaphores.emplace_back(std::make_unique<vk::raii::Semaphore>(*(m_logical_device->device), vk::SemaphoreCreateInfo()));
        m_draw_fences.emplace_back(std::make_unique<vk::raii::Fence>(*(m_logical_device->device), fence_info));
    }
}

void ModernRenderTriangle::mainLoop() {
    while (!glfwWindowShouldClose(m_window.get())) {
        glfwPollEvents();
        drawFrame();
    }

    // wait for GPU operations to be complete before exiting
    m_logical_device->device->waitIdle();
}

void ModernRenderTriangle::cleanup() {
    // cleanup graphics pipeline before cleanup of devices
    m_graphics_pipeline->~Pipeline();
    // cleanup swapchain before cleanup of devices
    m_swapchain->~SwapChain();
    // cleanup glfw
    glfwTerminate();
}

void ModernRenderTriangle::run() {
    initWindow("test");
    initVulkan();
    mainLoop();
    cleanup();
}