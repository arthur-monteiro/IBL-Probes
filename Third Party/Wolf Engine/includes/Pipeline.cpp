#include "Pipeline.h"

#include <fstream>
#include <filesystem>

#include "Debug.h"

Wolf::Pipeline::Pipeline(VkDevice device, RenderingPipelineCreateInfo renderingPipelineCreateInfo)
{
	m_device = device;
	
	/* Pipeline layout */
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(renderingPipelineCreateInfo.descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = renderingPipelineCreateInfo.descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
		Debug::sendError("Error : create pipeline layout");

	/* Shaders */
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

	std::vector<VkShaderModule> shaderModules;
	for(auto& shaderCreateInfo : renderingPipelineCreateInfo.shaderCreateInfos)
	{
		// Read code
		std::vector<char> shaderCode;
		if (!shaderCreateInfo.filename.empty())
			shaderCode = readFile(shaderCreateInfo.filename);
		else if (!shaderCreateInfo.fileContent.empty())
			std::copy(shaderCreateInfo.fileContent.begin(), shaderCreateInfo.fileContent.end(), std::back_inserter(shaderCode));
		else
			continue;

		// Create shader module
		shaderModules.push_back(createShaderModule(shaderCode, m_device));

		// Add stage
		VkPipelineShaderStageCreateInfo shaderStageInfo = {};
		shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageInfo.stage = shaderCreateInfo.stage;
		shaderStageInfo.module = shaderModules.back();
		shaderStageInfo.pName = shaderCreateInfo.entryPointName.data();

		shaderStages.push_back(shaderStageInfo);
	}

	/* Input */
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(renderingPipelineCreateInfo.vertexInputBindingDescriptions.size());
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(renderingPipelineCreateInfo.vertexInputAttributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = renderingPipelineCreateInfo.vertexInputBindingDescriptions.data();
	vertexInputInfo.pVertexAttributeDescriptions = renderingPipelineCreateInfo.vertexInputAttributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = renderingPipelineCreateInfo.topology;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	/* Viewport */
	VkViewport viewport = {};
	viewport.x = renderingPipelineCreateInfo.extent.width * renderingPipelineCreateInfo.viewportOffset[0];
	viewport.y = renderingPipelineCreateInfo.extent.height * renderingPipelineCreateInfo.viewportOffset[1];
	viewport.width = renderingPipelineCreateInfo.extent.width * renderingPipelineCreateInfo.viewportScale[0];
	viewport.height = renderingPipelineCreateInfo.extent.height * renderingPipelineCreateInfo.viewportScale[1];
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = renderingPipelineCreateInfo.extent;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	/* Rasterization */
	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = renderingPipelineCreateInfo.polygonMode;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	/* Multisampling */
	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = renderingPipelineCreateInfo.msaaSamples;

	/* Color blend */
	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(renderingPipelineCreateInfo.alphaBlending.size());
	for (int i(0); i < renderingPipelineCreateInfo.alphaBlending.size(); ++i)
	{
		if (renderingPipelineCreateInfo.alphaBlending[i])
		{
			if (renderingPipelineCreateInfo.addColors)
			{
				colorBlendAttachments[i].colorWriteMask = 0xf;
				colorBlendAttachments[i].blendEnable = VK_TRUE;
				colorBlendAttachments[i].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
				colorBlendAttachments[i].dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
				colorBlendAttachments[i].colorBlendOp = VK_BLEND_OP_ADD;
				colorBlendAttachments[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
				colorBlendAttachments[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
				colorBlendAttachments[i].alphaBlendOp = VK_BLEND_OP_MAX;
			}
			else
			{
				colorBlendAttachments[i].colorWriteMask = 0xf;
				colorBlendAttachments[i].blendEnable = VK_TRUE;
				colorBlendAttachments[i].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
				colorBlendAttachments[i].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
				colorBlendAttachments[i].colorBlendOp = VK_BLEND_OP_ADD;
				colorBlendAttachments[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
				colorBlendAttachments[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
				colorBlendAttachments[i].alphaBlendOp = VK_BLEND_OP_ADD;
			}
		}
		else
		{
			colorBlendAttachments[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
				VK_COLOR_COMPONENT_A_BIT;
			colorBlendAttachments[i].blendEnable = VK_FALSE;
		}
	}

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
	colorBlending.pAttachments = colorBlendAttachments.data();
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.layout = m_pipelineLayout;
	pipelineInfo.renderPass = renderingPipelineCreateInfo.renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = renderingPipelineCreateInfo.enableDepthTesting;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;

	pipelineInfo.pDepthStencilState = &depthStencil;

	// Conservative rasterization
	if (renderingPipelineCreateInfo.enableConservativeRasterization)
	{
		VkPipelineRasterizationConservativeStateCreateInfoEXT conservativeRasterStateCI{};
		conservativeRasterStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_CONSERVATIVE_STATE_CREATE_INFO_EXT;
		conservativeRasterStateCI.conservativeRasterizationMode = VK_CONSERVATIVE_RASTERIZATION_MODE_OVERESTIMATE_EXT;
		conservativeRasterStateCI.extraPrimitiveOverestimationSize = renderingPipelineCreateInfo.maxExtraPrimitiveOverestimationSize;

		rasterizer.pNext = &conservativeRasterStateCI;
	}

	// Tessellation
	if(renderingPipelineCreateInfo.patchControlPoint > 0)
	{
		VkPipelineTessellationStateCreateInfo tessellationStateCreateInfo{};
		tessellationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
		tessellationStateCreateInfo.patchControlPoints = renderingPipelineCreateInfo.patchControlPoint;

		pipelineInfo.pTessellationState = &tessellationStateCreateInfo;
	}

	if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline) != VK_SUCCESS)
		Debug::sendError("Error : graphic pipeline creation");

	for(auto& shaderModule : shaderModules)
		vkDestroyShaderModule(m_device, shaderModule, nullptr);
}

Wolf::Pipeline::Pipeline(VkDevice device, std::string computeShader, VkDescriptorSetLayout* descriptorSetLayout)
{
	m_device = device;
	
	createPipelineLayout(descriptorSetLayout);

	/* Shader */
	std::vector<char> computeShaderCode = readFile(computeShader);
	VkShaderModule computeShaderModule = createShaderModule(computeShaderCode, m_device);

	VkPipelineShaderStageCreateInfo compShaderStageInfo = {};
	compShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	compShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	compShaderStageInfo.module = computeShaderModule;
	compShaderStageInfo.pName = "main";

	/* Pipeline */
	VkComputePipelineCreateInfo pipelineInfo;
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.stage = compShaderStageInfo;
	pipelineInfo.layout = m_pipelineLayout;
	pipelineInfo.flags = 0;
	pipelineInfo.pNext = nullptr;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline) != VK_SUCCESS)
		Debug::sendError("Error : create compute pipeline");
}

Wolf::Pipeline::~Pipeline()
{
	vkDestroyPipeline(m_device, m_pipeline, nullptr);
	vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
}

void Wolf::Pipeline::createPipelineLayout(VkDescriptorSetLayout* descriptorSetLayout)
{
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
		Debug::sendError("Error : create pipeline layout");
}

std::vector<char> Wolf::Pipeline::readFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open())
		Debug::sendError("Error opening file : " + filename);

	size_t fileSize = std::filesystem::file_size(filename);
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

VkShaderModule Wolf::Pipeline::createShaderModule(const std::vector<char>& code, VkDevice device)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		Debug::sendError("Error : create shader module");

	return shaderModule;
}