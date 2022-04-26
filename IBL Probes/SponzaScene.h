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

	// Debug
	bool m_useDebug = true; // dynamically modified
	int m_debugCommandBufferID = -2;
	struct InstanceDebug
	{
		glm::vec3 posOffset;
		glm::uint32_t id;

		static VkVertexInputBindingDescription getBindingDescription(uint32_t binding)
		{
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = binding;
			bindingDescription.stride = sizeof(InstanceDebug);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

			return bindingDescription;
		}

		static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions(uint32_t binding, uint32_t startLocation)
		{
			std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);

			attributeDescriptions[0].binding = binding;
			attributeDescriptions[0].location = startLocation;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = 0;

			attributeDescriptions[1].binding = binding;
			attributeDescriptions[1].location = startLocation;
			attributeDescriptions[1].format = VK_FORMAT_R32_UINT;
			attributeDescriptions[1].offset = 1;

			return attributeDescriptions;
		}
	};
	struct DebugUBData
	{
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 projection;
	};
	DebugUBData m_debugUBData;
	Wolf::UniformBuffer* m_debugUniformBuffer;
};

