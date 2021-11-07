#include "Semaphore.h"

Wolf::Semaphore::~Semaphore()
{

}

bool Wolf::Semaphore::initialize(VkDevice device)
{
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_semaphore) != VK_SUCCESS)
        throw std::runtime_error("Error : create semaphore");

    return true;
}

void Wolf::Semaphore::cleanup(VkDevice device)
{
    vkDestroySemaphore(device, m_semaphore, nullptr);
}
