#pragma once

#include "WolfEngine.h"

namespace Wolf
{
	class Blur
	{
	public:
		Blur(Wolf::WolfInstance* engineInstance, Wolf::Scene* scene, int commandBufferID, Image* inputImage, Image* depthImage);

		Image* getOutputImage() { return m_downscaledBlurredImage2; }
		std::vector<int> getCommandBufferIDs()
		{
			std::vector<int> r =  m_downscaleCommandBufferIDs;
			r.push_back(m_horizontalBlurCommandBuffer);
			r.push_back(m_verticalBlurCommandBuffer);

			return r;
		}
		std::vector<std::pair<int, int>> getCommandBufferSynchronisation()
		{
			std::vector<std::pair<int, int>> r;
			for(int i(0); i < m_downscaleCommandBufferIDs.size() - 1; ++i)
			{
				r.emplace_back(m_downscaleCommandBufferIDs[i], m_downscaleCommandBufferIDs[i + 1]);
			}

			r.emplace_back(m_downscaleCommandBufferIDs.back(), m_horizontalBlurCommandBuffer);
			r.emplace_back(m_horizontalBlurCommandBuffer, m_verticalBlurCommandBuffer);

			return r;
		}

	private:
		// Data
		Image* m_inputImage;
		int m_commandBufferID = -2;

		// Downscale
		std::vector<int> m_downscaleCommandBufferIDs;
		std::vector<int> m_downscaleComputePasses;
		std::vector<Image*> m_downscaledImages;

		// Blur
		int m_horizontalBlurComputePass = -1;
		int m_horizontalBlurCommandBuffer = -2;
		int m_verticalBlurComputePass = -1;
		int m_verticalBlurCommandBuffer = -2;
		Image* m_downscaledBlurredImage;
		Image* m_downscaledBlurredImage2;
	};
}
