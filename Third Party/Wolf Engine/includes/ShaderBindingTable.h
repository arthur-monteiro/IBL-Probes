#pragma once

#include "VulkanHelper.h"

namespace Wolf
{
	class ShaderBindingTable
	{
	public:
		struct ShaderBindingTableCreateInfo
		{
			std::vector<uint32_t> indices;
			VkPipeline pipeline;
		};
		ShaderBindingTable(VkDevice device, VkPhysicalDevice physicalDevice, ShaderBindingTableCreateInfo shaderBindingTableCreateInfo);

		uint32_t getBaseAlignment() { return m_baseAlignement; }
		VkBuffer getBuffer() { return m_shaderBindingTableBuffer; }

	private:
		void copyShaderData(VkPipeline pipeline, uint8_t* outputData, const std::vector<uint32_t>& shaders, VkDeviceSize entrySize, const uint8_t* shaderHandleStorage);
		
	private:
		VkDevice m_device;

		uint32_t m_baseAlignement;
		VkBuffer m_shaderBindingTableBuffer;
		VkDeviceMemory m_shaderBindingTableMem;
	};
}
