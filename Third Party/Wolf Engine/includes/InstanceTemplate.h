#pragma once

#include "VulkanHelper.h"
#include <glm/glm.hpp>

namespace Wolf
{
	enum class InstanceTemplate { NO, SINGLE_ID };
	
	struct InstanceSingleID
	{
		glm::uint id;

		static VkVertexInputBindingDescription getBindingDescription(uint32_t binding)
		{
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = binding;
			bindingDescription.stride = sizeof(InstanceSingleID);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

			return bindingDescription;
		}

		static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions(uint32_t binding, uint32_t startLocation)
		{
			std::vector<VkVertexInputAttributeDescription> attributeDescriptions(1);

			attributeDescriptions[0].binding = binding;
			attributeDescriptions[0].location = startLocation;
			attributeDescriptions[0].format = VK_FORMAT_R32_UINT;
			attributeDescriptions[0].offset = 0;

			return attributeDescriptions;
		}
	};
}
