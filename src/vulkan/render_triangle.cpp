#include "vulkan/render_triangle.hpp"

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <memory>
#include <stdint.h>
#include <cstring>
#include <map>
#include <set>
#include <algorithm>
#include <limits>

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
    createSurface(); // create the surface to render to (befroe pshycial device selection as it can affect which device gets selected)
    selectPhysicalDevice(); // select physical device(s)
    createLogicalDevice(); // create logical device
    createSwapChain(); // create swapchain
    createImageViews(); // create image views
    createRenderPass(); // create frame buffer attachments and associated data
    createGraphicsPipeline(); // create graphics pipeline
}

void TriangleRenderer::createRenderPass() {
    // for now we have a single color buffer attachment for one of the images in the swapchain
    VkAttachmentDescription color_attachment{};
    color_attachment.format  = m_swapchain_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    // loadOp specifies what to do with attachment data before rendering
    // applies to color and depth data
    // - VK_ATTACHMENT_LOAD_OP_LOAD - preserve existing attachment contents
    // - VK_ATTACHMENT_LOAD_OP_CLEAR - clear values to constant at start
    // - VK_ATTACHMENT_LOAD_OP_DONT_CARE - existing contents undefined
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // sets everything black
    // storeOp specifies what to do with attachment data fater rendering
    // applies to color and depth data
    // - VK_ATTACHMENT_STORE_OP_STORE - rendered contents stored in memory, can be read later
    // - VK_ATTACHMENT_STORE_OP_DONT_CARE - contents undefined after operation
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // so we can see the triangle
    // apply stencilOps apply to stencils
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    // images need to be in correct layout for the operation to be performed, have a few options (these aren't all of them):
    // - VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL - images used as color attachment
    // - VK_IMAGE_LAYOUT_PRESENT_SRC_KHR - images to be presented to swap chain
    // - VK_IMAGE_LAYOUT_TRANSFER_DST - images to be used as destination for a mem copy
    // - VK_IMAGE_LAYOUT_UNDEFINED - don't care what format it's in
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // expected layout before render pass
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // expected layout after render pass

    // each sub pass references 1 or more attachments
    VkAttachmentReference color_attachment_ref{};
    color_attachment_ref.attachment = 0; // attachment index - this corresponds to the layout(location = 0) out vec4 outColor from the shader!
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // layout to be attached

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // may allow compute subpasses in future
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;
    subpass.pInputAttachments = nullptr;       // attachments read from a shader
    subpass.pResolveAttachments = nullptr;     // attachments for multisampling color attachments
    subpass.pDepthStencilAttachment = nullptr; // attachment for depth and stencil data
    subpass.pPreserveAttachments = nullptr;    // attachmenbts for data that is unused but needs to be preserved

    VkRenderPassCreateInfo render_pass_config{};
    render_pass_config.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_config.attachmentCount = 1;
    render_pass_config.pAttachments = &color_attachment;
    render_pass_config.subpassCount = 1;
    render_pass_config.pSubpasses = &subpass;

    if (vkCreateRenderPass(m_logical_device, &render_pass_config, nullptr, &m_render_pass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}

void TriangleRenderer::createGraphicsPipeline() {
    // load shaders
    ///\todo figure out a better way of doing the paths
    std::vector<char> vertex_shader_code = readBinaryFile("/home/chriz/Development/experiments/shaders/bin/triangle_vertex_shader.spv");
    std::vector<char> fragment_shader_code = readBinaryFile("/home/chriz/Development/experiments/shaders/bin/triangle_vertex_shader.spv");

    VkShaderModule vertex_shader = createShaderModule(vertex_shader_code);
    VkShaderModule fragment_shader = createShaderModule(fragment_shader_code);

    // setup the vertex shader stage config
    VkPipelineShaderStageCreateInfo vertex_stage_config{};
    vertex_stage_config.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertex_stage_config.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertex_stage_config.module = vertex_shader; // set shader module
    vertex_stage_config.pName = "main"; // entry function in shader
    vertex_stage_config.pSpecializationInfo = nullptr; // specify values for shader constants

    // setup the fragment stage config
    VkPipelineShaderStageCreateInfo fragment_stage_config{};
    fragment_stage_config.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragment_stage_config.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_stage_config.module = fragment_shader;
    fragment_stage_config.pName = "main"; 
    fragment_stage_config.pSpecializationInfo = nullptr;

    VkPipelineShaderStageCreateInfo shader_stages[] = {vertex_stage_config, fragment_stage_config};

    // Describes the format of vertex data
    // bindings - spacing between data and whether data is per vertex or per instance
    // attribute descriptions - types of attributes passed to vertex shader, binding to load from and offset
    VkPipelineVertexInputStateCreateInfo vertex_input_config {};
    vertex_input_config.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_config.vertexBindingDescriptionCount = 0; //
    vertex_input_config.pVertexBindingDescriptions = nullptr; // optional; points to array of structs describing vertex loading
    vertex_input_config.vertexAttributeDescriptionCount = 0;
    vertex_input_config.pVertexAttributeDescriptions = nullptr; // optional; points to array of structs describing vertex loading

    // Describes the kind of geometry drawn from vertices and primitive restart; options:
    // - VK_PRIMITIVE_TOPOLOGY_POINT_LIST - points from vertices
    // - VK_PRIMITIVE_TOPOLOGY_LINE_LIST - line from evert 2 vertices without reuse
    // - VK_PRIMITIVE_TOPOLOGY_LINE_STRIP - end vertex of each line used for start vertex of next line
    // - VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST - triangle from every 3 vertices without reuse
    // - VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP - the second and third vertex of every triangle are used as first two vertices of the next two triangles
    // vertices loaded from vertex buffer in sequential index order; can use element buffer to specify indexes manually
    // primitiveRestartEnable = 
    VkPipelineInputAssemblyStateCreateInfo input_assembly_config{};
    input_assembly_config.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_config.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_config.primitiveRestartEnable = VK_FALSE; // VK_TRUE lets you break up lines and triangles in the _STRIP topology modes using index of 0xFFFF or 0xFFFFFFFF

    // some pipeline states can be dynamic, most are fixed
    std::vector<VkDynamicState> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamic_state = {};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_state.pDynamicStates = dynamic_states.data();

    // view port
    VkViewport viewport_config{};
    viewport_config.x = 0.0f;
    viewport_config.y = 0.0f;
    viewport_config.width = (float) m_swapchain_extent.width; // match to swapchain size
    viewport_config.height = (float) m_swapchain_extent.height;
    viewport_config.minDepth = 0.0f; // near clipping plane
    viewport_config.maxDepth = 1.0f; // far clipping plane

    // scissor rectangle (full extend of image/same size as framebuffer)
    VkRect2D scissor_rectangle{};
    scissor_rectangle.offset = {0, 0};
    scissor_rectangle.extent = m_swapchain_extent;

    // view port state (dynamic)
    VkPipelineViewportStateCreateInfo viewport_state_config;
    viewport_state_config.sType - VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_config.viewportCount = 1; // some hardware supports multiple viewports and scissors
    viewport_state_config.scissorCount = 1;

    // rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer_config {};
    rasterizer_config.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer_config.depthClampEnable = VK_FALSE; // if enabled, geometry outside clip planes moved to clip plane limits (useful for thing slike shadow maps; needs GPU feature enabling)
    rasterizer_config.rasterizerDiscardEnable = VK_FALSE; // if enabled, geometry never passes through rasterizer (framebuffer output disabled)
    // rasterizer can be configured to create fragements of different types:
    // - VK_POLYGON_MODE_FILL - fill area of polygon with fragments
    // - VK_POLYGON_MODE_LINE - poygon edges drawn as lines (wireframe); requires GPU feature
    // - VK_POLYGON_MODE_POINT - polygon vertices drawn as points; requires GPU feature
    rasterizer_config.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer_config.lineWidth = 1.0f; // thickness of lines in number of fragments, values other than 1.0 require wideLines GPU feature
    // rasterizer can cull faces in different modes, works in tandem with front face setting
    // - VK_CULL_MODE_NONE - don't cull anything
    // - VK_CULL_MODE_FRONT_BIT - cull front faces
    // - VK_CULL_MODE_BACK_BIT - cull back faces
    // - VK_CULL_MODE_FRONT_AND_BACK - cull all faces
    rasterizer_config.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer_config.frontFace = VK_FRONT_FACE_CLOCKWISE; // other option is VK_FRONT_FACE_COUNTERCLOCKWISE
    // can optionally modify depth values using constant or fragments slope; sometimes useful for shadow maps
    rasterizer_config.depthBiasEnable = VK_FALSE;
    rasterizer_config.depthBiasConstantFactor = 0.0f; // optional
    rasterizer_config.depthBiasClamp = 0.0f; // optional
    rasterizer_config.depthBiasSlopeFactor = 0.0f; // optional

    // multisampling (one form of Antialiasing); currently disabled (requires GPU feature)
    VkPipelineMultisampleStateCreateInfo multisampling_config{};
    multisampling_config.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling_config.sampleShadingEnable = VK_FALSE;
    multisampling_config.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; // powers of 2 up to 8(!)
    multisampling_config.minSampleShading = 1.0f; //optional
    multisampling_config.pSampleMask = nullptr; //optional
    multisampling_config.alphaToCoverageEnable = VK_FALSE; // optional
    multisampling_config.alphaToOneEnable = VK_FALSE; //optional

    // Color attachment blending - combining fragment shader output with existing framebuffer contents
    VkPipelineColorBlendAttachmentState colorblend_attachment_config{};
    colorblend_attachment_config.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
    VK_COLOR_COMPONENT_G_BIT |
    VK_COLOR_COMPONENT_B_BIT |
    VK_COLOR_COMPONENT_A_BIT;
    colorblend_attachment_config.blendEnable = VK_FALSE;
    // color blending
    colorblend_attachment_config.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // overwrite output completley (optional)
    colorblend_attachment_config.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // don't take anything from the output (optional)
    colorblend_attachment_config.colorBlendOp = VK_BLEND_OP_ADD; // additive color blending (optional)
    // alpha blending
    colorblend_attachment_config.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // use alpha from source (optional)
    colorblend_attachment_config.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // don't use the destination (optional)
    colorblend_attachment_config.alphaBlendOp = VK_BLEND_OP_ADD; //optional
    /**
    // Alpha-based blending
    colorblend_attachment_config.blendEnable = VK_TRUE;
    colorblend_attachment_config.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorblend_attachment_config.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorblend_attachment_config.colorBlendOp = VK_BLEND_OP_ADD;
    colorblend_attachment_config.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorblend_attachment_config.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorblend_attachment_config.alphaBlendOp = VK_BLEND_OP_ADD;
    */

    // lets you set blend constants for previous blending calculation; enabling this auto disables first method
    // disabling both sends fragment chunks to framebuffer without modification
    VkPipelineColorBlendStateCreateInfo color_blending_config{};
    color_blending_config.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending_config.logicOpEnable = VK_FALSE;
    color_blending_config.logicOp = VK_LOGIC_OP_COPY; // optional
    color_blending_config.attachmentCount = 1;
    color_blending_config.pAttachments = &colorblend_attachment_config;
    color_blending_config.blendConstants[0] = 0.0f; // optional
    color_blending_config.blendConstants[1] = 0.0f; // optional
    color_blending_config.blendConstants[2] = 0.0f; // optional
    color_blending_config.blendConstants[3] = 0.0f; // optional

    VkPipelineLayout pipeline_layout;
    VkPipelineLayoutCreateInfo pipeline_layout_config{};
    pipeline_layout_config.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_config.setLayoutCount = 0; // lets you pass values to uniforms in shader (optional)
    pipeline_layout_config.pSetLayouts = nullptr; // optional
    pipeline_layout_config.pushConstantRangeCount = 0; // optional
    pipeline_layout_config.pPushConstantRanges = nullptr; // optional

    // create the pipeline layout
    if (vkCreatePipelineLayout(m_logical_device, &pipeline_layout_config, nullptr, &m_pipeline_layout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout");
    }

    // destroy shaders
    vkDestroyShaderModule(m_logical_device, vertex_shader, nullptr);
    vkDestroyShaderModule(m_logical_device, fragment_shader, nullptr);
}

VkShaderModule TriangleRenderer::createShaderModule(const std::vector<char>& shader_code) {
    VkShaderModuleCreateInfo shader_config{};
    shader_config.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_config.codeSize = shader_code.size();
    shader_config.pCode = reinterpret_cast<const uint32_t*>(shader_code.data());

    // create the shader
    VkShaderModule shader;
    if (vkCreateShaderModule(m_logical_device, &shader_config, nullptr, &shader)) {
        throw std::runtime_error("Could not create shader module!");
    }

    return shader;
}

void TriangleRenderer::createImageViews() {
    m_swapchain_image_views.resize(m_swapchain_images.size());

    // for each swapchain image view
    for (size_t i = 0; i < m_swapchain_images.size(); ++i) {
        VkImageViewCreateInfo image_view_config{};
        image_view_config.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_config.image = m_swapchain_images[i];
        image_view_config.viewType = VK_IMAGE_VIEW_TYPE_2D; // lets you choose how to interpret the image (1D, 2D, 3D or cubemap)
        image_view_config.format = m_swapchain_format;

        // color swizzling (swizzling is basically swapping color channels; below keeps it as it is)
        image_view_config.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_config.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_config.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_config.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        // sub resources (what is the purpose of the image and which part to access)
        image_view_config.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_view_config.subresourceRange.baseMipLevel = 0;
        image_view_config.subresourceRange.levelCount = 1;
        image_view_config.subresourceRange.baseArrayLayer = 0;
        image_view_config.subresourceRange.layerCount = 1; // may have more layers in 3D applications; e.g. a view for the left and right eye

        // create the image view
        if (vkCreateImageView(m_logical_device, &image_view_config, nullptr, &m_swapchain_image_views[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image views!");
        }
    }
}

void TriangleRenderer::createSwapChain() {
    SwapChainSupport swapchain_support = getSwapChainSupport(m_physical_device);

    VkSurfaceFormatKHR surface_format = selectSwapSurfaceFormat(swapchain_support.formats);
    VkPresentModeKHR present_mode = selectSwapPresentationMode(swapchain_support.modes);
    VkExtent2D extent = selectSwapExtent(swapchain_support.capabilites);

    // number of images in the swapchain (want at least 1 more than min so we don't have to wait to render next image)
    uint32_t image_count = swapchain_support.capabilites.minImageCount + 1;
    // if unlimited image count (maxImageCount = 0) has not been specified
    if (swapchain_support.capabilites.maxImageCount > 0) {
        image_count = std::max(image_count, swapchain_support.capabilites.maxImageCount);
    }

    VkSwapchainCreateInfoKHR swapchain_config{};
    swapchain_config.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_config.surface = m_surface;
    swapchain_config.minImageCount = image_count;
    swapchain_config.imageFormat = surface_format.format;
    swapchain_config.imageColorSpace = surface_format.colorSpace;
    swapchain_config.imageExtent = extent;
    swapchain_config.imageArrayLayers = 1; // want 1 layer if we're not rendering in 3D
    swapchain_config.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // rendering directly to images in swapchain, if want to do something with them before display (e.g. postprocessing) use VK_IMAGE_USAGE_TRANSFER_DST_BIT

    // determine if the graphics and presentation queues are the same and adjust settings accordingly
    QueueFamilyIndices inidices = findQueueFamilies(m_physical_device);
    uint32_t queueFamilyIndices[] = {inidices.graphics_family.value(), inidices.present_family.value()};
    // if graphics and presentation queues are distinct
    if (inidices.graphics_family.value() != inidices.present_family.value()) {
        swapchain_config.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // image used by multiple queues without explicit transfer
        swapchain_config.queueFamilyIndexCount = 2;
        swapchain_config.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        swapchain_config.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // image owned by one queue at a time, ownership transfer to new queue family is explicit (fastest)
        swapchain_config.queueFamilyIndexCount = 0; // optional
        swapchain_config.pQueueFamilyIndices = nullptr; // optional
    }
    swapchain_config.preTransform = swapchain_support.capabilites.currentTransform; // can flip image 90 degrees or mirror if it's in capabilities.supportedTransform
    swapchain_config.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // whether to use alpha channel for blending windows (generally want opaque)
    swapchain_config.presentMode = present_mode;
    swapchain_config.clipped = VK_TRUE; // we don't care about color of pixels in the window occluded by other pixels (faster)
    swapchain_config.oldSwapchain = VK_NULL_HANDLE; // if we have to make a new swap chain (e.g. window size changes), pass pointer to the old one

    // attempt to cerate the swapchain
    if (vkCreateSwapchainKHR(m_logical_device, &swapchain_config, nullptr, &m_swapchain) != VK_SUCCESS) {
        throw std::runtime_error("could not create swapchain!");
    }

    // get the swapchain image handles (number of images may have changed)
    vkGetSwapchainImagesKHR(m_logical_device, m_swapchain, &image_count, nullptr);
    m_swapchain_images.resize(image_count);
    vkGetSwapchainImagesKHR(m_logical_device, m_swapchain, &image_count, m_swapchain_images.data());

    // store the swapchain format and extent
    m_swapchain_format = surface_format.format;
    m_swapchain_extent = extent;
}

void TriangleRenderer::createSurface() {
    // if we were unable to create a surface to render to
    if (glfwCreateWindowSurface(m_vulkan_instance, m_window, nullptr, &m_surface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create window surface");
    }
}

void TriangleRenderer::createLogicalDevice() {
    QueueFamilyIndices indices = findQueueFamilies(m_physical_device);

    std::vector<VkDeviceQueueCreateInfo> queue_creation_configs;
    std::set<uint32_t> unique_queue_families = {indices.graphics_family.value(), indices.present_family.value()};
    float queue_priority = 1.0f; // influences scheduling priority, value between 0 and 1

    // for each unique queue type required
    for (uint32_t queue_family : unique_queue_families) {
        // configure the current queue
        VkDeviceQueueCreateInfo queue_creation_config{};
        queue_creation_config.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_creation_config.queueFamilyIndex = queue_family;
        queue_creation_config.queueCount = 1;
        queue_creation_config.pQueuePriorities = &queue_priority;
        queue_creation_configs.push_back(queue_creation_config);
    }

    // set physical device features to use
    VkPhysicalDeviceFeatures device_features{};

    VkDeviceCreateInfo logical_device_config{};
    logical_device_config.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    logical_device_config.queueCreateInfoCount = static_cast<uint32_t>(queue_creation_configs.size());
    logical_device_config.pQueueCreateInfos = queue_creation_configs.data();
    logical_device_config.pEnabledFeatures = &device_features;
    logical_device_config.enabledExtensionCount = static_cast<uint32_t>(m_device_extensions.size());
    logical_device_config.ppEnabledExtensionNames = m_device_extensions.data();

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
    vkGetDeviceQueue(m_logical_device, indices.present_family.value(), 0, &m_presentation_queue);
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

    bool extensions_supported = checkDeviceExtensionSupport(device);
    bool swapchain_adequate = false;
    if (extensions_supported) {
        SwapChainSupport swapchain_support = getSwapChainSupport(device);
        swapchain_adequate = swapchain_support.isAdequate();
    }

    // demand that we have a complete set of queue families
    if (!queue_families.isComplete() || !extensions_supported || !swapchain_adequate) {
        device_score = 0;
    }
    std::cout <<"\tscore: " << device_score << std::endl;
    return device_score;
}

bool TriangleRenderer::checkDeviceExtensionSupport(VkPhysicalDevice device) {
    // get the available extensions for the logical device
    uint32_t extension_count;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
    std::vector<VkExtensionProperties> available_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

    // identify which extensions are available
    std::set<std::string> required_extensions(m_device_extensions.begin(), m_device_extensions.end());
    for (const auto& extension : available_extensions) {
        // remove the extension form the list of extensions left to find
        required_extensions.erase(extension.extensionName);
        if (required_extensions.empty()) {
            return true;
        }
    }

    return false;
}

TriangleRenderer::SwapChainSupport TriangleRenderer::getSwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupport support_details;

    // get the swap chain capabilities for our target surface
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &support_details.capabilites);

    // get the supported formats
    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &format_count, nullptr);
    if (format_count > 0) {
        support_details.formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &format_count, support_details.formats.data());
    }

    // get the supported presentation modes
    uint32_t presentation_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentation_count, nullptr);
    if (presentation_count > 0) {
        support_details.modes.resize(presentation_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentation_count, support_details.modes.data());
    }

    return support_details;
}

VkSurfaceFormatKHR TriangleRenderer::selectSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats) {
    // VkSurfaceFormatKHR contains a format and colorSpace
    // format specifies color channels and types e.g. VK_FORMAT_B8G8R8A8_SRGB = 8 bits RGB + Alpha
    // colorSpace specifies if SRGB (standard RGB) is supported using VK_COLOR_SPACE_SRGB_NONLINEAR_KHR bit; standard image colorspace

    // for each supported surface format
    for (const auto& format : available_formats) {
        // for now, just choors 8 bit SRGB
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
    }

    // if we don't have our preference, take what we can get
    return available_formats[0];
}

VkPresentModeKHR TriangleRenderer::selectSwapPresentationMode(const std::vector<VkPresentModeKHR>& available_modes) {
    // VkPresentModeKHR has four options which affect how the displayed image is updated:
    //      1) VK_PRESENT_MODE_IMMEDIATE_KHR - submitted images immediatley displayed, can cause tearing
    //      2) VK_PRESENT_MODE_FIFO_KHR - image taken from front of queue when display refreshes, app has to wait if full (akin to VSync)
    //      3) VK_PRESENT_MODE_FIFO_RELAXED_KHR - same as 2 but if queue is empty image shown immediatley, can cause tearing
    //      4) VK_PRESENT_MODE_MAILBOX_KHR - same as 2 but if queue is full images in queue replaced by new ones, referred to as triple buffering
    //

    // for each available mode
    for (const auto& mode : available_modes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return mode;
        }
    }

    // if we don't have our preference, select the most widely supported
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D TriangleRenderer::selectSwapExtent(const VkSurfaceCapabilitiesKHR& capabilties) {
    // default is to match resolution of window by setting width and height in currentExtent to match window
    // some window managers lets us set max::uint32_t to match best within minImageExtent and maxImageExtent
    // GLFW uses pixels and screen coordinates (https://www.glfw.org/docs/latest/intro_guide.html#coordinate_systems)
    // Vulkan works in pixels, for high DPI screens, screen coordiantes and pixels aren't 1:1

    // if we can just use the maximum extent (full screen?)
    if (capabilties.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilties.currentExtent;
    // if we need to select a resolution within the limits (windowed?)
    } else {
        int width, height;
        glfwGetFramebufferSize(m_window, &width, &height);
        VkExtent2D window_extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
        // restrict the window surface to the limits of the swap chain
        window_extent.width = std::clamp(window_extent.width, capabilties.minImageExtent.width, capabilties.maxImageExtent.width);
        window_extent.height = std::clamp(window_extent.height, capabilties.minImageExtent.height, capabilties.maxImageExtent.height);

        return window_extent;
    }
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

        // if it's a presentation queue family
        VkBool32 presentation_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentation_support);
        if (presentation_support) {
            queue_indices.present_family = i;
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

    // ensure we are able to provide validation layers if they are available
    if (m_enable_validation_layers && !checkValidationLayerSupport(m_validation_layers)) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

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
    // destory pipeline layout
    vkDestroyPipelineLayout(m_logical_device, m_pipeline_layout, nullptr);
    vkDestroyRenderPass(m_logical_device, m_render_pass, nullptr);

    // destroy image views
    for (auto view : m_swapchain_image_views) {
        vkDestroyImageView(m_logical_device, view, nullptr);
    }
    vkDestroySwapchainKHR(m_logical_device, m_swapchain, nullptr);
    vkDestroyDevice(m_logical_device, nullptr);
    if (m_enable_validation_layers) {
        // destroy the debug messenger
        destroyDebugUtilsMessengerEXT(m_vulkan_instance, m_debug_messenger, nullptr);
    }
    vkDestroySurfaceKHR(m_vulkan_instance, m_surface, nullptr);
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