#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.h>
#include "Vulkan/vulkanexamplebase.h"
#include "Vulkan/VulkanTexture.hpp"
#include "Vulkan/VulkanModel.hpp"
#include "Vulkan/VulkanBuffer.hpp"

#include <memory>

#define ENABLE_VALIDATION true

// Offscreen frame buffer properties
#define FB_DIM 256
#define FB_COLOR_FORMAT VK_FORMAT_R8G8B8A8_UNORM

#ifndef _VVULKAN_H_
#define _VVULKAN_H_

class VTexture {
public:
	VkSampler sampler;
	VkImage image;
	VkImageLayout imageLayout;
	VkDeviceMemory deviceMemory;
	VkImageView view;
	VkDescriptorImageInfo descriptor;
	VkFormat format;
	VkDevice device;
	uint32_t w, h, d, bytes;
	uint32_t mipLevels;

	VTexture()
	{
		sampler = VK_NULL_HANDLE;
		image = VK_NULL_HANDLE;
		deviceMemory = VK_NULL_HANDLE;
		view = VK_NULL_HANDLE;
		device = VK_NULL_HANDLE;
	}

	~VTexture()
	{
		if (view != VK_NULL_HANDLE)
			vkDestroyImageView(device, view, nullptr);
		if (image != VK_NULL_HANDLE)
			vkDestroyImage(device, image, nullptr);
		//if (sampler != VK_NULL_HANDLE)
			//vkDestroySampler(device, sampler, nullptr);
		if (deviceMemory != VK_NULL_HANDLE)
			vkFreeMemory(device, deviceMemory, nullptr);
	}
};

class VFrameBuffer {
public:
	VkFramebuffer framebuffer;
	VkDevice device;
	int32_t w, h;
	std::shared_ptr<VTexture> color, depth;

	VFrameBuffer()
	{
		framebuffer = VK_NULL_HANDLE;
		device = VK_NULL_HANDLE;
	}

	~VFrameBuffer()
	{
		if (framebuffer != VK_NULL_HANDLE)
			vkDestroyFramebuffer(device, framebuffer, nullptr);
	}
};

class VVulkan : public VulkanExampleBase
{
public:
	struct Vertex {
		float pos[3];
		float uv[3];
	};

	vks::Buffer staging_buf;

	VkSampler linear_sampler;
	VkSampler nearest_sampler;

	VVulkan() : VulkanExampleBase(ENABLE_VALIDATION)
	{
		linear_sampler = VK_NULL_HANDLE;
		nearest_sampler = VK_NULL_HANDLE;
	}

	~VVulkan()
	{
		if (linear_sampler != VK_NULL_HANDLE)
			vkDestroySampler(device, linear_sampler, nullptr);
		if (nearest_sampler != VK_NULL_HANDLE)
			vkDestroySampler(device, nearest_sampler, nullptr);
	}

	void prepareSamplers();
	void prepare();

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

	void checkStagingBuffer(VkDeviceSize size);

	std::shared_ptr<VTexture> GenTexture2D(VkFormat format, VkFilter filter, uint32_t w, uint32_t h, VkImageUsageFlags usage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT);
	std::shared_ptr<VTexture> GenTexture3D(VkFormat format, VkFilter filter, uint32_t w, uint32_t h, uint32_t d);
	bool UploadTexture3D(const std::shared_ptr<const VTexture> &tex, void *data, VkOffset3D offset, uint32_t ypitch, uint32_t zpitch);
	bool UploadTexture(const std::shared_ptr<const VTexture> &tex, void *data);
	void CopyDataStagingBuf2Tex(const std::shared_ptr<const VTexture> &tex);
};

#endif