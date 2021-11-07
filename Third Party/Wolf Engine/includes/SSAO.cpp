#include "SSAO.h"
#include <random>

Wolf::SSAO::SSAO(Wolf::WolfInstance* engineInstance, Wolf::Scene* scene, int commandBufferID, VkExtent2D extent,
	glm::mat4 projection, Image* depth, Image* normal, float near, float far)
{
	// Data
	const std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // random floats between 0.0 - 1.0
	std::random_device rd;
	std::default_random_engine generator(rd());
	std::array<glm::vec4, 16> ssaoKernel;
	for (unsigned int i = 0; i < 16; ++i)
	{
		glm::vec3 sample(
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator) // normal is (0, 0, 1)
		);
		sample = glm::normalize(sample); // fit point into hemisphere (r = 1)
		sample *= randomFloats(generator);
		float scale = static_cast<float>(i) / 64.0f;
		scale = glm::mix(0.1f, 1.0f, scale * scale);
		sample *= scale;
		ssaoKernel[i] = glm::vec4(sample, 0.0f);
	}

	std::array<glm::vec4, 16> ssaoNoise;
	for (unsigned int i = 0; i < 16; i++)
	{
		glm::vec3 noise(
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator) * 2.0 - 1.0,
			0.0f);
		ssaoNoise[i] = glm::vec4(noise, 0.0f);
	}

	m_uboData.samples = ssaoKernel;
	m_uboData.noise = ssaoNoise;
	m_uboData.projection = projection;
	m_uboData.invProjection = glm::inverse(projection);
	m_uboData.projParams.x = far / (far - near);
	m_uboData.projParams.y = (-far * near) / (far - near);

	m_uniformBuffer = engineInstance->createUniformBufferObject(&m_uboData, sizeof(UBOData));

	Image::CreateImageInfo createImageInfo;
	createImageInfo.extent = { engineInstance->getWindowSize().width, engineInstance->getWindowSize().height, 1 };
	createImageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT;
	createImageInfo.format = VK_FORMAT_R32_SFLOAT;
	createImageInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
	createImageInfo.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
	createImageInfo.mipLevels = 1;
	m_outputImage = engineInstance->createImage(createImageInfo);
	m_outputImage->setImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	
	Scene::ComputePassCreateInfo computePassCreateInfo;
	computePassCreateInfo.extent = engineInstance->getWindowSize();
	computePassCreateInfo.dispatchGroups = { 16, 16, 1 };
	computePassCreateInfo.computeShaderPath = "Shaders/SSAO/comp.spv";
	computePassCreateInfo.commandBufferID = commandBufferID;

	DescriptorSetGenerator descriptorSetGenerator;
	descriptorSetGenerator.addImages({ depth }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 0);
	descriptorSetGenerator.addImages({ normal }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 1);
	descriptorSetGenerator.addImages({ m_outputImage }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 2);
	descriptorSetGenerator.addUniformBuffer(m_uniformBuffer, VK_SHADER_STAGE_COMPUTE_BIT, 3);

	computePassCreateInfo.descriptorSetCreateInfo = descriptorSetGenerator.getDescritorSetCreateInfo();
	
	m_computePassID = scene->addComputePass(computePassCreateInfo);

	m_blur = std::make_unique<Blur>(engineInstance, scene, commandBufferID, m_outputImage, nullptr);
}
