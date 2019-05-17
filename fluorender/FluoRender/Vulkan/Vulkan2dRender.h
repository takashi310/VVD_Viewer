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

	struct {
		VkPipelineVertexInputStateCreateInfo inputState;
		std::vector<VkVertexInputBindingDescription> inputBinding;
		std::vector<VkVertexInputAttributeDescription> inputAttributes;
	} m_vertices;

	struct V2dPipeline {
		VkPipeline pipeline;
		int shader;
		int blend;
		VkBool32 uniforms[V2DRENDER_UNIFORM_NUM] = { VK_FALSE };
		VkBool32 samplers[IMG_SHDR_SAMPLER_NUM] = { VK_FALSE };
	};
	
	FLIVR::ImgShaderFactory::ImgPipelineSettings m_img_pipeline_settings;
	std::vector<V2dPipeline> m_pipelines;
	int prev_pipeline;

	VkRenderPass m_pass; 

	bool m_init;
	std::shared_ptr<VVulkan> m_vulkan;

	static FLIVR::ImgShaderFactory m_img_shader_factory;

	Vulkan2dRender();
	Vulkan2dRender(std::shared_ptr<VVulkan> vulkan);
	~Vulkan2dRender();

	void init(std::shared_ptr<VVulkan> vulkan);
	void generateQuad();
	void setupVertexDescriptions();
	void prepareRenderPass();
	V2dPipeline preparePipeline(int shader, int blend_mode);

	void getEnabledUniforms(V2dPipeline pipeline, const std::string &code);

	struct V2DRenderParams {
		int shader = IMG_SHADER_TEXTURE_LOOKUP;
		int blend = V2DRENDER_BLEND_OVER;
		bool clear = false;
		VkClearColorValue clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
		VVulkan::VTexture *tex[IMG_SHDR_SAMPLER_NUM] = { NULL };
		glm::vec4 loc[V2DRENDER_UNIFORM_VEC_NUM] = { glm::vec4(0.0f) };
		glm::mat4 matrix[V2DRENDER_UNIFORM_MAT_NUM] = { glm::mat4(1.0f) };
	};

	void setupDescriptorSet(const V2DRenderParams &params);
	void buildCommandBuffer(VkCommandBuffer commandbufs[], int commandbuf_num, VVulkan::FrameBuffer framebuf, const V2DRenderParams &params);
};

#endif