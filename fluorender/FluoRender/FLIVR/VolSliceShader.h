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

#ifndef VolSliceShader_h
#define VolSliceShader_h

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "DLLExport.h"

#include "VulkanDevice.hpp"

namespace FLIVR
{
#define VOL_SLICE_ORDER_BACKWARD 0
#define VOL_SLICE_ORDER_FORWARD 1

	class ShaderProgram;

	class EXPORT_API VolSliceShader
	{
	public:
		VolSliceShader(VkDevice device, int order);
		~VolSliceShader();

		bool create();

		inline VkDevice device() { return device_; }
		inline int order() {return order_;}
		

		inline bool match(VkDevice device, int order)
		{ 
			return (device_ == device && order_ == order);
		}

		inline ShaderProgram* program() { return program_; }

	protected:
		bool emit(std::string& s);

		VkDevice device_;
		int order_;
		
		ShaderProgram* program_;
	};

	class EXPORT_API VolSliceShaderFactory
	{
	public:
		VolSliceShaderFactory();
		VolSliceShaderFactory(std::vector<vks::VulkanDevice*> &devices);
		~VolSliceShaderFactory();

		ShaderProgram* shader(VkDevice device, int order);

		void init(std::vector<vks::VulkanDevice*> &devices);

		struct VolSlicePipeline {
			VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
			VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
			VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
		};

		struct VolSliceShaderBrickConst {
			glm::vec4 b_min_tmin;
			glm::vec4 b_max_tmax;
			glm::vec4 tb_min;
			glm::vec4 tb_max;
			glm::vec4 view_origin_dt;
			glm::vec4 view_direction;
			glm::vec4 up;
			glm::uvec4 range;
		};

		void setupDescriptorSetLayout();
		
		std::map<vks::VulkanDevice*, VolSlicePipeline> pipeline_;

		std::vector<vks::VulkanDevice*> vdevices_;

		static inline VkWriteDescriptorSet writeDescriptorSetStrageBuf(
			VkDescriptorSet dstSet,
			uint32_t id,
			VkDescriptorBufferInfo* bufInfo,
			uint32_t descriptorCount = 1)
		{
			VkWriteDescriptorSet writeDescriptorSet{};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = dstSet;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			writeDescriptorSet.dstBinding = id;
			writeDescriptorSet.pBufferInfo = bufInfo;
			writeDescriptorSet.descriptorCount = descriptorCount;
			return writeDescriptorSet;
		}

	protected:
		std::vector<VolSliceShader*> shader_;
		int prev_shader_;
	};

} // end namespace FLIVR

#endif // VolSliceShader_h
