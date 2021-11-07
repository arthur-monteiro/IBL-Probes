#include "Model2DTextured.h"

Wolf::Model2DTextured::~Model2DTextured()
{
	for (auto& m_mesh : m_meshes)
		m_mesh.cleanup(m_device);
}

int Wolf::Model2DTextured::addMeshFromVertices(void* vertices, uint32_t vertexCount, size_t vertexSize,
	std::vector<uint32_t> indices)
{
	Mesh<Vertex2DTextured> mesh;
	std::vector<Vertex2DTextured> vVertices(vertexCount);
	for (size_t i(0); i < vertexCount; ++i)
	{
		vVertices[i] = reinterpret_cast<Vertex2DTextured*>(vertices)[i];
	}
	mesh.loadFromVertices(m_device, m_physicalDevice, m_commandPool, m_graphicsQueue, vVertices, std::move(indices));

	m_meshes.push_back(mesh);

	return static_cast<int>(m_meshes.size() - 1);
}

std::vector<Wolf::VertexBuffer> Wolf::Model2DTextured::getVertexBuffers() const
{
	std::vector<VertexBuffer> vertexBuffers;

	for (auto& m_mesh : m_meshes)
	{
		vertexBuffers.push_back(m_mesh.getVertexBuffer());
	}

	return vertexBuffers;
}
