#pragma once

#include "Model.h"
#include "Mesh.h"
#include "InputVertexTemplate.h"

namespace Wolf
{
	class Model2D : public Wolf::Model
	{
	public:
		Model2D(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, Queue graphicsQueue, InputVertexTemplate inputVertexTemplate) : Model(device, physicalDevice, commandPool, graphicsQueue, inputVertexTemplate) {};
		~Model2D();
		
		int addMeshFromVertices(void* vertices, uint32_t vertexCount, size_t vertexSize, std::vector<uint32_t> indices);

		std::vector<Wolf::VertexBuffer> getVertexBuffers() const;

	private:
		std::vector<Wolf::Mesh<Vertex2D>> m_meshes;
	};
}