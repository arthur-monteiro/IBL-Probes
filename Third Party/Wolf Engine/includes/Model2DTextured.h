#pragma once

#include "Model.h"
#include "Mesh.h"
#include "InputVertexTemplate.h"

namespace Wolf
{
	class Model2DTextured : public Wolf::Model
	{
	public:
		Model2DTextured(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, Queue graphicsQueue, InputVertexTemplate inputVertexTemplate) : Model(device, physicalDevice, commandPool, graphicsQueue, inputVertexTemplate) {};
		~Model2DTextured();
		
		int addMeshFromVertices(void* vertices, uint32_t vertexCount, size_t vertexSize, std::vector<uint32_t> indices) override;

		std::vector<Wolf::VertexBuffer> getVertexBuffers() const;

	private:
		std::vector<Wolf::Mesh<Vertex2DTextured>> m_meshes;
	};
}