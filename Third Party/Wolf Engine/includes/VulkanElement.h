#pragma once

#include "VulkanHelper.h"

namespace Wolf
{
	class VulkanElement
	{
	public:

	protected:
		VkDevice m_device = VK_NULL_HANDLE;
		VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
		VkCommandPool m_commandPool = VK_NULL_HANDLE;
		VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
		Queue m_graphicsQueue = {};
	};
}
