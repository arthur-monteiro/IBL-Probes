#pragma once

#include "VulkanHelper.h"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

namespace Wolf
{
	enum class InputVertexTemplate { NO, POSITION_2D, POSITION_TEXTURECOORD_2D, POSITION_TEXTURECOORD_ID_2D, FULL_3D_MATERIAL };

	struct Vertex2D
	{
		glm::vec2 pos;

		static VkVertexInputBindingDescription getBindingDescription(uint32_t binding)
		{
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = binding;
			bindingDescription.stride = sizeof(Vertex2D);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions(uint32_t binding)
		{
			std::vector<VkVertexInputAttributeDescription> attributeDescriptions(1);

			attributeDescriptions[0].binding = binding;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(Vertex2D, pos);

			return attributeDescriptions;
		}

		bool operator==(const Vertex2D& other) const
		{
			return pos == other.pos;
		}
	};

	struct Vertex2DTextured
	{
		glm::vec2 pos;
		glm::vec2 texCoord;

		static VkVertexInputBindingDescription getBindingDescription(uint32_t binding)
		{
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = binding;
			bindingDescription.stride = sizeof(Vertex2DTextured);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions(uint32_t binding)
		{
			std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);

			attributeDescriptions[0].binding = binding;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(Vertex2DTextured, pos);

			attributeDescriptions[1].binding = binding;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex2DTextured, texCoord);

			return attributeDescriptions;
		}

		bool operator==(const Vertex2DTextured& other) const
		{
			return pos == other.pos && texCoord == other.texCoord;
		}
	};

	struct Vertex2DTexturedWithMaterial
	{
		glm::vec2 pos;
		glm::vec2 texCoord;
		glm::uvec3 IDs;

		static VkVertexInputBindingDescription getBindingDescription(uint32_t binding)
		{
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = binding;
			bindingDescription.stride = sizeof(Vertex2DTexturedWithMaterial);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions(uint32_t binding)
		{
			std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);

			attributeDescriptions[0].binding = binding;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(Vertex2DTexturedWithMaterial, pos);

			attributeDescriptions[1].binding = binding;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex2DTexturedWithMaterial, texCoord);

			attributeDescriptions[2].binding = binding;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VK_FORMAT_R32G32B32_UINT;
			attributeDescriptions[2].offset = offsetof(Vertex2DTexturedWithMaterial, IDs);

			return attributeDescriptions;
		}

		bool operator==(const Vertex2DTexturedWithMaterial& other) const
		{
			return pos == other.pos && texCoord == other.texCoord && IDs == other.IDs;
		}
	};

	struct Vertex3D
	{
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec2 texCoord;
		glm::uint materialID;

		static VkVertexInputBindingDescription getBindingDescription(uint32_t binding)
		{
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = binding;
			bindingDescription.stride = sizeof(Vertex3D);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions(uint32_t binding)
		{
			std::vector<VkVertexInputAttributeDescription> attributeDescriptions(5);

			attributeDescriptions[0].binding = binding;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(Vertex3D, pos);

			attributeDescriptions[1].binding = binding;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex3D, normal);

			attributeDescriptions[2].binding = binding;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[2].offset = offsetof(Vertex3D, tangent);

			attributeDescriptions[3].binding = binding;
			attributeDescriptions[3].location = 3;
			attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[3].offset = offsetof(Vertex3D, texCoord);

			attributeDescriptions[4].binding = binding;
			attributeDescriptions[4].location = 4;
			attributeDescriptions[4].format = VK_FORMAT_R32_UINT;
			attributeDescriptions[4].offset = offsetof(Vertex3D, materialID);

			return attributeDescriptions;
		}

		bool operator==(const Vertex3D& other) const
		{
			return pos == other.pos && normal == other.normal && texCoord == other.texCoord && tangent == other.tangent && materialID == other.materialID;
		}
	};
}

namespace std
{
	template<> struct hash<Wolf::Vertex3D>
	{
		size_t operator()(Wolf::Vertex3D const& vertex) const
		{
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}