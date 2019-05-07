#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.h>
#include "Vulkan/vulkanexamplebase.h"
#include "Vulkan/VulkanTexture.hpp"
#include "Vulkan/VulkanModel.hpp"
#include "Vulkan/VulkanBuffer.hpp"

#define ENABLE_VALIDATION true

// Offscreen frame buffer properties
#define FB_DIM 256
#define FB_COLOR_FORMAT VK_FORMAT_R8G8B8A8_UNORM

#ifndef _VVULKAN_H_
#define _VVULKAN_H_

class VVulkan : public VulkanExampleBase
{
public:
	struct Vertex {
		float pos[3];
		float uv[3];
	};

	struct Texture {
		VkSampler sampler = VK_NULL_HANDLE;
		VkImage image = VK_NULL_HANDLE;
		VkImageLayout imageLayout;
		VkDeviceMemory deviceMemory = VK_NULL_HANDLE;
		VkImageView view = VK_NULL_HANDLE;
		VkDescriptorImageInfo descriptor;
		VkFormat format;
		uint32_t width, height, depth;
		uint32_t mipLevels;
	} texture;

	struct {
		VkPipelineVertexInputStateCreateInfo inputState;
		std::vector<VkVertexInputBindingDescription> inputBinding;
		std::vector<VkVertexInputAttributeDescription> inputAttributes;
	} vertices;

	vks::Buffer vertexBuffer;
	vks::Buffer indexBuffer;
	uint32_t indexCount;

	vks::Buffer uniformBufferVS;

	struct UboVS {
		glm::mat4 projection;
		glm::mat4 model;
		glm::vec4 viewPos;
		float depth = 0.0f;
	} uboVS;

	struct {
		VkPipeline over;
		VkPipeline add;
	} pipelines;

	// Framebuffer for offscreen rendering
	struct FrameBufferAttachment {
		VkImage image;
		VkDeviceMemory mem;
		VkImageView view;
	};
	struct FrameBuffer {
		VkFramebuffer framebuffer;
		FrameBufferAttachment color, depth;
		VkDescriptorImageInfo descriptor;
	};
	struct OffscreenPass {
		int32_t width, height;
		VkRenderPass renderPass;
		VkSampler sampler;
		FrameBuffer framebuffer;
	} offscreenPass;

	VkPipelineLayout pipelineLayout;
	VkDescriptorSet descriptorSet;
	VkDescriptorSetLayout descriptorSetLayout;

	VVulkan() : VulkanExampleBase(ENABLE_VALIDATION)
	{

	}

	~VVulkan()
	{
		vkDestroyPipeline(device, pipelines.solid, nullptr);

		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		vertexBuffer.destroy();
		indexBuffer.destroy();
		uniformBufferVS.destroy();
	}

	void generateQuad();
	void setupVertexDescriptions();
	void setupDescriptorPool();
	void setupDescriptorSetLayout();
	void setupDescriptorSet();
	void preparePipelines();
	void prepareUniformBuffers();
	void updateUniformBuffers(bool viewchanged = true);
	void prepare();
};

#endif