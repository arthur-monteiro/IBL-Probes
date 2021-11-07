#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <set>
#include <mutex>

#include "VulkanHelper.h"
#include <OVR_CAPI_Vk.h>

namespace Wolf
{
	class Vulkan
	{
	public:
		Vulkan(GLFWwindow* glfwWindowPointer, bool useOVR);
		~Vulkan();

		void cleanup();

		// Getters
	public:
		VkDevice getDevice() const { return m_device; }
		VkInstance getInstance() { return m_instance; }
		VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }
		VkSurfaceKHR getSurface() { return m_surface; }

		Queue getGraphicsQueue() { return { m_graphicsQueue, m_mutexGraphicsQueue }; }
		Queue getPresentQueue() { return { m_presentQueue, m_mutexPresentQueue }; }
		Queue getComputeQueue() { return { m_computeQueue, m_mutexComputeQueue }; }

		HardwareCapabilities getHardwareCapabilities() { return m_hardwareCapabilities; }

		ovrSession getOVRSession() { return m_session; }
		ovrGraphicsLuid getGraphicsLuid() { return m_luid; }

	private:
		/* Main Loading Functions */
		inline void createInstance();
		inline void setupDebugMessenger();
		inline void pickPhysicalDevice();
		inline void createDevice();

		void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	private:
		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, 
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
		{
			std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

			return VK_FALSE;
		}

		static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
			auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
			if (func != nullptr) 
			{
				return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
			}
			else 
			{
				return VK_ERROR_EXTENSION_NOT_PRESENT;
			}
		}
	private:
		/* Vulkan attributes */
		VkInstance m_instance;
		VkSurfaceKHR m_surface;
		VkPhysicalDevice m_physicalDevice;
		VkDevice m_device;

		/* Queues */
		VkQueue m_graphicsQueue;
		VkQueue m_presentQueue;
		VkQueue m_computeQueue;

		/* Mutex queues */
		std::mutex* m_mutexGraphicsQueue = nullptr;
		std::mutex* m_mutexPresentQueue = nullptr;
		std::mutex* m_mutexComputeQueue = nullptr;

		/* Extensions / Layers */
		std::vector<const char*> m_validationLayers = std::vector<const char*>();
		std::vector<const char*> m_deviceExtensions = std::vector<const char*>();
		VkDebugUtilsMessengerEXT m_debugMessenger;

		/* Ray Tracing Availability */
		bool m_raytracingAvailable = false;
		std::vector<const char*> m_raytracingDeviceExtensions = std::vector<const char*>();
		std::vector<const char*> m_OVRDeviceExtensions = std::vector<const char*>();
		VkPhysicalDeviceRayTracingPropertiesNV m_raytracingProperties = {};

		/* Properties */
		VkSampleCountFlagBits m_maxMsaaSamples = VK_SAMPLE_COUNT_1_BIT;
		HardwareCapabilities m_hardwareCapabilities;
		VkPhysicalDeviceConservativeRasterizationPropertiesEXT m_conservativeRasterProps{};

		/* VR */
		ovrSession                  m_session = nullptr;
		ovrGraphicsLuid             m_luid;
	};

}
