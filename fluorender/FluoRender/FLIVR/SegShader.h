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

#ifndef SegShader_h
#define SegShader_h

#include <string>
#include <vector>
#include "DLLExport.h"

namespace FLIVR
{
//type definitions
#define SEG_SHDR_INITIALIZE	1	//initialize the segmentation fragment shader
#define SEG_SHDR_DB_GROW	2	//diffusion based grow
#define LBL_SHDR_INITIALIZE	3	//initialize the labeling fragment shader
#define LBL_SHDR_MIF		4	//maximum intensity filtering
#define FLT_SHDR_NR			5	//remove noise

	class ShaderProgram;

	class EXPORT_API SegShader
	{
	public:
		SegShader(int type, int paint_mode, int hr_mode,
			bool use_2d, bool shading, int peel, bool clip, bool hiqual, bool use_stroke);
		~SegShader();

		bool create();

		inline int type() {return type_;}
		inline int paint_mode() {return paint_mode_;}
		inline int hr_mode() {return hr_mode_;}
		inline bool use_2d() {return use_2d_;}
		inline bool shading() {return shading_;}
		inline int peel() {return peel_;}
		inline bool clip() {return clip_;}
		inline bool hiqual() {return hiqual_;}

		inline bool match(int type, int paint_mode, int hr_mode,
			bool use_2d, bool shading, int peel, bool clip, bool hiqual, bool use_stroke)
		{ 
			return (type_ == type &&
				paint_mode_ == paint_mode &&
				hr_mode_ == hr_mode &&
				use_2d_ == use_2d &&
				shading_ == shading &&
				peel_ == peel &&
				clip_ == clip &&
				hiqual_ == hiqual &&
				use_stroke_ == use_stroke);
		}

		inline ShaderProgram* program() { return program_; }

	protected:
		bool emit(std::string& s);

		int type_;
		int paint_mode_;
		int hr_mode_;
		bool use_2d_;
		bool shading_;
		int peel_;
		bool clip_;
		bool hiqual_;
		bool use_stroke_;

		ShaderProgram* program_;
	};

	class EXPORT_API SegShaderFactory
	{
	public:
		SegShaderFactory();
		SegShaderFactory(std::shared_ptr<VVulkan> vulkan);
		~SegShaderFactory();

		ShaderProgram* shader(int type, int paint_mode, int hr_mode,
			bool use_2d, bool shading, int peel, bool clip, bool hiqual, bool use_stroke);

		void init(std::shared_ptr<VVulkan> vulkan);

		struct SegPipeline {
			VkDescriptorPool descriptorPool;
			VkDescriptorSetLayout descriptorSetLayout;
			VkPipelineLayout pipelineLayout;
			VkDescriptorSet descriptorSet;
		};

		struct SegFragShaderBaseUBO {
			glm::vec4 loc0_light_alpha;	//loc0
			glm::vec4 loc1_material;	//loc1
			glm::vec4 loc2_scscale_th;	//loc2
			glm::vec4 loc3_gamma_offset;//loc3
//			glm::vec4 loc4_dim;			//loc4
			glm::vec4 loc5_spc_id;		//loc5
			glm::vec4 loc6_colparam;	//loc6
			glm::vec4 loc7_view;		//loc7
			glm::vec4 loc8_fog;			//loc8
			glm::vec4 plane0;			//loc10
			glm::vec4 plane1;			//loc11
			glm::vec4 plane2;			//loc12
			glm::vec4 plane3;			//loc13
			glm::vec4 plane4;			//loc14
			glm::vec4 plane5;			//loc15
			glm::mat4 proj_mat;
			glm::mat4 model_mat;
			glm::mat4 model_mat_inv;
			glm::mat4 proj_mat_inv;
		};

		struct SegFragShaderBrickUBO {
			glm::vec4 loc_dim_inv;	//loc4
			glm::vec4 loc_dim;		//loc9
			glm::mat4 bmat;
			glm::mat4 mask_bmat;
		};

		struct SegUniformBufs {
			vks::Buffer frag_base;
			vks::Buffer frag_brick;
		};

		void setupDescriptorPool();
		void setupDescriptorSetLayout();
		void setupDescriptorSetUniforms();
		void setupDescriptorSetSamplers(uint32_t descriptorWriteCountconst, VkWriteDescriptorSet* pDescriptorWrites);
		void prepareUniformBuffers();
		void updateUniformBuffersFragBase(SegFragShaderBaseUBO ubo);
		void updateUniformBuffersFragBrick(SegFragShaderBrickUBO ubo);
		
		SegPipeline pipeline_;
		SegUniformBufs uniformBuffers_;

	protected:
		std::vector<SegShader*> shader_;
		int prev_shader_;
	};

} // end namespace FLIVR

#endif // SegShader_h
