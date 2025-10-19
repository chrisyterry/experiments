#pragma once
#include <vector>
#include <cstdint>
#include <string>
#include <memory>

#include <vulkan/vulkan_raii.hpp>

/**
 * @brief factory class for creating graphics pipelines
 */
class GraphicsPipelineFactory {

    public:
        /**
         * @brief create a graphics pipeline
         * 
         * @param the logical device to create the pipeline for
         */
        void createGraphicsPipeline(std::shared_ptr<vk::raii::Device> logical_device);
    private:
      /**
       * @brief loads a binary file into a byte array
       * 
       * @param path the path to the binary file
       */
      std::vector<char> readBinaryFile(const std::string& path);

      /**
       * @brief create a shader module from SPIRV byte code
       * 
       * @param logical_device the logical device to use to create the shader module
       * @param byte_code byte code for the shader module
       * 
       * @return the shader module
       */
      std::unique_ptr<vk::raii::ShaderModule> createShaderModule(std::shared_ptr<vk::raii::Device> logical_device, const std::vector<char>& byte_code) const;
};