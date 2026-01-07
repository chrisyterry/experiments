#include "vulkan/pipeline_utils.hpp"
#include <fstream>
#include <iostream>

std::vector<char> GraphicsPipelineFactory::readBinaryFile(const std::string& path) {
    // open file at end to get file size
    std::ifstream file(path, std::ios::ate | std::ios::binary);  // std::ios::ate opens and immediately goes to the end of the file

    if (!file.is_open()) {
        throw std::runtime_error("Could not open " + path);
    }

    // allocate a vector of the required file size
    std::vector<char> buffer(file.tellg());

    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    file.close();

    return buffer;
}

std::unique_ptr<vk::raii::ShaderModule> GraphicsPipelineFactory::createShaderModule(std::shared_ptr<LogicalDevice> logical_device, const std::vector<char>& byte_code) const {
    vk::ShaderModuleCreateInfo create_info{
        .codeSize = byte_code.size() * sizeof(char),
        .pCode    = reinterpret_cast<const uint32_t*>(byte_code.data())
    };

    std::unique_ptr<vk::raii::ShaderModule> shader_module = std::make_unique<vk::raii::ShaderModule>(*(logical_device->device), create_info);

    return std::move(shader_module);
}

std::unique_ptr<vk::raii::Pipeline> GraphicsPipelineFactory::createGraphicsPipeline(std::shared_ptr<LogicalDevice> logical_device, std::shared_ptr<SwapChain> swapchain, const std::string& shader_path) {

    // create the shader module for our shaders
    std::vector<char>                       shader_code   = readBinaryFile(shader_path);
    std::unique_ptr<vk::raii::ShaderModule> shader_module = createShaderModule(logical_device, shader_code);

    // setup vertex shader
    vk::PipelineShaderStageCreateInfo vertex_shader_create_info{
        .stage  = vk::ShaderStageFlagBits::eVertex,
        .module = *shader_module,
        .pName  = "vertMain",  // entry point for shader
        //.pSpecializationInfo - lets you set shader constants for reconfiguring your shaders
    };

    // setup fragment shader
    vk::PipelineShaderStageCreateInfo fragment_shader_create_info{
        .stage  = vk::ShaderStageFlagBits::eFragment,
        .module = *shader_module,
        .pName  = "fragMain",
    };

    // programmable pipeline stages - currently causes a double linked list corruption when the window is closed?
    vk::PipelineShaderStageCreateInfo programmable_stages[] = { 
        vertex_shader_create_info, 
        fragment_shader_create_info
    };

    // specify vertex data
    vk::PipelineVertexInputStateCreateInfo vertex_input_info{
        // .pVertexBindingDescriptions
        // .pVertexAttributeDescriptions
    };

    // specify input assembly parameters
    vk::PipelineInputAssemblyStateCreateInfo input_assembly{
        .topology = vk::PrimitiveTopology::eTriangleList
    };

    // dynamic states (some GPUs allow multiple scissors and viewports but have to enable feature)
    std::vector<vk::DynamicState> dynamic_states{
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };
    vk::PipelineDynamicStateCreateInfo dynamic_state{
        .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
        .pDynamicStates    = dynamic_states.data()
    };

    // region of framebuffer to render to
    vk::Viewport viewport{ 0.0f, 0.0f, static_cast<float>(swapchain->extent.width), static_cast<float>(swapchain->extent.height), 0.0f, 1.0f };  // last two values are framebuffer depth limits
    vk::Rect2D   scissor{ vk::Offset2D{ 0, 0 }, swapchain->extent };
    vk::PipelineViewportStateCreateInfo viewport_state{
        .viewportCount = 1,
        .scissorCount  = 1
    };

    // rasterizer
    vk::PipelineRasterizationStateCreateInfo rasterizer{
        .depthClampEnable        = vk::False,  // fragments beyond near and far clipping planes get clamped to the planes instead of being discarded (shadow mapping)
        .rasterizerDiscardEnable = vk::False,  // if true disables all framebuffer output
        .polygonMode             = vk::PolygonMode::eFill,  // modes other than fill require enabling GPU feature
        .cullMode                = vk::CullModeFlagBits::eBack,  // whether to cull front, back, both or non of the faces
        .frontFace               = vk::FrontFace::eClockwise,  // vertex order for front faces
        .depthBiasEnable         = vk::False,  // can alter depth values by constant or using a slope (sometimes used for shadow mapping)
        .depthBiasSlopeFactor    = 1.0f,
        .lineWidth               = 1.0f  // thickness of lines in number of fragments; have to enable wide lines GPU feature to use value greater than 1
    };

    // multi-sampling
    vk::PipelineMultisampleStateCreateInfo multisampling{
        .rasterizationSamples = vk::SampleCountFlagBits::e1,  // one sample
        .sampleShadingEnable  = vk::False
    };

    // color blending - set to alpha blend
    vk::PipelineColorBlendAttachmentState color_blend_attachment{
        // configuration per attached framebuffer
        .blendEnable         = vk::True,  // if blending is disabled, overwrite old color with new color
        .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
        .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
        .colorBlendOp        = vk::BlendOp::eAdd,
        .srcAlphaBlendFactor = vk::BlendFactor::eOne,
        .dstAlphaBlendFactor = vk::BlendFactor::eZero,
        .alphaBlendOp        = vk::BlendOp::eAdd,
        .colorWriteMask      = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA  // mask specifying which colors get passed through
    };
    vk::PipelineColorBlendStateCreateInfo color_blending{ // global color blending settings
                                                          .logicOpEnable   = vk::False,  // disable blending
                                                          .logicOp         = vk::LogicOp::eCopy,
                                                          .attachmentCount = 1,
                                                          .pAttachments    = &color_blend_attachment
    };

    // pipeline layout - specify uniform (shared constant) shader values here
    vk::PipelineLayoutCreateInfo pipeline_layout_info{
         .setLayoutCount = 0, 
         .pushConstantRangeCount = 0
    };
    vk::raii::PipelineLayout pipeline_layout = vk::raii::PipelineLayout(*(logical_device->device), pipeline_layout_info);

    // for dynamic rendering, specify format of the framebuffers that will be attached
    vk::PipelineRenderingCreateInfo pipeline_rendering_create_info{
        .colorAttachmentCount    = 1,
        .pColorAttachmentFormats = &swapchain->format
    };

    // graphics pipeline creation
    vk::GraphicsPipelineCreateInfo pipeline_info{
        .pNext               = &pipeline_rendering_create_info,
        .stageCount          = 2,
        .pStages             = programmable_stages,
        .pVertexInputState   = &vertex_input_info,
        .pInputAssemblyState = &input_assembly,
        .pViewportState      = &viewport_state,
        .pRasterizationState = &rasterizer,
        .pMultisampleState   = &multisampling,
        .pColorBlendState    = &color_blending,
        .pDynamicState       = &dynamic_state,
        .layout              = pipeline_layout,
        .renderPass          = nullptr
    };

    // create the pipeline
    return std::make_unique<vk::raii::Pipeline>(*logical_device->device, nullptr, pipeline_info);
}