#pragma once

#include "VulkanElement.h"

namespace Wolf
{
	class UniformBuffer : public VulkanElement
	{
	public:
		UniformBuffer(VkDevice device, VkPhysicalDevice physicalDevice, void* data, VkDeviceSize size);
		~UniformBuffer();

		void updateData(void* data);

		void cleanup();

		// Getters
	public:
		VkBuffer getUniformBuffer() const { return m_uniformBuffer; }
		VkDeviceSize getSize() const { return m_size; }

	private:
		VkBuffer m_uniformBuffer;
		VkDeviceMemory m_uniformBufferMemory;

		VkDeviceSize m_size = 0;
	};
}
