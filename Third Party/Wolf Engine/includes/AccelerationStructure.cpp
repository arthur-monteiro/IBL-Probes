#include "AccelerationStructure.h"

Wolf::AccelerationStructure::AccelerationStructure(VkDevice device, VkPhysicalDevice physicalDevice,
	VkCommandPool commandPool, Queue graphicsQueue, std::vector<BottomLevelAccelerationStructure::GeometryInfo> geometryInfo, VkBuildAccelerationStructureFlagsNV buildFlags)
{
	m_device = device;
	m_physicalDevice = physicalDevice;
	m_commandPool = commandPool;
	m_graphicsQueue = graphicsQueue;

	VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

	// Bottom Level AS
	BottomLevelAccelerationStructure::BottomLevelAccelerationStructureCreateInfo bottomLevelAccelerationStructureCreateInfo;
	bottomLevelAccelerationStructureCreateInfo.geometryInfos = geometryInfo;
	bottomLevelAccelerationStructureCreateInfo.buildFlags = buildFlags;

	m_bottomLevelAccelerationStructures.resize(1);
	m_bottomLevelAccelerationStructures[0] = std::make_unique<BottomLevelAccelerationStructure>(device, physicalDevice, commandPool, commandBuffer, bottomLevelAccelerationStructureCreateInfo);

	createTopLevelAcelerationStructure(commandBuffer);

	endSingleTimeCommands(device, graphicsQueue, commandBuffer, commandPool);
}

int Wolf::AccelerationStructure::addBottomLevelAccelerationStructure(std::vector<BottomLevelAccelerationStructure::GeometryInfo>& geometryInfo)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(m_device, m_commandPool);

	BottomLevelAccelerationStructure::BottomLevelAccelerationStructureCreateInfo bottomLevelAccelerationStructureCreateInfo;
	bottomLevelAccelerationStructureCreateInfo.geometryInfos = geometryInfo;

	m_bottomLevelAccelerationStructures.push_back(std::make_unique<BottomLevelAccelerationStructure>(m_device, m_physicalDevice, m_commandPool, commandBuffer, bottomLevelAccelerationStructureCreateInfo));

	createTopLevelAcelerationStructure(commandBuffer);

	endSingleTimeCommands(m_device, m_graphicsQueue, commandBuffer, m_commandPool);

	return static_cast<int>(m_bottomLevelAccelerationStructures.size()) - 1;
}

void Wolf::AccelerationStructure::rebuildBottomLevelAccelerationStructure(int bottomLevelAccelerationStructureID, std::vector<BottomLevelAccelerationStructure::GeometryInfo> geometryInfo, VkCommandBuffer commandBuffer)
{
	BottomLevelAccelerationStructure::BottomLevelAccelerationStructureCreateInfo bottomLevelAccelerationStructureCreateInfo;
	bottomLevelAccelerationStructureCreateInfo.geometryInfos = geometryInfo;

	m_bottomLevelAccelerationStructures[bottomLevelAccelerationStructureID]->update(commandBuffer, bottomLevelAccelerationStructureCreateInfo);

	std::vector<TopLevelAccelerationStructure::Instance> topLevelInstances(m_bottomLevelAccelerationStructures.size());

	int index = 0;
	for (auto& topLevelInstance : topLevelInstances)
	{
		topLevelInstance.bottomLevelAS = m_bottomLevelAccelerationStructures[index]->getAccelerationStructure();
		topLevelInstance.transform = m_bottomLevelAccelerationStructures[index]->getTransformMatrix();
		topLevelInstance.instanceID = 0;
		topLevelInstance.hitGroupIndex = 0;
	}

	m_topLevelAccelerationStructure->update(commandBuffer, topLevelInstances);
}

void Wolf::AccelerationStructure::createTopLevelAcelerationStructure(VkCommandBuffer& commandBuffer)
{
	std::vector<TopLevelAccelerationStructure::Instance> topLevelInstances(m_bottomLevelAccelerationStructures.size());

	int index = 0;
	for (auto& topLevelInstance : topLevelInstances)
	{
		topLevelInstance.bottomLevelAS = m_bottomLevelAccelerationStructures[index]->getAccelerationStructure();
		topLevelInstance.transform = m_bottomLevelAccelerationStructures[index]->getTransformMatrix();
		topLevelInstance.instanceID = 0;
		topLevelInstance.hitGroupIndex = 0;
	}

	m_topLevelAccelerationStructure = std::make_unique<TopLevelAccelerationStructure>(m_device, m_physicalDevice, commandBuffer, topLevelInstances);
}
