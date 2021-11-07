#include "WolfEngine.h"

#include <utility>

Wolf::WolfInstance::WolfInstance(WolfInstanceCreateInfo createInfo)
{
	if (!createInfo.debugCallback)
		return;
	Debug::setCallback(createInfo.debugCallback);
	
	if (createInfo.windowHeight <= 0 || createInfo.windowHeight > MAX_HEIGHT)
		Debug::sendError("Window height is invalid or exceed max height. Height sent : " + std::to_string(createInfo.windowHeight) + ", maximum height = " + std::to_string(MAX_HEIGHT));
	if (createInfo.windowWidth <= 0 || createInfo.windowWidth > MAX_WIDTH)
		Debug::sendError("Window width is invalid or exceed max width. Width sent : " + std::to_string(createInfo.windowWidth) + ", maximum width = " + std::to_string(MAX_WIDTH));
	
	m_window = std::make_unique<Window>(createInfo.applicationName, createInfo.windowWidth, createInfo.windowHeight, this, windowResizeCallback);
	m_vulkan = std::make_unique<Vulkan>(m_window->getWindow(), createInfo.useOVR);
	m_swapChain = std::make_unique<SwapChain>(m_vulkan->getDevice(), m_vulkan->getPhysicalDevice(), m_vulkan->getSurface(), m_window->getWindow());

	m_graphicsCommandPool.initializeForGraphicsQueue(m_vulkan->getDevice(), m_vulkan->getPhysicalDevice(), m_vulkan->getSurface());
	m_computeCommandPool.initializeForComputeQueue(m_vulkan->getDevice(), m_vulkan->getPhysicalDevice(), m_vulkan->getSurface());

	m_useOVR = createInfo.useOVR;
	if (createInfo.useOVR)
	{
		m_ovr = std::make_unique<OVR>(m_vulkan->getDevice(), m_graphicsCommandPool.getCommandPool(), m_vulkan->getGraphicsQueue(), m_vulkan->getOVRSession(), m_vulkan->getGraphicsLuid());
	}
}

Wolf::Scene* Wolf::WolfInstance::createScene(Scene::SceneCreateInfo createInfo)
{
	if(m_useOVR)
		m_scenes.push_back(std::make_unique<Scene>(createInfo, m_vulkan->getDevice(), m_vulkan->getPhysicalDevice(), m_ovr->getImages(), m_swapChain->getImages(), m_graphicsCommandPool.getCommandPool(), m_computeCommandPool.getCommandPool()));
	else
		m_scenes.push_back(std::make_unique<Scene>(createInfo, m_vulkan->getDevice(), m_vulkan->getPhysicalDevice(), m_swapChain->getImages(), m_graphicsCommandPool.getCommandPool(), m_computeCommandPool.getCommandPool()));
	return m_scenes[m_scenes.size() - 1].get();
}

Wolf::UniformBuffer* Wolf::WolfInstance::createUniformBufferObject(void* data, VkDeviceSize size)
{
	m_uniformBufferObjects.push_back(std::make_unique<UniformBuffer>(m_vulkan->getDevice(), m_vulkan->getPhysicalDevice(), data, size));

	return m_uniformBufferObjects[m_uniformBufferObjects.size() - 1].get();
}

Wolf::Buffer* Wolf::WolfInstance::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryPropertyFlags)
{
	m_buffers.push_back(std::make_unique<Buffer>(m_vulkan->getDevice(), m_vulkan->getPhysicalDevice(), m_graphicsCommandPool.getCommandPool(), m_vulkan->getGraphicsQueue(), size, usage, memoryPropertyFlags));

	return m_buffers.back().get();
}

Wolf::Image* Wolf::WolfInstance::createImageFromFile(std::string filename)
{
	m_images.push_back(std::make_unique<Image>(m_vulkan->getDevice(), m_vulkan->getPhysicalDevice(), m_graphicsCommandPool.getCommandPool(), m_vulkan->getGraphicsQueue(), 
		filename));

	return m_images.back().get();
}

Wolf::Image* Wolf::WolfInstance::createImage(Image::CreateImageInfo createImageInfo)
{
	m_images.push_back(std::make_unique<Image>(m_vulkan->getDevice(), m_vulkan->getPhysicalDevice(), m_graphicsCommandPool.getCommandPool(), m_vulkan->getGraphicsQueue(), createImageInfo));

	return m_images.back().get();
}

Wolf::Sampler* Wolf::WolfInstance::createSampler(VkSamplerAddressMode addressMode, float mipLevels, VkFilter filter, float maxAnisotropy, float minLod, float mipLodBias)
{
	m_samplers.push_back(std::make_unique<Sampler>(m_vulkan->getDevice(), addressMode, mipLevels, filter, maxAnisotropy, minLod, mipLodBias));

	return m_samplers.back().get();
}

Wolf::Font* Wolf::WolfInstance::createFont(int ySize, std::string path)
{
	m_fonts.push_back(std::make_unique<Font>(m_vulkan->getDevice(), m_vulkan->getPhysicalDevice(), m_graphicsCommandPool.getCommandPool(), m_vulkan->getGraphicsQueue(),
		ySize, path));

	return m_fonts[m_fonts.size() - 1].get();
}

Wolf::Text* Wolf::WolfInstance::createText()
{
	m_texts.push_back(std::make_unique<Text>(m_vulkan->getDevice(), m_vulkan->getPhysicalDevice(), m_graphicsCommandPool.getCommandPool(), m_vulkan->getGraphicsQueue()));

	return m_texts[m_texts.size() - 1].get();
}

Wolf::AccelerationStructure* Wolf::WolfInstance::createAccelerationStructure(std::vector<BottomLevelAccelerationStructure::GeometryInfo> geometryInfos,
	VkBuildAccelerationStructureFlagsNV buildFlags)
{
	m_accelerationStructures.push_back(std::make_unique<AccelerationStructure>(m_vulkan->getDevice(), m_vulkan->getPhysicalDevice(), m_graphicsCommandPool.getCommandPool(),
		m_vulkan->getGraphicsQueue(), std::move(geometryInfos), buildFlags));

	return m_accelerationStructures.back().get();
}

void Wolf::WolfInstance::updateOVR()
{
	m_ovr->update();
}

void Wolf::WolfInstance::frame(Scene* scene, std::vector<int> commandBufferIDs, std::vector<std::pair<int, int>> commandBufferSynchronisation)
{
	if(m_needResize)
	{
		m_swapChain->recreate(m_vulkan->getSurface(), m_window->getWindow());
		for (auto& scene : m_scenes)
			scene->resize(m_swapChain->getImages());

		m_needResize = false;
	}
	
	glfwPollEvents();

	if(m_useOVR)
	{
		const uint32_t swapChainImageIndex = m_ovr->getCurrentImage(m_vulkan->getDevice(), m_vulkan->getGraphicsQueue().queue);
		scene->frame(m_vulkan->getGraphicsQueue(), m_vulkan->getComputeQueue(), swapChainImageIndex, m_swapChain->getImageAvailableSemaphore(),
		             std::move(commandBufferIDs), std::move(commandBufferSynchronisation));
		m_ovr->present(swapChainImageIndex);

		const uint32_t windowSwapChainImageIndex = m_swapChain->getCurrentImage(m_vulkan->getDevice());
		m_swapChain->present(m_vulkan->getPresentQueue(), scene->getSwapChainSemaphore(), windowSwapChainImageIndex);
	}
	
	else
	{
		const uint32_t swapChainImageIndex = m_swapChain->getCurrentImage(m_vulkan->getDevice());
		scene->frame(m_vulkan->getGraphicsQueue(), m_vulkan->getComputeQueue(), swapChainImageIndex, m_swapChain->getImageAvailableSemaphore(), std::move(commandBufferIDs),
			std::move(commandBufferSynchronisation));
		m_swapChain->present(m_vulkan->getPresentQueue(), scene->getSwapChainSemaphore(), swapChainImageIndex);
	}
}

void Wolf::WolfInstance::submitCommandBuffers(Scene* scene, std::vector<int> commandBufferIDs, std::vector<std::pair<int, int>> commandBufferSynchronisation)
{
	scene->frame(m_vulkan->getGraphicsQueue(), m_vulkan->getComputeQueue(), -1, m_swapChain->getImageAvailableSemaphore(),
		std::move(commandBufferIDs), std::move(commandBufferSynchronisation), false);
}

bool Wolf::WolfInstance::windowShouldClose()
{
	return glfwWindowShouldClose(m_window->getWindow());
}

void Wolf::WolfInstance::waitIdle()
{
	vkDeviceWaitIdle(m_vulkan->getDevice());
}

void Wolf::WolfInstance::resize(int width, int height)
{
	m_needResize = true;
}

VkExtent2D Wolf::WolfInstance::getWindowSize()
{
	if (!m_ovr)
		return { m_swapChain->getImages()[0]->getExtent().width, m_swapChain->getImages()[0]->getExtent().height };
	else
		return { m_ovr->getImages()[0]->getExtent().width, m_ovr->getImages()[0]->getExtent().height };
}
