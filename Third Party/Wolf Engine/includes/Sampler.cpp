#include "Sampler.h"
#include "Debug.h"

Wolf::Sampler::Sampler(VkDevice device, VkSamplerAddressMode addressMode, float mipLevels, VkFilter filter, float maxAnisotropy, float minLod, float mipLodBias)
{
	m_device = device;

	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = filter;
	samplerInfo.minFilter = filter;

	samplerInfo.addressModeU = addressMode;
	samplerInfo.addressModeV = addressMode;
	samplerInfo.addressModeW = addressMode;

	samplerInfo.anisotropyEnable = maxAnisotropy > 0.0f;
	samplerInfo.maxAnisotropy = maxAnisotropy;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = mipLodBias;
	samplerInfo.minLod = minLod;
	samplerInfo.maxLod = mipLevels;

	samplerInfo.pNext = VK_NULL_HANDLE;

	if (vkCreateSampler(m_device, &samplerInfo, nullptr, &m_textureSampler) != VK_SUCCESS)
		Debug::sendError("Sampler creation");
}

Wolf::Sampler::~Sampler()
{
	vkDestroySampler(m_device, m_textureSampler, nullptr);
}