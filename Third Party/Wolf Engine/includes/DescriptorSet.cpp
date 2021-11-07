#include "DescriptorSet.h"

#include "Debug.h"

VkDescriptorSetLayout Wolf::createDescriptorSetLayout(VkDevice device, std::vector<DescriptorLayout> descriptorLayouts)
{
	std::vector<VkDescriptorSetLayoutBinding> bindings;

	for (auto descriptorLayout : descriptorLayouts)
	{
		VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {};
		descriptorSetLayoutBinding.binding = descriptorLayout.binding;
		descriptorSetLayoutBinding.descriptorType = descriptorLayout.descriptorType;
		descriptorSetLayoutBinding.descriptorCount = descriptorLayout.count;
		descriptorSetLayoutBinding.stageFlags = descriptorLayout.accessibility;
		descriptorSetLayoutBinding.pImmutableSamplers = nullptr;

		bindings.push_back(descriptorSetLayoutBinding);
	}

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	VkDescriptorSetLayout descriptorSetLayout;
	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
		Debug::sendError("Error : create descriptor set layout");

	return descriptorSetLayout;
}

VkDescriptorSet Wolf::createDescriptorSet(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool descriptorPool, DescriptorSetCreateInfo descriptorSetCreateInfo)
{
	VkDescriptorSetLayout layouts[] = { descriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	VkDescriptorSet descriptorSet;
	if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS)
		Debug::sendError("Error : allocate descriptor set");

	std::vector<VkWriteDescriptorSet> descriptorWrites;

	std::vector<std::vector<VkDescriptorBufferInfo>> descriptorBufferInfos(descriptorSetCreateInfo.descriptorBuffers.size());
	for (int i(0); i < descriptorBufferInfos.size(); ++i)
	{
		descriptorBufferInfos[i].resize(descriptorSetCreateInfo.descriptorBuffers[i].first.size());
		for (int j(0); j < descriptorBufferInfos[i].size(); ++j)
		{
			descriptorBufferInfos[i][j].buffer = descriptorSetCreateInfo.descriptorBuffers[i].first[j].buffer;
			descriptorBufferInfos[i][j].offset = 0;
			descriptorBufferInfos[i][j].range = descriptorSetCreateInfo.descriptorBuffers[i].first[j].size;
		}

		VkWriteDescriptorSet descriptorWrite;
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSet;
		descriptorWrite.dstBinding = descriptorSetCreateInfo.descriptorBuffers[i].second.binding;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = descriptorSetCreateInfo.descriptorBuffers[i].second.descriptorType;
		descriptorWrite.descriptorCount = static_cast<uint32_t>(descriptorBufferInfos[i].size());
		descriptorWrite.pBufferInfo = descriptorBufferInfos[i].data();
		descriptorWrite.pNext = NULL;

		descriptorWrites.push_back(descriptorWrite);
	}

	std::vector<std::vector<VkDescriptorImageInfo>> descriptorImageInfos(descriptorSetCreateInfo.descriptorImages.size());
	for (int i(0); i < descriptorImageInfos.size(); ++i)
	{
		descriptorImageInfos[i].resize(descriptorSetCreateInfo.descriptorImages[i].first.size());
		for (int j(0); j < descriptorImageInfos[i].size(); ++j)
		{
			if(descriptorSetCreateInfo.descriptorImages[i].first[j].image)
			{
				descriptorImageInfos[i][j].imageLayout = descriptorSetCreateInfo.descriptorImages[i].first[j].image->getImageLayout();
				descriptorImageInfos[i][j].imageView = descriptorSetCreateInfo.descriptorImages[i].first[j].image->getImageView();
			}
			if(descriptorSetCreateInfo.descriptorImages[i].first[j].sampler)
				descriptorImageInfos[i][j].sampler = descriptorSetCreateInfo.descriptorImages[i].first[j].sampler->getSampler();
		}

		VkWriteDescriptorSet descriptorWrite;
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSet;
		descriptorWrite.dstBinding = descriptorSetCreateInfo.descriptorImages[i].second.binding;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = descriptorSetCreateInfo.descriptorImages[i].second.descriptorType;
		descriptorWrite.descriptorCount = static_cast<uint32_t>(descriptorImageInfos[i].size());
		descriptorWrite.pImageInfo = descriptorImageInfos[i].data();
		descriptorWrite.pNext = NULL;

		descriptorWrites.push_back(descriptorWrite);
	}

	for(int i(0); i < descriptorSetCreateInfo.descriptorDefault.size(); ++i)
	{
		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSet;
		descriptorWrite.dstBinding = descriptorSetCreateInfo.descriptorDefault[i].second.binding;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = descriptorSetCreateInfo.descriptorDefault[i].second.descriptorType;
		descriptorWrite.descriptorCount = static_cast<uint32_t>(descriptorSetCreateInfo.descriptorDefault[i].first.size());
		descriptorWrite.pImageInfo = NULL;
		descriptorWrite.pNext = &descriptorSetCreateInfo.descriptorDefault[i].first[0];

		descriptorWrites.push_back(descriptorWrite);
	}

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

	return descriptorSet;
}

void Wolf::DescriptorSetGenerator::addUniformBuffer(UniformBuffer* ubo, VkShaderStageFlags accessibility,
	uint32_t binding)
{
	DescriptorSetCreateInfo::BufferData bufferData;
	bufferData.buffer = ubo->getUniformBuffer();
	bufferData.size = ubo->getSize();

	DescriptorLayout descriptorLayout;
	descriptorLayout.accessibility = accessibility;
	descriptorLayout.binding = binding;
	descriptorLayout.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	
	m_descriptorSetCreateInfo.descriptorBuffers.push_back({
			{ bufferData },
			descriptorLayout
		});
}

void Wolf::DescriptorSetGenerator::addImages(std::vector<Image*> images, VkDescriptorType descriptorType,
	VkShaderStageFlags accessibility, uint32_t binding)
{
	std::vector<DescriptorSetCreateInfo::ImageData> imageData(images.size());
	for(int i(0); i< images.size(); ++i)
	{
		imageData[i].image = images[i];
	}

	DescriptorLayout descriptorLayout;
	descriptorLayout.accessibility = accessibility;
	descriptorLayout.binding = binding;
	descriptorLayout.descriptorType = descriptorType;
	descriptorLayout.count = static_cast<uint32_t>(images.size());

	m_descriptorSetCreateInfo.descriptorImages.emplace_back(
		imageData,
		descriptorLayout
	);
}

void Wolf::DescriptorSetGenerator::addCombinedImageSampler(Image* image, Sampler* sampler,
	VkShaderStageFlags accessibility, uint32_t binding)
{
	DescriptorSetCreateInfo::ImageData imageData{};
	imageData.image = image;
	imageData.sampler = sampler;

	DescriptorLayout descriptorLayout;
	descriptorLayout.accessibility = accessibility;
	descriptorLayout.binding = binding;
	descriptorLayout.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	m_descriptorSetCreateInfo.descriptorImages.push_back({
			{ imageData },
			descriptorLayout
		});
}

void Wolf::DescriptorSetGenerator::addSampler(Sampler* sampler, VkShaderStageFlags accessibility, uint32_t binding)
{
	DescriptorSetCreateInfo::ImageData imageData{};
	imageData.sampler = sampler;

	DescriptorLayout descriptorLayout;
	descriptorLayout.accessibility = accessibility;
	descriptorLayout.binding = binding;
	descriptorLayout.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;

	m_descriptorSetCreateInfo.descriptorImages.push_back({
			{ imageData },
			descriptorLayout
		});
}

void Wolf::DescriptorSetGenerator::addAccelerationStructure(AccelerationStructure* accelerationStructure,
	VkShaderStageFlags accessibility, uint32_t binding)
{
	VkWriteDescriptorSetAccelerationStructureNV descriptorAccelerationStructureInfo;
	descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV;
	descriptorAccelerationStructureInfo.pNext = nullptr;
	descriptorAccelerationStructureInfo.accelerationStructureCount = 1;

	auto* topLevelAccelerationStructure = accelerationStructure->getTopLevelAccelerationStructure();
	descriptorAccelerationStructureInfo.pAccelerationStructures = topLevelAccelerationStructure;
	
	DescriptorLayout descriptorLayout;
	descriptorLayout.accessibility = accessibility;
	descriptorLayout.binding = binding;
	descriptorLayout.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;

	m_descriptorSetCreateInfo.descriptorDefault.push_back({
			{ descriptorAccelerationStructureInfo },
			descriptorLayout
		});
}

void Wolf::DescriptorSetGenerator::addBuffer(Buffer* buffer, VkShaderStageFlags accessibility, uint32_t binding)
{
	DescriptorSetCreateInfo::BufferData bufferData;
	bufferData.buffer = buffer ? buffer->getBuffer() : VK_NULL_HANDLE;
	bufferData.size = buffer ? buffer->getSize() : VK_WHOLE_SIZE;

	DescriptorLayout descriptorLayout;
	descriptorLayout.accessibility = accessibility;
	descriptorLayout.binding = binding;
	descriptorLayout.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

	m_descriptorSetCreateInfo.descriptorBuffers.push_back({
			{ bufferData },
			descriptorLayout
		});
}

void Wolf::DescriptorSetGenerator::addBuffer(VkBuffer buffer, VkDeviceSize range, VkShaderStageFlags accessibility, uint32_t binding)
{
	DescriptorSetCreateInfo::BufferData bufferData;
	bufferData.buffer = buffer;
	bufferData.size = range;

	DescriptorLayout descriptorLayout;
	descriptorLayout.accessibility = accessibility;
	descriptorLayout.binding = binding;
	descriptorLayout.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

	m_descriptorSetCreateInfo.descriptorBuffers.push_back({
			{ bufferData },
			descriptorLayout
		});
}

std::vector<Wolf::DescriptorLayout> Wolf::DescriptorSetGenerator::getDescriptorLayouts()
{
	std::vector<Wolf::DescriptorLayout> r(m_descriptorSetCreateInfo.descriptorImages.size() + m_descriptorSetCreateInfo.descriptorBuffers.size());
	
	int i = 0;
	for (auto& descriptorImage : m_descriptorSetCreateInfo.descriptorImages)
		r[i++] = descriptorImage.second;
	for (auto& descriptorBuffer : m_descriptorSetCreateInfo.descriptorBuffers)
		r[i++] = descriptorBuffer.second;
	for (auto& descriptorDefault : m_descriptorSetCreateInfo.descriptorDefault)
		r[i++] = descriptorDefault.second;

	return r;
}
