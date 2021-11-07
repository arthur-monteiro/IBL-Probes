#pragma once

#include <glm/gtx/transform.hpp>

#include "VulkanElement.h"
#include "Mesh.h"
#include "VulkanHelper.h"

namespace Wolf
{
	class BottomLevelAccelerationStructure : public VulkanElement
	{
	public:
		struct GeometryInfo
		{
			VertexBuffer vertexBuffer;
			VkDeviceSize vertexSize;
			VkFormat vertexFormat;

			VkBuffer transformBuffer;
			VkDeviceSize transformOffsetInBytes;

			glm::mat4 transform;
		};
		
		struct BottomLevelAccelerationStructureCreateInfo
		{
			std::vector<GeometryInfo> geometryInfos;
			VkBuildAccelerationStructureFlagsNV buildFlags;
		};
		
		BottomLevelAccelerationStructure(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkCommandBuffer commandBuffer,
			BottomLevelAccelerationStructureCreateInfo& bottomLevelAccelerationStructureCreateInfo);

		void update(VkCommandBuffer commandBuffer, BottomLevelAccelerationStructureCreateInfo& bottomLevelAccelerationStructureCreateInfo);

		VkAccelerationStructureNV getAccelerationStructure() { return m_accelerationStructureData.structure; }
		glm::mat4 getTransformMatrix() const { return m_transformMatrix; }

	private:
		void getAccelerationBufferSizes();
		void buildAccelerationStructureBuffers(bool update = false);

	private:
		VkCommandBuffer m_commandBuffer;
		
		std::vector<VkGeometryNV> m_vertexBuffers;
		VkDeviceSize m_scratchSizeInBytes = 0;
		VkDeviceSize m_resultSizeInBytes = 0;

		struct AccelerationStructureData
		{
			VkBuffer scratchBuffer = VK_NULL_HANDLE;
			VkDeviceMemory scratchMem = VK_NULL_HANDLE;
			
			VkBuffer resultBuffer = VK_NULL_HANDLE;
			VkDeviceMemory resultMem = VK_NULL_HANDLE;
			
			VkAccelerationStructureNV structure = VK_NULL_HANDLE;
		};
		AccelerationStructureData m_accelerationStructureData;

		glm::mat4 m_transformMatrix;
		VkBuildAccelerationStructureFlagsNV m_buildFlags;
	};
}