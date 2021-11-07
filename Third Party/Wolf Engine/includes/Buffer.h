#pragma once

#include "VulkanElement.h"

namespace Wolf
{
	class Buffer : public VulkanElement
	{
	public:
		Buffer(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, Queue graphicsQueue, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryPropertyFlags);
		~Buffer();

		void copy(Buffer* source);
		void map(void** data);
		void unmap();

		VkBuffer getBuffer() { return m_buffer; }
		VkDeviceSize getSize() { return m_size; }

	private:
		VkBuffer m_buffer;
		VkDeviceMemory m_bufferMemory;
		VkDeviceSize m_size;

		VkMemoryPropertyFlags m_memoryPropertyFlags;

#ifndef NDEBUG
		bool m_isMapped = false;
#endif
	};
}
