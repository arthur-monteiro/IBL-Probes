#pragma once

#include "Mesh.h"
#include "InputVertexTemplate.h"
#include "Image.h"
#include "Sampler.h"

namespace Wolf
{
	class Model
	{
	public:
		struct ModelCreateInfo
		{
			InputVertexTemplate inputVertexTemplate;
		};

		Model(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, Queue graphicsQueue, InputVertexTemplate inputVertexTemplate);
		virtual ~Model() = default;

		virtual int addMeshFromVertices(void* vertices, uint32_t vertexCount, size_t vertexSize, std::vector<uint32_t> indices) { return -1; }
		virtual bool checkIntersection(glm::vec3 point1, glm::vec3 point2) { return false; };

		struct ModelLoadingInfo
		{
			// Files paths
			std::string filename;
			std::string mtlFolder;

			// Default options
			glm::vec3 defaultNormal = glm::vec3(0.0f, 1.0f, 0.0f);

			// Material Options
			bool loadMaterials = true;
		};
		virtual void loadObj(ModelLoadingInfo modelLoadingInfo) {}

		virtual std::vector<VertexBuffer> getVertexBuffers() const { return {}; }
		virtual size_t getNumberOfImages() const { return m_images.size(); }
		virtual Sampler* getSampler() const { return m_sampler.get(); }
		virtual std::vector<Image*> getImages() const;
		
	protected:
		VkDevice m_device;
		VkPhysicalDevice m_physicalDevice;
		VkCommandPool m_commandPool;
		Queue m_graphicsQueue;

		InputVertexTemplate m_inputVertexTemplate;

		std::vector<std::unique_ptr<Image>> m_images;
		std::unique_ptr<Sampler> m_sampler;
	};
}
