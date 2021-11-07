#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

#include <map>

#include "VulkanElement.h"
#include "Image.h"
#include "Sampler.h"

namespace Wolf
{
	class Font : public VulkanElement
	{
	public:
		Font(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, Queue graphicsQueue, int ySize, std::string path);

		unsigned int getXSize(const wchar_t character) { return m_characters[character].xSize; }
		unsigned int getYSize(const wchar_t character) { return m_characters[character].ySize; }
		unsigned int getBearingX(const wchar_t character) { return m_characters[character].bearingX; }
		unsigned int getBearingY(const wchar_t character) { return m_characters[character].bearingY; }
		int getMaxSizeY() const { return m_maxYSize; }
		unsigned int getMaterialID(const wchar_t character) { return m_characters[character].textureID; }
		std::vector<Image*> getImages();
		Wolf::Sampler* getSampler() { return m_sampler.get(); }

	private:
		struct Character
		{
			unsigned int xSize = 0; // in pixel
			unsigned int ySize = 0;
			unsigned int bearingX = 0;
			unsigned int bearingY = 0;

			unsigned int textureID = 0;;
		};

		std::map<wchar_t, Character> m_characters;
		std::vector<std::unique_ptr<Image>> m_images;
		std::unique_ptr<Sampler> m_sampler;
		int m_maxYSize;
	};
}