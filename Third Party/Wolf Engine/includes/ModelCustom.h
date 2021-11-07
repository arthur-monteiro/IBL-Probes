#pragma once

#include "Model.h"
#include "Mesh.h"
#include "InputVertexTemplate.h"

namespace Wolf
{
	template<typename T>
	class ModelCustom : public Wolf::Model
	{
	public:
		ModelCustom(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, Queue graphicsQueue, InputVertexTemplate inputVertexTemplate) :
			Model(device, physicalDevice, commandPool, graphicsQueue, inputVertexTemplate) {};
		~ModelCustom();

		int addMeshFromVertices(void* vertices, uint32_t vertexCount, size_t vertexSize, std::vector<uint32_t> indices) override;

		std::vector<Wolf::VertexBuffer> getVertexBuffers() const;

	private:
		std::vector<Wolf::Mesh<T>> m_meshes;
	};

	template <typename T>
	ModelCustom<T>::~ModelCustom()
	{
		for (auto& m_mesh : m_meshes)
			m_mesh.cleanup(m_device);
	}

	template <typename T>
	int ModelCustom<T>::addMeshFromVertices(void* vertices, uint32_t vertexCount, size_t vertexSize,
		std::vector<uint32_t> indices)
	{
		Mesh<T> mesh;
		std::vector<T> vVertices(vertexCount);
		for (size_t i(0); i < vertexCount; ++i)
		{
			vVertices[i] = static_cast<T*>(vertices)[i];
		}
		mesh.loadFromVertices(m_device, m_physicalDevice, m_commandPool, m_graphicsQueue, vVertices, std::move(indices));

		m_meshes.push_back(mesh);

		return static_cast<int>(m_meshes.size() - 1);
	}

	template <typename T>
	std::vector<Wolf::VertexBuffer> ModelCustom<T>::getVertexBuffers() const
	{
		std::vector<VertexBuffer> vertexBuffers;

		for (auto& m_mesh : m_meshes)
		{
			vertexBuffers.push_back(m_mesh.getVertexBuffer());
		}

		return vertexBuffers;
	}
}
