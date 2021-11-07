#include "CommandBuffer.h"

Wolf::CommandBuffer::CommandBuffer(VkDevice device, VkCommandPool commandPool)
{
	m_device = device;
	m_commandPool = commandPool;
	
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(device, &allocInfo, &m_commandBuffer) != VK_SUCCESS)
		throw std::runtime_error("Error : command buffer allocation");
}

Wolf::CommandBuffer::~CommandBuffer()
{
	vkFreeCommandBuffers(m_device, m_commandPool, 1, &m_commandBuffer);
}

void Wolf::CommandBuffer::beginCommandBuffer()
{
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	vkBeginCommandBuffer(m_commandBuffer, &beginInfo);
}

void Wolf::CommandBuffer::endCommandBuffer()
{
	if (vkEndCommandBuffer(m_commandBuffer) != VK_SUCCESS)
		throw std::runtime_error("Error : end command buffer");
}

void Wolf::CommandBuffer::submit(VkDevice device, Queue queue, std::vector<Wolf::Semaphore*> waitSemaphores,
	std::vector<VkSemaphore> signalSemaphores)
{
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
	submitInfo.pSignalSemaphores = signalSemaphores.data();

	VkCommandBuffer commandBuffer = m_commandBuffer;
	submitInfo.pCommandBuffers = &commandBuffer;
	submitInfo.commandBufferCount = 1;

	submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
	std::vector<VkSemaphore> semaphores;
	std::vector<VkPipelineStageFlags> stages;
	for (int i(0); i < waitSemaphores.size(); ++i)
	{
		semaphores.push_back(waitSemaphores[i]->getSemaphore());
		stages.push_back(waitSemaphores[i]->getPipelineStage());
	}
	submitInfo.pWaitSemaphores = semaphores.data();
	submitInfo.pWaitDstStageMask = stages.data();

	queue.mutex->lock();
	if (vkQueueSubmit(queue.queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
	{
		queue.mutex->unlock();
		throw std::runtime_error("Error : submit to graphics queue");
	}
	queue.mutex->unlock();
}