#pragma once

#include <iostream>

#include "VulkanHelper.h"
#include "FrameBuffer.h"
#include "Attachment.h"
#include "Semaphore.h"
#include "Image.h"

namespace Wolf
{
	class RenderPass
	{
	public:
		RenderPass(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, Queue graphicsQueue, const std::vector<Attachment>& attachments, std::vector<VkExtent2D> extents);
		RenderPass(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, Queue graphicsQueue, 
			const std::vector<Attachment>& attachments, std::vector<Wolf::Image*> images);
		~RenderPass() = default;

		void initialize(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, Queue graphicsQueue, 
			const std::vector<Attachment>& attachments, std::vector<VkExtent2D> extents);
		void initialize(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, Queue graphicsQueue, const std::vector<Attachment>& attachments,
			std::vector<Wolf::Image*> images);

		void beginRenderPass(size_t framebufferID, std::vector<VkClearValue>& clearValues, VkCommandBuffer commandBuffer);
		void endRenderPass(VkCommandBuffer commandBuffer);

		void resize(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, Queue graphicsQueue, const std::vector<Attachment>& attachments, std::vector<Wolf::Image*> images);

		void cleanup(VkDevice device, VkCommandPool commandPool);

		// Getters
	public:
		std::vector<Wolf::Image*> getImages(int framebufferID) { return m_framebuffers[framebufferID].getImages(); }
		VkRenderPass getRenderPass() { return m_renderPass; }
		//VkFramebuffer getFramebuffer() { return m_framebuffers[0].getFramebuffer(); }
		int getFramebufferCount() { return static_cast<int>(m_framebuffers.size()); }

	private:
		VkRenderPass m_renderPass;
		std::vector<Framebuffer> m_framebuffers;

	private:
		static VkRenderPass createRenderPass(VkDevice device, std::vector<Attachment> attachments);
	};
}