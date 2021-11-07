#include "CommandPool.h"

Wolf::CommandPool::~CommandPool()
{
}

void Wolf::CommandPool::initializeForGraphicsQueue(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice, surface);

	m_commandPool = createCommandPool(device, physicalDevice, surface, queueFamilyIndices.graphicsFamily);
}

void Wolf::CommandPool::initializeForComputeQueue(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice, surface);

	m_commandPool = createCommandPool(device, physicalDevice, surface, queueFamilyIndices.computeFamily);
}

void Wolf::CommandPool::cleanup(VkDevice device)
{
	vkDestroyCommandPool(device, m_commandPool, nullptr);
}
