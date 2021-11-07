#include "RayTracingPass.h"

#include "Debug.h"

Wolf::RayTracingPass::RayTracingPass(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool,
                                     RayTracingPassCreateInfo rayTracingPassCreateInfo)
{
	m_device = device;
	m_physicalDevice = physicalDevice;
	m_commandPool = commandPool;

	m_descriptorSetCreateInfo = rayTracingPassCreateInfo.descriptorSetCreateInfo;

	createRayGenShaderStage(rayTracingPassCreateInfo.raygenShader);
	for (auto& missShader : rayTracingPassCreateInfo.missShaders)
		addMissShaderStage(missShader);
	for (auto& hitGroup : rayTracingPassCreateInfo.hitGroups)
		addHitGroup(hitGroup);

	std::vector<DescriptorLayout> descriptorLayouts;
	for (auto& descriptorLayout : rayTracingPassCreateInfo.descriptorSetCreateInfo.descriptorBuffers)
		descriptorLayouts.push_back(descriptorLayout.second);
	for (auto& descriptorLayout : rayTracingPassCreateInfo.descriptorSetCreateInfo.descriptorImages)
		descriptorLayouts.push_back(descriptorLayout.second);
	for (auto& descriptorLayout : rayTracingPassCreateInfo.descriptorSetCreateInfo.descriptorDefault)
		descriptorLayouts.push_back(descriptorLayout.second);
	m_descriptorSetLayout = createDescriptorSetLayout(m_device, descriptorLayouts);

	buildPipeline();

	// SBT
	ShaderBindingTable::ShaderBindingTableCreateInfo shaderBindingTableCreateInfo;
	shaderBindingTableCreateInfo.indices.push_back(m_rayGenIndex);
	for(auto& index : m_missIndices)
		shaderBindingTableCreateInfo.indices.push_back(index);
	for (auto& index : m_hitGroups)
		shaderBindingTableCreateInfo.indices.push_back(index);
	shaderBindingTableCreateInfo.pipeline = m_pipeline;
	
	m_shaderBindingTable = std::make_unique<ShaderBindingTable>(m_device, m_physicalDevice, shaderBindingTableCreateInfo);
}

void Wolf::RayTracingPass::create(VkDescriptorPool descriptorPool)
{
	m_descriptorSet = createDescriptorSet(m_device, m_descriptorSetLayout, descriptorPool, m_descriptorSetCreateInfo);
}

void Wolf::RayTracingPass::record(VkCommandBuffer commandBuffer, VkExtent3D extent)
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, m_pipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, m_pipelineLayout, 0, 1, &m_descriptorSet, 0, nullptr);

	VkDeviceSize rayGenOffset = 0;
	VkDeviceSize missOffset = m_shaderBindingTable->getBaseAlignment();
	VkDeviceSize missStride = m_shaderBindingTable->getBaseAlignment();
	VkDeviceSize hitGroupOffset = m_shaderBindingTable->getBaseAlignment() * (1 + m_missIndices.size());
	VkDeviceSize hitGroupStride = m_shaderBindingTable->getBaseAlignment();

	vkCmdTraceRaysNV(commandBuffer, m_shaderBindingTable->getBuffer(), rayGenOffset,
		m_shaderBindingTable->getBuffer(), missOffset, missStride,
		m_shaderBindingTable->getBuffer(), hitGroupOffset, hitGroupStride,
		VK_NULL_HANDLE, 0, 0, extent.width, extent.height, extent.depth);
}

void Wolf::RayTracingPass::createRayGenShaderStage(std::string raygenShader)
{
	VkShaderModule module = Pipeline::createShaderModule(Pipeline::readFile(raygenShader), m_device);
	
	VkPipelineShaderStageCreateInfo stageCreate;
	stageCreate.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageCreate.pNext = nullptr;
	stageCreate.stage = VK_SHADER_STAGE_RAYGEN_BIT_NV;
	stageCreate.module = module;
	// This member has to be 'main', regardless of the actual entry point of the shader
	stageCreate.pName = "main";
	stageCreate.flags = 0;
	stageCreate.pSpecializationInfo = nullptr;

	m_shaderStages.emplace_back(stageCreate);
	uint32_t shaderIndex = static_cast<uint32_t>(m_shaderStages.size() - 1);

	VkRayTracingShaderGroupCreateInfoNV groupInfo;
	groupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
	groupInfo.pNext = nullptr;
	groupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
	groupInfo.generalShader = shaderIndex;
	groupInfo.closestHitShader = VK_SHADER_UNUSED_NV;
	groupInfo.anyHitShader = VK_SHADER_UNUSED_NV;
	groupInfo.intersectionShader = VK_SHADER_UNUSED_NV;
	m_shaderGroups.emplace_back(groupInfo);

	m_rayGenIndex = static_cast<uint32_t>(m_shaderGroups.size() - 1);
}

void Wolf::RayTracingPass::addMissShaderStage(std::string missShader)
{
	VkShaderModule module = Pipeline::createShaderModule(Pipeline::readFile(missShader), m_device);
	
	VkPipelineShaderStageCreateInfo stageCreate;
	stageCreate.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageCreate.pNext = nullptr;
	stageCreate.stage = VK_SHADER_STAGE_MISS_BIT_NV;
	stageCreate.module = module;
	// This member has to be 'main', regardless of the actual entry point of the shader
	stageCreate.pName = "main";
	stageCreate.flags = 0;
	stageCreate.pSpecializationInfo = nullptr;

	m_shaderStages.emplace_back(stageCreate);
	uint32_t shaderIndex = static_cast<uint32_t>(m_shaderStages.size() - 1);

	VkRayTracingShaderGroupCreateInfoNV groupInfo;
	groupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
	groupInfo.pNext = nullptr;
	groupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
	groupInfo.generalShader = shaderIndex;
	groupInfo.closestHitShader = VK_SHADER_UNUSED_NV;
	groupInfo.anyHitShader = VK_SHADER_UNUSED_NV;
	groupInfo.intersectionShader = VK_SHADER_UNUSED_NV;
	m_shaderGroups.emplace_back(groupInfo);

	m_missIndices.push_back(static_cast<int>(m_shaderGroups.size() - 1));
}

void Wolf::RayTracingPass::addHitGroup(RayTracingPassCreateInfo::HitGroup hitGroup)
{
	VkRayTracingShaderGroupCreateInfoNV groupInfo;
	groupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
	groupInfo.pNext = nullptr;
	groupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_NV;
	groupInfo.generalShader = VK_SHADER_UNUSED_NV;
	groupInfo.closestHitShader = VK_SHADER_UNUSED_NV;
	groupInfo.anyHitShader = VK_SHADER_UNUSED_NV;
	groupInfo.intersectionShader = VK_SHADER_UNUSED_NV;
	
	if(hitGroup.closestHitShader != "")
	{
		VkShaderModule module = Pipeline::createShaderModule(Pipeline::readFile(hitGroup.closestHitShader), m_device);

		VkPipelineShaderStageCreateInfo stageCreate;
		stageCreate.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stageCreate.pNext = nullptr;
		stageCreate.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;
		stageCreate.module = module;
		// This member has to be 'main', regardless of the actual entry point of the shader
		stageCreate.pName = "main";
		stageCreate.flags = 0;
		stageCreate.pSpecializationInfo = nullptr;

		m_shaderStages.emplace_back(stageCreate);
		uint32_t shaderIndex = static_cast<uint32_t>(m_shaderStages.size() - 1);

		groupInfo.closestHitShader = shaderIndex;
	}
	
	m_shaderGroups.push_back(groupInfo);

	m_hitGroups.push_back(static_cast<uint32_t>(m_shaderGroups.size() - 1));
}

void Wolf::RayTracingPass::buildPipeline()
{
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.pNext = nullptr;
	pipelineLayoutCreateInfo.flags = 0;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &m_descriptorSetLayout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

	VkResult code = vkCreatePipelineLayout(m_device, &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout);

	if (code != VK_SUCCESS)
	{
		Debug::sendError("rt vkCreatePipelineLayout failed");
	}

	// Assemble the shader stages and recursion depth info into the raytracing pipeline
	VkRayTracingPipelineCreateInfoNV rayPipelineInfo;
	rayPipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_NV;
	rayPipelineInfo.pNext = nullptr;
	rayPipelineInfo.flags = 0;
	rayPipelineInfo.stageCount = static_cast<uint32_t>(m_shaderStages.size());
	rayPipelineInfo.pStages = m_shaderStages.data();
	rayPipelineInfo.groupCount = static_cast<uint32_t>(m_shaderGroups.size());
	rayPipelineInfo.pGroups = m_shaderGroups.data();
	rayPipelineInfo.maxRecursionDepth = 2;
	rayPipelineInfo.layout = m_pipelineLayout;
	rayPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	rayPipelineInfo.basePipelineIndex = 0;

	code = vkCreateRayTracingPipelinesNV(m_device, nullptr, 1, &rayPipelineInfo, nullptr, &m_pipeline);

	if (code != VK_SUCCESS)
	{
		Debug::sendError("rt vkCreateRayTracingPipelinesNV failed");
	}
}