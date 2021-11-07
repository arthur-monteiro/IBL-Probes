#pragma once
#include "VulkanHelper.h"
#include "FrameBuffer.h"
#include "Attachment.h"
#include "Semaphore.h"
#include "Image.h"
#include "UniformBuffer.h"
#include "Pipeline.h"
#include "VulkanElement.h"
#include "DescriptorSet.h"

namespace Wolf
{

	class ComputePass : public VulkanElement
	{
	public:
		ComputePass(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, std::string computeShader,
			DescriptorSetCreateInfo descriptorSetCreateInfo);

		void create(VkDescriptorPool descriptorPool);
		void record(VkCommandBuffer commandBuffer, VkExtent2D extent, VkExtent3D dispatchGroups);
		
	private:
		std::unique_ptr<Pipeline> m_pipeline;

		// Data
		DescriptorSetCreateInfo m_descriptorSetCreateInfo;

		VkDescriptorSet m_descriptorSet;
		VkDescriptorSetLayout m_descriptorSetLayout;
	};
}
