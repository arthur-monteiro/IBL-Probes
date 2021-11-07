#include "Model3D.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <unordered_map>
#include <array>
#include <glm/gtx/intersect.hpp>

#include "Debug.h"

Wolf::Model3D::~Model3D()
{
	for (auto& m_mesh : m_meshes)
		m_mesh.cleanup(m_device);
}

int Wolf::Model3D::addMeshFromVertices(void* vertices, uint32_t vertexCount, size_t vertexSize,
                                       std::vector<uint32_t> indices)
{
	Mesh<Vertex3D> mesh;
	std::vector<Vertex3D> vVertices(vertexCount);
	for (size_t i(0); i < vertexCount; ++i)
	{
		vVertices[i] = reinterpret_cast<Vertex3D*>(vertices)[i];
	}
	mesh.loadFromVertices(m_device, m_physicalDevice, m_commandPool, m_graphicsQueue, vVertices, std::move(indices));

	m_meshes.push_back(mesh);

	return static_cast<int>(m_meshes.size() - 1);
}

void Wolf::Model3D::loadObj(ModelLoadingInfo modelLoadingInfo)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err, warn;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, modelLoadingInfo.filename.c_str(), modelLoadingInfo.mtlFolder.c_str()))
		throw std::runtime_error(err);

#ifndef NDEBUG
	if (!err.empty())
		std::cout << "[Loading objet file] Error : " << err << " for " << modelLoadingInfo.filename << " !" << std::endl;
	if (!warn.empty())
		std::cout << "[Loading objet file]  Warning : " << warn << " for " << modelLoadingInfo.filename << " !" << std::endl;
#endif // !NDEBUG

	std::unordered_map<Vertex3D, uint32_t> uniqueVertices = {};
	std::vector<Vertex3D> vertices;
	std::vector<uint32_t> indices;

	std::vector<uint32_t> lastIndices;

	for (const auto& shape : shapes)
	{
		int numVertex = 0;
		for (const auto& index : shape.mesh.indices)
		{
			Vertex3D vertex = {};

			int materialID = shape.mesh.material_ids[numVertex / 3];

			if (modelLoadingInfo.loadMaterials && materialID < 0)
				continue;

			vertex.pos =
			{
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.texCoord =
			{
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			if(attrib.normals.size() <= 3 * index.normal_index + 2)
				vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
			else
			{
				vertex.normal =
				{
						attrib.normals[3 * index.normal_index + 0],
						attrib.normals[3 * index.normal_index + 1],
						attrib.normals[3 * index.normal_index + 2]
				};
			}

			if(!modelLoadingInfo.loadMaterials)
				vertex.materialID = 0;
			else vertex.materialID = shape.mesh.material_ids[numVertex / 3];

			if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}

			if (std::find(m_toBeLast.begin(), m_toBeLast.end(), materialID) == m_toBeLast.end())
				indices.push_back(uniqueVertices[vertex]);
			else
				lastIndices.push_back(uniqueVertices[vertex]);

			numVertex++;
		}
	}

	for (int i(0); i < lastIndices.size(); ++i)
		indices.push_back(lastIndices[i]);

	std::array<Vertex3D, 3> tempTriangle{};
	for (size_t i(0); i <= indices.size(); ++i)
	{
		if (i != 0 && i % 3 == 0)
		{
			glm::vec3 edge1 = tempTriangle[1].pos - tempTriangle[0].pos;
			glm::vec3 edge2 = tempTriangle[2].pos - tempTriangle[0].pos;
			glm::vec2 deltaUV1 = tempTriangle[1].texCoord - tempTriangle[0].texCoord;
			glm::vec2 deltaUV2 = tempTriangle[2].texCoord - tempTriangle[0].texCoord;

			float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

			glm::vec3 tangent;
			tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
			tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
			tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
			tangent = glm::normalize(tangent);

			for (size_t j(i - 1); j > i - 4; --j)
				vertices[indices[j]].tangent = tangent;
		}

		if (i == indices.size())
			break;

		tempTriangle[i % 3] = vertices[indices[i]];
	}

	if(modelLoadingInfo.loadMaterials)
	{
		m_images.resize(materials.size() * 5);
		int indexTexture = 0;
		for (int i(0); i < materials.size(); ++i)
		{
			m_images[indexTexture++] = std::make_unique<Image>(m_device, m_physicalDevice, m_commandPool, m_graphicsQueue, getTexName(materials[i].diffuse_texname, modelLoadingInfo.mtlFolder));
			m_images[indexTexture++] = std::make_unique<Image>(m_device, m_physicalDevice, m_commandPool, m_graphicsQueue, getTexName(materials[i].bump_texname, modelLoadingInfo.mtlFolder));
			m_images[indexTexture++] = std::make_unique<Image>(m_device, m_physicalDevice, m_commandPool, m_graphicsQueue, getTexName(materials[i].specular_highlight_texname, modelLoadingInfo.mtlFolder));
			m_images[indexTexture++] = std::make_unique<Image>(m_device, m_physicalDevice, m_commandPool, m_graphicsQueue, getTexName(materials[i].ambient_texname, modelLoadingInfo.mtlFolder));
			m_images[indexTexture++] = std::make_unique<Image>(m_device, m_physicalDevice, m_commandPool, m_graphicsQueue, getTexName(materials[i].ambient_texname, modelLoadingInfo.mtlFolder));
		}
	}

	if(!m_images.empty())
		m_sampler = std::make_unique<Sampler>(m_device, VK_SAMPLER_ADDRESS_MODE_REPEAT, static_cast<float>(m_images[0]->getMipLevels()), VK_FILTER_LINEAR);

	Mesh<Vertex3D> mesh;
	mesh.loadFromVertices(m_device, m_physicalDevice, m_commandPool, m_graphicsQueue, vertices, indices);
	m_meshes.push_back(mesh);
	
	Debug::sendInfo("Model loaded with " + std::to_string(indices.size() / 3) + " triangles");
}

bool Wolf::Model3D::checkIntersection(glm::vec3 point1, glm::vec3 point2)
{
	for (int i(0); i < m_meshes.size(); ++i)
	{
		const std::vector<uint32_t> indices = m_meshes[i].getIndices();
		const std::vector<Vertex3D> vertices = m_meshes[i].getVertices();
		for (int j(0); j < indices.size(); j += 3)
		{
			glm::vec3 p1 = vertices[indices[j]].pos;
			glm::vec3 p2 = vertices[indices[j + 1]].pos;
			glm::vec3 p3 = vertices[indices[j + 2]].pos;

			glm::vec3 bary;
			bool intersect = glm::intersectRayTriangle(point1, point2 - point1, p1, p2, p3, bary);
			glm::vec3 intersectPosition = point1 + (point2 - point1) * bary.z;

			if (intersect && glm::dot(point2 - point1, intersectPosition - point1) < glm::dot(point2 - point1, point2 - point1))
				return true;
		}
	}

	return false;
}

std::vector<Wolf::VertexBuffer> Wolf::Model3D::getVertexBuffers() const
{
	std::vector<VertexBuffer> vertexBuffers;

	for (auto& m_mesh : m_meshes)
	{
		vertexBuffers.push_back(m_mesh.getVertexBuffer());
	}

	return vertexBuffers;
}
