#pragma once

#include "WolfEngine.h"
#include "Blur.h"

namespace Wolf
{
	class DirectLightingPBR
	{
	public:
		DirectLightingPBR(Wolf::WolfInstance* engineInstance, Wolf::Scene* scene, int commandBufferID,
			VkExtent2D extent, Image* depth, Image* albedoImage, Image* normalRoughnessMetal, Image* shadowMask, Image* volumetricLight, Image* aoMaskImage, Image* lightPropagationVolumes,
			glm::mat4 projection, float near, float far);

		void update(glm::vec3 lightDirectionInViewPosSpace, glm::mat4 voxelProjection);

		Image* getOutputImage() { return m_outputImage; }

	private:
		int m_computePassID;
		Image* m_outputImage;

		struct UBOData
		{
			glm::mat4 invProjection;
			glm::mat4 voxelProjection;
			glm::vec4 projParams;
			glm::vec4 directionDirectionalLight;
			glm::vec4 colorDirectionalLight;
		};
		UBOData m_uboData;
		UniformBuffer* m_ubo;
	};
}
