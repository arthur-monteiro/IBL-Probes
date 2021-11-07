#pragma once

#include "WolfEngine.h"
#include "GBufferStereoscopic.h"
#include "CascadedShadowMappingStereoscopic.h"
#include "DirectLightingStereoscopic.h"

namespace Wolf
{
	class Template3D_VR
	{
	public:
		Template3D_VR(Wolf::WolfInstance* wolfInstance, Wolf::Scene* scene, std::string modelFilename, std::string mtlFolder);

		void update();

		// Getters
		std::vector<int> getCommandBufferToSubmit();
		std::vector<std::pair<int, int>> getCommandBufferSynchronisation();

	private:
		Wolf::WolfInstance* m_wolfInstance;
		Wolf::Scene* m_scene;

		// Render Pass
		int m_renderPassID = -1;
		std::array<int , 2> m_rendererIDs = { -1, -1 };

		// Data
		glm::vec3 m_lightDir = glm::vec3(4.0f, -5.0f, -1.5f);
		std::array<UniformBuffer*, 2> m_ubosMVP;
		glm::mat4 m_modelMatrix;

		// Effects
		int m_gBufferCommandBufferID = -2;
		std::unique_ptr<GBufferStereoscopic> m_GBuffer;

		std::unique_ptr<CascadedShadowMappingStereoscopic> m_cascadedShadowMapping;

		int m_directLightingCommandBufferID = -2;
		std::unique_ptr<DirectLightingStereoscopic> m_directLighting;

		int m_toneMappingCommandBufferID = -2;
		int m_toneMappingComputePassID = -1;
		Wolf::Image* m_toneMappingOutputImage;
	};
}

