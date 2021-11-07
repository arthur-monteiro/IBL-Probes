#pragma once

#include "VulkanHelper.h"

namespace Wolf
{
	struct VertexBuffer
	{
		VkBuffer vertexBuffer;
		unsigned int nbVertices;
		VkBuffer indexBuffer;
		unsigned int nbIndices;
	};
	
	template <typename T>
	class Mesh
	{
	public:
		Mesh() = default;
		~Mesh() {}

		void loadFromVertices(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, Queue graphicsQueue, std::vector<T> vertices, std::vector<uint32_t> indices)
		{
			m_vertices = vertices;
			m_indices = indices;

			createVertexBuffer(device, physicalDevice, commandPool, graphicsQueue, sizeof(m_vertices[0]) * m_vertices.size(), m_vertices.data());
			createIndexBuffer(device, physicalDevice, commandPool, graphicsQueue);
		}

		void cleanup(VkDevice device)
		{
			m_vertices.clear();
			m_indices.clear();

			vkDestroyBuffer(device, m_vertexBuffer, nullptr);
			vkFreeMemory(device, m_vertexBufferMemory, nullptr);

			vkDestroyBuffer(device, m_indexBuffer, nullptr);
			vkFreeMemory(device, m_indexBufferMemory, nullptr);
		}

		VertexBuffer getVertexBuffer() const { return { m_vertexBuffer, static_cast<unsigned int>(m_vertices.size()), m_indexBuffer, static_cast<unsigned int>(m_indices.size()) }; }

		const std::vector<T> getVertices() { return m_vertices; }
		const std::vector<uint32_t> getIndices() { return m_indices; }

	private:
		// Vertex
		std::vector<T> m_vertices = {};
		VkBuffer m_vertexBuffer;
		VkDeviceMemory m_vertexBufferMemory;

		// Indices
		std::vector<uint32_t> m_indices;
		VkBuffer m_indexBuffer;
		VkDeviceMemory m_indexBufferMemory;

	private:
		void createVertexBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, Queue graphicsQueue, VkDeviceSize size, void* data)
		{
			VkBuffer stagingBuffer;
			VkDeviceMemory stagingBufferMemory;
			createBuffer(device, physicalDevice, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

			void* tData;
			vkMapMemory(device, stagingBufferMemory, 0, size, 0, &tData);
			std::memcpy(tData, data, static_cast<size_t>(size));
			vkUnmapMemory(device, stagingBufferMemory);

			createBuffer(device, physicalDevice, size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vertexBuffer, m_vertexBufferMemory);

			copyBuffer(device, commandPool, graphicsQueue, stagingBuffer, m_vertexBuffer, size);

			vkDestroyBuffer(device, stagingBuffer, nullptr);
			vkFreeMemory(device, stagingBufferMemory, nullptr);
		}

		void createIndexBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, Queue graphicsQueue)
		{
			const VkDeviceSize bufferSize = sizeof(m_indices[0]) * m_indices.size();

			VkBuffer stagingBuffer;
			VkDeviceMemory stagingBufferMemory;
			createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

			void* data;
			vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
			memcpy(data, m_indices.data(), static_cast<size_t>(bufferSize));
			vkUnmapMemory(device, stagingBufferMemory);

			createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_indexBuffer, m_indexBufferMemory);

			copyBuffer(device, commandPool, graphicsQueue, stagingBuffer, m_indexBuffer, bufferSize);

			vkDestroyBuffer(device, stagingBuffer, nullptr);
			vkFreeMemory(device, stagingBufferMemory, nullptr);
		}
	};
}
