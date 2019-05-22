#include "VVulkan.h"

void VVulkan::prepare()
{
	VulkanExampleBase::prepare();
	initSubDevices();
	
	prepared = true;
}

void VVulkan::initSubDevices()
{
}

void VVulkan::GenTextures2DAllDevice(std::vector<std::shared_ptr<vks::VTexture>> &result, 
									 VkFormat format, VkFilter filter, uint32_t w, uint32_t h, VkImageUsageFlags usage)
{
	if (!result.empty())
		result.clear();

	for (auto dev : devices)
		result.push_back(dev->GenTexture2D(format, filter, w, h, usage));
}

bool VVulkan::UploadTextures(const std::vector<std::shared_ptr<vks::VTexture>> &tex, void *data)
{
	for (auto t : tex)
	{
		if (t)
		{
			vks::VulkanDevice *device = t->device;
			if (device)
				device->UploadTexture(t, data);
		}
	}
}