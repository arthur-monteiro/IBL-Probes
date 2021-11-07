#pragma once

#include <utility>

#include "DescriptorSet.h"
#include "VulkanHelper.h"
#include "Image.h"
#include "InputVertexTemplate.h"
#include "UniformBuffer.h"
#include "Instance.h"
#include "InstanceTemplate.h"
#include "Pipeline.h"
#include "Mesh.h"

namespace Wolf
{
	struct RendererCreateInfo
	{
		int renderPassID;
		int forceRendererID = -1;

		// Pipeline
		RenderingPipelineCreateInfo pipelineCreateInfo;

		// Templates
		InputVertexTemplate inputVerticesTemplate = InputVertexTemplate::NO;
		InstanceTemplate instanceTemplate = InstanceTemplate::NO;

		// Descriptor set layout
		std::vector<DescriptorLayout> descriptorSetLayout;
	};
	
	class Renderer
	{
	public:
		Renderer(VkDevice device, RendererCreateInfo rendererCreateInfo);
		~Renderer();

		struct AddMeshInfo
		{
			// Links
			int renderPassID;
			int rendererID;
			int frameBufferID = 0;

			// IA
			VertexBuffer vertexBuffer;
			InstanceBuffer instanceBuffer;

			VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

			DescriptorSetCreateInfo descriptorSetCreateInfo;

			bool needDescriptorSet() const
			{
				return !descriptorSetCreateInfo.descriptorBuffers.empty() || !descriptorSetCreateInfo.descriptorImages.empty();
			}
		};
		int addMesh(AddMeshInfo addMeshInfo);

		void updateVertexBuffer(int id, VertexBuffer& vertexBuffer);

		void create(VkDescriptorPool descriptorPool);

		void setViewport(std::array<float, 2> viewportScale, std::array<float, 2> viewportOffset);	

		VkPipeline getPipeline() { return m_pipeline->getPipeline(); }
		std::vector<std::tuple<VertexBuffer, InstanceBuffer, VkDescriptorSet>> getMeshes(int framebufferID = 0);
		std::vector<AddMeshInfo> getMeshInfos() { return m_meshes; }
		VkPipelineLayout getPipelineLayout() { return m_pipeline->getPipelineLayout(); }
		RendererCreateInfo getRendererCreateInfoStructure();

		//void setPipelineCreated(bool status) { m_pipelineCreated = status; }

	private:
		VkDevice m_device;
		VkDescriptorPool m_descriptorPool;
		
		// Information for pipeline
		RenderingPipelineCreateInfo m_renderingPipelineCreate;

		// Descriptor set layout
		std::vector<DescriptorLayout> m_descriptorLayouts;
		VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;

		// Meshes
		std::vector<AddMeshInfo> m_meshes;

		// Pipeline
		std::unique_ptr<Pipeline> m_pipeline = nullptr;

	private:
		void createDescriptorSetLayout(const std::vector<DescriptorLayout>& descriptorLayouts);
	};

}
