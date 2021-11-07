#pragma once

#include "WolfEngine.h"

// Effects
#include "GBuffer.h"
#include "CascadedShadowMapping.h"
#include "SSAO.h"
#include "DirectLightingPBR.h"
#include "LightPropagationVolumes.h"

namespace Wolf
{
	class Template3D
	{
	public:
		Template3D(Wolf::WolfInstance* wolfInstance, Wolf::Scene* scene, std::string modelFilename, std::string mtlFolder, float ratio);

		void update(glm::mat4 view, glm::vec3 cameraPosition, glm::vec3 cameraOrientation);

		// Getters
		std::vector<int> getCommandBufferToSubmit();
		std::vector<std::pair<int, int>> getCommandBufferSynchronisation();

	private:
		void updateMVP();
		
	private:
		Wolf::WolfInstance* m_wolfInstance;
		Wolf::Scene* m_scene;

		// Data
		glm::vec3 m_lightDir = glm::vec3(4.0f, -5.0f, -1.5f);
		UniformBuffer* m_uboMVP;
		glm::mat4 m_projectionMatrix;
		glm::mat4 m_viewMatrix;
		glm::mat4 m_modelMatrix;

		// Effects
		int m_gBufferCommandBufferID = -2;
		std::unique_ptr<GBuffer> m_GBuffer;

		int m_SSAOCommandBufferID = -2;
		std::unique_ptr<SSAO> m_ssao;
		
		std::unique_ptr<CascadedShadowMapping> m_cascadedShadowMapping;

		int m_directLightingSSRBloomCommandBufferID = -2;
		std::unique_ptr<DirectLightingPBR> m_directLighting;

		std::unique_ptr<LightPropagationVolumes> m_lightPropagationVolumes;

		// Merge
		int m_mergeComputePassID = -1;
	};
}

