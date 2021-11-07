#pragma once

#include "WolfEngine.h"
#include "Blur.h"

namespace Wolf
{
	class DirectLightingStereoscopic
	{
	public:
		DirectLightingStereoscopic(Wolf::WolfInstance* engineInstance, Wolf::Scene* scene, int commandBufferID,
			VkExtent2D extent, Image* depth, Image* albedoImage, Image* normalRoughnessMetal, Image* shadowMask, Image* volumetricLight, Image* aoMaskImage, Image* lightPropagationVolumes,
			std::array<glm::mat4, 2> projections, float near, float far);

		void update(std::array<glm::vec3, 2> lightDirectionsInViewPosSpace, glm::mat4 voxelProjection);

		Image* getOutputImage() { return m_outputImage; }

	private:
		int m_computePassID;
		Image* m_outputImage;

		struct UBOData
		{
			std::array<glm::mat4, 2> invProjections;
			glm::mat4 voxelProjection;
			glm::vec4 projParams;
			std::array<glm::vec4, 2> directionDirectionalLights;
			glm::vec4 colorDirectionalLight;
		};
		UBOData m_uboData;
		UniformBuffer* m_ubo;
	};
}
