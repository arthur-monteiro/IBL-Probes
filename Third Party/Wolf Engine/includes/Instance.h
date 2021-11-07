#pragma once

#include "Buffer.h"
#include "VulkanElement.h"

namespace Wolf
{
	struct InstanceBuffer
	{
		VkBuffer instanceBuffer = VK_NULL_HANDLE;
		uint32_t nInstances = 0;
	};

	class InstanceParent : public VulkanElement
	{
	public:
		~InstanceParent();
		
	protected:
		InstanceParent() = default;
		
	protected:
		VkBuffer m_instanceBuffer = nullptr;
		VkDeviceMemory m_instanceBufferMemory = nullptr;
	};
	
	template <typename T>
	class Instance : public InstanceParent
	{
	public:
		Instance(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool,
			Queue graphicsQueue);
		~Instance() = default;

		void loadFromVector(std::vector<T> data);
		void createFromBuffer(Buffer* buffer);

		void cleanup(VkDevice device);

	public: // Getters
		InstanceBuffer getInstanceBuffer() const { return { m_instanceBuffer, static_cast<uint32_t>(m_instances.size()) }; }

	private:
		std::vector<T> m_instances;
	};

	template <typename T>
	Instance<T>::Instance(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool,
		Queue graphicsQueue) : InstanceParent()
	{
		m_device = device;
		m_physicalDevice = physicalDevice;
		m_commandPool = commandPool;
		m_graphicsQueue = graphicsQueue;
	}

	template <typename T>
	void Instance<T>::loadFromVector(std::vector<T> data)
	{
		m_instances = std::move(data);
		const VkDeviceSize bufferSize = sizeof(m_instances[0]) * m_instances.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(m_device, m_physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* pData;
		vkMapMemory(m_device, stagingBufferMemory, 0, bufferSize, 0, &pData);
		memcpy(pData, m_instances.data(), bufferSize);
		vkUnmapMemory(m_device, stagingBufferMemory);

		createBuffer(m_device, m_physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_instanceBuffer, m_instanceBufferMemory);

		copyBuffer(m_device, m_commandPool, m_graphicsQueue, stagingBuffer, m_instanceBuffer, bufferSize);

		vkDestroyBuffer(m_device, stagingBuffer, nullptr);
		vkFreeMemory(m_device, stagingBufferMemory, nullptr);
	}

	template<typename T>
	void Instance<T>::createFromBuffer(Buffer* buffer)
	{
		m_instanceBuffer = buffer->getBuffer();
	}

	template <typename T>
	void Instance<T>::cleanup(VkDevice device)
	{
		m_instances.clear();
	}
}
