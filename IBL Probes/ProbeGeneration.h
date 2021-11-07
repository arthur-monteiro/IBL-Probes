#pragma once

#include "Common.h"

class ProbeGeneration
{
public:
	struct Probe
	{
		glm::vec3 position;
		Wolf::Image* cubeMap;
		Wolf::Image* irradiance;
		Wolf::Image* prefilter;
	};

	ProbeGeneration(Wolf::WolfInstance* wolfInstance, Wolf::Scene* scene, const Wolf::Model* model, glm::mat4 modelMatrix, glm::vec3 lightDir, glm::vec3 lightColor, Wolf::Image* skyCubemap, Wolf::Sampler* cubemapSampler, Wolf::Model* cube, std::array<Wolf::UniformBuffer*, 6> probeMatricesUBs);

	const Probe& GetProbe() const { return m_probes[0]; }

private:
	// Common
	Wolf::WolfInstance* m_wolfInstance;
	std::vector<Probe> m_probes;

	// Cubemap generation
	void createCubemapGenerationPipeline(const Wolf::Model* model);
	Wolf::Scene* m_cubemapGenerationScene;
	int m_cubemapGenerationCommandBuffer;
	int m_cubemapGenerationRenderPassId;
	int m_cubemapGenerationSponzaRendererId;
	int m_cubemapGenerationSkyRendererId;

	struct ProbeLightingUBData
	{
		glm::mat4 invModelView;
		glm::mat4 lightSpaceMatrix;
		glm::vec4 directionDirectionalLight;
		glm::vec4 colorDirectionalLight;
	};
	std::array<Wolf::UniformBuffer*, 6> m_probeLightingUBs;

	struct ProbeRoughnessUBData
	{
		float roughness;
	};
	Wolf::UniformBuffer* m_probeRougnessUB;
};

