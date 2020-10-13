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

#ifndef VolCalShader_h
#define VolCalShader_h

#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "DLLExport.h"
#include "VulkanDevice.hpp"

namespace FLIVR
{
	#define CAL_SUBSTRACTION	1	//initialize the segmentation fragment shader
	#define CAL_ADDITION		2	//diffusion based grow
	#define CAL_DIVISION		3	//initialize the labeling fragment shader
	#define CAL_INTERSECTION	4	//minimum of two
	#define CAL_APPLYMASK		5	//apply mask to volume
	#define CAL_APPLYMASKINV	6	//apply the inverted mask
	#define CAL_APPLYMASKINV2	7	//apply the inverted mask
	#define CAL_INTERSECTION_WITH_MASK	8	//minimum of two with mask
	#define CAL_MASK_THRESHOLD	9	//minimum of two with mask
	#define CAL_RGBAPPLYMASK	10	//apply mask to rgb volume
	#define CAL_RGBAPPLYMASKINV 11	//apply the inverted mask to rgb mask

	#define CAL_SAMPLER_NUM 4

	class ShaderProgram;

	class EXPORT_API VolCalShader
	{
	public:
		VolCalShader(VkDevice device, int type, int out_bytes);
		~VolCalShader();

		bool create();

		inline VkDevice device() { return device_; }
		inline int type() {return type_;}
		inline int out_bytes() { return out_bytes_; }

		inline bool match(VkDevice device, int type, int out_bytes)
		{ 
			return (device_ == device && type_ == type && out_bytes_ == out_bytes);
		}

		inline ShaderProgram* program() { return program_; }

	protected:
		bool emit(std::string& s);

		VkDevice device_;
		int type_;
		int out_bytes_;

		ShaderProgram* program_;
	};

	class EXPORT_API VolCalShaderFactory
	{
	public:
		VolCalShaderFactory();
		VolCalShaderFactory(std::vector<vks::VulkanDevice*> &devices);
		~VolCalShaderFactory();

		ShaderProgram* shader(VkDevice device, int type, int out_bytes);

		void init(std::vector<vks::VulkanDevice*> &devices);

		struct VolCalPipeline {
			VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
			VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
			VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
		};

		struct CalCompShaderBrickConst {
			glm::vec4 loc0_scale_usemask;	//(scale_a, scale_b, use_mask_a, use_mask_b)
			glm::vec4 loc1_dim_inv;
		};

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
			writeDescriptorSet.dstBinding = texid+1;
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

		static inline VkWriteDescriptorSet writeDescriptorSetOutput(
			VkDescriptorSet dstSet,
			VkDescriptorImageInfo* imageInfo,
			uint32_t descriptorCount = 1)
		{
			return writeDescriptorSetStrageImage(dstSet, 0, imageInfo, descriptorCount);
		}

		void setupDescriptorSetLayout();
			
		std::map<vks::VulkanDevice*, VolCalPipeline> pipeline_;

		std::vector<vks::VulkanDevice*> vdevices_;
		
	protected:
		std::vector<VolCalShader*> shader_;
		int prev_shader_;
	};

} // end namespace FLIVR

#endif // VolCalShader_h
