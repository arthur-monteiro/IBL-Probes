#pragma once

#include "WolfEngine.h"
#include "RenderPass.h"
#include "UniformBuffer.h"
#include "Model.h"

namespace Wolf
{
	class GBufferStereoscopic
	{
	public:
		GBufferStereoscopic(Wolf::WolfInstance* engineInstance, Wolf::Scene* scene, int commandBufferID, VkExtent2D extent, VkSampleCountFlagBits sampleCount,
			Model* model, glm::mat4 mvp, bool useDepthAsStorage);

		void updateMatrices(glm::mat4 m, glm::mat4 v0, glm::mat4 v1, glm::mat4 p0, glm::mat4 p1);

		Image* getDepth() { return m_scene->getRenderPassOutput(m_renderPassID, 0); }
		Image* getAlbedo() { return m_scene->getRenderPassOutput(m_renderPassID, 2); }
		Image* getNormalRoughnessMetal() { return m_scene->getRenderPassOutput(m_renderPassID, 1); }

	private:
		Wolf::WolfInstance* m_engineInstance;
		Wolf::Scene* m_scene;

		int m_renderPassID = -1;

		std::vector<Attachment> m_attachments;
		std::vector<VkClearValue> m_clearValues;

		struct RenderElements
		{
			UniformBuffer* ubMVP;
			std::array<glm::mat4, 3> mvp;
			int rendererID;
		};
		std::array<RenderElements, 2> m_renderElements;

		VkSampleCountFlagBits m_sampleCount = VK_SAMPLE_COUNT_1_BIT;
	};
}

