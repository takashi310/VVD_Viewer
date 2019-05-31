#include "Vulkan2dRender.h"
#include "ShaderProgram.h"
#include <regex>

Vulkan2dRender::Vulkan2dRender()
{
	m_init = false;
	prev_pipeline = -1;
}

Vulkan2dRender::Vulkan2dRender(std::shared_ptr<VVulkan> vulkan)
{
	init(vulkan);
}

void Vulkan2dRender::init(std::shared_ptr<VVulkan> vulkan)
{
	m_vulkan = vulkan;

	generateQuad();
	setupVertexDescriptions();
	prepareRenderPass();
}

Vulkan2dRender::~Vulkan2dRender()
{

}

void Vulkan2dRender::generateQuad()
{
	// Setup vertices for a single uv-mapped quad made from two triangles
	std::vector<Vulkan2dRender::Vertex> quad =
	{
		{ {  1.0f,  1.0f, 0.0f }, { 1.0f, 1.0f, 0.0f } },
		{ { -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
		{ { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
		{ {  1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } }
	};

	// Setup indices
	std::vector<uint32_t> indices = { 0,1,2, 2,3,0 };
	m_indexCount = static_cast<uint32_t>(indices.size());

	// Create buffers
	// For the sake of simplicity we won't stage the vertex data to the gpu memory
	// Vertex buffer
	VK_CHECK_RESULT(m_vulkan->vulkanDevice->createBuffer(
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&m_vertexBuffer,
		quad.size() * sizeof(Vertex),
		quad.data()));
	// Index buffer
	VK_CHECK_RESULT(m_vulkan->vulkanDevice->createBuffer(
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&m_indexBuffer,
		indices.size() * sizeof(uint32_t),
		indices.data()));
}

void Vulkan2dRender::setupVertexDescriptions()
{
	// Binding description
	m_vertices.inputBinding.resize(1);
	m_vertices.inputBinding[0] =
		vks::initializers::vertexInputBindingDescription(
		0, 
		sizeof(Vertex), 
		VK_VERTEX_INPUT_RATE_VERTEX);

	// Attribute descriptions
	// Describes memory layout and shader positions
	m_vertices.inputAttributes.resize(3);
	// Location 0 : Position
	m_vertices.inputAttributes[0] =
		vks::initializers::vertexInputAttributeDescription(
		0,
		0,
		VK_FORMAT_R32G32B32_SFLOAT,
		offsetof(Vertex, pos));			
	// Location 1 : Texture coordinates
	m_vertices.inputAttributes[1] =
		vks::initializers::vertexInputAttributeDescription(
		0,
		1,
		VK_FORMAT_R32G32B32_SFLOAT,
		offsetof(Vertex, uv));

	m_vertices.inputState = vks::initializers::pipelineVertexInputStateCreateInfo();
	m_vertices.inputState.vertexBindingDescriptionCount = static_cast<uint32_t>(m_vertices.inputBinding.size());
	m_vertices.inputState.pVertexBindingDescriptions = m_vertices.inputBinding.data();
	m_vertices.inputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_vertices.inputAttributes.size());
	m_vertices.inputState.pVertexAttributeDescriptions = m_vertices.inputAttributes.data();
}

void Vulkan2dRender::prepareRenderPass()
{
	VkPhysicalDevice physicalDevice = m_vulkan->getPhysicalDevice();
	VkDevice device = m_vulkan->getDevice();

	// Create a separate render pass for the offscreen rendering as it may differ from the one used for scene rendering
	std::array<VkAttachmentDescription, 1> attchmentDescriptions = {};
	// Color attachment
	attchmentDescriptions[0].format = VK_FORMAT_R8G8B8A8_UNORM;
	attchmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attchmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attchmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attchmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attchmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attchmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attchmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	
	VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	VkSubpassDescription subpassDescription = {};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorReference;

	// Use subpass dependencies for layout transitions
	std::array<VkSubpassDependency, 2> dependencies;

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	// Create the actual renderpass
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attchmentDescriptions.size());
	renderPassInfo.pAttachments = attchmentDescriptions.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpassDescription;
	renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
	renderPassInfo.pDependencies = dependencies.data();

	VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_pass8));


	attchmentDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attchmentDescriptions.size());
	VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_pass32));
}

Vulkan2dRender::V2dPipeline Vulkan2dRender::preparePipeline(int shader, int blend_mode, VkFormat dstformat)
{
	if (prev_pipeline >= 0) {
		if (m_pipelines[prev_pipeline].shader == shader &&
			m_pipelines[prev_pipeline].blend == blend_mode &&
			m_pipelines[prev_pipeline].dstformat == dstformat)
			return m_pipelines[prev_pipeline];
	}
	for (int i = 0; i < m_pipelines.size(); i++) {
		if (m_pipelines[i].shader == shader &&
			m_pipelines[i].blend == blend_mode &&
			m_pipelines[i].dstformat == dstformat)
		{
			prev_pipeline = i;
			return m_pipelines[i];
		}
	}

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
		vks::initializers::pipelineInputAssemblyStateCreateInfo(
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		0,
		VK_FALSE);

	VkPipelineRasterizationStateCreateInfo rasterizationState =
		vks::initializers::pipelineRasterizationStateCreateInfo(
		VK_POLYGON_MODE_FILL,
		VK_CULL_MODE_NONE,
		VK_FRONT_FACE_COUNTER_CLOCKWISE,
		0);

	VkPipelineDepthStencilStateCreateInfo depthStencilState =
		vks::initializers::pipelineDepthStencilStateCreateInfo(
		VK_FALSE,
		VK_FALSE,
		VK_COMPARE_OP_NEVER);

	VkPipelineViewportStateCreateInfo viewportState =
		vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);

	VkPipelineMultisampleStateCreateInfo multisampleState =
		vks::initializers::pipelineMultisampleStateCreateInfo(
		VK_SAMPLE_COUNT_1_BIT,
		0);

	std::vector<VkDynamicState> dynamicStateEnables = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo dynamicState =
		vks::initializers::pipelineDynamicStateCreateInfo(
		dynamicStateEnables.data(),
		static_cast<uint32_t>(dynamicStateEnables.size()),
		0);

	m_img_pipeline_settings = m_vulkan->img_shader_factory_->pipeline_settings_[m_vulkan->vulkanDevice];
	VkGraphicsPipelineCreateInfo pipelineCreateInfo =
		vks::initializers::pipelineCreateInfo(
		m_img_pipeline_settings.pipelineLayout,
		dstformat==VK_FORMAT_R8G8B8A8_UNORM ? m_pass8 : m_pass32,
		0);

	pipelineCreateInfo.pVertexInputState = &m_vertices.inputState;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineCreateInfo.pRasterizationState = &rasterizationState;
	pipelineCreateInfo.pMultisampleState = &multisampleState;
	pipelineCreateInfo.pViewportState = &viewportState;
	pipelineCreateInfo.pDepthStencilState = &depthStencilState;
	pipelineCreateInfo.pDynamicState = &dynamicState;
	pipelineCreateInfo.stageCount = 2;
	
	//blend mode
	VkBool32 enable_blend;
	VkBlendFactor src_blend, dst_blend;
	switch(blend_mode)
	{
	case V2DRENDER_BLEND_OVER:
		enable_blend = VK_TRUE;
		src_blend = VK_BLEND_FACTOR_ONE;
		dst_blend = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		break;
	case V2DRENDER_BLEND_ADD:
		enable_blend = VK_TRUE;
		src_blend = VK_BLEND_FACTOR_ONE;
		dst_blend = VK_BLEND_FACTOR_ONE;
	default:
		enable_blend = VK_FALSE;
		src_blend = VK_BLEND_FACTOR_ONE;
		dst_blend = VK_BLEND_FACTOR_ZERO;
	}

	VkPipelineColorBlendAttachmentState blendAttachmentState =
		vks::initializers::pipelineColorBlendAttachmentState(
		enable_blend,
		VK_BLEND_OP_ADD,
		src_blend,
		dst_blend,
		0xf);
	
	VkPipelineColorBlendStateCreateInfo colorBlendState =
		vks::initializers::pipelineColorBlendStateCreateInfo(
		1, 
		&blendAttachmentState);
	pipelineCreateInfo.pColorBlendState = &colorBlendState;

	// Load shaders
	std::array<VkPipelineShaderStageCreateInfo,2> shaderStages;
	FLIVR::ShaderProgram *sh = m_vulkan->img_shader_factory_->shader(shader);
	shaderStages[0] = sh->get_vertex_shader();
	shaderStages[1] = sh->get_fragment_shader();
	pipelineCreateInfo.pStages = shaderStages.data();

	V2dPipeline v2d_pipeline;
	v2d_pipeline.shader = shader;
	v2d_pipeline.blend = blend_mode;
	v2d_pipeline.dstformat = dstformat;
	getEnabledUniforms(v2d_pipeline, sh->get_fragment_shader_code());

	VK_CHECK_RESULT(
		vkCreateGraphicsPipelines(m_vulkan->getDevice(), m_vulkan->getPipelineCache(), 1, &pipelineCreateInfo, nullptr, &v2d_pipeline.pipeline)
	);

	m_pipelines.push_back(v2d_pipeline);
	prev_pipeline = m_pipelines.size() - 1;

	return v2d_pipeline;
}

void Vulkan2dRender::getEnabledUniforms(V2dPipeline pipeline, const std::string &code)
{
	std::vector<int> loc;
	std::vector<int> mat;
	std::vector<int> sampler;

	if (V2DRENDER_UNIFORM_VEC_NUM > 0) {
		std::vector<std::string> v = {};
		std::regex pt{ "uniform vec4 loc[0-9]+" };
		std::sregex_iterator end, ite{ code.begin(), code.end(), pt };
		for (; ite != end; ++ite) {
			v.push_back(ite->str());
		}

		for (auto t : v) {
			size_t len = t.size();
			if (len < 2) continue;
			char d = t[len - 2];
			if (isdigit(d)) {
				int id = atoi(t.substr(len - 2, 2).c_str());
				loc.push_back(id);
			}
			else {
				int id = t[len - 1] - '0';
				loc.push_back(id);
			}
		}
	}

	if (V2DRENDER_UNIFORM_MAT_NUM > 0) {
		std::vector<std::string> v = {};
		std::regex pt{ "uniform mat4 matrix[0-9]+" };
		std::sregex_iterator end, ite{ code.begin(), code.end(), pt };
		for (; ite != end; ++ite) {
			v.push_back(ite->str());
		}

		for (auto t : v) {
			size_t len = t.size();
			if (len < 2) continue;
			char d = t[len - 2];
			if (isdigit(d)) {
				int id = atoi(t.substr(len - 2, 2).c_str());
				mat.push_back(id);
			}
			else {
				int id = t[len - 1] - '0';
				mat.push_back(id);
			}
		}
	}

	if (IMG_SHDR_SAMPLER_NUM > 0) {
		std::vector<std::string> v = {};
		std::regex pt{ "sampler[2-3]D tex[0-9]+" };
		std::sregex_iterator end, ite{ code.begin(), code.end(), pt };
		for (; ite != end; ++ite) {
			v.push_back(ite->str());
		}

		for (auto t : v) {
			size_t len = t.size();
			if (len < 2) continue;
			char d = t[len - 2];
			if (isdigit(d)) {
				int id = atoi(t.substr(len - 2, 2).c_str());
				sampler.push_back(id);
			}
			else {
				int id = t[len - 1] - '0';
				sampler.push_back(id);
			}
		}
	}

	for (auto i : loc) {
		if ( i < V2DRENDER_UNIFORM_VEC_NUM )
			pipeline.uniforms[i] = VK_TRUE;
	}
	for (auto i : mat) {
		if ( i < V2DRENDER_UNIFORM_MAT_NUM )
			pipeline.uniforms[i+V2DRENDER_UNIFORM_VEC_NUM] = VK_TRUE;
	}
	for (auto i : sampler) {
		if ( i < IMG_SHDR_SAMPLER_NUM )
			pipeline.samplers[i] = VK_TRUE;
	}
}

void Vulkan2dRender::setupDescriptorSet(const Vulkan2dRender::V2DRenderParams &params, const Vulkan2dRender::V2dPipeline &pipeline)
{
	std::vector<VkWriteDescriptorSet> descriptorWrites;

	for (int i = 0; i < IMG_SHDR_SAMPLER_NUM; i++) {
		if (params.tex[i] && pipeline.samplers[i]) {
			VkWriteDescriptorSet dw;
			dw.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			dw.dstSet = m_img_pipeline_settings.descriptorSet;
			dw.dstBinding = i;
			dw.dstArrayElement = 0;
			dw.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			dw.descriptorCount = 1;
			dw.pImageInfo = &params.tex[i]->descriptor;
			descriptorWrites.push_back(dw);
		}
	}
	vkUpdateDescriptorSets(m_vulkan->getDevice(), descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

void Vulkan2dRender::buildCommandBuffer(VkCommandBuffer commandbufs[], int commandbuf_num, const std::unique_ptr<vks::VFrameBuffer> &framebuf, const V2DRenderParams &params)
{
	if (!commandbufs || commandbuf_num <= 0)
		return;

	V2dPipeline pipeline = preparePipeline(params.shader, params.blend);

	setupDescriptorSet(params, pipeline);

	uint64_t constsize = 0;
	for (int32_t i = 0; i < V2DRENDER_UNIFORM_VEC_NUM; i++) {
		if (pipeline.uniforms[i]) {
			memcpy(constant_buf+constsize, &params.loc[i], sizeof(glm::vec4));
			constsize += sizeof(glm::vec4);
		}
	}
	for (int32_t i = V2DRENDER_UNIFORM_VEC_NUM; i < V2DRENDER_UNIFORM_NUM; i++) {
		if (pipeline.uniforms[i]) {
			memcpy(constant_buf+constsize, &params.matrix[i-V2DRENDER_UNIFORM_VEC_NUM], sizeof(glm::mat4));
			constsize += sizeof(glm::mat4);
		}
	}

	VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

	VkClearValue clearValues[1];
	clearValues[0].color = params.clearColor;

	VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
	renderPassBeginInfo.renderPass = m_pass;
	renderPassBeginInfo.renderArea.offset.x = 0;
	renderPassBeginInfo.renderArea.offset.y = 0;
	renderPassBeginInfo.renderArea.extent.width = framebuf->w;
	renderPassBeginInfo.renderArea.extent.height = framebuf->h;

	for (int32_t i = 0; i < commandbuf_num; ++i)
	{
		// Set target frame buffer
		renderPassBeginInfo.framebuffer = framebuf->framebuffer;

		VK_CHECK_RESULT(vkBeginCommandBuffer(commandbufs[i], &cmdBufInfo));

		vkCmdBeginRenderPass(commandbufs[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vks::initializers::viewport((float)framebuf->w, (float)framebuf->h, 0.0f, 1.0f);
		vkCmdSetViewport(commandbufs[i], 0, 1, &viewport);

		VkRect2D scissor = vks::initializers::rect2D(framebuf->w, framebuf->h, 0, 0);
		vkCmdSetScissor(commandbufs[i], 0, 1, &scissor);

		vkCmdBindDescriptorSets(
			commandbufs[i], 
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_img_pipeline_settings.pipelineLayout,
			0,
			1,
			&m_img_pipeline_settings.descriptorSet,
			0,
			NULL);

		if (params.clear)
		{
			VkClearAttachment clearAttachments[1] = {};
			clearAttachments[0].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			clearAttachments[0].clearValue = clearValues[0];
			clearAttachments[0].colorAttachment = 0;

			VkClearRect clearRect = {};
			clearRect.layerCount = 1;
			clearRect.rect.offset = { 0, 0 };
			clearRect.rect.extent = { framebuf->w, framebuf->h };

			vkCmdClearAttachments(
				commandbufs[i],
				1,
				clearAttachments,
				1,
				&clearRect);
		}

		vkCmdBindPipeline(commandbufs[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

		//push constants
		if (constsize > 0)
		{
			vkCmdPushConstants(
				commandbufs[i],
				m_img_pipeline_settings.pipelineLayout,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				constsize,
				constant_buf);
		}

		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(commandbufs[i], 0, 1, &m_vertexBuffer.buffer, offsets);
		vkCmdBindIndexBuffer(commandbufs[i], m_indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(commandbufs[i], m_indexCount, 1, 0, 0, 0);

		vkCmdEndRenderPass(commandbufs[i]);

		VK_CHECK_RESULT(vkEndCommandBuffer(commandbufs[i]));
	}

}