#include "DescriptorPool.h"
#include <array>

void Wolf::DescriptorPool::allocate(VkDevice device)
{
	uint32_t maxSets = m_uniformBufferCount + m_combinedImageSamplerCount + m_storageImageCount + m_samplerCount + m_sampledImageCount + m_storageBufferCount;
	if (maxSets == 0)
		return;
	
	std::vector<VkDescriptorPoolSize> poolSizes{};
	addDescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_uniformBufferCount, poolSizes);
	addDescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_combinedImageSamplerCount, poolSizes);
	addDescriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, m_storageImageCount, poolSizes);
	addDescriptorPoolSize(VK_DESCRIPTOR_TYPE_SAMPLER, m_samplerCount, poolSizes);
	addDescriptorPoolSize(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, m_sampledImageCount, poolSizes);
	addDescriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_storageBufferCount, poolSizes);
	addDescriptorPoolSize(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV, m_accelerationStructureCount, poolSizes);
	
	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = maxSets;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
		throw std::runtime_error("Error : create descriptor pool");
}

void Wolf::DescriptorPool::cleanup(VkDevice device)
{
	vkDestroyDescriptorPool(device, m_descriptorPool, nullptr);
}

void Wolf::DescriptorPool::addDescriptorPoolSize(VkDescriptorType descriptorType, uint32_t descriptorCount,
	std::vector<VkDescriptorPoolSize>& poolSizes)
{
	if(descriptorCount > 0)
	{
		VkDescriptorPoolSize poolSize;
		poolSize.type = descriptorType;
		poolSize.descriptorCount = descriptorCount;
		
		poolSizes.emplace_back(poolSize);
	}
}
