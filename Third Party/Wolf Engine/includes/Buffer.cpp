#include "Buffer.h"
#include "Debug.h"

Wolf::Buffer::Buffer(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, Queue graphicsQueue, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryPropertyFlags)
{
	m_device = device;
	m_size = size;
	m_commandPool = commandPool;
	m_graphicsQueue = graphicsQueue;

	m_memoryPropertyFlags = memoryPropertyFlags;

	createBuffer(device, physicalDevice, size, usage, memoryPropertyFlags, m_buffer, m_bufferMemory);
}

Wolf::Buffer::~Buffer()
{
	vkDestroyBuffer(m_device, m_buffer, nullptr);
	vkFreeMemory(m_device, m_bufferMemory, nullptr);
}

void Wolf::Buffer::copy(Buffer* source)
{
	copyBuffer(m_device, m_commandPool, m_graphicsQueue, source->getBuffer(), m_buffer, m_size);
}

void Wolf::Buffer::map(void** data)
{
#ifndef NDEBUG
	if (!(m_memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
		Debug::sendError("Can't map a non host visible buffer");
	m_isMapped = true;
#endif
	vkMapMemory(m_device, m_bufferMemory, 0, m_size, 0, data);
}

void Wolf::Buffer::unmap()
{
#ifndef NDEBUG
	if (!m_isMapped)
		Debug::sendError("Trying to unmap a non mapped buffer");
	m_isMapped = false;
#endif
	vkUnmapMemory(m_device, m_bufferMemory);
}
