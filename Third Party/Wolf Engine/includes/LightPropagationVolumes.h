#pragma once

#include "WolfEngine.h"
#include "DepthPass.h"

namespace Wolf
{
	class LightPropagationVolumes
	{
	public:
		LightPropagationVolumes(Wolf::WolfInstance* engineInstance, Wolf::Scene* scene, Model* model, glm::mat4 projection, glm::mat4 modelMat, glm::vec3 lightDir,
			glm::vec4 cascadeSplits, std::array<Image*, 4> depthTextures);

		void update(glm::mat4 view, std::array<glm::mat4, 4> lightSpaceMatrices, glm::mat4 modelMat);

		std::vector<int> getCommandBufferIDs() { return { m_clearCommandBufferID, m_commandBufferID, m_injectionCommandBufferID,
			m_propagationCommandBufferID, m_viewerBufferID }; }
		std::vector<std::pair<int, int>> getCommandBufferSynchronisations()
		{
			std::vector<std::pair<int, int>> r;

			r.emplace_back(m_clearCommandBufferID, m_injectionCommandBufferID);
			r.emplace_back(m_injectionCommandBufferID, m_propagationCommandBufferID);
			r.emplace_back(m_propagationCommandBufferID, m_viewerBufferID);

			return r;
		}
		Image* getPropagationImage() { return m_lightVolumesPropagationImage; }
		Image* getVoxelViewerOutput() { return m_viewerOutput; }

	private:
		void buildInjection(Model* model, glm::vec4 cascadeSplits, std::array<Image*, 4> depthTextures);
		void buildPropagation();

	private:
		const uint32_t VOXEL_SIZE = 32;
		
		Wolf::WolfInstance* m_engineInstance;
		Wolf::Scene* m_scene;

		int m_commandBufferID = -2;
		int m_renderPassID = -1;
		int m_voxelisationRendererID = -1;
		Image* m_voxelImage;

		UniformBuffer* m_uboVoxelization;
		std::array<glm::mat4, 3> m_projections;

		std::vector<Attachment> m_attachments;
		std::vector<VkClearValue> m_clearValues;

		/* Voxel viewer*/
		int m_viewerBufferID = -2;
		int m_viewerComputePassID = -1;
		Image* m_viewerOutput;
		UniformBuffer* m_uboVoxelViewer;
		std::array<glm::mat4, 3> m_voxelViewerMatrices;

		/* Clear */
		int m_clearCommandBufferID = -2;
		int m_clearComputePassID = -1;

		/* Injection */
		int m_injectionCommandBufferID = -2;
		int m_injectionRenderPassID = -1;
		int m_injectionRendererID = -1;

		std::array<Image*, 7> m_injectionImages;
		UniformBuffer* m_uboInjection;

		struct InjectionUBO
		{
			glm::mat4 projectionX;
			glm::mat4 projectionY;
			glm::mat4 projectionZ;

			std::array<glm::mat4, 4> lightSpaceMatrices;
			glm::vec4 cascadeSplits;
			glm::mat4 modelView;
		};
		InjectionUBO m_uboInjectionData;

		std::vector<Attachment> m_injectionAttachments;
		std::vector<VkClearValue> m_injectionClearValues;

		/* Injection */
		/*int m_rsmCommandBufferID = -2;
		int m_rsmRenderPassID = -1;
		int m_rsmRendererID = -1;

		UniformBuffer* m_uboRSM;
		glm::mat4 m_rsmMatrix;

		std::vector<Attachment> m_rsmAttachments;
		std::vector<VkClearValue> m_rsmClearValues;

		Texture* m_lightVolumesInjectionTexture;
		Texture* m_lightVolumesInjectionDirTexture;
		int m_injectionCommandBufferID = -2;
		int m_injectionComputePassID = -1;
		UniformBuffer* m_uboInjection;
		struct UniformBufferObjectInjection
		{
			glm::mat4 voxelProjection;
		};
		UniformBufferObjectInjection m_uboInjectionData;*/

		/* Propagation */
		Image* m_lightVolumesPropagationImage;
		int m_propagationCommandBufferID = -2;
		int m_propagationComputePassID = -1;
	};
}
