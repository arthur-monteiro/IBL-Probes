#include "Attachment.h"

Wolf::Attachment::Attachment(VkExtent2D extent, VkFormat format, VkSampleCountFlagBits sampleCount, VkImageLayout finalLayout,
                             VkAttachmentStoreOp storeOperation, VkImageUsageFlags usageType, Image* image)
{
	this->extent = extent;
	this->format = format;
	this->sampleCount = sampleCount;
	this->finalLayout = finalLayout;
	this->storeOperation = storeOperation;
	this->usageType = usageType;
	this->image = image;
}
