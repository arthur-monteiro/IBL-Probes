#pragma once

#include <iostream>
#include <cmath>
#include <cstring>
#include <array>

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "VulkanHelper.h"
#include "VulkanElement.h"

namespace Wolf
{
	class Image : public VulkanElement
	{
	public:
		struct CreateImageInfo
		{
			VkExtent3D extent = { 0, 0, 0};
			VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT;
			VkFormat format = VK_FORMAT_UNDEFINED;
			VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;
			VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
			uint32_t mipLevels = UINT32_MAX;
			uint32_t arrayLayers = 1;
		};

		Image(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, Queue graphicsQueue, const CreateImageInfo& createImageInfo);
		Image(VkDevice device, VkCommandPool commandPool, Queue graphicsQueue, VkImage image, VkFormat format, VkImageAspectFlags aspect, VkExtent2D extent);
		Image(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, Queue graphicsQueue, std::string filename);

		~Image();

		Image(const Image&) = default;
		Image& operator=(const Image&) = default;

		void copyPixels(unsigned char* pixels);
		void copyBuffer(VkBuffer buffer);
		void copyImagesToCubemap(std::array<Image*, 6> images, std::vector<std::pair<uint8_t, uint8_t>> mipsToCopy, bool generateMipsLevels);

		void setImageLayout(VkImageLayout newLayout, VkPipelineStageFlags sourceStage, VkPipelineStageFlags destinationStage);
		void setImageLayoutWithoutOperation(VkImageLayout newImageLayout) { m_imageLayout = newImageLayout; }

		VkImage getImage() { return m_image; }
		VkDeviceMemory getImageMemory() { return m_imageMemory; }
		VkImageView getImageView() { return m_imageView; }
		VkFormat getFormat() { return m_imageFormat; }
		VkSampleCountFlagBits getSampleCount() { return m_sampleCount; }
		VkExtent3D getExtent() { return m_extent; }
		VkImageLayout getImageLayout() { return m_imageLayout; }
		uint32_t getMipLevels() { return m_mipLevels; }

	private:		
		VkImage m_image;
		VkDeviceMemory  m_imageMemory = VK_NULL_HANDLE;
		VkImageView m_imageView = VK_NULL_HANDLE;

		VkImageLayout m_imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkFormat m_imageFormat;

		uint32_t m_mipLevels;
		VkExtent3D m_extent;
		VkSampleCountFlagBits m_sampleCount;
		uint32_t m_arrayLayers;

	private:
		static void createImage(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels, VkSampleCountFlagBits numSamples, 
			VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, uint32_t arrayLayers, VkImageCreateFlags flags, VkImageLayout initialLayout,
			VkImage& image, VkDeviceMemory& imageMemory);
		static VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels, VkImageViewType viewType);
		static void transitionImageLayout(VkDevice device, VkCommandPool commandPool, Queue graphicsQueue, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout,
			uint32_t mipLevels, uint32_t arrayLayers, VkPipelineStageFlags sourceStage, VkPipelineStageFlags destinationStage);
		static void copyBufferToImage(VkDevice device, VkCommandPool commandPool, Queue graphicsQueue, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t baseArrayLayer);
		static void generateMipmaps(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, Queue graphicsQueue, VkImage image, VkFormat imageFormat, int32_t texWidth,
			int32_t texHeight, uint32_t mipLevels, uint32_t baseArrayLayer);

	public:
		static void transitionImageLayoutUsingCommandBuffer(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout,
			uint32_t mipLevels, VkPipelineStageFlags sourceStage, VkPipelineStageFlags destinationStage,
			uint32_t arrayLayer);
	};
}
