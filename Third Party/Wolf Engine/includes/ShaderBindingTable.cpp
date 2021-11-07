#include "ShaderBindingTable.h"

#include "Debug.h"

Wolf::ShaderBindingTable::ShaderBindingTable(VkDevice device, VkPhysicalDevice physicalDevice, ShaderBindingTableCreateInfo shaderBindingTableCreateInfo)
{
	VkPhysicalDeviceRayTracingPropertiesNV props = getPhysicalDeviceRayTracingProperties(physicalDevice);

	uint32_t groupHandleSize = props.shaderGroupHandleSize;
	m_baseAlignement = props.shaderGroupBaseAlignment;

	uint32_t sbtSize = m_baseAlignement * static_cast<uint32_t>(shaderBindingTableCreateInfo.indices.size());

	createBuffer(device, physicalDevice, sbtSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, m_shaderBindingTableBuffer, m_shaderBindingTableMem);

	// Generation
	uint32_t groupCount = static_cast<uint32_t>(shaderBindingTableCreateInfo.indices.size());

	// Fetch all the shader handles used in the pipeline, so that they can be written in the SBT
	// Note that this could be also done by fetching the handles one by one when writing the SBT entries
	std::vector<uint8_t>     shaderHandleStorage (sbtSize);
	VkResult code = vkGetRayTracingShaderGroupHandlesNV(device, shaderBindingTableCreateInfo.pipeline, 0, groupCount, sbtSize, shaderHandleStorage.data());

	// Map the SBT
	void* vData;

	code = vkMapMemory(device, m_shaderBindingTableMem, 0, sbtSize, 0, &vData);

	if (code != VK_SUCCESS)
		Debug::sendError("SBT vkMapMemory failed");

	auto* data = static_cast<uint8_t*>(vData);

	int index(0);
	for(auto& shaderIndex : shaderBindingTableCreateInfo.indices)
	{
		memcpy(data, shaderHandleStorage.data() + index * groupHandleSize, groupHandleSize);
		data += m_baseAlignement;
		index++;
	}

	// Unmap the SBT
	vkUnmapMemory(device, m_shaderBindingTableMem);
}