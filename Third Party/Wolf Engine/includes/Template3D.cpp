#include "Template3D.h"

#include <utility>

Wolf::Template3D::Template3D(Wolf::WolfInstance* wolfInstance, Wolf::Scene* scene, std::string modelFilename,
                             std::string mtlFolder, float ratio) : m_wolfInstance(wolfInstance), m_scene(scene)
{
	// Model creation
	Model::ModelCreateInfo modelCreateInfo{};
	modelCreateInfo.inputVertexTemplate = InputVertexTemplate::FULL_3D_MATERIAL;
	Model* model = m_wolfInstance->createModel<>(modelCreateInfo);

	Model::ModelLoadingInfo modelLoadingInfo;
	modelLoadingInfo.filename = std::move(modelFilename);
	modelLoadingInfo.mtlFolder = std::move(mtlFolder);
	model->loadObj(modelLoadingInfo);

	// Data
	{
		m_projectionMatrix = glm::perspective(glm::radians(45.0f), 16.0f / 9.0f, 0.1f, 100.0f);
		m_projectionMatrix[1][1] *= -1;
		m_viewMatrix = glm::lookAt(glm::vec3(-2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		m_modelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.01f));
	}

	// Draw
	{
		// GBuffer
		Scene::CommandBufferCreateInfo commandBufferCreateInfo;
		commandBufferCreateInfo.finalPipelineStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		commandBufferCreateInfo.commandType = Scene::CommandType::GRAPHICS;
		m_gBufferCommandBufferID = scene->addCommandBuffer(commandBufferCreateInfo);
		
		m_GBuffer = std::make_unique<GBuffer>(wolfInstance, scene, m_gBufferCommandBufferID, wolfInstance->getWindowSize(), VK_SAMPLE_COUNT_1_BIT, model, glm::mat4(1.0f), true);

		Image* depth = m_GBuffer->getDepth();
		Image* albedo = m_GBuffer->getAlbedo();
		Image* normalRoughnessMetal = m_GBuffer->getNormalRoughnessMetal();

		// SSAO
		commandBufferCreateInfo.finalPipelineStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		commandBufferCreateInfo.commandType = Scene::CommandType::COMPUTE;
		m_SSAOCommandBufferID = scene->addCommandBuffer(commandBufferCreateInfo);
		m_ssao = std::make_unique<SSAO>(wolfInstance, scene, m_SSAOCommandBufferID, wolfInstance->getWindowSize(), m_projectionMatrix, depth, normalRoughnessMetal, 0.1f, 100.0f);

		// CSM
		m_cascadedShadowMapping = std::make_unique<CascadedShadowMapping>(wolfInstance, scene, model, 0.1f, 100.0f, 32.f, glm::radians(45.0f), m_wolfInstance->getWindowSize(), 
			depth, m_projectionMatrix);

		// Light Propagation Volume
		m_lightPropagationVolumes = std::make_unique<LightPropagationVolumes>(wolfInstance, scene, model, m_projectionMatrix, m_modelMatrix, m_lightDir, m_cascadedShadowMapping->getCascadeSplits(),
			m_cascadedShadowMapping->getDepthTextures());

		// Direct Lighting
		m_directLightingSSRBloomCommandBufferID = scene->addCommandBuffer(commandBufferCreateInfo);
		m_directLighting = std::make_unique<DirectLightingPBR>(wolfInstance, scene, m_directLightingSSRBloomCommandBufferID, m_wolfInstance->getWindowSize(), depth,
			albedo, normalRoughnessMetal, m_cascadedShadowMapping->getOutputShadowMaskTexture(), m_cascadedShadowMapping->getOutputVolumetricLightMaskImage(),
			m_ssao->getOutputImage(), m_lightPropagationVolumes->getPropagationImage(), m_projectionMatrix, 0.1f, 100.0f);

		// Merge
		Scene::ComputePassCreateInfo mergeComputePassCreateInfo;
		mergeComputePassCreateInfo.computeShaderPath = "Shaders/Merge/comp.spv";
		mergeComputePassCreateInfo.outputIsSwapChain = true;
		mergeComputePassCreateInfo.commandBufferID = -1;
		mergeComputePassCreateInfo.dispatchGroups = { 16, 16, 1 };

		DescriptorSetGenerator mergeDescriptorSetGenerator;
		mergeDescriptorSetGenerator.addImages({ m_directLighting->getOutputImage() }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 0);

		mergeComputePassCreateInfo.descriptorSetCreateInfo = mergeDescriptorSetGenerator.getDescritorSetCreateInfo();
		mergeComputePassCreateInfo.outputBinding = 1;

		m_mergeComputePassID = scene->addComputePass(mergeComputePassCreateInfo);
	}

	m_scene->record();
}

void Wolf::Template3D::update(glm::mat4 view, glm::vec3 cameraPosition, glm::vec3 cameraOrientation)
{
	m_viewMatrix = view;
	updateMVP();
	m_cascadedShadowMapping->updateMatrices(m_lightDir, cameraPosition, cameraOrientation, m_modelMatrix, glm::inverse(m_viewMatrix * m_modelMatrix));

	glm::mat4 voxelProjection = glm::ortho(-32.0f, 32.0f, -4.0f, 16.0f, 0.0f, 64.0f) *
		glm::lookAt(glm::vec3(0.0f, 0.0f, -32.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	
	m_directLighting->update(glm::transpose(glm::inverse(m_viewMatrix)) * glm::vec4(m_lightDir, 1.0f),
		voxelProjection * glm::inverse(m_viewMatrix));
	m_lightPropagationVolumes->update(view, m_cascadedShadowMapping->getLightSpaceMatrices(), m_modelMatrix);
}

std::vector<int> Wolf::Template3D::getCommandBufferToSubmit()
{
	std::vector<int> r;
	r.push_back(m_gBufferCommandBufferID);
	std::vector<int> csmCommandBuffer = m_cascadedShadowMapping->getCascadeCommandBuffers();
	for (auto& commandBuffer : csmCommandBuffer)
		r.push_back(commandBuffer);
	r.push_back(m_SSAOCommandBufferID);
	std::vector<int> ssaoCommandBuffer = m_ssao->getCommandBufferIDs();
	for (auto& commandBuffer : ssaoCommandBuffer)
		r.push_back(commandBuffer);
	std::vector<int> lpvCommandBuffers = m_lightPropagationVolumes->getCommandBufferIDs();
	for(auto& commandBuffer : lpvCommandBuffers)
		r.push_back(commandBuffer);
	r.push_back(m_directLightingSSRBloomCommandBufferID);
	return r;
}

std::vector<std::pair<int, int>> Wolf::Template3D::getCommandBufferSynchronisation()
{
	std::vector<std::pair<int, int>> r =
			{ {m_gBufferCommandBufferID, m_SSAOCommandBufferID }, { m_directLightingSSRBloomCommandBufferID, -1 } };

	std::vector<std::pair<int, int>> csmSynchronisation = m_cascadedShadowMapping->getCommandBufferSynchronisation();
	for(auto& sync : csmSynchronisation)
	{
		r.push_back(sync);
	}
	r.emplace_back(m_cascadedShadowMapping->getCascadeCommandBuffers().back(), m_directLightingSSRBloomCommandBufferID);

	std::vector<std::pair<int, int>> ssaoSynchronisation = m_ssao->getCommandBufferSynchronisation();
	r.emplace_back(m_SSAOCommandBufferID, ssaoSynchronisation[0].first);
	for (auto& sync : ssaoSynchronisation)
	{
		r.push_back(sync);
	}
	r.emplace_back(ssaoSynchronisation.back().second, m_directLightingSSRBloomCommandBufferID);

	std::vector<std::pair<int, int>> lpvSync = m_lightPropagationVolumes->getCommandBufferSynchronisations();
	for (auto& sync : lpvSync)
	{
		r.push_back(sync);		
	}
	
	r.emplace_back(m_lightPropagationVolumes->getCommandBufferIDs().back(), m_directLightingSSRBloomCommandBufferID);
	r.emplace_back(m_lightPropagationVolumes->getCommandBufferIDs()[1], m_directLightingSSRBloomCommandBufferID);
	
	return r;
}

void Wolf::Template3D::updateMVP()
{
	m_GBuffer->updateMVPMatrix(m_modelMatrix, m_viewMatrix, m_projectionMatrix);
}