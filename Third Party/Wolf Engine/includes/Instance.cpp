#include "Instance.h"

Wolf::InstanceParent::~InstanceParent()
{
	vkDestroyBuffer(m_device, m_instanceBuffer, nullptr);
	vkFreeMemory(m_device, m_instanceBufferMemory, nullptr);
}
