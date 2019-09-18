//  
//  For more information, please see: http://software.sci.utah.edu
//  
//  The MIT License
//  
//  Copyright (c) 2004 Scientific Computing and Imaging Institute,
//  University of Utah.
//  
//  
//  Permission is hereby granted, free of charge, to any person obtaining a
//  copy of this software and associated documentation files (the "Software"),
//  to deal in the Software without restriction, including without limitation
//  the rights to use, copy, modify, merge, publish, distribute, sublicense,
//  and/or sell copies of the Software, and to permit persons to whom the
//  Software is furnished to do so, subject to the following conditions:
//  
//  The above copyright notice and this permission notice shall be included
//  in all copies or substantial portions of the Software.
//  
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
//  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
//  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//  DEALINGS IN THE SOFTWARE.
//  

#ifndef SLIVR_MeshRenderer_h
#define SLIVR_MeshRenderer_h

#include "ShaderProgram.h"
#include "MshShader.h"
#include "glm.h"
#include "Plane.h"
#include "BBox.h"
#include "VVulkan.h"
#include <vector>
#include <glm/glm.hpp>

#include "DLLExport.h"

using namespace std;

namespace FLIVR
{
	class MshShaderFactory;

	class EXPORT_API MeshRenderer
	{
	public:
		static constexpr int MSHRENDER_BLEND_DISABLE = 0;
		static constexpr int MSHRENDER_BLEND_OVER = 1;
		static constexpr int MSHRENDER_BLEND_OVER_INV = 2;
		static constexpr int MSHRENDER_BLEND_OVER_UI = 3;
		static constexpr int MSHRENDER_BLEND_ADD = 4;
		static constexpr int MSHRENDER_BLEND_SHADE_SHADOW = 5;

		MeshRenderer(GLMmodel* data);
		MeshRenderer(MeshRenderer&);
		~MeshRenderer();

		//draw
		void draw(const std::unique_ptr<vks::VFrameBuffer>& framebuf, bool clear_framebuf);
		void draw_wireframe(const std::unique_ptr<vks::VFrameBuffer>& framebuf, bool clear_framebuf);
		void draw_integer(unsigned int name, const std::unique_ptr<vks::VFrameBuffer>& framebuf, bool clear_framebuf);
		void update();

		//depth peeling
		void set_depth_peel(int peel) {depth_peel_ = peel;}
		int get_depth_peel() {return depth_peel_;}

		//clipping planes
		void set_planes(vector<Plane*> *p);
		vector<Plane*> *get_planes();

		//size limiter
		void set_limit(int val) {limit_ = val;}
		int get_limit() {return limit_;}

		//matrices
		void SetMatrices(glm::mat4 &mv_mat, glm::mat4 &proj_mat)
		{ m_mv_mat = mv_mat; m_proj_mat = proj_mat; }

		//alpha
		void set_alpha(float alpha)
		{ alpha_ = alpha; }
		float get_alpha()
		{ return alpha_; }
		//effects
		void set_lighting(bool val)
		{ light_ = val; }
		bool get_lighting()
		{ return light_; }
		void set_fog(bool use_fog, double fog_intensity, double fog_start, double fog_end)
		{ fog_ = use_fog; m_fog_intensity = fog_intensity; m_fog_start = fog_start; m_fog_end = fog_end; }
		bool get_fog()
		{ return fog_; }

		std::shared_ptr<vks::VTexture> m_depth_tex;

		void set_depth_tex(const std::shared_ptr<vks::VTexture> &depth_tex)
		{
			m_depth_tex = depth_tex;
		}
		std::shared_ptr<vks::VTexture> get_depth_tex()
		{
			return m_depth_tex;
		}

		void set_device(vks::VulkanDevice* device)
		{
			device_ = device;
		}
		vks::VulkanDevice* get_device()
		{
			return device_;
		}

		void set_bounds(BBox b) { bounds_ = b; }
		BBox get_bounds() { return bounds_; } 

		struct MeshVertexBuffers {
			vks::Buffer vertexBuffer;
			vks::Buffer indexBuffer;
			uint32_t indexCount;
		};
		
		struct Vertex4 {
			float pos[4];
		};
		struct Vertex44 {
			float pos[4];
			float att[4];
		};
		struct Vertex442 {
			float pos[4];
			float att1[4];
			float att2[2];
		};

		struct MshVertexSettings {
			VkPipelineVertexInputStateCreateInfo inputState;
			std::vector<VkVertexInputBindingDescription> inputBinding;
			std::vector<VkVertexInputAttributeDescription> inputAttributes;
		};

		struct MeshPipeline {
			VkPipeline vkpipeline;
			VkRenderPass renderpass;
			ShaderProgram* shader;
			vks::VulkanDevice* device;
			int blend_mode;
			VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			VkPolygonMode polymode = VK_POLYGON_MODE_FILL;
		};

		MeshPipeline prepareMeshPipeline(
			vks::VulkanDevice* device,
			int type,
			int blend,
			bool tex,
			VkPrimitiveTopology topo = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			VkPolygonMode poly = VK_POLYGON_MODE_FILL);

		static VkRenderPass prepareRenderPass(vks::VulkanDevice* device, VkFormat framebuf_format, int attachment_num, bool isSwapChainImage);

		static void init(std::shared_ptr<VVulkan> vulkan);
		static void finalize();

	protected:
		//data and display list
		GLMmodel* data_;
		//depth peeling
		int depth_peel_;	//0:no peeling; 1:peel positive; 2:peel both; -1:peel negative
		//planes
		vector<Plane *> planes_;
		//draw with clipping
		bool draw_clip_;
		int limit_;
		//matrices
		glm::mat4 m_mv_mat, m_proj_mat;
		//lighting
		bool light_;
		//fog
		bool fog_;
		double m_fog_intensity;
		double m_fog_start;
		double m_fog_end;
		float alpha_;
		//bool update
		bool update_;
		BBox bounds_;

		vks::VulkanDevice* device_;
		MshVertexSettings m_vertices4, m_vertices42, m_vertices44, m_vertices442;
		VkFormat depthformat_;
		//vertex buffer
		std::vector<MeshVertexBuffers> m_vertbufs;
		int m_prev_msh_pipeline;

		static std::vector<MeshPipeline> m_msh_pipelines;
		static std::map<vks::VulkanDevice*, VkRenderPass> m_msh_draw_pass;

		static std::shared_ptr<VVulkan> m_vulkan;

		void setupVertexDescriptions();
	};

} // End namespace FLIVR

#endif 