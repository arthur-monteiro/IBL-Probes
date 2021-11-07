#pragma once

#include "Image.h"
#include "Sampler.h"
#include "UniformBuffer.h"
#include "VulkanHelper.h"
#include "AccelerationStructure.h"
#include "Buffer.h"

namespace Wolf
{
	struct DescriptorLayout
	{
		VkDescriptorType descriptorType;
		VkShaderStageFlags accessibility{};
		uint32_t binding{};
		uint32_t count = 1;
	};
	
	struct DescriptorSetCreateInfo
	{
		struct BufferData
		{
			VkBuffer buffer;
			VkDeviceSize size;
		};
		std::vector<std::pair<std::vector<BufferData>, DescriptorLayout>> descriptorBuffers;

		struct ImageData
		{
			Image* image = nullptr;
			Sampler* sampler = nullptr;
		};
		std::vector<std::pair<std::vector<ImageData>, DescriptorLayout>> descriptorImages;

		std::vector<std::pair<std::vector<VkWriteDescriptorSetAccelerationStructureNV>, DescriptorLayout>> descriptorDefault;
	};

	VkDescriptorSetLayout createDescriptorSetLayout(VkDevice device, std::vector<DescriptorLayout> descriptorLayouts);
	VkDescriptorSet createDescriptorSet(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool descriptorPool, DescriptorSetCreateInfo descriptorSetCreateInfo);

	class DescriptorSetGenerator
	{
	public:
		void addUniformBuffer(UniformBuffer* ubo, VkShaderStageFlags accessibility, uint32_t binding);
		void addImages(std::vector<Image*> images, VkDescriptorType descriptorType, VkShaderStageFlags accessibility, uint32_t binding);
		void addCombinedImageSampler(Image* image, Sampler* sampler, VkShaderStageFlags accessibility, uint32_t binding);
		void addSampler(Sampler* sampler, VkShaderStageFlags accessibility, uint32_t binding);
		void addAccelerationStructure(AccelerationStructure* accelerationStructure, VkShaderStageFlags accessibility, uint32_t binding);
		void addBuffer(Buffer* buffer, VkShaderStageFlags accessibility, uint32_t binding);
		void addBuffer(VkBuffer buffer, VkDeviceSize range, VkShaderStageFlags accessibility, uint32_t binding);
		
		DescriptorSetCreateInfo getDescritorSetCreateInfo() { return m_descriptorSetCreateInfo; }
		std::vector<DescriptorLayout> getDescriptorLayouts();
		
	private:
		DescriptorSetCreateInfo m_descriptorSetCreateInfo;
	};
}
