#include "ProbeGeneration.h"

#include <DepthPass.h>

using namespace Wolf;

ProbeGeneration::ProbeGeneration(WolfInstance* wolfInstance, Scene* scene, const Model* model, glm::mat4 modelMatrix, glm::vec3 lightDir, glm::vec3 lightColor, Image* skyCubemap, Sampler* cubemapSampler, Model* cube, std::array<Wolf::UniformBuffer*, 6> probeMatricesUBs)
{
	m_wolfInstance = wolfInstance;

	m_probes.emplace_back();
	m_probes.back().position = glm::vec3(5.0f, 1.5f, 0.0f);

	glm::mat4 cubemaCaptureViews[] =
	{
		glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
		glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
		glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
	};

	// Cubemap generation
	{
		createCubemapGenerationPipeline(model);
		glm::mat4 captureViews[] =
		{
			glm::lookAt(m_probes[0].position, m_probes[0].position + glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(m_probes[0].position, m_probes[0].position + glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(m_probes[0].position, m_probes[0].position + glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
			glm::lookAt(m_probes[0].position, m_probes[0].position + glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
			glm::lookAt(m_probes[0].position, m_probes[0].position + glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(m_probes[0].position, m_probes[0].position + glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
		};

		glm::mat4 depthMapProj = glm::ortho(-30.0f, 30.0f, -30.0f, 30.0f, -30.0f * 6.0f, 30.0f * 6.0f);
		glm::mat4 lightViewMatrix = glm::lookAt(-50.0f * glm::normalize(lightDir), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

		std::unique_ptr<DepthPass> depthPass = std::make_unique<DepthPass>(wolfInstance, m_cubemapGenerationScene, false, VkExtent2D{ 4096, 4096 }, VK_SAMPLE_COUNT_1_BIT, model, glm::mat4(1.0f), false, true);
		depthPass->update(depthMapProj * lightViewMatrix * modelMatrix);

		int cubemapFace = 0;
		for (Wolf::UniformBuffer*& probeUB : probeMatricesUBs)
		{
			DescriptorSetGenerator sponzaDescriptorSetGenerator;

			ProbeMatricesUBData dataMVP;
			dataMVP.projectionMatrix = glm::perspective(glm::radians(90.0f), 1.0f, 0.01f, 100.0f);
			dataMVP.viewMatrix = captureViews[cubemapFace];
			dataMVP.modelMatrix = modelMatrix;
			probeUB->updateData(&dataMVP);
			sponzaDescriptorSetGenerator.addUniformBuffer(probeMatricesUBs[cubemapFace], VK_SHADER_STAGE_VERTEX_BIT, 0);

			ProbeLightingUBData dataLighting;
			dataLighting.directionDirectionalLight = glm::transpose(glm::inverse(dataMVP.viewMatrix)) * glm::vec4(lightDir, 1.0f);
			dataLighting.colorDirectionalLight = glm::vec4(lightColor, 1.0f);
			dataLighting.invModelView = glm::inverse(captureViews[cubemapFace] * modelMatrix);
			dataLighting.lightSpaceMatrix = depthMapProj * lightViewMatrix * modelMatrix;
			m_probeLightingUBs[cubemapFace] = wolfInstance->createUniformBufferObject(&dataLighting, sizeof(dataLighting));

			sponzaDescriptorSetGenerator.addUniformBuffer(m_probeLightingUBs[cubemapFace], VK_SHADER_STAGE_FRAGMENT_BIT, 3);
			sponzaDescriptorSetGenerator.addSampler(model->getSampler(), VK_SHADER_STAGE_FRAGMENT_BIT, 1);
			sponzaDescriptorSetGenerator.addImages(model->getImages(), VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, 2);
			sponzaDescriptorSetGenerator.addImages({ depthPass->getResult() }, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, 4);

			Wolf::Sampler* depthMapSampler = wolfInstance->createSampler(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1.0f, VK_FILTER_NEAREST);
			sponzaDescriptorSetGenerator.addSampler(depthMapSampler, VK_SHADER_STAGE_FRAGMENT_BIT, 5);

			Renderer::AddMeshInfo addMeshInfo{};
			addMeshInfo.vertexBuffer = model->getVertexBuffers()[0];
			addMeshInfo.renderPassID = m_cubemapGenerationRenderPassId;
			addMeshInfo.rendererID = m_cubemapGenerationSponzaRendererId;
			addMeshInfo.frameBufferID = cubemapFace;

			addMeshInfo.descriptorSetCreateInfo = sponzaDescriptorSetGenerator.getDescritorSetCreateInfo();

			m_cubemapGenerationScene->addMesh(addMeshInfo);

			DescriptorSetGenerator skyDescriptorSetGenerator;

			skyDescriptorSetGenerator.addUniformBuffer(probeMatricesUBs[cubemapFace], VK_SHADER_STAGE_VERTEX_BIT, 0);
			skyDescriptorSetGenerator.addCombinedImageSampler(skyCubemap, cubemapSampler, VK_SHADER_STAGE_FRAGMENT_BIT, 1);

			addMeshInfo.vertexBuffer = cube->getVertexBuffers()[0];
			addMeshInfo.renderPassID = m_cubemapGenerationRenderPassId;
			addMeshInfo.rendererID = m_cubemapGenerationSkyRendererId;
			addMeshInfo.frameBufferID = cubemapFace;

			addMeshInfo.descriptorSetCreateInfo = skyDescriptorSetGenerator.getDescritorSetCreateInfo();

			m_cubemapGenerationScene->addMesh(addMeshInfo);

			cubemapFace++;
		}

		m_cubemapGenerationScene->record();
		wolfInstance->submitCommandBuffers(m_cubemapGenerationScene, { depthPass->getCommandBufferID(), m_cubemapGenerationCommandBuffer }, { { depthPass->getCommandBufferID(), m_cubemapGenerationCommandBuffer } });

		wolfInstance->waitIdle();

		Image::CreateImageInfo createImageInfo;
		createImageInfo.extent = m_cubemapGenerationScene->getRenderPassOutput(m_cubemapGenerationRenderPassId, 0, 0)->getExtent();
		createImageInfo.arrayLayers = 6;
		createImageInfo.format = m_cubemapGenerationScene->getRenderPassOutput(m_cubemapGenerationRenderPassId, 0, 0)->getFormat();
		createImageInfo.mipLevels = 1;
		createImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		m_probes[0].cubeMap = wolfInstance->createImage(createImageInfo);
		m_probes[0].cubeMap->copyImagesToCubemap({ m_cubemapGenerationScene->getRenderPassOutput(m_cubemapGenerationRenderPassId, 0, 0), 
			m_cubemapGenerationScene->getRenderPassOutput(m_cubemapGenerationRenderPassId, 0, 1),
			m_cubemapGenerationScene->getRenderPassOutput(m_cubemapGenerationRenderPassId, 0, 2),
			m_cubemapGenerationScene->getRenderPassOutput(m_cubemapGenerationRenderPassId, 0, 3),
			m_cubemapGenerationScene->getRenderPassOutput(m_cubemapGenerationRenderPassId, 0, 4) ,
			m_cubemapGenerationScene->getRenderPassOutput(m_cubemapGenerationRenderPassId, 0, 5) }, { { 0, 0 } }, false);
	}

	// Convolution
	{
		Scene::CommandBufferCreateInfo commandBufferCreateInfo;
		commandBufferCreateInfo.finalPipelineStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		commandBufferCreateInfo.commandType = Scene::CommandType::GRAPHICS;
		int probeConvolutionCommandBuffer = scene->addCommandBuffer(commandBufferCreateInfo);

		Attachment colorAttachment({ 512, 512 }, VK_FORMAT_R32G32B32A32_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
		Attachment depthAttachment({ 512, 512 }, VK_FORMAT_D32_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

		Scene::RenderPassCreateInfo probeGenerationInfo;
		probeGenerationInfo.framebufferCount = 6;
		probeGenerationInfo.outputIsSwapChain = false;
		probeGenerationInfo.commandBufferID = probeConvolutionCommandBuffer;

		probeGenerationInfo.outputs.resize(2);
		probeGenerationInfo.outputs[0] = { { 0.0f, 0.0f, 0.0f, 1.0f } , colorAttachment };
		probeGenerationInfo.outputs[1] = { { 1.0f } , depthAttachment };

		int renderPassId = scene->addRenderPass(probeGenerationInfo);

		RendererCreateInfo rendererCreateInfo;

		ShaderCreateInfo vertexShaderCreateInfo{};
		vertexShaderCreateInfo.filename = "Shaders/probeGeneration/convolutionVert.spv";
		vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		rendererCreateInfo.pipelineCreateInfo.shaderCreateInfos.push_back(vertexShaderCreateInfo);

		ShaderCreateInfo fragmentShaderCreateInfo{};
		fragmentShaderCreateInfo.filename = "Shaders/probeGeneration/convolutionFrag.spv";
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

		int cubemapFace = 0;
		for (Wolf::UniformBuffer*& probeUB : probeMatricesUBs)
		{
			DescriptorSetGenerator descriptorSetGenerator;

			ProbeMatricesUBData dataMVP;
			dataMVP.projectionMatrix = glm::perspective(glm::radians(90.0f), 1.0f, 0.01f, 100.0f);
			dataMVP.viewMatrix = cubemaCaptureViews[cubemapFace];
			dataMVP.modelMatrix = glm::mat4(1.0f);
			probeUB->updateData(&dataMVP);
			descriptorSetGenerator.addUniformBuffer(probeMatricesUBs[cubemapFace], VK_SHADER_STAGE_VERTEX_BIT, 0);

			descriptorSetGenerator.addCombinedImageSampler(m_probes[0].cubeMap, cubemapSampler, VK_SHADER_STAGE_FRAGMENT_BIT, 1);

			rendererCreateInfo.descriptorSetLayout = descriptorSetGenerator.getDescriptorLayouts();

			int rendererId = scene->addRenderer(rendererCreateInfo);

			Renderer::AddMeshInfo addMeshInfo{};
			addMeshInfo.vertexBuffer = cube->getVertexBuffers()[0];
			addMeshInfo.renderPassID = renderPassId;
			addMeshInfo.rendererID = rendererId;
			addMeshInfo.frameBufferID = cubemapFace;

			addMeshInfo.descriptorSetCreateInfo = descriptorSetGenerator.getDescritorSetCreateInfo();

			scene->addMesh(addMeshInfo);

			cubemapFace++;
		}

		scene->record();
		wolfInstance->submitCommandBuffers(scene, { probeConvolutionCommandBuffer }, { });

		wolfInstance->waitIdle();

		Image::CreateImageInfo createImageInfo;
		createImageInfo.extent = scene->getRenderPassOutput(renderPassId, 0, 0)->getExtent();
		createImageInfo.arrayLayers = 6;
		createImageInfo.format = scene->getRenderPassOutput(renderPassId, 0, 0)->getFormat();
		createImageInfo.mipLevels = 1;
		createImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		m_probes[0].irradiance = wolfInstance->createImage(createImageInfo);
		m_probes[0].irradiance->copyImagesToCubemap({ scene->getRenderPassOutput(renderPassId, 0, 0), scene->getRenderPassOutput(renderPassId, 0, 1), scene->getRenderPassOutput(renderPassId, 0, 2),
		scene->getRenderPassOutput(renderPassId, 0,3) , scene->getRenderPassOutput(renderPassId, 0, 4) , scene->getRenderPassOutput(renderPassId, 0, 5) }, { { 0, 0 } }, false);
	}

	// Prefilter
	/*{
		Scene::CommandBufferCreateInfo commandBufferCreateInfo;
		commandBufferCreateInfo.finalPipelineStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		commandBufferCreateInfo.commandType = Scene::CommandType::GRAPHICS;
		int probePrefilterCommandBuffer = m_scene->addCommandBuffer(commandBufferCreateInfo);

		Attachment colorAttachment({ 512, 512 }, VK_FORMAT_R32G32B32A32_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
		Attachment depthAttachment({ 512, 512 }, VK_FORMAT_D32_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

		Scene::RenderPassCreateInfo probeGenerationInfo;
		probeGenerationInfo.framebufferCount = 6;
		probeGenerationInfo.outputIsSwapChain = false;
		probeGenerationInfo.commandBufferID = probePrefilterCommandBuffer;

		probeGenerationInfo.outputs.resize(2);
		probeGenerationInfo.outputs[0] = { { 0.0f, 0.0f, 0.0f, 1.0f } , colorAttachment };
		probeGenerationInfo.outputs[1] = { { 1.0f } , depthAttachment };

		int renderPassId = m_scene->addRenderPass(probeGenerationInfo);

		RendererCreateInfo rendererCreateInfo;

		ShaderCreateInfo vertexShaderCreateInfo{};
		vertexShaderCreateInfo.filename = "Shaders/probeGeneration/convolutionVert.spv";
		vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		rendererCreateInfo.pipelineCreateInfo.shaderCreateInfos.push_back(vertexShaderCreateInfo);

		ShaderCreateInfo fragmentShaderCreateInfo{};
		fragmentShaderCreateInfo.filename = "Shaders/probeGeneration/prefilterFrag.spv";
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

		Image::CreateImageInfo createImageInfo;
		createImageInfo.extent = m_scene->getRenderPassOutput(renderPassId, 0, 0)->getExtent();
		createImageInfo.arrayLayers = 6;
		createImageInfo.format = m_scene->getRenderPassOutput(renderPassId, 0, 0)->getFormat();
		createImageInfo.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(colorAttachment.extent.width, colorAttachment.extent.height)))) + 1;
		createImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		m_probes[0].prefilter = wolfInstance->createImage(createImageInfo);

		float roughness = 0.0f;
		m_probeRougnessUB = wolfInstance->createUniformBufferObject(&roughness, sizeof(float));
		for (uint32_t mipLevel = 0, maxMipLevel = static_cast<uint32_t>(std::floor(std::log2(std::max(colorAttachment.extent.width, colorAttachment.extent.height)))) + 1; mipLevel < maxMipLevel; ++mipLevel)
		{
			roughness = static_cast<float>(mipLevel) / static_cast<float>(maxMipLevel - 1);
			m_probeRougnessUB->updateData(&roughness);

			for (int cubemapFace = 0; cubemapFace < 6; ++cubemapFace)
			{
				DescriptorSetGenerator descriptorSetGenerator;

				UniformBuffer* probeUB = m_probeMatricesUBs[cubemapFace];
				ProbeMatricesUBData dataMVP;
				dataMVP.projectionMatrix = glm::perspective(glm::radians(90.0f), 1.0f, 0.01f, 100.0f);
				dataMVP.viewMatrix = cubemaCaptureViews[cubemapFace];
				dataMVP.modelMatrix = glm::mat4(1.0f);
				probeUB->updateData(&dataMVP);
				descriptorSetGenerator.addUniformBuffer(m_probeMatricesUBs[cubemapFace], VK_SHADER_STAGE_VERTEX_BIT, 0);
				descriptorSetGenerator.addUniformBuffer(m_probeRougnessUB, VK_SHADER_STAGE_FRAGMENT_BIT, 1);
				descriptorSetGenerator.addCombinedImageSampler(m_probes[0].cubeMap, cubemapSampler, VK_SHADER_STAGE_FRAGMENT_BIT, 2);

				rendererCreateInfo.descriptorSetLayout = descriptorSetGenerator.getDescriptorLayouts();

				int rendererId = m_scene->addRenderer(rendererCreateInfo);

				Renderer::AddMeshInfo addMeshInfo{};
				addMeshInfo.vertexBuffer = cube->getVertexBuffers()[0];
				addMeshInfo.renderPassID = renderPassId;
				addMeshInfo.rendererID = rendererId;
				addMeshInfo.frameBufferID = cubemapFace;

				addMeshInfo.descriptorSetCreateInfo = descriptorSetGenerator.getDescritorSetCreateInfo();

				m_scene->addMesh(addMeshInfo);
			}

			rendererCreateInfo.pipelineCreateInfo.viewportScale[0] /= 2.0f;
			rendererCreateInfo.pipelineCreateInfo.viewportScale[1] /= 2.0f;

			m_scene->record();
			wolfInstance->submitCommandBuffers(m_scene, { probePrefilterCommandBuffer }, { });
			wolfInstance->waitIdle();

			m_probes[0].prefilter->copyImagesToCubemap({ m_scene->getRenderPassOutput(renderPassId, 0, 0), m_scene->getRenderPassOutput(renderPassId, 0, 1), m_scene->getRenderPassOutput(renderPassId, 0, 2),
				m_scene->getRenderPassOutput(renderPassId, 0,3) , m_scene->getRenderPassOutput(renderPassId, 0, 4) , m_scene->getRenderPassOutput(renderPassId, 0, 5) }, { { 0, mipLevel } }, false);
		}
	}*/
}

void ProbeGeneration::createCubemapGenerationPipeline(const Model* model)
{
	Scene::SceneCreateInfo sceneCreateInfo;
	m_cubemapGenerationScene = m_wolfInstance->createScene(sceneCreateInfo);

	Scene::CommandBufferCreateInfo commandBufferCreateInfo;
	commandBufferCreateInfo.finalPipelineStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	commandBufferCreateInfo.commandType = Scene::CommandType::GRAPHICS;
	m_cubemapGenerationCommandBuffer = m_cubemapGenerationScene->addCommandBuffer(commandBufferCreateInfo);

	Attachment colorAttachment({ 512, 512 }, VK_FORMAT_R32G32B32A32_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
	Attachment depthAttachment({ 512, 512 }, VK_FORMAT_D32_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

	Scene::RenderPassCreateInfo probeGenerationInfo;
	probeGenerationInfo.framebufferCount = 6;
	probeGenerationInfo.outputIsSwapChain = false;
	probeGenerationInfo.commandBufferID = m_cubemapGenerationCommandBuffer;

	probeGenerationInfo.outputs.resize(2);
	probeGenerationInfo.outputs[0] = { { 0.0f, 0.0f, 0.0f, 1.0f } , colorAttachment };
	probeGenerationInfo.outputs[1] = { { 1.0f } , depthAttachment };

	m_cubemapGenerationRenderPassId = m_cubemapGenerationScene->addRenderPass(probeGenerationInfo);

	RendererCreateInfo sponzaRendererCreateInfo;

	ShaderCreateInfo vertexShaderCreateInfo{};
	vertexShaderCreateInfo.filename = "Shaders/probeGeneration/vert.spv";
	vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	sponzaRendererCreateInfo.pipelineCreateInfo.shaderCreateInfos.push_back(vertexShaderCreateInfo);

	ShaderCreateInfo fragmentShaderCreateInfo{};
	fragmentShaderCreateInfo.filename = "Shaders/probeGeneration/frag.spv";
	fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	sponzaRendererCreateInfo.pipelineCreateInfo.shaderCreateInfos.push_back(fragmentShaderCreateInfo);

	sponzaRendererCreateInfo.inputVerticesTemplate = InputVertexTemplate::FULL_3D_MATERIAL;
	sponzaRendererCreateInfo.instanceTemplate = InstanceTemplate::NO;
	sponzaRendererCreateInfo.renderPassID = m_cubemapGenerationRenderPassId;
	sponzaRendererCreateInfo.pipelineCreateInfo.extent = { 512, 512 };
	sponzaRendererCreateInfo.pipelineCreateInfo.alphaBlending = { false };

	std::vector<DescriptorLayout> sponzaDescriptorLayouts =
	{
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT,		0},
		{ VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT,			1},
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT,	2, static_cast<uint32_t>(model->getNumberOfImages())},
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT,	3},
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT,	4},
		{ VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT,			5}
	};
	sponzaRendererCreateInfo.descriptorSetLayout = sponzaDescriptorLayouts;

	m_cubemapGenerationSponzaRendererId = m_cubemapGenerationScene->addRenderer(sponzaRendererCreateInfo);

	RendererCreateInfo skyRendererCreateInfo;

	vertexShaderCreateInfo.filename = "Shaders/probeGeneration/convolutionVert.spv";
	vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	skyRendererCreateInfo.pipelineCreateInfo.shaderCreateInfos.push_back(vertexShaderCreateInfo);

	fragmentShaderCreateInfo.filename = "Shaders/probeGeneration/skyFrag.spv";
	fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	skyRendererCreateInfo.pipelineCreateInfo.shaderCreateInfos.push_back(fragmentShaderCreateInfo);

	skyRendererCreateInfo.inputVerticesTemplate = InputVertexTemplate::NO;
	skyRendererCreateInfo.instanceTemplate = InstanceTemplate::NO;
	skyRendererCreateInfo.renderPassID = m_cubemapGenerationRenderPassId;
	skyRendererCreateInfo.pipelineCreateInfo.extent = { 512, 512 };
	skyRendererCreateInfo.pipelineCreateInfo.alphaBlending = { false };

	VkVertexInputBindingDescription bindingDescription = {};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(glm::vec3);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	skyRendererCreateInfo.pipelineCreateInfo.vertexInputBindingDescriptions = { bindingDescription };

	VkVertexInputAttributeDescription attributeDescription;
	attributeDescription.binding = 0;
	attributeDescription.location = 0;
	attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescription.offset = 0;
	skyRendererCreateInfo.pipelineCreateInfo.vertexInputAttributeDescriptions = { attributeDescription };

	std::vector<DescriptorLayout> skyDescriptorLayouts =
	{
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT,				0},
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT,	1}
	};
	skyRendererCreateInfo.descriptorSetLayout = skyDescriptorLayouts;

	m_cubemapGenerationSkyRendererId = m_cubemapGenerationScene->addRenderer(skyRendererCreateInfo);
}
