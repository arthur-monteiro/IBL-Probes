#include "Template3D_VR.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

Wolf::Template3D_VR::Template3D_VR(Wolf::WolfInstance* wolfInstance, Wolf::Scene* scene, std::string modelFilename,
	std::string mtlFolder) : m_wolfInstance(wolfInstance), m_scene(scene)
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
		m_modelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.01f));
	}

	// GBuffer
	{
		Scene::CommandBufferCreateInfo commandBufferCreateInfo;
		commandBufferCreateInfo.finalPipelineStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		commandBufferCreateInfo.commandType = Scene::CommandType::GRAPHICS;
		m_gBufferCommandBufferID = scene->addCommandBuffer(commandBufferCreateInfo);

		m_GBuffer = std::make_unique<GBufferStereoscopic>(wolfInstance, scene, m_gBufferCommandBufferID, wolfInstance->getWindowSize(), VK_SAMPLE_COUNT_1_BIT, model, glm::mat4(1.0f), true);
	}

	Image* depth = m_GBuffer->getDepth();
	Image* albedo = m_GBuffer->getAlbedo();
	Image* normalRoughnessMetal = m_GBuffer->getNormalRoughnessMetal();

	// CSM
	{
		m_cascadedShadowMapping = std::make_unique<CascadedShadowMappingStereoscopic>(wolfInstance, scene, model, 0.2f, 100.0f, 32.f, glm::radians(45.0f), m_wolfInstance->getWindowSize(),
			depth, m_wolfInstance->getVRProjMatrices());
	}

	// Direct Lighting
 	{
 		Scene::CommandBufferCreateInfo commandBufferCreateInfo;
 		commandBufferCreateInfo.finalPipelineStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
 		commandBufferCreateInfo.commandType = Scene::CommandType::COMPUTE;
		m_directLightingCommandBufferID = scene->addCommandBuffer(commandBufferCreateInfo);
 
 		m_directLightingCommandBufferID = scene->addCommandBuffer(commandBufferCreateInfo);
 		m_directLighting = std::make_unique<DirectLightingStereoscopic>(wolfInstance, scene, m_directLightingCommandBufferID, m_wolfInstance->getWindowSize(), depth,
 			albedo, normalRoughnessMetal, m_cascadedShadowMapping->getOutputShadowMaskImage(), m_cascadedShadowMapping->getOutputVolumetricLightMaskTexture(),
 			nullptr, nullptr, m_wolfInstance->getVRProjMatrices(), 0.2f, 100.0f);
 	}

	// Tone mapping
	{
		Scene::CommandBufferCreateInfo commandBufferCreateInfo;
		commandBufferCreateInfo.finalPipelineStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		commandBufferCreateInfo.commandType = Scene::CommandType::COMPUTE;
		m_toneMappingCommandBufferID = scene->addCommandBuffer(commandBufferCreateInfo);

		Scene::ComputePassCreateInfo toneMappingComputePassCreateInfo;
		toneMappingComputePassCreateInfo.computeShaderPath = "Shaders/toneMapping/comp.spv";
		toneMappingComputePassCreateInfo.outputIsSwapChain = false;
		toneMappingComputePassCreateInfo.commandBufferID = m_toneMappingCommandBufferID;
		toneMappingComputePassCreateInfo.dispatchGroups = { 16, 16, 1 };
		toneMappingComputePassCreateInfo.extent = m_wolfInstance->getWindowSize();

		DescriptorSetGenerator mergeDescriptorSetGenerator;
		mergeDescriptorSetGenerator.addImages({ m_directLighting->getOutputImage() }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 0);

		Image::CreateImageInfo createImageInfo;
		createImageInfo.extent = { m_wolfInstance->getWindowSize().width, m_wolfInstance->getWindowSize().height, 1 };
		createImageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		createImageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
		createImageInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
		createImageInfo.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
		createImageInfo.mipLevels = 1;
		m_toneMappingOutputImage = wolfInstance->createImage(createImageInfo);
		m_toneMappingOutputImage->setImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
		mergeDescriptorSetGenerator.addImages({ m_toneMappingOutputImage }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 1);

		toneMappingComputePassCreateInfo.descriptorSetCreateInfo = mergeDescriptorSetGenerator.getDescritorSetCreateInfo();

		m_toneMappingComputePassID = scene->addComputePass(toneMappingComputePassCreateInfo);
	}

	// Transfer to swapchain
	{
		Scene::TransferAddInfo transferAddInfo;
		transferAddInfo.origin = m_toneMappingOutputImage;
		transferAddInfo.outputIsSwapChain = true;
		transferAddInfo.commandBufferID = -1;

		transferAddInfo.beforeRecord = [](void* thisTemplate, VkCommandBuffer commandBuffer) 
		{
			Image::transitionImageLayoutUsingCommandBuffer(commandBuffer, reinterpret_cast<Template3D_VR*>(thisTemplate)->m_toneMappingOutputImage->getImage(), VK_FORMAT_R8G8B8A8_UNORM,
				VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 1, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0);
		};
		transferAddInfo.dataForBeforeRecordCallback = this;

		transferAddInfo.afterRecord = [](void* thisTemplate, VkCommandBuffer commandBuffer)
		{
			Image::transitionImageLayoutUsingCommandBuffer(commandBuffer, reinterpret_cast<Template3D_VR*>(thisTemplate)->m_toneMappingOutputImage->getImage(), VK_FORMAT_R8G8B8A8_UNORM,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, 1, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0);
		};
		transferAddInfo.dataForAfterRecordCallback = this;

		scene->addTransfer(transferAddInfo);
	}

	//// Render Pass Creation
	//Scene::RenderPassCreateInfo renderPassCreateInfo{};
	//renderPassCreateInfo.commandBufferID = -1; // default command buffer
	//renderPassCreateInfo.outputIsSwapChain = true;
	//m_renderPassID = m_scene->addRenderPass(renderPassCreateInfo);

	//RendererCreateInfo rendererCreateInfo;

	//ShaderCreateInfo vertexShaderCreateInfo{};
	//vertexShaderCreateInfo.filename = "Shaders/copyToSwapchain/vert.spv";
	//vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	//rendererCreateInfo.pipelineCreateInfo.shaderCreateInfos.push_back(vertexShaderCreateInfo);

	//ShaderCreateInfo fragmentShaderCreateInfo{};
	//fragmentShaderCreateInfo.filename = "Shaders/copyToSwapchain/frag.spv";
	//fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	//rendererCreateInfo.pipelineCreateInfo.shaderCreateInfos.push_back(fragmentShaderCreateInfo);

	//rendererCreateInfo.inputVerticesTemplate = InputVertexTemplate::POSITION_TEXTURECOORD_2D;
	//rendererCreateInfo.instanceTemplate = InstanceTemplate::NO;
	//rendererCreateInfo.renderPassID = m_renderPassID;
	//rendererCreateInfo.pipelineCreateInfo.alphaBlending = { false };

	//DescriptorSetGenerator descriptorSetGenerator;
	//descriptorSetGenerator.addImages({ m_directLighting->getOutputTexture()->getImage() }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, 0)

	//// For each eye (normally it should stay 2)
	//for(int i(0); i < 2; ++i)
	//{
	//	RendererCreateInfo rendererCreateInfo;

	//	ShaderCreateInfo vertexShaderCreateInfo{};
	//	vertexShaderCreateInfo.filename = "Shaders/template3D/vert.spv";
	//	vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	//	rendererCreateInfo.pipelineCreateInfo.shaderCreateInfos.push_back(vertexShaderCreateInfo);

	//	ShaderCreateInfo fragmentShaderCreateInfo{};
	//	fragmentShaderCreateInfo.filename = "Shaders/template3D/frag.spv";
	//	fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	//	rendererCreateInfo.pipelineCreateInfo.shaderCreateInfos.push_back(fragmentShaderCreateInfo);

	//	rendererCreateInfo.inputVerticesTemplate = InputVertexTemplate::FULL_3D_MATERIAL;
	//	rendererCreateInfo.instanceTemplate = InstanceTemplate::NO;
	//	rendererCreateInfo.renderPassID = m_renderPassID;
	//	rendererCreateInfo.pipelineCreateInfo.alphaBlending = { false };

	//	DescriptorSetGenerator descriptorSetGenerator;

	//	glm::mat4 mvp = glm::mat4(1.0f);
	//	m_ubosMVP[i] = wolfInstance->createUniformBufferObject(&mvp, sizeof(glm::mat4));
	//	descriptorSetGenerator.addUniformBuffer(m_ubosMVP[i], VK_SHADER_STAGE_VERTEX_BIT, 0);

	//	descriptorSetGenerator.addSampler(model->getSampler(), VK_SHADER_STAGE_FRAGMENT_BIT, 1);
	//	descriptorSetGenerator.addImages(model->getImages(), VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, 2);

	//	if(i == 0) // left
	//		rendererCreateInfo.pipelineCreateInfo.viewportScale = { 0.5f, 1.0f };
	//	else
	//	{
	//		rendererCreateInfo.pipelineCreateInfo.viewportScale = { 0.5f, 1.0f };
	//		rendererCreateInfo.pipelineCreateInfo.viewportOffset = { 0.5f, 0.0f };
	//	}

	//	rendererCreateInfo.descriptorLayouts = descriptorSetGenerator.getDescriptorLayouts();
	//		
	//	m_rendererIDs[i] = m_scene->addRenderer(rendererCreateInfo);

	//	Renderer::AddMeshInfo addMeshInfo{};
	//	addMeshInfo.vertexBuffer = model->getVertexBuffers()[0];
	//	addMeshInfo.renderPassID = m_renderPassID;
	//	addMeshInfo.rendererID = m_rendererIDs[i];

	//	addMeshInfo.descriptorSetCreateInfo = descriptorSetGenerator.getDescritorSetCreateInfo();

	//	m_scene->addMesh(addMeshInfo);
	//}

	m_scene->record();
}


void Wolf::Template3D_VR::update()
{
	m_GBuffer->updateMatrices(m_modelMatrix, m_wolfInstance->getVRViewMatrices()[0], m_wolfInstance->getVRViewMatrices()[1], m_wolfInstance->getVRProjMatrices()[0],
		m_wolfInstance->getVRProjMatrices()[1]);

	m_cascadedShadowMapping->updateMatrices(m_lightDir, m_wolfInstance->getVREyePositions()[0], m_wolfInstance->getVREyeDirections()[0], m_modelMatrix,
		{ glm::inverse(m_wolfInstance->getVRViewMatrices()[0] * m_modelMatrix), glm::inverse(m_wolfInstance->getVRViewMatrices()[1] * m_modelMatrix) });

	m_directLighting->update({ glm::transpose(glm::inverse(m_wolfInstance->getVRViewMatrices()[0])) * glm::vec4(m_lightDir, 1.0f),
		glm::transpose(glm::inverse(m_wolfInstance->getVRViewMatrices()[1])) * glm::vec4(m_lightDir, 1.0f) }, glm::mat4(1.0f));

// 	for (int eye = 0; eye < 2; ++eye)
// 	{
// 		glm::mat4 glmView = m_wolfInstance->getVRViewMatrices()[eye];
// 		glm::mat4 glmProjection = m_wolfInstance->getVRProjMatrices()[eye];
// 
// 		glm::mat4 mvp = glmProjection * glmView * glm::scale(glm::mat4(1.0f), glm::vec3(0.01f));
// 		m_ubosMVP[eye]->updateData(&mvp);
// 	}
}

std::vector<int> Wolf::Template3D_VR::getCommandBufferToSubmit()
{
	std::vector<int> r;

	r.push_back(m_gBufferCommandBufferID);
	std::vector<int> csmCommandBuffer = m_cascadedShadowMapping->getCascadeCommandBuffers();
	for (auto& commandBuffer : csmCommandBuffer)
		r.push_back(commandBuffer);
	r.push_back(m_directLightingCommandBufferID);
	r.push_back(m_toneMappingCommandBufferID);

	return r;
}

std::vector<std::pair<int, int>> Wolf::Template3D_VR::getCommandBufferSynchronisation()
{
	std::vector<std::pair<int, int>> r = { { m_gBufferCommandBufferID, -1 } };

	std::vector<std::pair<int, int>> csmSynchronisation = m_cascadedShadowMapping->getCommandBufferSynchronisation();
	for (auto& sync : csmSynchronisation)
	{
		r.push_back(sync);
	}
	r.emplace_back(m_cascadedShadowMapping->getCascadeCommandBuffers().back(), m_directLightingCommandBufferID);
	r.emplace_back(m_directLightingCommandBufferID, m_toneMappingCommandBufferID);
	r.emplace_back(m_toneMappingCommandBufferID, -1);

	return r;
}
