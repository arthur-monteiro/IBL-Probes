#include "CascadedShadowMapping.h"

Wolf::CascadedShadowMapping::CascadedShadowMapping(Wolf::WolfInstance* engineInstance, Wolf::Scene* scene, Model* model, float cameraNear, float cameraFar, float shadowFar, 
	float cameraFOV, VkExtent2D extent, Image* depth, glm::mat4 projection, std::array<DepthPass*, CASCADE_COUNT> depthPasses)
{
	m_engineInstance = engineInstance;
	m_scene = scene;
	m_cameraNear = cameraNear;
	m_cameraFOV = cameraFOV;
	m_cameraFar = shadowFar;
	m_ratio = static_cast<float>(extent.height) / static_cast<float>(extent.width);
	m_extent = extent;

	m_shadowMapExtents = { { 2048, 2048 }, { 2048, 2048 }, { 1024, 1024 }, { 1024, 1024 } };
	if (!depthPasses[0])
	{
		for (int i(0); i < m_depthPasses.size(); ++i)
		{
			// We use separate command buffers because we want to update cascade separately -> Crytek paper
			m_depthPasses[i] = std::make_unique<DepthPass>(engineInstance, scene, false, m_shadowMapExtents[i], VK_SAMPLE_COUNT_1_BIT, model, glm::mat4(1.0f), true,
				true);
			m_cascadeCommandBuffers[i] = m_depthPasses[i]->getCommandBufferID();
		}
	}
	else
	{
		for (int i(0); i < m_depthPasses.size(); ++i)
		{
			m_depthPasses[i] = std::unique_ptr<DepthPass>(depthPasses[i]);
			m_cascadeCommandBuffers[i] = m_depthPasses[i]->getCommandBufferID();
		}
	}

	// Cascade splits
	float near = m_cameraNear;
	float far = m_cameraFar; // we don't render shadows on all the range
	for (float i(1.0f / CASCADE_COUNT); i <= 1.0f; i += 1.0f / CASCADE_COUNT)
	{
		float d_uni = glm::mix(near, far, i);
		float d_log = near * glm::pow((far / near), i);

		m_cascadeSplits.push_back(glm::mix(d_uni, d_log, 0.5f));
	}

	// Data
	m_uboData.cascadeSplits = glm::vec4(m_cascadeSplits[0], m_cascadeSplits[1], m_cascadeSplits[2], m_cascadeSplits[3]);
	m_uboData.invProjection = glm::inverse(projection);
	m_uboData.projectionParams.x = cameraFar / (cameraFar - cameraNear);
	m_uboData.projectionParams.y = (-cameraFar * cameraNear) / (cameraFar - cameraNear);
	m_uniformBuffer = engineInstance->createUniformBufferObject(&m_uboData, sizeof(ShadowMaskUBO));

	Image::CreateImageInfo createImageInfo;
	createImageInfo.extent = { engineInstance->getWindowSize().width, engineInstance->getWindowSize().height, 1 };
	createImageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT;
	createImageInfo.format = VK_FORMAT_R32_SFLOAT;
	createImageInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
	createImageInfo.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
	createImageInfo.mipLevels = 1;

	m_shadowMaskOutputImage = engineInstance->createImage(createImageInfo);
	m_shadowMaskOutputImage->setImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

	m_volumetricLightOutputImage = engineInstance->createImage(createImageInfo);
	m_volumetricLightOutputImage->setImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

	// Shadow Mask
	Scene::CommandBufferCreateInfo commandBufferCreateInfo;
	commandBufferCreateInfo.finalPipelineStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	commandBufferCreateInfo.commandType = Scene::CommandType::COMPUTE;
	m_shadowMaskCommandBufferID = scene->addCommandBuffer(commandBufferCreateInfo);

	Scene::ComputePassCreateInfo computePassCreateInfo;
	computePassCreateInfo.name = "Cascaded shadow mapping";
	computePassCreateInfo.extent = engineInstance->getWindowSize();
	computePassCreateInfo.dispatchGroups = { 16, 16, 1 };
	computePassCreateInfo.computeShaderPath = "Shaders/CSM/comp.spv";
	computePassCreateInfo.commandBufferID = m_shadowMaskCommandBufferID;

	DescriptorSetGenerator descriptorSetGenerator;
	descriptorSetGenerator.addImages({ depth }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 0);
	std::vector<Image*> depthPassResults(CASCADE_COUNT);
	for (int i(0); i < CASCADE_COUNT; ++i)
		depthPassResults[i] = m_depthPasses[i]->getResult();
	descriptorSetGenerator.addImages(depthPassResults, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 1);
	descriptorSetGenerator.addImages({ m_shadowMaskOutputImage }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, CASCADE_COUNT + 1);
	descriptorSetGenerator.addImages({ m_volumetricLightOutputImage }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, CASCADE_COUNT + 2);

	descriptorSetGenerator.addUniformBuffer(m_uniformBuffer, VK_SHADER_STAGE_COMPUTE_BIT, CASCADE_COUNT + 3);

	computePassCreateInfo.descriptorSetCreateInfo = descriptorSetGenerator.getDescritorSetCreateInfo();

	m_shadowMaskComputePassID = scene->addComputePass(computePassCreateInfo);

	m_blur = std::make_unique<Blur>(engineInstance, scene, m_shadowMaskCommandBufferID, m_volumetricLightOutputImage, nullptr);
}

void Wolf::CascadedShadowMapping::updateMatrices(glm::vec3 lightDir,
	glm::vec3 cameraPosition, glm::vec3 cameraOrientation, glm::mat4 model, glm::mat4 invModelView)
{	
	float lastSplitDist = m_cameraNear;
	for (int cascade(0); cascade < CASCADE_COUNT; ++cascade)
	{
		const float startCascade = lastSplitDist;
		const float endCascade = m_cascadeSplits[cascade];

		float radius = (endCascade - startCascade) / 2.0f;

		const float ar = m_ratio;
		const float cosHalfHFOV = static_cast<float>(glm::cos((m_cameraFOV * (1.0f / ar)) / 2.0f));
		const float b = endCascade / cosHalfHFOV;
		radius = glm::sqrt(b * b + (startCascade + radius) * (startCascade + radius) - 2.0f * b * startCascade * cosHalfHFOV);

		const float texelPerUnit = static_cast<float>(m_shadowMapExtents[cascade].width) / (radius * 2.0f);
		glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(texelPerUnit));
		glm::mat4 lookAt = scaleMat * glm::lookAt(glm::vec3(0.0f), -lightDir, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 lookAtInv = glm::inverse(lookAt);

		glm::vec3 frustumCenter = cameraPosition + cameraOrientation * startCascade + cameraOrientation * (endCascade / 2.0f);
		frustumCenter = lookAt * glm::vec4(frustumCenter, 1.0f);
		frustumCenter.x = static_cast<float>(floor(frustumCenter.x));
		frustumCenter.y = static_cast<float>(floor(frustumCenter.y));
		frustumCenter = lookAtInv * glm::vec4(frustumCenter, 1.0f);

		glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter - 50.0f * glm::normalize(lightDir), frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));

		glm::mat4 proj = glm::ortho(-radius, radius, -radius, radius, -30.0f * 6.0f, 30.0f * 6.0f);
		m_lightSpaceMatrices[cascade] = proj * lightViewMatrix * model;
		m_uboData.lightSpaceMatrices[cascade] = m_lightSpaceMatrices[cascade];
		m_depthPasses[cascade]->update(m_lightSpaceMatrices[cascade]);

		lastSplitDist += m_cascadeSplits[cascade];
	}

	m_uboData.invModelView = invModelView;
	m_uniformBuffer->updateData(&m_uboData);
}
