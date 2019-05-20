#include "VVulkan.h"

void VVulkan::prepare()
{
	VulkanExampleBase::prepare();
	prepareSamplers();
	
	prepared = true;
}

void VVulkan::prepareSamplers()
{
	// Create samplers
	VkSamplerCreateInfo sampler = vks::initializers::samplerCreateInfo();
	sampler.magFilter = VK_FILTER_NEAREST;
	sampler.minFilter = VK_FILTER_NEAREST;
	sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.mipLodBias = 0.0f;
	sampler.compareOp = VK_COMPARE_OP_NEVER;
	sampler.minLod = 0.0f;
	sampler.maxLod = 0.0f;
	sampler.maxAnisotropy = 1.0;
	sampler.anisotropyEnable = VK_FALSE;
	sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	VK_CHECK_RESULT(vkCreateSampler(device, &sampler, nullptr, &nearest_sampler));

	sampler.magFilter = VK_FILTER_LINEAR;
	sampler.minFilter = VK_FILTER_LINEAR;
	sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.mipLodBias = 0.0f;
	sampler.compareOp = VK_COMPARE_OP_NEVER;
	sampler.minLod = 0.0f;
	sampler.maxLod = 0.0f;
	sampler.maxAnisotropy = 1.0;
	sampler.anisotropyEnable = VK_FALSE;
	sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	VK_CHECK_RESULT(vkCreateSampler(device, &sampler, nullptr, &linear_sampler));
}

std::shared_ptr<VTexture> VVulkan::GenTexture2D(VkFormat format, VkFilter filter, uint32_t w, uint32_t h, VkImageUsageFlags usage)
{
	std::shared_ptr<VTexture> ret = std::make_shared<VTexture>(VTexture());

	ret->width = w;
	ret->height = h;
	ret->depth = 1;
	ret->mipLevels = 1;
	ret->format = format;
	ret->image = VK_NULL_HANDLE;
	ret->deviceMemory = VK_NULL_HANDLE;
	ret->sampler = VK_NULL_HANDLE;
	ret->view = VK_NULL_HANDLE;
	ret->device = device;

	// Format support check
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(physicalDevice, ret->format, &formatProperties);
	// Check if format supports transfer
	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_TRANSFER_DST_BIT))
	{
		std::cout << "Error: Device does not support flag TRANSFER_DST for selected texture format!" << std::endl;
		return ret;
	}
	// Check if GPU supports requested 3D texture dimensions
	uint32_t maxImageDimension2D(vulkanDevice->properties.limits.maxImageDimension2D);
	if (width > maxImageDimension2D || height > maxImageDimension2D)
	{
		std::cout << "Error: Requested texture dimensions is greater than supported 2D texture dimension!" << std::endl;
		return ret;
	}

	// Create optimal tiled target image
	VkImageCreateInfo imageCreateInfo = vks::initializers::imageCreateInfo();
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = ret->format;
	imageCreateInfo.mipLevels = ret->mipLevels;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.extent.width = ret->width;
	imageCreateInfo.extent.height = ret->height;
	imageCreateInfo.extent.depth = ret->depth;
	// Set initial layout of the image to undefined
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.usage = usage;
	VK_CHECK_RESULT(vkCreateImage(device, &imageCreateInfo, nullptr, &ret->image));

	// Device local memory to back up image
	VkMemoryAllocateInfo memAllocInfo = vks::initializers::memoryAllocateInfo();
	VkMemoryRequirements memReqs = {};
	vkGetImageMemoryRequirements(device, ret->image, &memReqs);
	memAllocInfo.allocationSize = memReqs.size;
	memAllocInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK_RESULT(vkAllocateMemory(device, &memAllocInfo, nullptr, &ret->deviceMemory));
	VK_CHECK_RESULT(vkBindImageMemory(device, ret->image, ret->deviceMemory, 0));

	// Create sampler
	VkSamplerCreateInfo sampler = vks::initializers::samplerCreateInfo();
	sampler.magFilter = filter;
	sampler.minFilter = filter;
	sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.mipLodBias = 0.0f;
	sampler.compareOp = VK_COMPARE_OP_NEVER;
	sampler.minLod = 0.0f;
	sampler.maxLod = 0.0f;
	sampler.maxAnisotropy = 1.0;
	sampler.anisotropyEnable = VK_FALSE;
	sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	VK_CHECK_RESULT(vkCreateSampler(device, &sampler, nullptr, &ret->sampler));

	// Create image view
	VkImageViewCreateInfo view = vks::initializers::imageViewCreateInfo();
	view.image = ret->image;
	view.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view.format = ret->format;
	if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
		view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	else
	{
		view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	view.subresourceRange.baseMipLevel = 0;
	view.subresourceRange.baseArrayLayer = 0;
	view.subresourceRange.layerCount = 1;
	view.subresourceRange.levelCount = 1;
	VK_CHECK_RESULT(vkCreateImageView(device, &view, nullptr, &ret->view));

	// Fill image descriptor image info to be used descriptor set setup
	ret->descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	ret->descriptor.imageView = ret->view;
	ret->descriptor.sampler = ret->sampler;

	return ret;
}

std::shared_ptr<VTexture> VVulkan::GenTexture3D(VkFormat format, VkFilter filter, uint32_t w, uint32_t h, uint32_t d)
{
	std::shared_ptr<VTexture> ret = std::make_shared<VTexture>(VTexture());

	ret->width = w;
	ret->height = h;
	ret->depth = 1;
	ret->mipLevels = 1;
	ret->format = format;
	ret->image = VK_NULL_HANDLE;
	ret->deviceMemory = VK_NULL_HANDLE;
	ret->sampler = VK_NULL_HANDLE;
	ret->view = VK_NULL_HANDLE;
	ret->device = device;

	// Format support check
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(physicalDevice, ret->format, &formatProperties);
	// Check if format supports transfer
	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_TRANSFER_DST_BIT))
	{
		std::cout << "Error: Device does not support flag TRANSFER_DST for selected texture format!" << std::endl;
		return ret;
	}
	// Check if GPU supports requested 3D texture dimensions
	uint32_t maxImageDimension3D(vulkanDevice->properties.limits.maxImageDimension3D);
	if (width > maxImageDimension3D || height > maxImageDimension3D)
	{
		std::cout << "Error: Requested texture dimensions is greater than supported 3D texture dimension!" << std::endl;
		return ret;
	}

	// Create optimal tiled target image
	VkImageCreateInfo imageCreateInfo = vks::initializers::imageCreateInfo();
	imageCreateInfo.imageType = VK_IMAGE_TYPE_3D;
	imageCreateInfo.format = ret->format;
	imageCreateInfo.mipLevels = ret->mipLevels;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.extent.width = ret->width;
	imageCreateInfo.extent.height = ret->height;
	imageCreateInfo.extent.depth = ret->depth;
	// Set initial layout of the image to undefined
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	VK_CHECK_RESULT(vkCreateImage(device, &imageCreateInfo, nullptr, &ret->image));

	// Device local memory to back up image
	VkMemoryAllocateInfo memAllocInfo = vks::initializers::memoryAllocateInfo();
	VkMemoryRequirements memReqs = {};
	vkGetImageMemoryRequirements(device, ret->image, &memReqs);
	memAllocInfo.allocationSize = memReqs.size;
	memAllocInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK_RESULT(vkAllocateMemory(device, &memAllocInfo, nullptr, &ret->deviceMemory));
	VK_CHECK_RESULT(vkBindImageMemory(device, ret->image, ret->deviceMemory, 0));

	// Set sampler
	if (filter = VK_FILTER_LINEAR)
		ret->sampler = linear_sampler;
	else
		ret->sampler = nearest_sampler;

	// Create image view
	VkImageViewCreateInfo view = vks::initializers::imageViewCreateInfo();
	view.image = ret->image;
	view.viewType = VK_IMAGE_VIEW_TYPE_3D;
	view.format = ret->format;
	view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	view.subresourceRange.baseMipLevel = 0;
	view.subresourceRange.baseArrayLayer = 0;
	view.subresourceRange.layerCount = 1;
	view.subresourceRange.levelCount = 1;
	VK_CHECK_RESULT(vkCreateImageView(device, &view, nullptr, &ret->view));

	// Fill image descriptor image info to be used descriptor set setup
	ret->descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	ret->descriptor.imageView = ret->view;
	ret->descriptor.sampler = ret->sampler;

	return ret;
}

void VVulkan::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memRequirements.memoryTypeBits, properties);

	VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory));
	VK_CHECK_RESULT(vkBindBufferMemory(device, buffer, bufferMemory, 0));
}

void VVulkan::checkStagingBuffer(VkDeviceSize size)
{
	if (size > staging_buf.size)
	{
		staging_buf.unmap();
		staging_buf.destroy();
	}
	if (staging_buf.buffer == VK_NULL_HANDLE)
	{
		vulkanDevice->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
								   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
								   &staging_buf, size);
		// Map persistent
		VK_CHECK_RESULT(staging_buf.map());
	}
}


bool VVulkan::UploadTexture3D(VTexture tex, void *data, VkOffset3D offset, VkExtent3D extent, uint32_t xpitch, uint32_t ypitch, uint32_t bytes)
{
	const VkDeviceSize texMemSize = (VkDeviceSize)tex.width * (VkDeviceSize)tex.height * (VkDeviceSize)tex.depth * (VkDeviceSize)tex.bytes;

	checkStagingBuffer(texMemSize);

	// Copy texture data into staging buffer
	uint64_t poffset = offset.z*ypitch*xpitch + offset.y*xpitch + offset.x*bytes;
	uint64_t dst_xpitch = (VkDeviceSize)texture.width * (VkDeviceSize)texture.bytes;
	unsigned char* dstp = (unsigned char*)staging_buf.mapped;
	unsigned char* tp = (unsigned char *)data + poffset;
	unsigned char* tp2; 
	for (uint32_t z = 0; z < extent.depth; z++)
	{
		tp2 = tp;
		for (uint32_t y = 0; y < extent.height; y++)
		{
			memcpy(dstp, tp2, dst_xpitch);
			dstp += dst_xpitch;
			tp2 += xpitch;
		}
		tp += ypitch*xpitch;
	}

	CopyDataStagingBuf2Tex3D(tex);
}

bool VVulkan::UploadTexture3D(VTexture tex, void *data)
{
	const VkDeviceSize texMemSize = (VkDeviceSize)tex.width * (VkDeviceSize)tex.height * (VkDeviceSize)tex.depth * (VkDeviceSize)tex.bytes;

	checkStagingBuffer(texMemSize);

	// Copy texture data into staging buffer
	memcpy(staging_buf.mapped, data, texMemSize);

	CopyDataStagingBuf2Tex3D(tex);
}

void VVulkan::CopyDataStagingBuf2Tex3D(VTexture tex)
{
	VkCommandBuffer copyCmd = VulkanExampleBase::createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	// The sub resource range describes the regions of the image we will be transitioned
	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.baseArrayLayer = 0;
	subresourceRange.levelCount = 1;
	subresourceRange.layerCount = 1;

	// Optimal image will be used as destination for the copy, so we must transfer from our
	// initial undefined image layout to the transfer destination layout
	vks::tools::setImageLayout(
		copyCmd,
		tex.image,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		subresourceRange);

	// Setup buffer copy regions
	VkBufferImageCopy bufferCopyRegion{};
	bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	bufferCopyRegion.imageSubresource.mipLevel = 0;
	bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
	bufferCopyRegion.imageSubresource.layerCount = 1;
	bufferCopyRegion.imageExtent.width = tex.width;
	bufferCopyRegion.imageExtent.height = tex.height;
	bufferCopyRegion.imageExtent.depth = tex.depth;

	vkCmdCopyBufferToImage(
		copyCmd,
		staging_buf.buffer,
		tex.image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&bufferCopyRegion);

	// Change texture image layout to shader read after all mip levels have been copied
	tex.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	vks::tools::setImageLayout(
		copyCmd,
		tex.image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		tex.imageLayout,
		subresourceRange);

	VulkanExampleBase::flushCommandBuffer(copyCmd, queue, true);
}