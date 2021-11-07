#pragma once

#include "WolfEngine.h"
#include "RenderPass.h"
#include "UniformBuffer.h"
#include "Model.h"

namespace Wolf
{
	class DepthPass
	{
	public:
		DepthPass() = default;
		DepthPass(Wolf::WolfInstance* engineInstance, Wolf::Scene* scene, bool outputIsSwapChain, VkExtent2D extent, VkSampleCountFlagBits sampleCount,
			const Model* model, glm::mat4 mvp, bool useAsStorage, bool useAsSampled);
		~DepthPass() = default;

		void update(glm::mat4 mvp);

		int getCommandBufferID() { return m_commandBufferID; }
		Image* getResult() { return m_scene->getRenderPassOutput(m_renderPassID, 0); }

	protected:
		Wolf::WolfInstance* m_engineInstance;
		Wolf::Scene* m_scene;

		int m_commandBufferID = -2;
		int m_renderPassID = -1;
		
		Attachment m_attachment;
		std::vector<VkClearValue> m_clearValues;

		UniformBuffer* m_uboMVP;
		glm::mat4 m_mvp;
		int m_rendererID;

		VkSampleCountFlagBits m_sampleCount = VK_SAMPLE_COUNT_1_BIT;
	};
}
