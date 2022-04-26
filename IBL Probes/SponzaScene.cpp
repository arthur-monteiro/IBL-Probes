#include "SponzaScene.h"
#include <random>

using namespace Wolf;

SponzaScene::SponzaScene(Wolf::WolfInstance* wolfInstance)
{
	m_window = wolfInstance->getWindowPtr();

	// Scene creation
	Scene::SceneCreateInfo sceneCreateInfo;
	sceneCreateInfo.swapChainCommandType = Scene::CommandType::COMPUTE;

	m_scene = wolfInstance->createScene(sceneCreateInfo);

	// Model creation
	Model::ModelCreateInfo modelCreateInfo{};
	modelCreateInfo.inputVertexTemplate = InputVertexTemplate::FULL_3D_MATERIAL;
	Model* model = wolfInstance->createModel<>(modelCreateInfo);

	Model::ModelLoadingInfo modelLoadingInfo;
	modelLoadingInfo.filename = std::move("Models/sponza/sponza.obj");
	modelLoadingInfo.mtlFolder = std::move("Models/sponza");
	model->loadObj(modelLoadingInfo);
	m_modelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.01f));

	Wolf::Sampler* cubemapSampler = wolfInstance->createSampler(VK_SAMPLER_ADDRESS_MODE_REPEAT, 1.0f, VK_FILTER_LINEAR);
	Wolf::Image* hdrSky = wolfInstance->createImageFromFile("Textures/kiara_4_mid-morning_4k.hdr");

	glm::mat4 cubemaCaptureViews[] =
	{
		glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
		glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
		glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
	};

	std::vector<glm::vec3> cubeVertices = {
			glm::vec3(-0.5f, -0.5f, -0.5f),
			glm::vec3(+0.5f, -0.5f, -0.5f),
			glm::vec3(+0.5f, +0.5f, -0.5f),
			glm::vec3(+0.5f, +0.5f, -0.5f),
			glm::vec3(-0.5f, +0.5f, -0.5f),
			glm::vec3(-0.5f, -0.5f, -0.5f),

			glm::vec3(-0.5f, -0.5f, +0.5f),
			glm::vec3(+0.5f, +0.5f, +0.5f),
			glm::vec3(+0.5f, -0.5f, +0.5f),
			glm::vec3(+0.5f, +0.5f, +0.5f),
			glm::vec3(-0.5f, -0.5f, +0.5f),
			glm::vec3(-0.5f, +0.5f, +0.5f),

			glm::vec3(-0.5f, +0.5f, +0.5f),
			glm::vec3(-0.5f, -0.5f, -0.5f),
			glm::vec3(-0.5f, +0.5f, -0.5f),
			glm::vec3(-0.5f, -0.5f, -0.5f),
			glm::vec3(-0.5f, +0.5f, +0.5f),
			glm::vec3(-0.5f, -0.5f, +0.5f),

			glm::vec3(+0.5f, +0.5f, +0.5f),
			glm::vec3(+0.5f, +0.5f, -0.5f),
			glm::vec3(+0.5f, -0.5f, -0.5f),
			glm::vec3(+0.5f, -0.5f, -0.5f),
			glm::vec3(+0.5f, -0.5f, +0.5f),
			glm::vec3(+0.5f, +0.5f, +0.5f),

			glm::vec3(-0.5f, -0.5f, -0.5f),
			glm::vec3(+0.5f, -0.5f, +0.5f),
			glm::vec3(+0.5f, -0.5f, -0.5f),
			glm::vec3(+0.5f, -0.5f, +0.5f),
			glm::vec3(-0.5f, -0.5f, -0.5f),
			glm::vec3(-0.5f, -0.5f, +0.5f),

			glm::vec3(-0.5f, +0.5f, -0.5f),
			glm::vec3(+0.5f, +0.5f, -0.5f),
			glm::vec3(+0.5f, +0.5f, +0.5f),
			glm::vec3(+0.5f, +0.5f, +0.5f),
			glm::vec3(-0.5f, +0.5f, +0.5f),
			glm::vec3(-0.5f, +0.5f, -0.5f)
	};

	std::vector<uint32_t> cubeIndices(cubeVertices.size());
	uint32_t currentIndice = 0;
	for (uint32_t& indice : cubeIndices)
	{
		indice = currentIndice;
		currentIndice++;
	}

	Model::ModelCreateInfo cubeModelCreateInfo{};
	cubeModelCreateInfo.inputVertexTemplate = InputVertexTemplate::NO;
	Model* cube = wolfInstance->createModel<glm::vec3>(cubeModelCreateInfo);

	cube->addMeshFromVertices(cubeVertices.data(), 36, sizeof(glm::vec3), cubeIndices);

	// Sky generation
	{
		Scene::CommandBufferCreateInfo commandBufferCreateInfo;
		commandBufferCreateInfo.finalPipelineStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		commandBufferCreateInfo.commandType = Scene::CommandType::GRAPHICS;
		int skyCommandBuffer = m_scene->addCommandBuffer(commandBufferCreateInfo);

		Attachment colorAttachment({ 512, 512 }, VK_FORMAT_R32G32B32A32_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
		Attachment depthAttachment({ 512, 512 }, VK_FORMAT_D32_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

		Scene::RenderPassCreateInfo probeGenerationInfo;
		probeGenerationInfo.framebufferCount = 6;
		probeGenerationInfo.outputIsSwapChain = false;
		probeGenerationInfo.commandBufferID = skyCommandBuffer;

		probeGenerationInfo.outputs.resize(2);
		probeGenerationInfo.outputs[0] = { { 0.0f, 0.0f, 0.0f, 1.0f } , colorAttachment };
		probeGenerationInfo.outputs[1] = { { 1.0f } , depthAttachment };

		int renderPassId = m_scene->addRenderPass(probeGenerationInfo);

		RendererCreateInfo rendererCreateInfo;

		ShaderCreateInfo vertexShaderCreateInfo{};
		vertexShaderCreateInfo.filename = "Shaders/sky/vert.spv";
		vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		rendererCreateInfo.pipelineCreateInfo.shaderCreateInfos.push_back(vertexShaderCreateInfo);

		ShaderCreateInfo fragmentShaderCreateInfo{};
		fragmentShaderCreateInfo.filename = "Shaders/sky/cubemapGenFrag.spv";
		fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		rendererCreateInfo.pipelineCreateInfo.shaderCreateInfos.push_back(fragmentShaderCreateInfo);

		rendererCreateInfo.inputVerticesTemplate = InputVertexTemplate::NO;
		rendererCreateInfo.instanceTemplate = InstanceTemplate::NO;

		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(glm::vec3);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		rendererCreateInfo.pipelineCreateInfo.vertexInputBindingDescriptions = { bindingDescription };

		VkVertexInputAttributeDescription attributeDescription;
		attributeDescription.binding = 0;
		attributeDescription.location = 0;
		attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescription.offset = 0;
		rendererCreateInfo.pipelineCreateInfo.vertexInputAttributeDescriptions = { attributeDescription };

		rendererCreateInfo.renderPassID = renderPassId;
		rendererCreateInfo.pipelineCreateInfo.extent = { 512, 512 };
		rendererCreateInfo.pipelineCreateInfo.alphaBlending = { false };

		std::vector<DescriptorLayout> skyDescriptorLayouts =
		{
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT,				0},
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT,	1}
		};
		rendererCreateInfo.descriptorSetLayout = skyDescriptorLayouts;

		int rendererId = m_scene->addRenderer(rendererCreateInfo);

		int cubemapFace = 0;
		for (Wolf::UniformBuffer*& probeUB : m_probeMatricesUBs)
		{
			DescriptorSetGenerator descriptorSetGenerator;

			ProbeMatricesUBData dataMVP;
			dataMVP.projectionMatrix = glm::perspective(glm::radians(90.0f), 1.0f, 0.01f, 100.0f);
			dataMVP.viewMatrix = cubemaCaptureViews[cubemapFace];
			dataMVP.modelMatrix = glm::mat4(1.0f);
			probeUB = wolfInstance->createUniformBufferObject(&dataMVP, sizeof(dataMVP));
			descriptorSetGenerator.addUniformBuffer(m_probeMatricesUBs[cubemapFace], VK_SHADER_STAGE_VERTEX_BIT, 0);
			descriptorSetGenerator.addCombinedImageSampler(hdrSky, cubemapSampler, VK_SHADER_STAGE_FRAGMENT_BIT, 1);

			Renderer::AddMeshInfo addMeshInfo{};
			addMeshInfo.vertexBuffer = cube->getVertexBuffers()[0];
			addMeshInfo.renderPassID = renderPassId;
			addMeshInfo.rendererID = rendererId;
			addMeshInfo.frameBufferID = cubemapFace;

			addMeshInfo.descriptorSetCreateInfo = descriptorSetGenerator.getDescritorSetCreateInfo();

			m_scene->addMesh(addMeshInfo);

			cubemapFace++;
		}

		m_scene->record();
		wolfInstance->submitCommandBuffers(m_scene, { skyCommandBuffer }, { });
		wolfInstance->waitIdle();

		Image::CreateImageInfo createImageInfo;
		createImageInfo.extent = m_scene->getRenderPassOutput(renderPassId, 0, 0)->getExtent();
		createImageInfo.arrayLayers = 6;
		createImageInfo.format = m_scene->getRenderPassOutput(renderPassId, 0, 0)->getFormat();
		createImageInfo.mipLevels = 1;
		createImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		m_skyCubemap = wolfInstance->createImage(createImageInfo);
		m_skyCubemap->copyImagesToCubemap({ m_scene->getRenderPassOutput(renderPassId, 0, 0), m_scene->getRenderPassOutput(renderPassId, 0, 1), m_scene->getRenderPassOutput(renderPassId, 0, 2),
		m_scene->getRenderPassOutput(renderPassId, 0,3) , m_scene->getRenderPassOutput(renderPassId, 0, 4) , m_scene->getRenderPassOutput(renderPassId, 0, 5) }, { { 0, 0 } }, false);
	}

	// Probes creation
	m_probeGeneration = std::make_unique<ProbeGeneration>(wolfInstance, m_scene, model, m_modelMatrix, m_lightDir, m_lightColor, m_skyCubemap, cubemapSampler, cube, m_probeMatricesUBs);

	// BRDF
	{
		std::vector<Vertex2DTextured> squareVertices =
		{
			{ glm::vec2(-1.0f, -1.0f), glm::vec2(0.0f, 0.0f) }, // top left
			{ glm::vec2(-1.0f, 1.0f), glm::vec2(0.0f, 1.0f) }, // bot left
			{ glm::vec2(1.0f, -1.0f), glm::vec2(1.0f, 0.0f) }, // top right
			{ glm::vec2(1.0f, 1.0f), glm::vec2(1.0f, 1.0f) } // bot right
		};

		std::vector<uint32_t> squareIndices =
		{
			0, 1, 2,
			1, 3, 2
		};

		Model::ModelCreateInfo squareCreateInfo{};
		squareCreateInfo.inputVertexTemplate = InputVertexTemplate::POSITION_TEXTURECOORD_2D;
		Model* square = wolfInstance->createModel(squareCreateInfo);

		square->addMeshFromVertices(squareVertices.data(), 4, sizeof(Vertex2DTextured), squareIndices);

		Scene::CommandBufferCreateInfo commandBufferCreateInfo;
		commandBufferCreateInfo.finalPipelineStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		commandBufferCreateInfo.commandType = Scene::CommandType::GRAPHICS;
		int brdfCommandBuffer = m_scene->addCommandBuffer(commandBufferCreateInfo);

		Attachment colorAttachment({ 512, 512 }, VK_FORMAT_R32G32_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		Attachment depthAttachment({ 512, 512 }, VK_FORMAT_D32_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

		Scene::RenderPassCreateInfo renderPassCreateInfo;
		renderPassCreateInfo.outputIsSwapChain = false;
		renderPassCreateInfo.commandBufferID = brdfCommandBuffer;

		renderPassCreateInfo.outputs.resize(2);
		renderPassCreateInfo.outputs[0] = { { 0.0f, 0.0f, 0.0f, 1.0f } , colorAttachment };
		renderPassCreateInfo.outputs[1] = { { 1.0f } , depthAttachment };

		int renderPassId = m_scene->addRenderPass(renderPassCreateInfo);

		RendererCreateInfo rendererCreateInfo;

		ShaderCreateInfo vertexShaderCreateInfo{};
		vertexShaderCreateInfo.filename = "Shaders/probeGeneration/brdfVert.spv";
		vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		rendererCreateInfo.pipelineCreateInfo.shaderCreateInfos.push_back(vertexShaderCreateInfo);

		ShaderCreateInfo fragmentShaderCreateInfo{};
		fragmentShaderCreateInfo.filename = "Shaders/probeGeneration/brdfFrag.spv";
		fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		rendererCreateInfo.pipelineCreateInfo.shaderCreateInfos.push_back(fragmentShaderCreateInfo);

		rendererCreateInfo.inputVerticesTemplate = InputVertexTemplate::POSITION_TEXTURECOORD_2D;
		rendererCreateInfo.instanceTemplate = InstanceTemplate::NO;
		rendererCreateInfo.renderPassID = renderPassId;

		rendererCreateInfo.pipelineCreateInfo.alphaBlending = { false };

		int rendererId = m_scene->addRenderer(rendererCreateInfo);

		// Link the model to the renderer
		Renderer::AddMeshInfo addMeshInfo{};
		addMeshInfo.vertexBuffer = square->getVertexBuffers()[0];
		addMeshInfo.renderPassID = renderPassId;
		addMeshInfo.rendererID = rendererId;

		m_scene->addMesh(addMeshInfo);

		m_scene->record();
		wolfInstance->submitCommandBuffers(m_scene, { brdfCommandBuffer }, { });
		wolfInstance->waitIdle();

		m_brdfImage = m_scene->getRenderPassOutput(renderPassId, 0, 0);
	}

	// Data
	float near = 0.1f;
	float far = 100.0f;
	{
		m_projectionMatrix = glm::perspective(glm::radians(45.0f), 16.0f / 9.0f, near, far);
		m_projectionMatrix[1][1] *= -1;
		m_viewMatrix = glm::lookAt(glm::vec3(-2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	{
		// GBuffer
		Scene::CommandBufferCreateInfo commandBufferCreateInfo;
		commandBufferCreateInfo.finalPipelineStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		commandBufferCreateInfo.commandType = Scene::CommandType::GRAPHICS;
		m_gBufferCommandBufferID = m_scene->addCommandBuffer(commandBufferCreateInfo);

		m_GBuffer = std::make_unique<GBuffer>(wolfInstance, m_scene, m_gBufferCommandBufferID, wolfInstance->getWindowSize(), VK_SAMPLE_COUNT_1_BIT, model, glm::mat4(1.0f), true);

		Image* depth = m_GBuffer->getDepth();
		Image* albedo = m_GBuffer->getAlbedo();
		Image* normalRoughnessMetal = m_GBuffer->getNormalRoughnessMetal();

		// CSM
		m_cascadedShadowMapping = std::make_unique<CascadedShadowMapping>(wolfInstance, m_scene, model, 0.1f, 100.0f, 32.f, glm::radians(45.0f), wolfInstance->getWindowSize(),
			depth, m_projectionMatrix);

		// Direct lighting
#pragma warning(disable: 4996)
		Image::CreateImageInfo createImageInfo;
		createImageInfo.extent = { wolfInstance->getWindowSize().width, wolfInstance->getWindowSize().height, 1 };
		createImageInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
		createImageInfo.mipLevels = 1;
		createImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
		m_directLightingOutputImage = wolfInstance->createImage(createImageInfo);
		m_directLightingOutputImage->setImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

		m_directLightingUBData.directionDirectionalLight = glm::vec4(m_lightDir, 1.0f);
		m_directLightingUBData.colorDirectionalLight = glm::vec4(m_lightColor, 1.0f);
		m_directLightingUBData.invProjection = glm::inverse(m_projectionMatrix);
		m_directLightingUBData.invView = glm::inverse(m_viewMatrix);
		m_directLightingUBData.projParams.x = far / (far - near);
		m_directLightingUBData.projParams.y = (-far * near) / (far - near);
		m_directLightingUniformBuffer = wolfInstance->createUniformBufferObject(&m_directLightingUBData, sizeof(m_directLightingUBData));

		Scene::ComputePassCreateInfo directLigtingComputePassCreateInfo;
		directLigtingComputePassCreateInfo.extent = wolfInstance->getWindowSize();
		directLigtingComputePassCreateInfo.dispatchGroups = { 16, 16, 1 };
		directLigtingComputePassCreateInfo.computeShaderPath = "Shaders/directLighting/comp.spv";

		commandBufferCreateInfo.finalPipelineStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		commandBufferCreateInfo.commandType = Scene::CommandType::COMPUTE;
		m_directLightingCommandBufferID = m_scene->addCommandBuffer(commandBufferCreateInfo);
		directLigtingComputePassCreateInfo.commandBufferID = m_directLightingCommandBufferID;

		{
			DescriptorSetGenerator descriptorSetGenerator;
			descriptorSetGenerator.addImages({ depth }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 0);
			descriptorSetGenerator.addImages({ albedo }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 1);
			descriptorSetGenerator.addImages({ normalRoughnessMetal }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 2);
			descriptorSetGenerator.addImages({ m_cascadedShadowMapping->getOutputShadowMaskTexture() }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 3);
			descriptorSetGenerator.addImages({ m_cascadedShadowMapping->getOutputVolumetricLightMaskImage() }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 4);
			descriptorSetGenerator.addImages({ m_directLightingOutputImage }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 5);
			descriptorSetGenerator.addSampler(cubemapSampler, VK_SHADER_STAGE_COMPUTE_BIT, 6);
			descriptorSetGenerator.addImages({ m_probeGeneration->GetProbe(0).irradiance }, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 7);
			descriptorSetGenerator.addImages({ m_probeGeneration->GetProbe(0).prefilter }, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 8);
			descriptorSetGenerator.addCombinedImageSampler(m_brdfImage, cubemapSampler, VK_SHADER_STAGE_COMPUTE_BIT, 9);
			descriptorSetGenerator.addCombinedImageSampler(m_skyCubemap, cubemapSampler, VK_SHADER_STAGE_COMPUTE_BIT, 10);
			descriptorSetGenerator.addCombinedImageSampler(m_probeGeneration->GetProbe(0).cubeMap, cubemapSampler, VK_SHADER_STAGE_COMPUTE_BIT, 11);
			descriptorSetGenerator.addUniformBuffer(m_directLightingUniformBuffer, VK_SHADER_STAGE_COMPUTE_BIT, 12);

			directLigtingComputePassCreateInfo.descriptorSetCreateInfo = descriptorSetGenerator.getDescritorSetCreateInfo();
		}

		m_scene->addComputePass(directLigtingComputePassCreateInfo);

		// Debug
		if (m_useDebug)
		{
			Model::ModelCreateInfo sphereModelCreateInfo{};
			sphereModelCreateInfo.inputVertexTemplate = InputVertexTemplate::FULL_3D_MATERIAL;
			Model* sphere = wolfInstance->createModel<>(sphereModelCreateInfo);

			Model::ModelLoadingInfo sphereLoadingInfo;
			sphereLoadingInfo.filename = std::move("Models/sphere.obj");
			sphereLoadingInfo.loadMaterials = false;
			sphere->loadObj(sphereLoadingInfo);

			std::vector<Attachment> debugPassAttachments(2);
			debugPassAttachments[0] = Attachment(wolfInstance->getWindowSize(), VK_FORMAT_D32_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL,
				VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depth);
			debugPassAttachments[0].loadOperation = VK_ATTACHMENT_LOAD_OP_LOAD;
			debugPassAttachments[1] = Attachment(wolfInstance->getWindowSize(), m_directLightingOutputImage->getFormat(), VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_GENERAL,
				VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT, m_directLightingOutputImage);
			debugPassAttachments[1].loadOperation = VK_ATTACHMENT_LOAD_OP_LOAD;

			Scene::CommandBufferCreateInfo debugCommandBufferCreateInfo;
			debugCommandBufferCreateInfo.finalPipelineStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			debugCommandBufferCreateInfo.commandType = Scene::CommandType::GRAPHICS;
			m_debugCommandBufferID = m_scene->addCommandBuffer(debugCommandBufferCreateInfo);

			Scene::RenderPassCreateInfo debugRenderPassCreateInfo{};
			debugRenderPassCreateInfo.name = "Debug render pass";
			debugRenderPassCreateInfo.commandBufferID = m_debugCommandBufferID;
			debugRenderPassCreateInfo.outputIsSwapChain = false;

			// Change image layout
			debugRenderPassCreateInfo.beforeRecord = [](void* data, VkCommandBuffer commandBuffer) -> void
			{
				const SponzaScene* thisPtr = static_cast<SponzaScene*>(data);
				Image::transitionImageLayoutUsingCommandBuffer(commandBuffer, thisPtr->m_directLightingOutputImage->getImage(),
					thisPtr->m_directLightingOutputImage->getFormat(), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0);
			};
			debugRenderPassCreateInfo.dataForBeforeRecordCallback = this;

			debugRenderPassCreateInfo.afterRecord = [](void* data, VkCommandBuffer commandBuffer) -> void
			{
				const SponzaScene* thisPtr = static_cast<SponzaScene*>(data);
				Image::transitionImageLayoutUsingCommandBuffer(commandBuffer, thisPtr->m_directLightingOutputImage->getImage(),
					thisPtr->m_directLightingOutputImage->getFormat(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, 1,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0);
			};
			debugRenderPassCreateInfo.dataForAfterRecordCallback = this;

			std::vector<VkClearValue> debugRenderPassClearValues;
			debugRenderPassClearValues.resize(2);
			debugRenderPassClearValues[0] = { -1.0f };
			debugRenderPassClearValues[1] = { -1.0f, 0.0f, 0.0f, 1.0f };

			int i(0);
			for (auto& attachment : debugPassAttachments)
			{
				Scene::RenderPassOutput renderPassOutput;
				renderPassOutput.attachment = attachment;
				renderPassOutput.clearValue = debugRenderPassClearValues[i++];

				debugRenderPassCreateInfo.outputs.push_back(renderPassOutput);
			}

			int debugRenderPassID = m_scene->addRenderPass(debugRenderPassCreateInfo);

			// Renderer
			RendererCreateInfo rendererCreateInfo;

			ShaderCreateInfo vertexShaderCreateInfo{};
			vertexShaderCreateInfo.filename = "Shaders/debug/vert.spv";
			vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			rendererCreateInfo.pipelineCreateInfo.shaderCreateInfos.push_back(vertexShaderCreateInfo);

			ShaderCreateInfo fragmentShaderCreateInfo{};
			fragmentShaderCreateInfo.filename = "Shaders/debug/frag.spv";
			fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			rendererCreateInfo.pipelineCreateInfo.shaderCreateInfos.push_back(fragmentShaderCreateInfo);

			rendererCreateInfo.inputVerticesTemplate = InputVertexTemplate::FULL_3D_MATERIAL;

			rendererCreateInfo.instanceTemplate = InstanceTemplate::NO;
			std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions = InstanceDebug::getAttributeDescriptions(1, 5);
			std::vector<VkVertexInputBindingDescription> inputBindingDescriptions = { InstanceDebug::getBindingDescription(1) };

			for (VkVertexInputAttributeDescription& inputAttributeDescription : inputAttributeDescriptions)
				rendererCreateInfo.pipelineCreateInfo.vertexInputAttributeDescriptions.push_back(inputAttributeDescription);
			for (VkVertexInputBindingDescription& inputBindingDescription : inputBindingDescriptions)
				rendererCreateInfo.pipelineCreateInfo.vertexInputBindingDescriptions.push_back(inputBindingDescription);

			rendererCreateInfo.renderPassID = debugRenderPassID;

			rendererCreateInfo.pipelineCreateInfo.alphaBlending = { true };

			DescriptorSetGenerator descriptorSetGenerator;

			m_debugUBData.model = glm::scale(glm::vec3(0.05f));
			m_debugUBData.projection = m_projectionMatrix;
			m_debugUniformBuffer = wolfInstance->createUniformBufferObject(&m_debugUBData, sizeof(m_debugUBData));
			descriptorSetGenerator.addUniformBuffer(m_debugUniformBuffer, VK_SHADER_STAGE_VERTEX_BIT, 0);
			descriptorSetGenerator.addSampler(cubemapSampler, VK_SHADER_STAGE_FRAGMENT_BIT, 1);
			descriptorSetGenerator.addImages({ m_probeGeneration->GetProbe(0).irradiance }, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, 2);

			rendererCreateInfo.descriptorSetLayout = descriptorSetGenerator.getDescriptorLayouts();

			int debugRendererID = m_scene->addRenderer(rendererCreateInfo);

			Renderer::AddMeshInfo addMeshInfo{};
			addMeshInfo.vertexBuffer = sphere->getVertexBuffers()[0];

			Instance<InstanceDebug>* instance = wolfInstance->createInstanceBuffer<InstanceDebug>();
			std::vector<InstanceDebug> instanceData;
			instanceData.push_back({ m_probeGeneration->GetProbe(0).position, 0 });
			instance->loadFromVector(instanceData);

			addMeshInfo.instanceBuffer = instance->getInstanceBuffer();
			addMeshInfo.renderPassID = debugRenderPassID;
			addMeshInfo.rendererID = debugRendererID;

			addMeshInfo.descriptorSetCreateInfo = descriptorSetGenerator.getDescritorSetCreateInfo();

			m_scene->addMesh(addMeshInfo);
		}

		// Tone mapping
		Scene::ComputePassCreateInfo toneMappingComputePassCreateInfo;
		toneMappingComputePassCreateInfo.computeShaderPath = "Shaders/toneMapping/comp.spv";
		toneMappingComputePassCreateInfo.outputIsSwapChain = true;
		toneMappingComputePassCreateInfo.commandBufferID = -1;
		toneMappingComputePassCreateInfo.dispatchGroups = { 16, 16, 1 };

		DescriptorSetGenerator toneMappingDescriptorSetGenerator;
		toneMappingDescriptorSetGenerator.addImages({ m_directLightingOutputImage }, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 0);

		toneMappingComputePassCreateInfo.descriptorSetCreateInfo = toneMappingDescriptorSetGenerator.getDescritorSetCreateInfo();
		toneMappingComputePassCreateInfo.outputBinding = 1;

		m_scene->addComputePass(toneMappingComputePassCreateInfo);
	}

	m_scene->record();

	m_camera.initialize(glm::vec3(1.4f, 1.2f, 0.3f), glm::vec3(2.0f, 0.9f, -0.3f), glm::vec3(0.0f, 1.0f, 0.0f), 0.01f, 5.0f,
		16.0f / 9.0f);
}

void SponzaScene::update()
{
	m_camera.update(m_window);
	m_viewMatrix = m_camera.getViewMatrix();

	m_GBuffer->updateMVPMatrix(m_modelMatrix, m_viewMatrix, m_projectionMatrix);

	m_cascadedShadowMapping->updateMatrices(m_lightDir, m_camera.getPosition(), m_camera.getOrientation(), m_modelMatrix, glm::inverse(m_viewMatrix * m_modelMatrix));

	m_directLightingUBData.invView = glm::mat4(glm::mat3(glm::inverse(m_viewMatrix)));
	m_directLightingUBData.directionDirectionalLight = glm::transpose(m_directLightingUBData.invView) * glm::vec4(m_lightDir, 1.0f);
	m_directLightingUniformBuffer->updateData(&m_directLightingUBData);

	m_debugUBData.view = m_viewMatrix;
	m_debugUniformBuffer->updateData(&m_debugUBData);
}

std::vector<int> SponzaScene::getCommandBufferToSubmit()
{
	std::vector<int> r;
	r.push_back(m_gBufferCommandBufferID);
	std::vector<int> csmCommandBuffer = m_cascadedShadowMapping->getCascadeCommandBuffers();
	for (auto& commandBuffer : csmCommandBuffer)
		r.push_back(commandBuffer);
	r.push_back(m_directLightingCommandBufferID);
	r.push_back(m_debugCommandBufferID);

	return r;
}

std::vector<std::pair<int, int>> SponzaScene::getCommandBufferSynchronisation()
{
	std::vector<std::pair<int, int>> r =
	{ { m_gBufferCommandBufferID, m_directLightingCommandBufferID} };

	std::vector<std::pair<int, int>> csmSynchronisation = m_cascadedShadowMapping->getCommandBufferSynchronisation();
	for (auto& sync : csmSynchronisation)
	{
		r.push_back(sync);
	}
	r.emplace_back(m_cascadedShadowMapping->getCascadeCommandBuffers().back(), m_directLightingCommandBufferID);
	r.emplace_back(m_directLightingCommandBufferID, m_debugCommandBufferID);
	r.emplace_back(m_debugCommandBufferID, -1);
	
	return r;
}