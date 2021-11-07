#pragma once

#include "AccelerationStructure.h"
#include "VulkanElement.h"
#include "DescriptorSet.h"
#include "Pipeline.h"
#include "ShaderBindingTable.h"

namespace Wolf
{

	class RayTracingPass : public VulkanElement
	{
	public:
		struct RayTracingPassCreateInfo
		{
			std::string raygenShader;
			std::vector<std::string> missShaders;

			struct HitGroup
			{
				std::string closestHitShader = "";
			};
			std::vector<HitGroup> hitGroups;

			DescriptorSetCreateInfo descriptorSetCreateInfo;
		};

		RayTracingPass(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool,
			RayTracingPassCreateInfo rayTracingPassCreateInfo);

		void create(VkDescriptorPool descriptorPool);
		void record(VkCommandBuffer commandBuffer, VkExtent3D extent);

	private:
		void createRayGenShaderStage(std::string raygenShader);
		void addMissShaderStage(std::string missShader);
		void addHitGroup(RayTracingPassCreateInfo::HitGroup hitGroup);
		void buildPipeline();

	private:
		//std::unique_ptr<Pipeline> m_pipeline;

		// Shaders
		std::vector<VkPipelineShaderStageCreateInfo> m_shaderStages;
		std::vector<VkRayTracingShaderGroupCreateInfoNV> m_shaderGroups;

		// Indices
		uint32_t m_rayGenIndex;
		std::vector<uint32_t> m_missIndices;
		std::vector<uint32_t> m_hitGroups;

		// Pipeline info
		DescriptorSetCreateInfo m_descriptorSetCreateInfo;
		VkDescriptorSet m_descriptorSet;
		VkDescriptorSetLayout m_descriptorSetLayout;

		VkPipelineLayout m_pipelineLayout;
		VkPipeline m_pipeline;

		// SBT
		std::unique_ptr<ShaderBindingTable> m_shaderBindingTable;
	};
}

