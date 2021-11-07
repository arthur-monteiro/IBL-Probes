#pragma once

#include <glm/gtx/transform.hpp>

#include "VulkanHelper.h"

namespace Wolf
{
	class TopLevelAccelerationStructure
	{
	public:
    struct Instance
    {
        VkAccelerationStructureNV bottomLevelAS;
        glm::mat4 transform;
        uint32_t instanceID; // Instance ID visible in the shader
        uint32_t hitGroupIndex; // Hit group index used to fetch the shaders from the SBT
    };

		TopLevelAccelerationStructure(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandBuffer commandBuffer, std::vector<Instance> instances);

		void update(VkCommandBuffer commandBuffer, std::vector<Instance> instances);

		VkAccelerationStructureNV* getStructure() { return &m_accelerationStructureData.structure; }

	private:
		void getAccelerationBufferSizes();
		void buildAccelerationStructureBuffers(bool rebuild = false);

		// Geometry instance, with the layout expected by VK_NV_ray_tracing
		struct VkGeometryInstance
		{
			// Transform matrix, containing only the top 3 rows
			float transform[12];
			// Instance index
			uint32_t instanceId : 24;
			// Visibility mask
			uint32_t mask : 8;
			// Index of the hit group which will be invoked when a ray hits the instance
			uint32_t instanceOffset : 24;
			// Instance flags, such as culling
			uint32_t flags : 8;
			// Opaque handle of the bottom-level acceleration structure
			uint64_t accelerationStructureHandle;
		};

		static_assert(sizeof(VkGeometryInstance) == 64,
			"VkGeometryInstance structure compiles to incorrect size");
		
	private:
		VkDevice m_device;
		VkPhysicalDevice m_physicalDevice;
		VkCommandBuffer m_commandBuffer;

		std::vector<Instance> m_instances;

		VkDeviceSize m_scratchSizeInBytes = 0;
		VkDeviceSize m_resultSizeInBytes = 0;
		VkDeviceSize m_instanceDescsSizeInBytes;

		struct AccelerationStructureData
		{
			VkBuffer scratchBuffer = VK_NULL_HANDLE;
			VkDeviceMemory scratchMem = VK_NULL_HANDLE;

			VkBuffer resultBuffer = VK_NULL_HANDLE;
			VkDeviceMemory resultMem = VK_NULL_HANDLE;

			VkBuffer instancesBuffer = VK_NULL_HANDLE;
			VkDeviceMemory instancesMem = VK_NULL_HANDLE;

			VkAccelerationStructureNV structure = VK_NULL_HANDLE;
		};
		AccelerationStructureData m_accelerationStructureData;
	};
}
