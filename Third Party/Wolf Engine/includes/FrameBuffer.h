#pragma once

#include "VulkanHelper.h"
#include "Attachment.h"
#include "Image.h"

namespace Wolf
{
	class Framebuffer
	{
	public:
		bool initialize(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, Queue graphicsQueue, 
			VkRenderPass renderPass, VkExtent2D extent, std::vector<Attachment> attachments);
		bool initialize(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, Queue graphicsQueue, 
			VkRenderPass renderPass, Image* image, std::vector<Attachment> attachments);

		void cleanup(VkDevice device);

		VkFramebuffer getFramebuffer() { return m_framebuffer; }
		VkExtent2D getExtent() { return m_extent; }
		std::vector<Wolf::Image*> getImages();

	private:
		VkFramebuffer m_framebuffer;
		VkExtent2D m_extent;
		std::vector<std::unique_ptr<Image>> m_images;
	};


}