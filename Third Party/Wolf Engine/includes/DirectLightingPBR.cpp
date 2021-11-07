#include "DirectLightingPBR.h"

Wolf::DirectLightingPBR::DirectLightingPBR(Wolf::WolfInstance* engineInstance, Wolf::Scene* scene, int commandBufferID,
	VkExtent2D extent, Image* depth, Image* albedoImage, Image* normalRoughnessMetal, Image* shadowMask, Image* volumetricLight, Image* aoMaskImage, Image* lightPropagationVolumes,
	glm::mat4 projection, float near, float far)
{
	// Data
	Image::CreateImageInfo createImageInfo;
	createImageInfo.extent = { engineInstance->getWindowSize().width, engineInstance->getWindowSize().height, 1 };
	createImageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT;
	createImageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	createImageInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
	createImageInfo.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
	createImageInfo.mipLevels = 1;
	m_outputImage = engineInstance->createImage(createImageInfo);
	m_outputImage->setImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

	m_uboData.directionDirectionalLight = glm::vec4(-1.0f, -5.0f, 0.0f, 1.0f);
	m_uboData.colorDirectionalLight = glm::vec4(10.0f, 9.0f, 6.0f, 1.0f);
	m_uboData.invProjection = glm::inverse(projection);
	m_uboData.projParams.x = far / (far - near);
	m_uboData.projParams.y = (-far * near) / (far - near);

	m_ubo = engineInstance->createUniformBufferObject(&m_uboData, sizeof(UBOData));
	
	Scene::ComputePassCreateInfo computePassCreateInfo;
	computePassCreateInfo.extent = engineInstance->getWindowSize();
	computePassCreateInfo.dispatchGroups = { 16, 16, 1 };
	computePassCreateInfo.computeShaderPath = "Shaders/directLighting/comp.spv";
	computePassCreateInfo.commandBufferID = commandBufferID;

	DescriptorSetGenerator descriptorSetGenerator;
	descriptorSetGenerator.addImages({ depth }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 0);
	descriptorSetGenerator.addImages({ albedoImage }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 1);
	descriptorSetGenerator.addImages({ normalRoughnessMetal }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 2);
	descriptorSetGenerator.addImages({ shadowMask }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 3);
	descriptorSetGenerator.addImages({ volumetricLight }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 4);
	descriptorSetGenerator.addImages({ aoMaskImage }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 5);
	descriptorSetGenerator.addImages({ lightPropagationVolumes }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 6);
	descriptorSetGenerator.addImages({ m_outputImage }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 7);
	descriptorSetGenerator.addUniformBuffer(m_ubo, VK_SHADER_STAGE_COMPUTE_BIT, 8);

	computePassCreateInfo.descriptorSetCreateInfo = descriptorSetGenerator.getDescritorSetCreateInfo();

	m_computePassID = scene->addComputePass(computePassCreateInfo);
}

void Wolf::DirectLightingPBR::update(glm::vec3 lightDirectionInViewPosSpace, glm::mat4 voxelProjection)
{
	m_uboData.directionDirectionalLight = glm::vec4(lightDirectionInViewPosSpace, 1.0f);
	m_uboData.voxelProjection = voxelProjection;
	m_ubo->updateData(&m_uboData);
}
