#include "Model2D.h"

int Wolf::Model2D::addMeshFromVertices(void* vertices, uint32_t vertexCount, size_t vertexSize, std::vector<uint32_t> indices)
{
	Mesh<Vertex2D> mesh;
	std::vector<Vertex2D> vVertices(vertexCount);
	for(size_t i(0); i < vertexCount; ++i)
	{
		vVertices[i] = reinterpret_cast<Vertex2D*>(vertices)[i];
	}
	mesh.loadFromVertices(m_device, m_physicalDevice, m_commandPool, m_graphicsQueue, vVertices, std::move(indices));

	m_meshes.push_back(mesh);

	return static_cast<int>(m_meshes.size() - 1);
}

Wolf::Model2D::~Model2D()
{
	for (auto& m_mesh : m_meshes)
		m_mesh.cleanup(m_device);
}

std::vector<Wolf::VertexBuffer> Wolf::Model2D::getVertexBuffers() const
{
	std::vector<VertexBuffer> vertexBuffers;

	for (auto& m_mesh : m_meshes)
	{
		vertexBuffers.push_back(m_mesh.getVertexBuffer());
	}

	return vertexBuffers;
}
