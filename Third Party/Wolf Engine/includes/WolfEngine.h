#pragma once

#include <string>

#include "Vulkan.h"
#include "Window.h"
#include "Debug.h"
#include "SwapChain.h"
#include "Scene.h"
#include "Model.h"
#include "Font.h"
#include "Text.h"
#include "OVR.h"
#include "AccelerationStructure.h"
#include "Buffer.h"

#include "Model2D.h"
#include "Model2DTextured.h"
#include "Model3D.h"
#include "ModelCustom.h"

namespace Wolf
{	
	struct WolfInstanceCreateInfo
	{
		uint32_t majorVersion;
		uint32_t minorVersion;
		std::string applicationName;

		uint32_t windowWidth = 0;
		uint32_t windowHeight = 0;

		bool useOVR = false;

		std::function<void(Debug::Severity, std::string)> debugCallback;
	};
	
	class WolfInstance
	{
	public:
		WolfInstance(WolfInstanceCreateInfo createInfo);
		~WolfInstance() = default;

		Scene* createScene(Scene::SceneCreateInfo createInfo);
		template<typename T = float>
		Model* createModel(Model::ModelCreateInfo createInfo);
		template<typename T>
		Instance<T>* createInstanceBuffer();
		UniformBuffer* createUniformBufferObject(void* data, VkDeviceSize size);
		Buffer* createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryPropertyFlags);

		// Image creation
		Image* createImageFromFile(std::string filename);
		Image* createImage(Image::CreateImageInfo createImageInfo);

		// Sampler creation
		Sampler* createSampler(VkSamplerAddressMode addressMode, float mipLevels, VkFilter filter, float maxAnisotropy = 16.0f, float minLod = 0.0f, float mipLodBias = 0.0f);
		
		Font* createFont(int ySize, std::string path);
		Text* createText();

		AccelerationStructure* createAccelerationStructure(std::vector<BottomLevelAccelerationStructure::GeometryInfo> geometryInfos, 
			VkBuildAccelerationStructureFlagsNV buildFlags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);

		void updateOVR();
		void frame(Scene* scene, std::vector<int> commandBufferIDs, std::vector<std::pair<int, int>> commandBufferSynchronisation);
		void submitCommandBuffers(Scene* scene, std::vector<int> commandBufferIDs, std::vector<std::pair<int, int>> commandBufferSynchronisation);
		bool windowShouldClose();

		void waitIdle();

		void resize(int width, int height);

		// Getters
	public:
		GLFWwindow* getWindowPtr() { return m_window->getWindow(); }
		ovrSession getOVRSession() { return m_vulkan->getOVRSession(); }
		std::array < glm::mat4, 2>& getVRProjMatrices() { return m_ovr->getProjMatrices(); }
		std::array < glm::mat4, 2>& getVRViewMatrices() { return m_ovr->getViewMatrices(); }
		std::array < glm::vec3, 2>& getVREyePositions() { return m_ovr->getEyePostions(); }
		std::array < glm::vec3, 2>& getVREyeDirections() { return m_ovr->getEyeDirections(); }
		void setVRPlayerPosition(glm::vec3 playerPosition) { m_ovr->setPlayerPos(playerPosition); }
		VkExtent2D getWindowSize();

	private:
		static void windowResizeCallback(void* systemManagerInstance, int width, int height)
		{
			reinterpret_cast<WolfInstance*>(systemManagerInstance)->resize(width, height);
		}

	private:
		std::unique_ptr<Vulkan> m_vulkan;
		std::unique_ptr<Window> m_window;
		std::unique_ptr<SwapChain> m_swapChain;
		std::unique_ptr<OVR> m_ovr;
		bool m_useOVR = false;

		CommandPool m_graphicsCommandPool;
		CommandPool m_computeCommandPool;

		std::vector<std::unique_ptr<Scene>> m_scenes;
		std::vector<std::unique_ptr<Model>> m_models;
		std::vector<std::unique_ptr<InstanceParent>> m_instances;
		std::vector<std::unique_ptr<UniformBuffer>> m_uniformBufferObjects;
		std::vector<std::unique_ptr<Image>> m_images;
		std::vector<std::unique_ptr<Sampler>> m_samplers;
		std::vector<std::unique_ptr<Font>> m_fonts;
		std::vector<std::unique_ptr<Text>> m_texts;
		std::vector<std::unique_ptr<AccelerationStructure>> m_accelerationStructures;
		std::vector<std::unique_ptr<Buffer>> m_buffers;

		bool m_needResize = false;

	private:
		uint32_t MAX_HEIGHT = 2160;
		uint32_t MAX_WIDTH = 3840;
	};

	template <typename T>
	Instance<T>* WolfInstance::createInstanceBuffer()
	{
		Instance<T> *instance = new Instance<T>(m_vulkan->getDevice(), m_vulkan->getPhysicalDevice(), m_graphicsCommandPool.getCommandPool(), m_vulkan->getGraphicsQueue());
		m_instances.push_back(std::unique_ptr<InstanceParent>(instance));
		return instance;
	}

	template<typename  T>
	Model* WolfInstance::createModel(Model::ModelCreateInfo createInfo)
	{
		switch (createInfo.inputVertexTemplate)
		{
		case InputVertexTemplate::POSITION_2D:
			m_models.push_back(std::unique_ptr<Model>(static_cast<Model*>(new Model2D(m_vulkan->getDevice(), m_vulkan->getPhysicalDevice(), m_graphicsCommandPool.getCommandPool(),
				m_vulkan->getGraphicsQueue(), createInfo.inputVertexTemplate))));
			break;
		case InputVertexTemplate::POSITION_TEXTURECOORD_2D:
			m_models.push_back(std::unique_ptr<Model>(static_cast<Model*>(new Model2DTextured(m_vulkan->getDevice(), m_vulkan->getPhysicalDevice(), m_graphicsCommandPool.getCommandPool(), m_vulkan->getGraphicsQueue(), createInfo.inputVertexTemplate))));
			break;
		case InputVertexTemplate::FULL_3D_MATERIAL:
			m_models.push_back(std::unique_ptr<Model>(static_cast<Model*>(new Model3D(m_vulkan->getDevice(), m_vulkan->getPhysicalDevice(), m_graphicsCommandPool.getCommandPool(), m_vulkan->getGraphicsQueue(), createInfo.inputVertexTemplate))));
			break;
		case InputVertexTemplate::NO:
			m_models.push_back(std::unique_ptr<Model>(static_cast<Model*>(new ModelCustom<T>(m_vulkan->getDevice(), m_vulkan->getPhysicalDevice(), m_graphicsCommandPool.getCommandPool(),
				m_vulkan->getGraphicsQueue(), createInfo.inputVertexTemplate))));
			break;
		}

		return m_models[m_models.size() - 1].get();
	}
}
