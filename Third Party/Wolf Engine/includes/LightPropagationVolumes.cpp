#include "LightPropagationVolumes.h"

Wolf::LightPropagationVolumes::LightPropagationVolumes(Wolf::WolfInstance* engineInstance, Wolf::Scene* scene, Model* model, glm::mat4 projection, glm::mat4 modelMat, glm::vec3 lightDir,
	glm::vec4 cascadeSplits, std::array<Image*, 4> depthTextures)
{
	m_engineInstance = engineInstance;
	m_scene = scene;

	Image::CreateImageInfo createImageInfo;
	createImageInfo.extent = { VOXEL_SIZE, VOXEL_SIZE, VOXEL_SIZE };
	createImageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT;
	createImageInfo.format = VK_FORMAT_R8_UNORM;
	createImageInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
	createImageInfo.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
	createImageInfo.mipLevels = 1;
	m_voxelImage = engineInstance->createImage(createImageInfo);
	m_voxelImage->setImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

	//modelMat = glm::mat4(1.0f);
	// X
	m_projections[0] = glm::ortho(-32.0f, 32.0f, -4.0f, 16.0f, 0.0f, 64.0f) *
		glm::lookAt(glm::vec3(-32.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
		modelMat;

	// Y
	glm::mat4 YLookAtMatrix = glm::mat4(1.0f);
	glm::vec3 f(0.0f, 1.0f, 0.0f);
	glm::vec3 s(1.0f, 0.0f, 0.0f);
	glm::vec3 u(0.0f, 0.0f, 1.0f);

	glm::vec3 eye(0.0f, -4.0f, 0.0f);
	
	YLookAtMatrix[0][0] = s.x;
	YLookAtMatrix[1][0] = s.y;
	YLookAtMatrix[2][0] = s.z;
	YLookAtMatrix[0][1] = u.x;
	YLookAtMatrix[1][1] = u.y;
	YLookAtMatrix[2][1] = u.z;
	YLookAtMatrix[0][2] = -f.x;
	YLookAtMatrix[1][2] = -f.y;
	YLookAtMatrix[2][2] = -f.z;
	YLookAtMatrix[3][0] = -dot(s, eye);
	YLookAtMatrix[3][1] = -dot(u, eye);
	YLookAtMatrix[3][2] = dot(f, eye);
	
	m_projections[1] = glm::ortho(-32.0f, 32.0f, -32.0f, 32.0f, 0.0f, 20.0f) * YLookAtMatrix *
		/*glm::lookAt(glm::vec3(0.0f, -4.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)) **/
		modelMat;

	// Z	
	m_projections[2] = glm::ortho(-32.0f, 32.0f, -4.0f, 16.0f, 0.0f, 64.0f) *
		glm::lookAt(glm::vec3(0.0f, 0.0f, -32.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
		modelMat;

	m_uboVoxelization = engineInstance->createUniformBufferObject(&m_projections, 3 * sizeof(glm::mat4));

	Scene::CommandBufferCreateInfo commandBufferCreateInfo;
	commandBufferCreateInfo.finalPipelineStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	commandBufferCreateInfo.commandType = Scene::CommandType::GRAPHICS;
	m_commandBufferID = scene->addCommandBuffer(commandBufferCreateInfo);

	Scene::RenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.name = "Light Propagation Volumes";
	renderPassCreateInfo.commandBufferID = m_commandBufferID;
	renderPassCreateInfo.outputIsSwapChain = false;
	renderPassCreateInfo.extent = { VOXEL_SIZE, VOXEL_SIZE };

	m_attachments.resize(1);
	m_attachments[0] = Attachment({ VOXEL_SIZE, VOXEL_SIZE }, VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
	m_clearValues.resize(1);
	m_clearValues[0] = { 0.5f, 0.0f, 0.5f, 1.0f };

	int i(0);
	for (auto& attachment : m_attachments)
	{
		Scene::RenderPassOutput renderPassOutput;
		renderPassOutput.attachment = attachment;
		renderPassOutput.clearValue = m_clearValues[i++];

		renderPassCreateInfo.outputs.push_back(renderPassOutput);
	}

	m_renderPassID = m_scene->addRenderPass(renderPassCreateInfo);

	{
		RendererCreateInfo rendererCreateInfo;

		ShaderCreateInfo vertexShaderCreateInfo{};
		vertexShaderCreateInfo.filename = "Shaders/LightPropagationVolumes/vert.spv";
		vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		rendererCreateInfo.pipelineCreateInfo.shaderCreateInfos.push_back(vertexShaderCreateInfo);

		ShaderCreateInfo fragmentShaderCreateInfo{};
		fragmentShaderCreateInfo.filename = "Shaders/LightPropagationVolumes/frag.spv";
		fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		rendererCreateInfo.pipelineCreateInfo.shaderCreateInfos.push_back(fragmentShaderCreateInfo);

		rendererCreateInfo.inputVerticesTemplate = InputVertexTemplate::FULL_3D_MATERIAL;
		rendererCreateInfo.instanceTemplate = InstanceTemplate::NO;
		rendererCreateInfo.renderPassID = m_renderPassID;
		rendererCreateInfo.pipelineCreateInfo.extent = { VOXEL_SIZE, VOXEL_SIZE };
		rendererCreateInfo.pipelineCreateInfo.enableDepthTesting = false;
		rendererCreateInfo.pipelineCreateInfo.enableConservativeRasterization = false;
		rendererCreateInfo.pipelineCreateInfo.alphaBlending = { false };

		DescriptorSetGenerator descriptorSetGenerator;
		descriptorSetGenerator.addImages({ m_voxelImage }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, 1);
		descriptorSetGenerator.addUniformBuffer(m_uboVoxelization, VK_SHADER_STAGE_VERTEX_BIT, 0);

		rendererCreateInfo.descriptorSetLayout = descriptorSetGenerator.getDescriptorLayouts();

		m_voxelisationRendererID = m_scene->addRenderer(rendererCreateInfo);

		Renderer::AddMeshInfo addMeshInfo{};
		addMeshInfo.vertexBuffer = model->getVertexBuffers()[0];
		addMeshInfo.renderPassID = m_renderPassID;
		addMeshInfo.rendererID = m_voxelisationRendererID;

		addMeshInfo.descriptorSetCreateInfo = descriptorSetGenerator.getDescritorSetCreateInfo();

		m_scene->addMesh(addMeshInfo);
	}

	buildInjection(model, cascadeSplits, depthTextures);
	buildPropagation();

	// Voxel viewer
	{
		// Data
		createImageInfo.extent = { engineInstance->getWindowSize().width, engineInstance->getWindowSize().height, 1 };
		createImageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT;
		createImageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
		createImageInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
		createImageInfo.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
		createImageInfo.mipLevels = 1;
		m_viewerOutput = engineInstance->createImage(createImageInfo);
		m_viewerOutput->setImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

		m_voxelViewerMatrices[2] = glm::ortho(-32.0f, 32.0f, -4.0f, 16.0f, 0.0f, 64.0f) *
			glm::lookAt(glm::vec3(0.0f, 0.0f, -32.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		m_voxelViewerMatrices[1] = glm::inverse(projection);
		m_uboVoxelViewer = engineInstance->createUniformBufferObject(&m_voxelViewerMatrices, 3 * sizeof(glm::mat4));
		
		commandBufferCreateInfo.finalPipelineStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		commandBufferCreateInfo.commandType = Scene::CommandType::COMPUTE;
		m_viewerBufferID = scene->addCommandBuffer(commandBufferCreateInfo);

		Scene::ComputePassCreateInfo voxelViewerComputePassCreateInfo;
		voxelViewerComputePassCreateInfo.extent = engineInstance->getWindowSize();
		voxelViewerComputePassCreateInfo.computeShaderPath = "Shaders/LightPropagationVolumes/comp.spv";
		voxelViewerComputePassCreateInfo.outputIsSwapChain = false;
		voxelViewerComputePassCreateInfo.commandBufferID = m_viewerBufferID;
		voxelViewerComputePassCreateInfo.dispatchGroups = { 16, 16, 1 };

		DescriptorSetGenerator descriptorSetGenerator;
		descriptorSetGenerator.addImages({ m_injectionImages[0] }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 0);
		descriptorSetGenerator.addImages({ m_injectionImages[1] }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 1);
		descriptorSetGenerator.addImages({ m_injectionImages[2] }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 2);
		descriptorSetGenerator.addImages({ m_injectionImages.back() }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 3);
		descriptorSetGenerator.addImages({ m_viewerOutput }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 4);
		descriptorSetGenerator.addUniformBuffer(m_uboVoxelViewer, VK_SHADER_STAGE_COMPUTE_BIT, 5);

		voxelViewerComputePassCreateInfo.descriptorSetCreateInfo = descriptorSetGenerator.getDescritorSetCreateInfo();

		m_viewerComputePassID = scene->addComputePass(voxelViewerComputePassCreateInfo);
	}

	/* Clear */
	{
		commandBufferCreateInfo.finalPipelineStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		commandBufferCreateInfo.commandType = Scene::CommandType::COMPUTE;
		m_clearCommandBufferID = scene->addCommandBuffer(commandBufferCreateInfo);

		Scene::ComputePassCreateInfo clearComputePassCreateInfo;
		clearComputePassCreateInfo.extent = { VOXEL_SIZE, VOXEL_SIZE };
		clearComputePassCreateInfo.computeShaderPath = "Shaders/LightPropagationVolumes/clear.spv";
		clearComputePassCreateInfo.outputIsSwapChain = false;
		clearComputePassCreateInfo.commandBufferID = m_clearCommandBufferID;
		clearComputePassCreateInfo.dispatchGroups = { 8, 8, VOXEL_SIZE / 8 };

		DescriptorSetGenerator descriptorSetGenerator;

		for (int i(0); i < m_injectionImages.size(); ++i)
			descriptorSetGenerator.addImages({ m_injectionImages[i] }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, i);

		clearComputePassCreateInfo.descriptorSetCreateInfo = descriptorSetGenerator.getDescritorSetCreateInfo();
			
		m_clearComputePassID = scene->addComputePass(clearComputePassCreateInfo);
	}
	
}

void Wolf::LightPropagationVolumes::update(glm::mat4 view, std::array<glm::mat4, 4> lightSpaceMatrices, glm::mat4 modelMat)
{
	m_uboInjectionData.lightSpaceMatrices = std::move(lightSpaceMatrices);
	m_uboInjectionData.modelView = view * modelMat;
	m_uboInjection->updateData(&m_uboInjectionData);
	
	m_voxelViewerMatrices[0] = glm::inverse(view);
	m_uboVoxelViewer->updateData(&m_voxelViewerMatrices);
}

void Wolf::LightPropagationVolumes::buildInjection(Model* model, glm::vec4 cascadeSplits, std::array<Image*, 4> depthTextures)
{
	// Data
	for(int i(0); i < m_injectionImages.size(); ++i)
	{
		Image::CreateImageInfo createImageInfo;
		createImageInfo.extent = { VOXEL_SIZE, VOXEL_SIZE, VOXEL_SIZE };
		createImageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT;
		createImageInfo.format = VK_FORMAT_R32_UINT;
		createImageInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
		createImageInfo.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
		createImageInfo.mipLevels = 1;
		m_injectionImages[i] = m_engineInstance->createImage(createImageInfo);
		m_injectionImages[i]->setImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	}
	
	m_uboInjectionData.projectionX = m_projections[0];
	m_uboInjectionData.projectionY = m_projections[1];
	m_uboInjectionData.projectionZ = m_projections[2];
	m_uboInjectionData.cascadeSplits = cascadeSplits;

	m_uboInjection = m_engineInstance->createUniformBufferObject(&m_uboInjectionData, sizeof(m_uboInjectionData));

	// Command Buffer
	Scene::CommandBufferCreateInfo commandBufferCreateInfo;
	commandBufferCreateInfo.finalPipelineStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	commandBufferCreateInfo.commandType = Scene::CommandType::GRAPHICS;
	m_injectionCommandBufferID = m_scene->addCommandBuffer(commandBufferCreateInfo);

	// Render Pass
	Scene::RenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.name = "Light Propagation Volumes injection";
	renderPassCreateInfo.commandBufferID = m_injectionCommandBufferID;
	renderPassCreateInfo.outputIsSwapChain = false;
	renderPassCreateInfo.extent = { VOXEL_SIZE, VOXEL_SIZE };

	m_injectionAttachments.resize(1);
	m_injectionAttachments[0] = Attachment({ VOXEL_SIZE, VOXEL_SIZE }, VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
	m_injectionClearValues.resize(1);
	m_injectionClearValues[0] = { 0.5f, 0.0f, 0.5f, 1.0f };

	int i(0);
	for (auto& attachment : m_injectionAttachments)
	{
		Scene::RenderPassOutput renderPassOutput;
		renderPassOutput.attachment = attachment;
		renderPassOutput.clearValue = m_injectionClearValues[i++];

		renderPassCreateInfo.outputs.push_back(renderPassOutput);
	}

	m_injectionRenderPassID = m_scene->addRenderPass(renderPassCreateInfo);

	// Renderer
	RendererCreateInfo rendererCreateInfo;

	ShaderCreateInfo vertexShaderCreateInfo{};
	vertexShaderCreateInfo.filename = "Shaders/LightPropagationVolumes/injectionVert.spv";
	vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	rendererCreateInfo.pipelineCreateInfo.shaderCreateInfos.push_back(vertexShaderCreateInfo);

	ShaderCreateInfo fragmentShaderCreateInfo{};
	fragmentShaderCreateInfo.filename = "Shaders/LightPropagationVolumes/injectionFrag.spv";
	fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	rendererCreateInfo.pipelineCreateInfo.shaderCreateInfos.push_back(fragmentShaderCreateInfo);
	
	rendererCreateInfo.inputVerticesTemplate = InputVertexTemplate::FULL_3D_MATERIAL;
	rendererCreateInfo.instanceTemplate = InstanceTemplate::NO;
	rendererCreateInfo.renderPassID = m_injectionRenderPassID;
	rendererCreateInfo.pipelineCreateInfo.extent = { VOXEL_SIZE, VOXEL_SIZE };
	rendererCreateInfo.pipelineCreateInfo.enableDepthTesting = false;
	rendererCreateInfo.pipelineCreateInfo.enableConservativeRasterization = false;
	rendererCreateInfo.pipelineCreateInfo.alphaBlending = { false };

	DescriptorSetGenerator descriptorSetGenerator;
	
	std::vector<Image*> voxelImages(m_injectionImages.size());
	for (int i(0); i < m_injectionImages.size(); ++i)
		voxelImages[i] = m_injectionImages[i];
	descriptorSetGenerator.addImages(voxelImages, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, 2);
	
	descriptorSetGenerator.addSampler(model->getSampler(), VK_SHADER_STAGE_FRAGMENT_BIT, 1);
	
	std::vector<Image*> depthAndMaterialImages(model->getNumberOfImages() + 4);
	std::vector<Image*> materialImages = model->getImages();
	for(int i(0); i < depthAndMaterialImages.size(); ++i)
	{
		if (i < 4)
			depthAndMaterialImages[i] = depthTextures[i];
		else
			depthAndMaterialImages[i] = materialImages[i - 4];
	}
	descriptorSetGenerator.addImages(depthAndMaterialImages, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, 9);
	
	descriptorSetGenerator.addUniformBuffer(m_uboInjection, VK_SHADER_STAGE_VERTEX_BIT, 0);

	rendererCreateInfo.descriptorSetLayout = descriptorSetGenerator.getDescriptorLayouts();

	m_voxelisationRendererID = m_scene->addRenderer(rendererCreateInfo);

	// Add Model
	Renderer::AddMeshInfo addMeshInfo{};
	addMeshInfo.vertexBuffer = model->getVertexBuffers()[0];
	addMeshInfo.renderPassID = m_injectionRenderPassID;
	addMeshInfo.rendererID = m_voxelisationRendererID;

	addMeshInfo.descriptorSetCreateInfo = descriptorSetGenerator.getDescritorSetCreateInfo();

	m_scene->addMesh(addMeshInfo);
}

void Wolf::LightPropagationVolumes::buildPropagation()
{
	// Data
	Image::CreateImageInfo createImageInfo;
	createImageInfo.extent = { VOXEL_SIZE, VOXEL_SIZE, VOXEL_SIZE };
	createImageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT;
	createImageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	createImageInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
	createImageInfo.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
	createImageInfo.mipLevels = 1;
	m_lightVolumesPropagationImage = m_engineInstance->createImage(createImageInfo);
	m_lightVolumesPropagationImage->setImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

	// Command Buffer
	Scene::CommandBufferCreateInfo commandBufferCreateInfo;
	commandBufferCreateInfo.finalPipelineStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	commandBufferCreateInfo.commandType = Scene::CommandType::COMPUTE;
	m_propagationCommandBufferID = m_scene->addCommandBuffer(commandBufferCreateInfo);

	// Compute pass  
	Scene::ComputePassCreateInfo propagationComputePassCreateInfo;
	propagationComputePassCreateInfo.extent = { VOXEL_SIZE, VOXEL_SIZE };
	propagationComputePassCreateInfo.computeShaderPath = "Shaders/LightPropagationVolumes/propagation.spv";
	propagationComputePassCreateInfo.outputIsSwapChain = false;
	propagationComputePassCreateInfo.commandBufferID = m_propagationCommandBufferID;
	propagationComputePassCreateInfo.dispatchGroups = { 8, 8, VOXEL_SIZE / 8 };

	DescriptorSetGenerator descriptorSetGenerator;

	for(int i(0);  i < m_injectionImages.size(); ++i)
		descriptorSetGenerator.addImages({ m_injectionImages[i] }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, i);

	descriptorSetGenerator.addImages({ m_lightVolumesPropagationImage }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 
		static_cast<uint32_t>(m_injectionImages.size()));

	propagationComputePassCreateInfo.descriptorSetCreateInfo = descriptorSetGenerator.getDescritorSetCreateInfo();

	m_propagationComputePassID = m_scene->addComputePass(propagationComputePassCreateInfo);
}
