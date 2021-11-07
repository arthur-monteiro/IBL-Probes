#include "TopLevelAccelerationStructure.h"

#include "Debug.h"

Wolf::TopLevelAccelerationStructure::TopLevelAccelerationStructure(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandBuffer commandBuffer,
                                                                   std::vector<Instance> instances)
{
	m_device = device;
  m_physicalDevice = physicalDevice;
	m_commandBuffer = commandBuffer;
	m_instances = std::move(instances);

	m_accelerationStructureData.structure = createAccelerationStructure(device, {}, VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV, static_cast<uint32_t>(m_instances.size()), 0);
  getAccelerationBufferSizes();
  buildAccelerationStructureBuffers();
}

void Wolf::TopLevelAccelerationStructure::update(VkCommandBuffer commandBuffer, std::vector<Instance> instances)
{
  m_commandBuffer = commandBuffer;

  buildAccelerationStructureBuffers(true);
}

void Wolf::TopLevelAccelerationStructure::getAccelerationBufferSizes()
{
  VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo;
  memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
  memoryRequirementsInfo.pNext = nullptr;
  memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
  memoryRequirementsInfo.accelerationStructure = m_accelerationStructureData.structure;

  // Query the memory requirements. Note that the number of instances in the AS has already
  // been provided when creating the AS descriptor
  VkMemoryRequirements2 memoryRequirements;
  vkGetAccelerationStructureMemoryRequirementsNV(m_device, &memoryRequirementsInfo, &memoryRequirements);

  // Size of the resulting acceleration structure
  m_resultSizeInBytes = memoryRequirements.memoryRequirements.size;

  // Store the memory requirements for use during build
  memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;
  vkGetAccelerationStructureMemoryRequirementsNV(m_device, &memoryRequirementsInfo, &memoryRequirements);
  m_scratchSizeInBytes = memoryRequirements.memoryRequirements.size;

  memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_UPDATE_SCRATCH_NV;
  vkGetAccelerationStructureMemoryRequirementsNV(m_device, &memoryRequirementsInfo, &memoryRequirements);

  m_scratchSizeInBytes = m_scratchSizeInBytes > memoryRequirements.memoryRequirements.size ?
      m_scratchSizeInBytes :
      memoryRequirements.memoryRequirements.size;

  // Amount of memory required to store the instance descriptors
  m_instanceDescsSizeInBytes = m_instances.size() * sizeof(VkGeometryInstance);
}

void Wolf::TopLevelAccelerationStructure::buildAccelerationStructureBuffers(bool rebuild)
{
  if (m_resultSizeInBytes == 0 || m_scratchSizeInBytes == 0)
  {
      Debug::sendError("Invalid scratch and result buffer sizes in buildAccelerationStructureBuffers");
  }

  if (!rebuild)
  {
		createBuffer(m_device, m_physicalDevice, m_scratchSizeInBytes, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			m_accelerationStructureData.scratchBuffer, m_accelerationStructureData.scratchMem);
		createBuffer(m_device, m_physicalDevice, m_resultSizeInBytes, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			m_accelerationStructureData.resultBuffer, m_accelerationStructureData.resultMem);

		createBuffer(m_device, m_physicalDevice, m_instanceDescsSizeInBytes, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			m_accelerationStructureData.instancesBuffer, m_accelerationStructureData.instancesMem);
  }
  
  std::vector<VkGeometryInstance> geometryInstances;
  for (const auto& inst : m_instances)
  {
      uint64_t accelerationStructureHandle = 0;
      VkResult code = vkGetAccelerationStructureHandleNV(m_device, inst.bottomLevelAS, sizeof(uint64_t), &accelerationStructureHandle);
      if (code != VK_SUCCESS)
      {
          Debug::sendError("vkGetAccelerationStructureHandleNV failed");
      }

      VkGeometryInstance gInst{};
      glm::mat4x4 transp = glm::transpose(inst.transform);
      memcpy(gInst.transform, &transp, sizeof(gInst.transform));
      gInst.instanceId = inst.instanceID;
      // The visibility mask is always set of 0xFF, but if some instances would need to be ignored in
      // some cases, this flag should be passed by the application
      gInst.mask = 0xff;
      // Set the hit group index, that will be used to find the shader code to execute when hitting
      // the geometry
      gInst.instanceOffset = inst.hitGroupIndex;
      // Disable culling - more fine control could be provided by the application
      gInst.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV;
      gInst.accelerationStructureHandle = accelerationStructureHandle;
      geometryInstances.push_back(gInst);
  }

  // Copy the instance descriptors into the provided mappable buffer
  VkDeviceSize instancesBufferSize = geometryInstances.size() * sizeof(VkGeometryInstance);
  void* data;
  vkMapMemory(m_device, m_accelerationStructureData.instancesMem, 0, instancesBufferSize, 0, &data);
  memcpy(data, geometryInstances.data(), instancesBufferSize);
  vkUnmapMemory(m_device, m_accelerationStructureData.instancesMem);

  if (!rebuild)
  {
		// Bind the acceleration structure descriptor to the actual memory that will store the AS itself
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
		{
			Debug::sendError("vkBindAccelerationStructureMemoryNV failed");
		}
  }

  // Build the acceleration structure and store it in the result memory
  VkAccelerationStructureInfoNV buildInfo;
  buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
  buildInfo.pNext = nullptr;
  buildInfo.flags = 0;
  buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
  buildInfo.instanceCount = static_cast<uint32_t>(geometryInstances.size());
  buildInfo.geometryCount = 0;
  buildInfo.pGeometries = nullptr;

  vkCmdBuildAccelerationStructureNV(m_commandBuffer, &buildInfo, m_accelerationStructureData.instancesBuffer, 0, VK_FALSE,
    m_accelerationStructureData.structure, VK_NULL_HANDLE, m_accelerationStructureData.scratchBuffer, 0);

  // Ensure that the build will be finished before using the AS using a barrier
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
