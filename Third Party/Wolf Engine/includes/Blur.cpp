#include "Blur.h"

Wolf::Blur::Blur(Wolf::WolfInstance* engineInstance, Wolf::Scene* scene, int commandBufferID, Image* inputImage, Image* depthImage)
{
	m_inputImage = inputImage;
	m_commandBufferID = commandBufferID;

	// Downscale
	m_downscaledImages.resize(3);
	m_downscaleComputePasses.resize(3);
	m_downscaleCommandBufferIDs.resize(3);
	VkExtent2D extent = { inputImage->getExtent().width / 2, inputImage->getExtent().height / 2 };
	for(int i(0);  i < 3; ++i)
	{
		Image::CreateImageInfo createImageInfo;
		createImageInfo.extent = { extent.width, extent.height, 1 };
		createImageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT;
		createImageInfo.format = inputImage->getFormat();
		createImageInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
		createImageInfo.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
		createImageInfo.mipLevels = 1;
		m_downscaledImages[i] = engineInstance->createImage({ createImageInfo });
		m_downscaledImages[i]->setImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

		Scene::CommandBufferCreateInfo commandBufferCreateInfo;
		commandBufferCreateInfo.finalPipelineStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		commandBufferCreateInfo.commandType = Scene::CommandType::COMPUTE;
		m_downscaleCommandBufferIDs[i] = scene->addCommandBuffer(commandBufferCreateInfo);

		Scene::ComputePassCreateInfo downscaleComputePassCreateInfo;
		downscaleComputePassCreateInfo.extent = extent;
		downscaleComputePassCreateInfo.dispatchGroups = { 16, 16, 1 };
		downscaleComputePassCreateInfo.computeShaderPath = "Shaders/Blur/downscale.spv";
		downscaleComputePassCreateInfo.commandBufferID = m_downscaleCommandBufferIDs[i];

		DescriptorSetGenerator descriptorSetGenerator;
		descriptorSetGenerator.addImages({ i == 0 ? inputImage : m_downscaledImages[i - 1] }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT,
			0); // Input
		descriptorSetGenerator.addImages({ m_downscaledImages[i] }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT,
			1); // Output

		downscaleComputePassCreateInfo.descriptorSetCreateInfo = descriptorSetGenerator.getDescritorSetCreateInfo();

		m_downscaleComputePasses[i] = scene->addComputePass(downscaleComputePassCreateInfo);

		extent.width /= 2;
		extent.height /= 2;
	}	

	// Blur
	{
		Image::CreateImageInfo createImageInfo;
		createImageInfo.extent = { m_downscaledImages.back()->getExtent().width, m_downscaledImages.back()->getExtent().height, 1 };
		createImageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT;
		createImageInfo.format = inputImage->getFormat();
		createImageInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
		createImageInfo.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
		createImageInfo.mipLevels = 1;
		m_downscaledBlurredImage = engineInstance->createImage(createImageInfo);
		m_downscaledBlurredImage->setImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

		Scene::CommandBufferCreateInfo commandBufferCreateInfo;
		commandBufferCreateInfo.finalPipelineStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		commandBufferCreateInfo.commandType = Scene::CommandType::COMPUTE;
		m_horizontalBlurCommandBuffer = scene->addCommandBuffer(commandBufferCreateInfo);

		// Horizontal
		{
			Scene::ComputePassCreateInfo horizontalBlurComputePassCreateInfo;
			horizontalBlurComputePassCreateInfo.extent = { m_downscaledImages.back()->getExtent().width, m_downscaledImages.back()->getExtent().height };
			horizontalBlurComputePassCreateInfo.dispatchGroups = { 16, 16, 1 };
			horizontalBlurComputePassCreateInfo.computeShaderPath = "Shaders/Blur/horizontal.spv";
			horizontalBlurComputePassCreateInfo.commandBufferID = m_horizontalBlurCommandBuffer;

			DescriptorSetGenerator descriptorSetGenerator;
			descriptorSetGenerator.addImages({ m_downscaledImages.back() }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 0);
			descriptorSetGenerator.addImages({ m_downscaledBlurredImage }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 1);

			horizontalBlurComputePassCreateInfo.descriptorSetCreateInfo = descriptorSetGenerator.getDescritorSetCreateInfo();

			m_horizontalBlurComputePass = scene->addComputePass(horizontalBlurComputePassCreateInfo);
		}

		// Vertical
		{
			Image::CreateImageInfo createImageInfo;
			createImageInfo.extent = { m_downscaledImages.back()->getExtent().width, m_downscaledImages.back()->getExtent().height, 1 };
			createImageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT;
			createImageInfo.format = inputImage->getFormat();
			createImageInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
			createImageInfo.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
			createImageInfo.mipLevels = 1;
			m_downscaledBlurredImage2 = engineInstance->createImage(createImageInfo);
			m_downscaledBlurredImage2->setImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

			m_verticalBlurCommandBuffer = scene->addCommandBuffer(commandBufferCreateInfo);

			Scene::ComputePassCreateInfo verticalBlurComputePassCreateInfo;
			verticalBlurComputePassCreateInfo.extent = { m_downscaledImages.back()->getExtent().width, m_downscaledImages.back()->getExtent().height };
			verticalBlurComputePassCreateInfo.dispatchGroups = { 16, 16, 1 };
			verticalBlurComputePassCreateInfo.computeShaderPath = "Shaders/Blur/vertical.spv";
			verticalBlurComputePassCreateInfo.commandBufferID = m_verticalBlurCommandBuffer;

			DescriptorSetGenerator descriptorSetGenerator;
			descriptorSetGenerator.addImages({ m_downscaledBlurredImage }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 0);
			descriptorSetGenerator.addImages({ m_downscaledBlurredImage2 }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 1);

			verticalBlurComputePassCreateInfo.descriptorSetCreateInfo = descriptorSetGenerator.getDescritorSetCreateInfo();

			m_verticalBlurComputePass = scene->addComputePass(verticalBlurComputePassCreateInfo);
		}
	}
}
