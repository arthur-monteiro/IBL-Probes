#pragma once

#include <OVR_CAPI_Vk.h>

#include "Image.h"
#include <glm/glm.hpp>
#include <iostream>
#include <fstream>

namespace Wolf
{
	class OVR
	{
	public:
		OVR(VkDevice device, VkCommandPool commandPool, Queue graphicsQueue, ovrSession session, ovrGraphicsLuid luid);
		~OVR();

		void update();
		int getCurrentImage(VkDevice device, VkQueue presentQueue);
		void present(int imageIndex);

		std::array<glm::mat4, 2>& getProjMatrices() { return m_projMatrices; }
		std::array<glm::mat4, 2>& getViewMatrices() { return m_viewMatrices; }
		std::array<glm::vec3, 2>& getEyePostions() { return m_eyePositions; }
		std::array<glm::vec3, 2>& getEyeDirections() { return m_eyeDirections; }

		void setPlayerPos(glm::vec3 playerPos) { m_playerPos = playerPos; }

	public:
		std::vector<Image*> getImages();

	private:
		ovrSession                  m_session;
		ovrGraphicsLuid             m_luid;

		ovrTextureSwapChain         m_textureChain = nullptr;
		ovrTextureSwapChain         m_depthChain= nullptr;
		
		std::vector<Image>			m_swapchainImages;
		ovrSizei m_size;

		long long m_frameIndex = 0;

		ovrLayerEyeFovDepth m_layer = {};
		glm::vec3 m_playerPos = glm::vec3(0.0f, 0.0f, 3.0f);
		std::array < glm::mat4, 2> m_projMatrices;
		std::array < glm::mat4, 2> m_viewMatrices;

		std::array<glm::vec3, 2> m_eyePositions;
		std::array<glm::vec3, 2> m_eyeDirections;
	};
}
