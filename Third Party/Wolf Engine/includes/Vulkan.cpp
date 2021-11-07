#include "Vulkan.h"
#include "Debug.h"

Wolf::Vulkan::Vulkan(GLFWwindow* glfwWindowPointer, bool useOVR)
{
	if (useOVR)
	{
		ovrInitParams initParams = { ovrInit_RequestVersion | ovrInit_FocusAware, OVR_MINOR_VERSION, NULL, 0, 0 };
		ovrResult result = ovr_Initialize(&initParams);
		if (!OVR_SUCCESS(result))
		{
			Debug::sendError("Failed to initialize OVR");
			return;
		}

		result = ovr_Create(&m_session, &m_luid);
		if (!OVR_SUCCESS(result))
		{
			Debug::sendError("Failed to create OVR");
			return;
		}
	}

#ifndef NDEBUG
	m_validationLayers = { "VK_LAYER_KHRONOS_validation" };
#endif
	createInstance();
#ifndef NDEBUG
	setupDebugMessenger();
#endif
	if (glfwCreateWindowSurface(m_instance, glfwWindowPointer, nullptr, &m_surface) != VK_SUCCESS)
		throw std::runtime_error("Error : window surface creation");

	m_deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME, "VK_KHR_external_memory_win32", VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME,
		"VK_KHR_external_semaphore_win32", VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME, "VK_KHR_external_fence", "VK_KHR_external_fence_win32" };
	m_raytracingDeviceExtensions = { VK_NV_RAY_TRACING_EXTENSION_NAME };

	pickPhysicalDevice();
	createDevice();
}

Wolf::Vulkan::~Vulkan()
{
	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	//vkDestroyDebugReportCallbackEXT(m_instance, m_debugCallback, nullptr);
	//vkDestroyInstance(m_instance, nullptr);
	//vkDestroyDevice(m_device, nullptr);
}

void Wolf::Vulkan::cleanup()
{
	this->~Vulkan();
}

void Wolf::Vulkan::createInstance()
{
	char extensionNames[4096];
	uint32_t extensionNamesSize = sizeof(extensionNames);
	auto ret = ovr_GetInstanceExtensionsVk(m_luid, extensionNames, &extensionNamesSize);
	ovrErrorInfo errorInfo;
	ovr_GetLastErrorInfo(&errorInfo);
	
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "App Name";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "Wolf Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	std::vector<const char*> extensions = getRequiredExtensions();
	extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	extensions.push_back(VK_KHR_EXTERNAL_FENCE_CAPABILITIES_EXTENSION_NAME);
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
#ifndef NDEBUG
	createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
	createInfo.ppEnabledLayerNames = m_validationLayers.data();

	populateDebugMessengerCreateInfo(debugCreateInfo);
	createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
#else
	createInfo.enabledLayerCount = 0;
	createInfo.pNext = nullptr;
#endif

	if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS)
		throw std::runtime_error("Error: instance creation");
}

void Wolf::Vulkan::setupDebugMessenger()
{
	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	populateDebugMessengerCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to set up debug messenger!");
	}
}

void Wolf::Vulkan::pickPhysicalDevice()
{
	if(m_session)
	{
		ovrResult result = ovr_GetSessionPhysicalDeviceVk(m_session, m_luid, m_instance, &m_physicalDevice);
		if (!OVR_SUCCESS(result))
		{
			Debug::sendError("Failed to get physical device from OVR");
			return;
		}
		return;
	}
	
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

	if (deviceCount == 0)
		throw std::runtime_error("Error : No GPU with Vulkan support found !");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

	for (auto& device : devices)
	{
		if (isDeviceSuitable(device, m_surface, m_deviceExtensions, m_hardwareCapabilities))
		{
			m_raytracingAvailable = isDeviceSuitable(device, m_surface, m_raytracingDeviceExtensions, m_hardwareCapabilities);
			m_hardwareCapabilities.rayTracingAvailable = m_raytracingAvailable;

			if (m_raytracingAvailable)
				for (int i(0); i < m_raytracingDeviceExtensions.size(); ++i)
					m_deviceExtensions.push_back(m_raytracingDeviceExtensions[i]);

			m_physicalDevice = device;
			m_maxMsaaSamples = getMaxUsableSampleCount(m_physicalDevice);

			PFN_vkGetPhysicalDeviceProperties2KHR vkGetPhysicalDeviceProperties2KHR = 
				reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2KHR>(vkGetInstanceProcAddr(m_instance, "vkGetPhysicalDeviceProperties2KHR"));
			VkPhysicalDeviceProperties2KHR deviceProps2{};
			m_conservativeRasterProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONSERVATIVE_RASTERIZATION_PROPERTIES_EXT;
			deviceProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
			deviceProps2.pNext = &m_conservativeRasterProps;
			vkGetPhysicalDeviceProperties2KHR(m_physicalDevice, &deviceProps2);
			
			if (m_raytracingAvailable)
				m_raytracingProperties = getPhysicalDeviceRayTracingProperties(m_physicalDevice);
			break;
		}
	}

	if (m_physicalDevice == VK_NULL_HANDLE)
		throw std::runtime_error("Error : No suitable GPU found :(");
}

void Wolf::Vulkan::createDevice()
{
	QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice, m_surface);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily };

	float queuePriority = 1.0f;
	for (int queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	/*VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceFeatures.sampleRateShading = VK_TRUE;
	deviceFeatures.shaderSampledImageArrayDynamicIndexing = VK_TRUE;*/

	VkPhysicalDeviceDescriptorIndexingFeaturesEXT descIndexFeatures = {};
	descIndexFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;

	VkPhysicalDeviceFeatures2 supportedFeatures = {};
	supportedFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	supportedFeatures.pNext = &descIndexFeatures;
	supportedFeatures.features.shaderStorageImageMultisample = VK_TRUE;
	vkGetPhysicalDeviceFeatures2(m_physicalDevice, &supportedFeatures);

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();

	createInfo.pEnabledFeatures = &(supportedFeatures.features);
	createInfo.pNext = &descIndexFeatures;

	createInfo.enabledExtensionCount = static_cast<uint32_t>(m_deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = m_deviceExtensions.data();

#ifndef NDEBUG
	createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
	createInfo.ppEnabledLayerNames = m_validationLayers.data();
#else
	createInfo.enabledLayerCount = 0;
#endif

	if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS)
		throw std::runtime_error("Error : create device");

	vkGetDeviceQueue(m_device, indices.graphicsFamily, 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_device, indices.presentFamily, 0, &m_presentQueue);
	vkGetDeviceQueue(m_device, indices.computeFamily, 0, &m_computeQueue);

	m_mutexGraphicsQueue = new std::mutex();
// 	if (indices.graphicsFamily != indices.presentFamily)
// 		m_mutexPresentQueue = new std::mutex();
// 	else
		m_mutexPresentQueue = m_mutexGraphicsQueue;
// 	if (indices.graphicsFamily != indices.computeFamily)
// 		m_mutexComputeQueue = new std::mutex();
// 	else
		m_mutexComputeQueue = m_mutexGraphicsQueue;
}

void Wolf::Vulkan::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
}
