#include "Font.h"

Wolf::Font::Font(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, Queue graphicsQueue, int ySize, std::string path)
{
	m_device = device;
	m_physicalDevice = physicalDevice;
	m_commandPool = commandPool;
	m_graphicsQueue = graphicsQueue;

	m_maxYSize = ySize;

	unsigned int texWidth(0), texHeight(0), texChannels(0);
	FT_Library ft;
	if (FT_Init_FreeType(&ft))
		throw std::runtime_error("Error : freeType init");

	FT_Face face;
	if (FT_New_Face(ft, path.c_str(), 0, &face))
		throw std::runtime_error("Error : font loading");

	FT_Set_Pixel_Sizes(face, 0, ySize);

	for (wchar_t c = 33; c < 123; ++c)
	{
		if (FT_Load_Char(face, c, FT_LOAD_RENDER))
			throw std::runtime_error(&"Error : character loading "[c]);

		if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL)) /* convert to an anti-aliased bitmap */
			throw std::runtime_error(&"Error : render glyph "[c]);

		texWidth = face->glyph->bitmap.width;
		texHeight = face->glyph->bitmap.rows;

		if (texWidth == 0 || texHeight == 0)
			throw std::runtime_error(&"Error : create pixel from character "[c]);

		auto* pixels = new unsigned char[static_cast<size_t>(texWidth) * texHeight];
		memcpy(pixels, face->glyph->bitmap.buffer, static_cast<size_t>(texWidth) * texHeight);

		m_characters[c].xSize = texWidth;
		m_characters[c].ySize = texHeight;
		m_characters[c].bearingX = face->glyph->bitmap_left;
		m_characters[c].bearingY = texHeight - face->glyph->bitmap_top;

		m_images.emplace_back();
		VkExtent3D extent3D = { texWidth, texHeight, 1 };

		Image::CreateImageInfo createImageInfo;
		createImageInfo.extent = extent3D;
		createImageInfo.format = VK_FORMAT_R8_UNORM;
		createImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		m_images[m_images.size() - 1] = std::make_unique<Image>(m_device, m_physicalDevice, m_commandPool, m_graphicsQueue, createImageInfo);
		m_images.back()->copyPixels(pixels);

		m_characters[c].textureID = static_cast<unsigned int>(m_images.size() - 1);

		delete[] pixels;
	}

	FT_Done_Face(face);
	FT_Done_FreeType(ft);

	m_sampler = std::make_unique<Sampler>(device, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1.0f, VK_FILTER_LINEAR);
}

std::vector<Wolf::Image*> Wolf::Font::getImages()
{
	std::vector<Wolf::Image*> r(m_images.size());

	for (int i(0); i < m_images.size(); ++i)
		r[i] = m_images[i].get();

	return r;
}
