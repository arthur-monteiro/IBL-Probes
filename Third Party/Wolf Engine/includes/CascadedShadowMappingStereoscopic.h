#pragma once

#include "DepthPass.h"
#include "Blur.h"
#include "Model.h"

namespace Wolf
{
	constexpr int CASCADE_COUNT = 4;

	class CascadedShadowMappingStereoscopic
	{
	public:
		CascadedShadowMappingStereoscopic(Wolf::WolfInstance* engineInstance, Wolf::Scene* scene, Model* model, float cameraNear, float cameraFar, float shadowFar, float cameraFOV,
			VkExtent2D extent, Image* depth, std::array<glm::mat4, 2> projections);

		void updateMatrices(glm::vec3 lightDir, glm::vec3 cameraPosition, glm::vec3 cameraOrientation, glm::mat4 model, std::array<glm::mat4, 2> invModelView);

		std::vector<int> getCascadeCommandBuffers()
		{
			std::vector<int> r(m_cascadeCommandBuffers.begin(), m_cascadeCommandBuffers.end());
			r.push_back(m_shadowMaskCommandBufferID);
			std::vector<int> blurCommandBuffers = m_blur->getCommandBufferIDs();
			for (auto& commandBuffer : blurCommandBuffers)
				r.push_back(commandBuffer);
			return r;
		}
		std::vector<std::pair<int, int>> getCommandBufferSynchronisation()
		{
			std::vector<std::pair<int, int>> r;
			for (auto& commandBuffer : m_cascadeCommandBuffers)
			{
				r.emplace_back(commandBuffer, m_shadowMaskCommandBufferID);
			}

			std::vector<std::pair<int, int>> blurSync = m_blur->getCommandBufferSynchronisation();
			r.emplace_back(m_shadowMaskCommandBufferID, blurSync[0].first);
			for (auto& sync : blurSync)
				r.push_back(sync);

			return r;
		}

		Image* getOutputShadowMaskImage() { return m_shadowMaskOutputImage; }
		Image* getOutputVolumetricLightMaskTexture() { return m_blur->getOutputImage(); }

		glm::vec4 getCascadeSplits() { return glm::vec4(m_cascadeSplits[0], m_cascadeSplits[1], m_cascadeSplits[2], m_cascadeSplits[3]); }
		std::array<glm::mat4, CASCADE_COUNT> getLightSpaceMatrices() { return m_lightSpaceMatrices; }
		std::array<Image*, CASCADE_COUNT> getDepthTextures()
		{
			std::array<Image*, CASCADE_COUNT> r{};
			for (int i(0); i < CASCADE_COUNT; ++i)
				r[i] = m_depthPasses[i]->getResult();

			return r;
		}

	private:
		Wolf::WolfInstance* m_engineInstance;
		Wolf::Scene* m_scene;

		std::array<std::unique_ptr<DepthPass>, CASCADE_COUNT> m_depthPasses;
		std::array<int, CASCADE_COUNT> m_cascadeCommandBuffers;
		std::array<glm::mat4, CASCADE_COUNT> m_lightSpaceMatrices;

		/* Camera params */
		float m_cameraNear;
		float m_cameraFar;
		float m_cameraFOV;
		float m_ratio;
		VkExtent2D m_extent;

		/* Shadow Mask*/
		int m_shadowMaskComputePassID;
		Image* m_shadowMaskOutputImage;
		Image* m_volumetricLightOutputImage;
		int m_shadowMaskCommandBufferID = -2;

		struct ShadowMaskUBO
		{
			std::array<glm::mat4, 2> invModelView;
			std::array<glm::mat4, 2> invProjection;
			glm::vec4 projectionParams;
			std::array<glm::mat4, CASCADE_COUNT> lightSpaceMatrices;
			glm::vec4 cascadeSplits;
		};
		ShadowMaskUBO m_uboData;
		UniformBuffer* m_uniformBuffer;

		std::vector<float> m_cascadeSplits;
		std::vector<VkExtent2D> m_shadowMapExtents;

		std::unique_ptr<Blur> m_blur;
	};
}