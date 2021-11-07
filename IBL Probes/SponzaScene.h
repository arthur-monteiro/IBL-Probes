#pragma once

#include <WolfEngine.h>
#include <Template3D.h>
#include "ProbeGeneration.h"

#include "Camera.h"

class SponzaScene
{
public:
	SponzaScene(Wolf::WolfInstance* wolfInstance);

	void update();

	[[nodiscard]] Wolf::Scene* getScene() const { return m_scene; }
	[[nodiscard]] std::vector<int> getCommandBufferToSubmit();
	[[nodiscard]] std::vector<std::pair<int, int>> getCommandBufferSynchronisation();

private:
	Camera m_camera;
	GLFWwindow* m_window;

	Wolf::Scene* m_scene = nullptr;

	// Data
	glm::vec3 m_lightDir = glm::vec3(4.0f, -5.0f, -1.5f);
	glm::vec3 m_lightColor = glm::vec3(10.0f, 9.0f, 6.0f);
	glm::mat4 m_projectionMatrix;
	glm::mat4 m_viewMatrix;
	glm::mat4 m_modelMatrix;

	// Sky
	Wolf::Image* m_skyCubemap;

	std::array<Wolf::UniformBuffer*, 6> m_probeMatricesUBs;
	std::unique_ptr<ProbeGeneration> m_probeGeneration;
	Wolf::Image* m_brdfImage;
	
	// Effects
	int m_gBufferCommandBufferID = -2;
	std::unique_ptr<Wolf::GBuffer> m_GBuffer;

	std::unique_ptr<Wolf::CascadedShadowMapping> m_cascadedShadowMapping;

	Wolf::Image* m_directLightingOutputImage;
	struct DirectLightingUBData
	{
		glm::mat4 invProjection;
		glm::mat4 invView;
		glm::mat4 voxelProjection;
		glm::vec4 projParams;
		glm::vec4 directionDirectionalLight;
		glm::vec4 colorDirectionalLight;
	};
	DirectLightingUBData m_directLightingUBData;
	Wolf::UniformBuffer* m_directLightingUniformBuffer;
	int m_directLightingCommandBufferID;
};

