#pragma once

#include "VulkanElement.h"
#include "BottomLevelAccelerationStructure.h"
#include "TopLevelAccelerationStructure.h"

namespace Wolf
{
	class AccelerationStructure : public VulkanElement
	{
	public:		
		AccelerationStructure(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, Queue graphicsQueue,
			std::vector<BottomLevelAccelerationStructure::GeometryInfo> geometryInfo, VkBuildAccelerationStructureFlagsNV buildFlags);

		int addBottomLevelAccelerationStructure(std::vector<BottomLevelAccelerationStructure::GeometryInfo>& geometryInfo);
		void rebuildBottomLevelAccelerationStructure(int geometryID, std::vector<BottomLevelAccelerationStructure::GeometryInfo> geometryInfo, VkCommandBuffer commandBuffer);

		VkAccelerationStructureNV* getTopLevelAccelerationStructure() { return m_topLevelAccelerationStructure->getStructure(); }

	private:
		void createTopLevelAcelerationStructure(VkCommandBuffer& commandBuffer);
		
	private:
		std::vector<std::unique_ptr<BottomLevelAccelerationStructure>> m_bottomLevelAccelerationStructures;
		std::unique_ptr<TopLevelAccelerationStructure> m_topLevelAccelerationStructure;
	};

}
