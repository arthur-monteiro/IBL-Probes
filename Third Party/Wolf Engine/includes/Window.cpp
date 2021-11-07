#include "Window.h"

Wolf::Window::Window(std::string appName, int width, int height, void* systemManagerInstance,
	std::function<void(void*, int, int)> resizeCallback)
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	m_window = glfwCreateWindow(width, height, appName.c_str(), nullptr, nullptr);

	glfwSetWindowUserPointer(m_window, this);
	glfwSetWindowSizeCallback(m_window, Window::onWindowResized);
	//glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

	m_systemManagerInstance = systemManagerInstance;
	m_resizeCallback = resizeCallback;
}

Wolf::Window::~Window()
{
	glfwDestroyWindow(m_window);

	glfwTerminate();
}

void Wolf::Window::cleanup()
{
	this->~Window();
}
