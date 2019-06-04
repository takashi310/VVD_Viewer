#include <VVulkan.h>
#include <FLIVR/ImgShader.h>
#include <vector>
#include <memory>

#ifndef _Vulkan2dRender_H_
#define _Vulkan2dRender_H_

class Vulkan2dRender
{
	#define V2DRENDER_BLEND_DISABLE 0
	#define V2DRENDER_BLEND_OVER	1
	#define V2DRENDER_BLEND_ADD		2

	#define V2DRENDER_UNIFORM_VEC_NUM	3
	#define V2DRENDER_UNIFORM_MAT_NUM	1
	#define V2DRENDER_UNIFORM_NUM	(V2DRENDER_UNIFORM_VEC_NUM+V2DRENDER_UNIFORM_MAT_NUM)

public:
	vks::Buffer m_vertexBuffer;
	vks::Buffer m_indexBuffer;
	uint32_t m_indexCount;
	
	struct Vertex {
		float pos[3];
		float uv[3];
	};

	struct V2dVertexSettings{
		VkPipelineVertexInputStateCreateInfo inputState;
		std::vector<VkVertexInputBindingDescription> inputBinding;
		std::vector<VkVertexInputAttributeDescription> inputAttributes;
	} m_vertices;

	struct V2dPipeline {
		VkPipeline pipeline;
		VkRenderPass pass;
		int shader;
		int blend;
		VkFormat framebuf_format;
		int framebuf_num;
		VkBool32 uniforms[V2DRENDER_UNIFORM_NUM] = { VK_FALSE };
		VkBool32 samplers[IMG_SHDR_SAMPLER_NUM] = { VK_FALSE };
	};

	unsigned char constant_buf[V2DRENDER_UNIFORM_VEC_NUM*sizeof(glm::vec4) + V2DRENDER_UNIFORM_MAT_NUM*sizeof(glm::mat4)];
	
	FLIVR::ImgShaderFactory::ImgPipelineSettings m_img_pipeline_settings;
	std::vector<V2dPipeline> m_pipelines;
	int prev_pipeline;

	bool m_init;
	std::shared_ptr<VVulkan> m_vulkan;

	VkCommandBuffer m_default_cmdbuf;

	Vulkan2dRender();
	Vulkan2dRender(std::shared_ptr<VVulkan> vulkan);
	~Vulkan2dRender();

	void init(std::shared_ptr<VVulkan> vulkan);
	void generateQuad();
	void setupVertexDescriptions();
	VkRenderPass prepareRenderPass(VkFormat framebuf_format, int framebuf_num);
	V2dPipeline preparePipeline(int shader, int blend_mode, VkFormat framebuf_format, int framebuf_num);

	void getEnabledUniforms(V2dPipeline pipeline, const std::string &code);

	struct V2DRenderParams {
		int shader = IMG_SHADER_TEXTURE_LOOKUP;
		int blend = V2DRENDER_BLEND_OVER;
		bool clear = false;
		VkClearColorValue clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
		vks::VTexture *tex[IMG_SHDR_SAMPLER_NUM] = { NULL };
		glm::vec4 loc[V2DRENDER_UNIFORM_VEC_NUM] = { glm::vec4(0.0f) };
		glm::mat4 matrix[V2DRENDER_UNIFORM_MAT_NUM] = { glm::mat4(1.0f) };
		uint32_t waitSemaphoreCount = 0;
		uint32_t signalSemaphoreCount = 0;
		VkSemaphore *waitSemaphores = nullptr;
		VkSemaphore *signalSemaphores = nullptr;
	};
	
	void setupDescriptorSetWrites(const V2DRenderParams &params, const V2dPipeline &pipeline, std::vector<VkWriteDescriptorSet> &descriptorWrites);
	void buildCommandBuffer(VkCommandBuffer commandbufs[], int commandbuf_num, const std::unique_ptr<vks::VFrameBuffer> &framebuf, const V2DRenderParams &params);
	void render(const std::unique_ptr<vks::VFrameBuffer>& framebuf, const V2DRenderParams& params);
};

#endif