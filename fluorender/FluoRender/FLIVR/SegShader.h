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

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "DLLExport.h"

#include "VulkanDevice.hpp"

namespace FLIVR
{
#define SEG_SAMPLER_NUM 7

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
		SegShader(VkDevice device, int type, int paint_mode, int hr_mode,
			bool use_2d, bool shading, int peel, bool clip, bool hiqual, bool use_stroke, bool stroke_clear, int out_bytes, bool use_abs);
		~SegShader();

		bool create();

		inline VkDevice device() { return device_; }
		inline int type() {return type_;}
		inline int paint_mode() {return paint_mode_;}
		inline int hr_mode() {return hr_mode_;}
		inline bool use_2d() {return use_2d_;}
		inline bool shading() {return shading_;}
		inline int peel() {return peel_;}
		inline bool clip() {return clip_;}
		inline bool hiqual() {return hiqual_;}

		inline bool match(VkDevice device, int type, int paint_mode, int hr_mode,
			bool use_2d, bool shading, int peel, bool clip, bool hiqual, bool use_stroke, bool stroke_clear, int out_bytes, bool use_abs)
		{ 
			return (device_ == device &&
				type_ == type &&
				paint_mode_ == paint_mode &&
				hr_mode_ == hr_mode &&
				use_2d_ == use_2d &&
				shading_ == shading &&
				peel_ == peel &&
				clip_ == clip &&
				hiqual_ == hiqual &&
				use_stroke_ == use_stroke &&
				stroke_clear_ == stroke_clear &&
				out_bytes_ == out_bytes &&
                use_abs_ == use_abs);
		}

		inline ShaderProgram* program() { return program_; }

	protected:
		bool emit(std::string& s);

		VkDevice device_;
		int type_;
		int paint_mode_;
		int hr_mode_;
		bool use_2d_;
		bool shading_;
		int peel_;
		bool clip_;
		bool hiqual_;
		bool use_stroke_;
		bool stroke_clear_;
		int out_bytes_;
        bool use_abs_;

		ShaderProgram* program_;
	};

	class EXPORT_API SegShaderFactory
	{
	public:
		SegShaderFactory();
		SegShaderFactory(std::vector<vks::VulkanDevice*> &devices);
		~SegShaderFactory();

		ShaderProgram* shader(VkDevice device, int type, int paint_mode, int hr_mode,
			bool use_2d, bool shading, int peel, bool clip, bool hiqual, bool use_stroke, bool stroke_clear, int out_bytes, bool use_abs);

		void init(std::vector<vks::VulkanDevice*> &devices);

		struct SegPipeline {
			VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
			VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
			VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
		};

		struct SegCompShaderBaseUBO {
			glm::vec4 loc0_light_alpha;	//loc0
			glm::vec4 loc1_material;	//loc1
			glm::vec4 loc2_scscale_th;	//loc2
			glm::vec4 loc3_gamma_offset;//loc3
//			glm::vec4 loc4_dim;			//loc4
			glm::vec4 loc5_spc_id;		//loc5
			glm::vec4 loc6_colparam;	//loc6
			glm::vec4 loc7_th;			//loc7
			glm::vec4 loc8_w2d;			//loc8
			glm::vec4 plane0;			//loc10
			glm::vec4 plane1;			//loc11
			glm::vec4 plane2;			//loc12
			glm::vec4 plane3;			//loc13
			glm::vec4 plane4;			//loc14
			glm::vec4 plane5;			//loc15
			glm::mat4 mv_mat;
			glm::mat4 proj_mat;
			glm::mat4 mv_mat_inv;
			glm::mat4 proj_mat_inv;
		};

		struct SegCompShaderBrickConst {
			glm::vec4 loc_dim_inv;	//loc4
			glm::vec4 loc_dim;		//loc9
			glm::vec4 b_scale;
			glm::vec4 b_trans;
			glm::vec4 mask_b_scale;
			glm::vec4 mask_b_trans;
		};

		struct SegUniformBufs {
			vks::Buffer frag_base;
		};

		void setupDescriptorSetLayout();
		void getDescriptorSetWriteUniforms(vks::VulkanDevice* vdev, SegUniformBufs& uniformBuffers, std::vector<VkWriteDescriptorSet>& writeDescriptorSets);
		void prepareUniformBuffers(std::map<vks::VulkanDevice*, SegUniformBufs>& uniformBuffers);
		static void updateUniformBuffers(SegUniformBufs& uniformBuffers, SegCompShaderBaseUBO fubo);
		
		static inline VkWriteDescriptorSet writeDescriptorSetTex(
			VkDescriptorSet dstSet,
			uint32_t texid,
			VkDescriptorImageInfo* imageInfo,
			uint32_t descriptorCount = 1)
		{
			VkWriteDescriptorSet writeDescriptorSet{};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = dstSet;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptorSet.dstBinding = texid + 2;
			writeDescriptorSet.pImageInfo = imageInfo;
			writeDescriptorSet.descriptorCount = descriptorCount;
			return writeDescriptorSet;
		}

		static inline VkWriteDescriptorSet writeDescriptorSetStrageImage(
			VkDescriptorSet dstSet,
			uint32_t id,
			VkDescriptorImageInfo* imageInfo,
			uint32_t descriptorCount = 1)
		{
			VkWriteDescriptorSet writeDescriptorSet{};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = dstSet;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			writeDescriptorSet.dstBinding = id;
			writeDescriptorSet.pImageInfo = imageInfo;
			writeDescriptorSet.descriptorCount = descriptorCount;
			return writeDescriptorSet;
		}

		static inline VkWriteDescriptorSet writeDescriptorSetMask(
			VkDescriptorSet dstSet,
			VkDescriptorImageInfo* imageInfo,
			uint32_t descriptorCount = 1)
		{
			return writeDescriptorSetStrageImage(dstSet, 0, imageInfo, descriptorCount);
		}

		static inline VkWriteDescriptorSet writeDescriptorSetLabel(
			VkDescriptorSet dstSet,
			VkDescriptorImageInfo* imageInfo,
			uint32_t descriptorCount = 1)
		{
			return writeDescriptorSetStrageImage(dstSet, 9, imageInfo, descriptorCount);
		}

		static inline VkWriteDescriptorSet writeDescriptorSetStroke(
			VkDescriptorSet dstSet,
			VkDescriptorImageInfo* imageInfo,
			uint32_t descriptorCount = 1)
		{
			return writeDescriptorSetStrageImage(dstSet, 10, imageInfo, descriptorCount);
		}

		std::map<vks::VulkanDevice*, SegPipeline> pipeline_;

		std::vector<vks::VulkanDevice*> vdevices_;

	protected:
		std::vector<SegShader*> shader_;
		int prev_shader_;
	};

} // end namespace FLIVR

#endif // SegShader_h
