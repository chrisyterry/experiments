#include "vulkan/pipeline_utils.hpp"
#include <fstream>

std::vector<char> GraphicsPipelineFactory::readBinaryFile(const std::string& path) {
    // open file at end to get file size
    std::ifstream file(path, std::ios::ate | std::ios::binary); // std::ios::ate opens and immediately goes to the end of the file

    if (!file.is_open()) {
        throw std::runtime_error("Could not open" + path);
    }

    // allocate a vector of the required file size
    std::vector<char> buffer(file.tellg());

    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    file.close();

    return buffer;
}

std::unique_ptr<vk::raii::ShaderModule> GraphicsPipelineFactory::createShaderModule(std::shared_ptr<vk::raii::Device> logical_device, const std::vector<char>& byte_code) const {
    vk::ShaderModuleCreateInfo create_info{
        .codeSize = byte_code.size() * sizeof(char),
        .pCode    = reinterpret_cast<const uint32_t*>(byte_code.data())
    };

    std::unique_ptr<vk::raii::ShaderModule> shader_module = std::make_unique<vk::raii::ShaderModule>(*logical_device, create_info);

    return std::move(shader_module);
}

void GraphicsPipelineFactory::createGraphicsPipeline(std::shared_ptr<vk::raii::Device> logical_device) {
    
    // create the shader module for our shaders
    std::vector<char> shader_code = readBinaryFile("shaders/nu_triangle_shaders.spv");
    std::unique_ptr<vk::raii::ShaderModule> shader_module = createShaderModule(logical_device, shader_code);

    // setup vertex shader
    vk::PipelineShaderStageCreateInfo vertex_shader_create_info{
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = *shader_module,
        .pName = "vertMain", // entry point for shader
        //.pSpecializationInfo - lets you set shader constants for reconfiguring your shaders
    };

    // setup fragment shader
    vk::PipelineShaderStageCreateInfo fragment_shader_create_info{
        .stage = vk::ShaderStageFlagBits::eFragment,
        .module = *shader_module,
        .pName = "vertFrag",
    };

    // programmable pipeline stages
    vk::PipelineShaderStageCreateInfo programmable_stages[] = {vertex_shader_create_info, fragment_shader_create_info};
}