#include "Model.h"

Wolf::Model::Model(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, Queue graphicsQueue, InputVertexTemplate inputVertexTemplate)
{
	m_device = device;
	m_physicalDevice = physicalDevice;
	m_commandPool = commandPool;
	m_graphicsQueue = graphicsQueue;

	m_inputVertexTemplate = inputVertexTemplate;
}

std::vector<Wolf::Image*> Wolf::Model::getImages() const
{
	std::vector<Image*> r(m_images.size());
	for (size_t i(0); i < m_images.size(); ++i)
		r[i] = m_images[i].get();

	return r;
}
