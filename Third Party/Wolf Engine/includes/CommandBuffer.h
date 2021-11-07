#pragma once

#include <array>

#include "VulkanElement.h"
#include "Semaphore.h"

namespace Wolf
{
	class CommandBuffer : public VulkanElement
	{
	public:
		CommandBuffer(VkDevice device, VkCommandPool commandPool);
		~CommandBuffer();

		void beginCommandBuffer();
		void endCommandBuffer();
		void submit(VkDevice device, Queue queue, std::vector<Wolf::Semaphore*> waitSemaphores, std::vector<VkSemaphore> signalSemaphores);

		// Getter
	public:
		VkCommandBuffer getCommandBuffer() const { return m_commandBuffer; }

	private:
		VkCommandBuffer m_commandBuffer;
	};
}