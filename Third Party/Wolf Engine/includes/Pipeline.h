#pragma once

#include "VulkanHelper.h"
#include <array>

namespace Wolf
{
	struct ShaderCreateInfo
	{
		std::string filename = "";
		std::string fileContent = "";
		std::string entryPointName = "main";
		VkShaderStageFlagBits stage;
	};
	
	struct RenderingPipelineCreateInfo
	{
		VkRenderPass renderPass;
		
		// Programming stages
		std::vector<ShaderCreateInfo> shaderCreateInfos;

		// IA
		std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
		std::vector<VkVertexInputBindingDescription> vertexInputBindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions;
		VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		// Viewport
		VkExtent2D extent = {0, 0 };
		std::array<float, 2> viewportScale = { 1.0f, 1.0f };
		std::array<float, 2> viewportOffset = { 0.0f, 0.0f };

		// Rasterization
		VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
		VkCullModeFlags cullMode = VK_CULL_MODE_NONE;
		bool enableConservativeRasterization = false;
		float maxExtraPrimitiveOverestimationSize = 0.75f;

		// Multi-sampling
		VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

		// Color Blend
		std::vector<bool> alphaBlending;
		bool addColors = false;

		// Depth testing
		VkBool32 enableDepthTesting = VK_TRUE;

		// Tessellation
		uint32_t patchControlPoint = 0;
	};
	
	class Pipeline
	{
	public:
		Pipeline(VkDevice device, RenderingPipelineCreateInfo renderingPipelineCreateInfo);
		Pipeline(VkDevice device, std::string computeShader, VkDescriptorSetLayout* descriptorSetLayout);
		~Pipeline();

		VkPipeline getPipeline() const { return m_pipeline; }
		VkPipelineLayout getPipelineLayout() const { return m_pipelineLayout; }

	private:
		void createPipelineLayout(VkDescriptorSetLayout* descriptorSetLayout);
		
	private:
		VkDevice m_device;
		
		VkPipelineLayout m_pipelineLayout;
		VkPipeline m_pipeline;

	public:
		static std::vector<char> readFile(const std::string& filename);
		static VkShaderModule createShaderModule(const std::vector<char>& code, VkDevice device);
	};


}
