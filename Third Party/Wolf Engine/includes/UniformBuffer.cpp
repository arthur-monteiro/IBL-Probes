#include "UniformBuffer.h"
#include "Debug.h"

Wolf::UniformBuffer::UniformBuffer(VkDevice device, VkPhysicalDevice physicalDevice, void* data, VkDeviceSize size)
{
	m_device = device;
	m_physicalDevice = physicalDevice;

	createBuffer(m_device, m_physicalDevice, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_uniformBuffer, m_uniformBufferMemory);

	m_size = size;
	if (size == 0)
		Debug::sendWarning("Initializing uniform buffer with size = 0");
	if (!data)
		Debug::sendError("Invalid data for uniform buffer initialization");
		
	void* pData;
	vkMapMemory(m_device, m_uniformBufferMemory, 0, m_size, 0, &pData);
	memcpy(pData, data, m_size);
	vkUnmapMemory(m_device, m_uniformBufferMemory);
}

Wolf::UniformBuffer::~UniformBuffer()
{
	if (m_size <= 0)
		return;

	vkDestroyBuffer(m_device, m_uniformBuffer, nullptr);
	vkFreeMemory(m_device, m_uniformBufferMemory, nullptr);

	m_size = 0;
}

void Wolf::UniformBuffer::updateData(void* data)
{
	void* pData;
	vkMapMemory(m_device, m_uniformBufferMemory, 0, m_size, 0, &pData);
	memcpy(pData, data, m_size);
	vkUnmapMemory(m_device, m_uniformBufferMemory);
}

void Wolf::UniformBuffer::cleanup()
{
	this->~UniformBuffer();
}
