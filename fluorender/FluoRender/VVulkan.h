#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.h>
#include "Vulkan/vulkanexamplebase.h"
#include "Vulkan/VulkanTexture.hpp"
#include "Vulkan/VulkanModel.hpp"
#include "Vulkan/VulkanBuffer.hpp"
#include "Vulkan/VulkanDevice.hpp"

#include <vector>
#include <memory>

#define ENABLE_VALIDATION true

#ifndef _VVULKAN_H_
#define _VVULKAN_H_

class VVulkan : public VulkanExampleBase
{
public:

	std::vector<vks::VulkanDevice*> devices;
	
	VVulkan() : VulkanExampleBase(ENABLE_VALIDATION)
	{
		//#0 is a primary device
		devices.push_back(vulkanDevice);
	}

	~VVulkan()
	{
		for (auto dev : devices)
			if (dev) delete dev;
	}

	void prepare();
	void initSubDevices();

	void GenTextures2DAllDevice(std::vector<std::shared_ptr<vks::VTexture>> &result,
								VkFormat format, VkFilter filter, uint32_t w, uint32_t h,
								VkImageUsageFlags usage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT);
	bool UploadTextures(const std::vector<std::shared_ptr<vks::VTexture>> &tex, void *data);
};

#endif