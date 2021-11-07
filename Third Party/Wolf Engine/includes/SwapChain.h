#pragma once

#include "VulkanElement.h"
#include "Image.h"
#include "Semaphore.h"

#include <iostream>

namespace Wolf
{
	class SwapChain : public VulkanElement
	{
	public:
		SwapChain(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, GLFWwindow* window);
		~SwapChain();

		uint32_t getCurrentImage(VkDevice device);
		void present(Queue presentQueue, VkSemaphore waitSemaphore, uint32_t imageIndex);
		void recreate(VkSurfaceKHR surface, GLFWwindow* window);

		void cleanup();

		// Getters
	public:
		void initialize(VkSurfaceKHR surface, GLFWwindow* window);
		
		std::vector<Image*> getImages();
		Semaphore* getImageAvailableSemaphore() { return &m_imageAvailableSemaphore; }
		bool getInvertColors() { return m_invertColors; }

	private:
		VkSwapchainKHR m_swapChain;
		std::vector<std::unique_ptr<Image>> m_images;
		bool m_invertColors = false;

		Semaphore m_imageAvailableSemaphore;

	private:
		static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);
	};
}
