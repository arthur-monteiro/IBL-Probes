#include "Framebuffer.h"

bool Wolf::Framebuffer::initialize(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, Queue graphicsQueue, 
	VkRenderPass renderPass, VkExtent2D extent, std::vector<Attachment> attachments)
{
	m_extent = extent;
	m_images.resize(attachments.size());
	std::vector<VkImageView> imageViewAttachments(attachments.size());

	for (int i(0); i < attachments.size(); ++i)
	{
		VkImageAspectFlagBits aspect;
		if (attachments[i].usageType & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
			aspect = VK_IMAGE_ASPECT_COLOR_BIT;
		else
			aspect = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (attachments[i].image)
		{
			m_images[i] = std::unique_ptr<Image>(attachments[i].image);
			imageViewAttachments[i] = m_images[i]->getImageView();
		}
		else
		{
			VkExtent3D extent3D = { extent.width, extent.height, 1 };
			Image::CreateImageInfo createImageInfo;
			createImageInfo.extent = extent3D;
			createImageInfo.usage = attachments[i].usageType;
			createImageInfo.format = attachments[i].format;
			createImageInfo.sampleCount = attachments[i].sampleCount;
			createImageInfo.aspect = aspect;
			createImageInfo.mipLevels = 1;
			m_images[i] = std::make_unique<Image>(device, physicalDevice, commandPool, graphicsQueue, createImageInfo);
			imageViewAttachments[i] = m_images[i]->getImageView();
		}
	}

	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = renderPass;
	framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	framebufferInfo.pAttachments = imageViewAttachments.data();
	framebufferInfo.width = extent.width;
	framebufferInfo.height = extent.height;
	framebufferInfo.layers = 1;

	return vkCreateFramebuffer(device, &framebufferInfo, nullptr, &m_framebuffer) != VK_SUCCESS;
}

bool Wolf::Framebuffer::initialize(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, Queue graphicsQueue, 
	VkRenderPass renderPass, Image* image, std::vector<Attachment> attachments)
{
	m_extent = { image->getExtent().width, image->getExtent().height };

	// Find result image
	VkImageUsageFlagBits resultType = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	unsigned int nbImage = 0;
	for (int i(0); i < attachments.size(); ++i)
	{
		if (attachments[i].usageType == (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT))
			resultType = static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT);
		else if (attachments[i].usageType == VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT && resultType == VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
			resultType = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}
	for (int i(0); i < attachments.size(); ++i)
		if (attachments[i].usageType != resultType)
			nbImage++;

	if (nbImage < attachments.size() - 1)
		throw std::runtime_error("Error : multiple result with single image");

	m_images.resize(nbImage);
	std::vector<VkImageView> imageViewAttachments(attachments.size());

	// Create necessary images
	int currentImage = 0;
	for (int i(0); i < attachments.size(); ++i)
	{
		if (attachments[i].usageType != resultType)
		{
			VkImageAspectFlagBits aspect;
			if (attachments[i].usageType & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
				aspect = VK_IMAGE_ASPECT_COLOR_BIT;
			else
				aspect = VK_IMAGE_ASPECT_DEPTH_BIT;

			VkExtent3D extent = { image->getExtent().width, image->getExtent().height, 1 };
			Image::CreateImageInfo createImageInfo;
			createImageInfo.extent = extent;
			createImageInfo.usage = attachments[i].usageType;
			createImageInfo.format = attachments[i].format;
			createImageInfo.sampleCount = attachments[i].sampleCount;
			createImageInfo.aspect = aspect;
			createImageInfo.mipLevels = 1;
			m_images[currentImage] = std::make_unique<Image>(device, physicalDevice, commandPool, graphicsQueue, createImageInfo);

			imageViewAttachments[i] = m_images[currentImage]->getImageView();

			currentImage++;
		}
		else
		{
			imageViewAttachments[i] = image->getImageView();
		}
	}

	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = renderPass;
	framebufferInfo.attachmentCount = static_cast<uint32_t>(imageViewAttachments.size());
	framebufferInfo.pAttachments = imageViewAttachments.data();
	framebufferInfo.width = image->getExtent().width;
	framebufferInfo.height = image->getExtent().height;
	framebufferInfo.layers = 1;

	if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &m_framebuffer) != VK_SUCCESS)
		throw std::runtime_error("Error : create framebuffer");

	return true;
}

void Wolf::Framebuffer::cleanup(VkDevice device)
{
	vkDestroyFramebuffer(device, m_framebuffer, nullptr);
	/*for (int i(0); i < m_images.size(); ++i)
		m_images[i].cleanup(device);*/
}

std::vector<Wolf::Image*> Wolf::Framebuffer::getImages()
{
	std::vector<Image*> images(m_images.size());
	for (int i(0); i < m_images.size(); ++i)
		images[i] = m_images[i].get();

	return images;
}
