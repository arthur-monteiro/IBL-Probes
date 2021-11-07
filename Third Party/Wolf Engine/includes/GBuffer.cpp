#include "GBuffer.h"

Wolf::GBuffer::GBuffer(Wolf::WolfInstance* engineInstance, Wolf::Scene* scene, int commandBufferID, VkExtent2D extent,
	VkSampleCountFlagBits sampleCount, Model* model, glm::mat4 mvp, bool useDepthAsStorage)
{
	m_engineInstance = engineInstance;
	m_scene = scene;
	m_sampleCount = sampleCount;

	// Render Pass
	Scene::RenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.name = "GBuffer";
	renderPassCreateInfo.commandBufferID = commandBufferID;
	renderPassCreateInfo.outputIsSwapChain = false;

	// Attachments -> depth + (normal compressed + roughness + metal) + (albedo + alpha)
	m_attachments.resize(3);
	VkImageUsageFlags depthUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	if (useDepthAsStorage)
		depthUsage |= VK_IMAGE_USAGE_STORAGE_BIT;
	VkImageLayout depthFinalLayout;
	if (useDepthAsStorage)
		depthFinalLayout = VK_IMAGE_LAYOUT_GENERAL;
	else
		depthFinalLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
	VkAttachmentStoreOp depthStoreOp;
	if (useDepthAsStorage)
		depthStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
	else
		depthStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	m_attachments[0] = Attachment(extent, VK_FORMAT_D32_SFLOAT, m_sampleCount, depthFinalLayout, depthStoreOp, depthUsage);
	m_attachments[1] = Attachment(extent, VK_FORMAT_R8G8B8A8_UNORM, m_sampleCount, VK_IMAGE_LAYOUT_GENERAL, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
	m_attachments[2] = Attachment(extent, VK_FORMAT_R8G8B8A8_UNORM, m_sampleCount, VK_IMAGE_LAYOUT_GENERAL, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
	
	m_clearValues.resize(3);
	m_clearValues[0] = { 1.0f };
	m_clearValues[1] = { 0.0f, 0.0f, 0.0f, 1.0f };
	m_clearValues[2] = { 0.0f, 0.0f, 0.0f, 1.0f };
	
	int i(0);
	for(auto& attachment : m_attachments)
	{
		Scene::RenderPassOutput renderPassOutput;
		renderPassOutput.attachment = attachment;
		renderPassOutput.clearValue = m_clearValues[i++];
		
		renderPassCreateInfo.outputs.push_back(renderPassOutput);
	}

	m_renderPassID = m_scene->addRenderPass(renderPassCreateInfo);

	// Data
	m_uboMVP = engineInstance->createUniformBufferObject(&m_mvp, 3 * sizeof(glm::mat4));

	// Renderer
	RendererCreateInfo rendererCreateInfo;

	ShaderCreateInfo vertexShaderCreateInfo{};
	vertexShaderCreateInfo.filename = "Shaders/GBuffer/vert.spv";
	vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	rendererCreateInfo.pipelineCreateInfo.shaderCreateInfos.push_back(vertexShaderCreateInfo);

	ShaderCreateInfo fragmentShaderCreateInfo{};
	fragmentShaderCreateInfo.filename = "Shaders/GBuffer/frag.spv";
	fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	rendererCreateInfo.pipelineCreateInfo.shaderCreateInfos.push_back(fragmentShaderCreateInfo);
	
	rendererCreateInfo.inputVerticesTemplate = InputVertexTemplate::FULL_3D_MATERIAL;
	rendererCreateInfo.instanceTemplate = InstanceTemplate::NO;
	rendererCreateInfo.renderPassID = m_renderPassID;
	rendererCreateInfo.pipelineCreateInfo.extent = extent;

	DescriptorSetGenerator descriptorSetGenerator;
	descriptorSetGenerator.addUniformBuffer(m_uboMVP, VK_SHADER_STAGE_VERTEX_BIT, 0);
	descriptorSetGenerator.addSampler(model->getSampler(), VK_SHADER_STAGE_FRAGMENT_BIT, 1);
	descriptorSetGenerator.addImages(model->getImages(), VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, 2);

	rendererCreateInfo.descriptorSetLayout = descriptorSetGenerator.getDescriptorLayouts();
	
	rendererCreateInfo.pipelineCreateInfo.alphaBlending = { false, false };

	m_rendererID = m_scene->addRenderer(rendererCreateInfo);

	Renderer::AddMeshInfo addMeshInfo{};
	addMeshInfo.vertexBuffer = model->getVertexBuffers()[0];
	addMeshInfo.renderPassID = m_renderPassID;
	addMeshInfo.rendererID = m_rendererID;

	addMeshInfo.descriptorSetCreateInfo = descriptorSetGenerator.getDescritorSetCreateInfo();

	m_scene->addMesh(addMeshInfo);
}

void Wolf::GBuffer::updateMVPMatrix(glm::mat4 m, glm::mat4 v, glm::mat4 p)
{
	m_mvp = { p, m, v};
	m_uboMVP->updateData(&m_mvp);
}
