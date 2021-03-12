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

#ifndef ImgShader_h
#define ImgShader_h

#include <string>
#include <vector>
#include "DLLExport.h"

#include "VulkanDevice.hpp"

namespace FLIVR
{
#define IMG_SHADER_TEXTURE_LOOKUP			0
#define IMG_SHDR_BRIGHTNESS_CONTRAST		1
#define IMG_SHDR_BRIGHTNESS_CONTRAST_HDR	2
#define IMG_SHDR_GRADIENT_MAP				3
#define IMG_SHDR_FILTER_BLUR				4
#define IMG_SHDR_FILTER_MAX					5
#define IMG_SHDR_FILTER_SHARPEN				6
#define IMG_SHDR_DEPTH_TO_OUTLINES			7
#define IMG_SHDR_DEPTH_TO_GRADIENT			8
#define IMG_SHDR_GRADIENT_TO_SHADOW			9
#define IMG_SHDR_GRADIENT_TO_SHADOW_MESH	10
#define IMG_SHDR_BLEND_BRIGHT_BACKGROUND	11
#define IMG_SHDR_BLEND_BRIGHT_BACKGROUND_HDR	12
#define IMG_SHDR_PAINT						13
#define	IMG_SHDR_DRAW_GEOMETRY				14
#define	IMG_SHDR_DRAW_GEOMETRY_COLOR3		15
#define	IMG_SHDR_DRAW_GEOMETRY_COLOR4		16
#define IMG_SHDR_BLEND_FOR_DEPTH_MODE			17
#define IMG_SHDR_BLEND_ID_COLOR_FOR_DEPTH_MODE	18
#define IMG_SHADER_TEXTURE_LOOKUP_BLEND			19
#define IMG_SHDR_BRIGHTNESS_CONTRAST_HDR_BLEND	20
#define IMG_SHDR_BLEND_BRIGHT_BACKGROUND_HDR_PREMULTI	21
#define IMG_SHDR_TEXT						22
#define IMG_SHDR_LEVELS                     23

#define IMG_SHDR_SAMPLER_NUM	3

	class ShaderProgram;

	class EXPORT_API ImgShader
	{
	public:
		ImgShader(VkDevice device, int type, int colormap);
		~ImgShader();

		bool create();

		inline VkDevice device() { return device_; }
		inline int type() {return type_;}
		inline int colormap() {return colormap_;}

		inline bool match(VkDevice device, int type, int colormap)
		{
			if (device_ == device && type_ == type)
			{
				if (type_ == IMG_SHDR_GRADIENT_MAP)
					return (colormap_==colormap);
				else
					return true;
			}
			else
				return false;
		}

		inline ShaderProgram* program() { return program_; }

	protected:
		bool emit_v(std::string& s);
		bool emit_f(std::string& s);
		std::string get_colormap_code();

		VkDevice device_;
		int type_;
		int colormap_;

		ShaderProgram* program_;
	};

	class EXPORT_API ImgShaderFactory
	{
	public:
		ImgShaderFactory();
		ImgShaderFactory(std::vector<vks::VulkanDevice*> &devices);
		~ImgShaderFactory();

		void init(std::vector<vks::VulkanDevice*> &devices);

		ShaderProgram* shader(VkDevice device, int type, int colormap_=0);

		struct ImgPipelineSettings {
			VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
			VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
			VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
		};
		
		void setupDescriptorSetLayout();
		
		std::map<vks::VulkanDevice*, ImgPipelineSettings> pipeline_settings_;
		
		std::vector<vks::VulkanDevice*> vdevices_;

	protected:
		std::vector<ImgShader*> shader_;
		int prev_shader_;
	};

} // end namespace FLIVR

#endif // ImgShader_h
