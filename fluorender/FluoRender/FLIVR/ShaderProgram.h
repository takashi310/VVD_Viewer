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

#ifndef ShaderProgram_h 
#define ShaderProgram_h

#include <string>
#include <memory>

#include "VVulkan.h"

#if (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK))
#include <MoltenVKGLSLToSPIRVConverter/GLSLToSPIRVConverter.h>
#else
#include "SPIRV/GlslangToSpv.h"
#endif

#include "DLLExport.h"

namespace FLIVR
{

	class EXPORT_API ShaderProgram
	{
	public:
		ShaderProgram(const std::string& vert_shader,const std::string& frag_shader);
		ShaderProgram(const std::string& frag_shader);
		~ShaderProgram();

		unsigned int id();
		bool create();
		bool valid();
		void destroy();

		VkPipelineShaderStageCreateInfo get_vertex_shader() { return vert_shader_stage_; }
		VkPipelineShaderStageCreateInfo get_fragment_shader() { return frag_shader_stage_; }
		std::string get_vertex_shader_code() { return vert_shader_; }
		std::string get_fragment_shader_code() { return frag_shader_; }

		// Call init_shaders_supported before shaders_supported queries!
		static bool init();
		static void init_shaders_supported(std::shared_ptr<VVulkan> vulkan);
		static void finalize_shaders_supported();
		static bool shaders_supported();
		static int max_texture_size();
		static bool texture_non_power_of_two();
		static const int MAX_SHADER_UNIFORMS = 16;
		static string glsl_version_;

#if !(defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK))
		static EShLanguage FindLanguage(const VkShaderStageFlagBits shader_type);
		static void init_resources(TBuiltInResource &Resources);
#endif
		static bool GLSLtoSPV(const VkShaderStageFlagBits shader_type, const char *pshader, std::vector<unsigned int> &spirv);
		static void init_glslang();
		static void finalize_glslang();
		static const std::shared_ptr<VVulkan> get_vulkan() { return vulkan_; }

	protected:
		unsigned int id_;
		std::string  vert_shader_;
		std::string  frag_shader_;
		VkPipelineShaderStageCreateInfo vert_shader_stage_;
		VkPipelineShaderStageCreateInfo frag_shader_stage_;

		//validation
		bool valid_;

		static bool init_;
		static bool supported_;
		static bool non_2_textures_;
		static int  max_texture_size_;

		static std::shared_ptr<VVulkan> vulkan_;
	};

} // end namespace FLIVR

#endif // ShaderProgram_h
