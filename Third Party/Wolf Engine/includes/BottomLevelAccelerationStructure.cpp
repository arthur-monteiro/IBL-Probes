#include "BottomLevelAccelerationStructure.h"
#include "Debug.h"

Wolf::BottomLevelAccelerationStructure::BottomLevelAccelerationStructure(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkCommandBuffer commandBuffer,
                                                                         BottomLevelAccelerationStructureCreateInfo& bottomLevelAccelerationStructureCreateInfo)
{
	m_device = device;
	m_physicalDevice = physicalDevice;
	m_commandPool = commandPool;
	m_commandBuffer = commandBuffer;
	m_transformMatrix = bottomLevelAccelerationStructureCreateInfo.geometryInfos[0].transform;
	m_buildFlags = bottomLevelAccelerationStructureCreateInfo.buildFlags;
	
	m_vertexBuffers.resize(bottomLevelAccelerationStructureCreateInfo.geometryInfos.size());

	int i(0);
	for(auto& geometryInfo : bottomLevelAccelerationStructureCreateInfo.geometryInfos)
	{
		VkGeometryNV geometry;
		geometry.sType = VK_STRUCTURE_TYPE_GEOMETRY_NV;
		geometry.pNext = nullptr;
		geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_NV;
		geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
		geometry.geometry.triangles.pNext = nullptr;
		geometry.geometry.triangles.vertexData = geometryInfo.vertexBuffer.vertexBuffer;
		geometry.geometry.triangles.vertexOffset = 0;
		geometry.geometry.triangles.vertexCount = geometryInfo.vertexBuffer.nbVertices;
		geometry.geometry.triangles.vertexStride = geometryInfo.vertexSize;
		// Limitation to 3xfloat32 for vertices
		geometry.geometry.triangles.vertexFormat = geometryInfo.vertexFormat;
		geometry.geometry.triangles.indexData = geometryInfo.vertexBuffer.indexBuffer;
		geometry.geometry.triangles.indexOffset = 0;
		geometry.geometry.triangles.indexCount = geometryInfo.vertexBuffer.nbIndices;
		// Limitation to 32-bit indices
		geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
		geometry.geometry.triangles.transformData = geometryInfo.transformBuffer;
		geometry.geometry.triangles.transformOffset = geometryInfo.transformOffsetInBytes;
		geometry.geometry.aabbs = { VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV };
		geometry.flags = VK_GEOMETRY_OPAQUE_BIT_NV;

		m_vertexBuffers[i++] = geometry;
	}

	m_accelerationStructureData.structure = createAccelerationStructure(m_device, m_vertexBuffers, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV, 0, m_buildFlags);
	getAccelerationBufferSizes();
	buildAccelerationStructureBuffers();
}

void Wolf::BottomLevelAccelerationStructure::update(VkCommandBuffer commandBuffer, BottomLevelAccelerationStructureCreateInfo& bottomLevelAccelerationStructureCreateInfo)
{
	m_commandBuffer = commandBuffer;

	int i(0);
	for (auto& geometryInfo : bottomLevelAccelerationStructureCreateInfo.geometryInfos)
	{
		VkGeometryNV geometry;
		geometry.sType = VK_STRUCTURE_TYPE_GEOMETRY_NV;
		geometry.pNext = nullptr;
		geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_NV;
		geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
		geometry.geometry.triangles.pNext = nullptr;
		geometry.geometry.triangles.vertexData = geometryInfo.vertexBuffer.vertexBuffer;
		geometry.geometry.triangles.vertexOffset = 0;
		geometry.geometry.triangles.vertexCount = geometryInfo.vertexBuffer.nbVertices;
		geometry.geometry.triangles.vertexStride = geometryInfo.vertexSize;
		// Limitation to 3xfloat32 for vertices
		geometry.geometry.triangles.vertexFormat = geometryInfo.vertexFormat;
		geometry.geometry.triangles.indexData = geometryInfo.vertexBuffer.indexBuffer;
		geometry.geometry.triangles.indexOffset = 0;
		geometry.geometry.triangles.indexCount = geometryInfo.vertexBuffer.nbIndices;
		// Limitation to 32-bit indices
		geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
		geometry.geometry.triangles.transformData = geometryInfo.transformBuffer;
		geometry.geometry.triangles.transformOffset = geometryInfo.transformOffsetInBytes;
		geometry.geometry.aabbs = { VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV };
		geometry.flags = VK_GEOMETRY_OPAQUE_BIT_NV;

		m_vertexBuffers[i++] = geometry;
	}

	buildAccelerationStructureBuffers(true);
}

void Wolf::BottomLevelAccelerationStructure::getAccelerationBufferSizes()
{
	VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo;
	memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
	memoryRequirementsInfo.pNext = nullptr;
	memoryRequirementsInfo.accelerationStructure = m_accelerationStructureData.structure;
	memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;

	// Get memory requirements
	VkMemoryRequirements2 memoryRequirements;
	vkGetAccelerationStructureMemoryRequirementsNV(m_device, &memoryRequirementsInfo, &memoryRequirements);

	// Size of the resulting AS
	m_resultSizeInBytes = memoryRequirements.memoryRequirements.size;

	// Store the memory requirements for use during build/update
	memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;
	vkGetAccelerationStructureMemoryRequirementsNV(m_device, &memoryRequirementsInfo, &memoryRequirements);
	m_scratchSizeInBytes = memoryRequirements.memoryRequirements.size;

	memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_UPDATE_SCRATCH_NV;
	vkGetAccelerationStructureMemoryRequirementsNV(m_device, &memoryRequirementsInfo,
		&memoryRequirements);

	m_scratchSizeInBytes = m_scratchSizeInBytes > memoryRequirements.memoryRequirements.size ?
		m_scratchSizeInBytes :
		memoryRequirements.memoryRequirements.size;
}

void Wolf::BottomLevelAccelerationStructure::buildAccelerationStructureBuffers(bool update)
{
	if (m_resultSizeInBytes == 0 || m_scratchSizeInBytes == 0)
	{
		Debug::sendError("Invalid scratch and result buffer sizes in buildAccelerationStructureBuffers");
	}
	
	if (!update)
	{
		createBuffer(m_device, m_physicalDevice, m_scratchSizeInBytes, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			m_accelerationStructureData.scratchBuffer, m_accelerationStructureData.scratchMem);
		createBuffer(m_device, m_physicalDevice, m_resultSizeInBytes, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			m_accelerationStructureData.resultBuffer, m_accelerationStructureData.resultMem);

		VkBindAccelerationStructureMemoryInfoNV bindInfo;
		bindInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
		bindInfo.pNext = nullptr;
		bindInfo.accelerationStructure = m_accelerationStructureData.structure;
		bindInfo.memory = m_accelerationStructureData.resultMem;
		bindInfo.memoryOffset = 0;
		bindInfo.deviceIndexCount = 0;
		bindInfo.pDeviceIndices = nullptr;

		VkResult code = vkBindAccelerationStructureMemoryNV(m_device, 1, &bindInfo);

		if (code != VK_SUCCESS)
			Debug::sendError("vkBindAccelerationStructureMemoryNV failed");
	}

	// Build the actual bottom-level acceleration structure
	VkAccelerationStructureInfoNV buildInfo;
	buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	buildInfo.pNext = nullptr;
	buildInfo.flags = m_buildFlags;
	buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
	buildInfo.geometryCount = static_cast<uint32_t>(m_vertexBuffers.size());
	buildInfo.pGeometries = m_vertexBuffers.data();
	buildInfo.instanceCount = 0;

	vkCmdBuildAccelerationStructureNV(m_commandBuffer, &buildInfo, VK_NULL_HANDLE, 0, VK_FALSE,
		m_accelerationStructureData.structure, VK_NULL_HANDLE, m_accelerationStructureData.scratchBuffer,
		0);

	VkMemoryBarrier memoryBarrier;
	memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	memoryBarrier.pNext = nullptr;
	memoryBarrier.srcAccessMask =
		VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
	memoryBarrier.dstAccessMask =
		VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;

	vkCmdPipelineBarrier(m_commandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV,
		VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memoryBarrier,
		0, nullptr, 0, nullptr);
}
