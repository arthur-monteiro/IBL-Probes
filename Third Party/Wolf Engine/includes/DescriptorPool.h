#pragma once

#include "VulkanHelper.h"

namespace Wolf
{
	class DescriptorPool
	{
	public:
		DescriptorPool() = default;

		void addUniformBuffer(unsigned int count) { m_uniformBufferCount += count; };
		void addCombinedImageSampler(unsigned int count) { m_combinedImageSamplerCount += count; }
		void addStorageImage(unsigned int count) { m_storageImageCount += count; }
		void addSampler(unsigned int count) { m_samplerCount += count; }
		void addSampledImage(unsigned int count) { m_sampledImageCount += count; }
		void addStorageBuffer(unsigned int count) { m_storageBufferCount += count; }
		void addAccelerationStructure(unsigned int count) { m_accelerationStructureCount += count; }
		
		void allocate(VkDevice device);

		void cleanup(VkDevice device);

		VkDescriptorPool getDescriptorPool() const { return m_descriptorPool; }

	private:
		static void addDescriptorPoolSize(VkDescriptorType descriptorType, uint32_t descriptorCount, std::vector<VkDescriptorPoolSize>& poolSizes);

	private:
		unsigned int m_uniformBufferCount = 0;
		unsigned int m_combinedImageSamplerCount = 0;
		unsigned int m_storageImageCount = 0;
		unsigned int m_samplerCount = 0;
		unsigned int m_sampledImageCount = 0;
		unsigned int m_storageBufferCount = 0;
		unsigned int m_accelerationStructureCount = 0;
		
		VkDescriptorPool m_descriptorPool;
	};


}
