#pragma once

#include "Model.h"
#include "Mesh.h"
#include "InputVertexTemplate.h"

namespace Wolf
{
	class Model3D : public Model
	{
	public:
		Model3D(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, Queue graphicsQueue, InputVertexTemplate inputVertexTemplate) : Model(device, physicalDevice, commandPool, graphicsQueue, inputVertexTemplate) {};
		~Model3D();

		int addMeshFromVertices(void* vertices, uint32_t vertexCount, size_t vertexSize, std::vector<uint32_t> indices);
		void loadObj(ModelLoadingInfo modelLoadingInfo);

		bool checkIntersection(glm::vec3 point1, glm::vec3 point2);

		std::vector<Wolf::VertexBuffer> getVertexBuffers() const;

	private:
		static std::string getTexName(std::string texName, std::string folder);

	private:
		std::vector<Wolf::Mesh<Vertex3D>> m_meshes;
		std::vector<int> m_toBeLast = { 2, 19, 0 }; // flower contains alpha blending
	};

	inline std::string Model3D::getTexName(std::string texName, std::string folder)
	{
		return texName != "" ? folder + "/" + texName : "Textures/white_pixel.jpg";
	}
}
