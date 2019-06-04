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

#include <string>
#include <sstream>
#include <iostream>
#include <FLIVR/PaintShader.h>
#include <FLIVR/ShaderProgram.h>

using std::string;
using std::vector;
using std::ostringstream;

namespace FLIVR {

#define PAINT_SHADER_CODE \
	"// PAINT_SHADER_CODE\n" \
	"layout (push_constant) uniform PushConsts {" \
	"	vec4 loc0; //(mouse_x, mouse_y, radius1, radius2)\n" \
	"	vec4 loc1; //(width, height, 0, 0)\n" \
	"} ct;" \
	"layout (binding = 0) uniform sampler2D tex0;\n" \
	"\n" \
	"void main()\n" \
	"{\n" \
	"	vec4 t = gl_TexCoord[0];\n" \
	"	vec4 c = texture2D(tex0, t.xy);\n" \
	"	vec2 center = vec2(ct.loc0.x, ct.loc0.y);\n" \
	"	vec2 pos = vec2(t.x*ct.loc1.x, t.y*ct.loc1.y);\n" \
	"	float d = length(pos - center);\n" \
	"	vec4 ctemp = d<ct.loc0.z?vec4(1.0, 1.0, 1.0, 1.0):(d<ct.loc0.w?vec4(0.5, 0.5, 0.5, 1.0):vec4(0.0, 0.0, 0.0, 1.0));\n" \
	"	gl_FragColor = ctemp.r>c.r?ctemp:c;\n" \
	"	gl_FragColor.a = gl_FragColor.r>0.1?0.5:0.0;\n" \
	"}\n"

	/*"	gl_FragColor = pow(c, loc0)*b;\n" \*/
	PaintShader::PaintShader() : program_(0)
	{}

	PaintShader::~PaintShader()
	{
		delete program_;
	}

	bool
		PaintShader::create()
	{
		string s;
		if (emit(s)) return true;
		program_ = new ShaderProgram(s);
		return false;
	}

	bool
		PaintShader::emit(string& s)
	{
		ostringstream z;

		z << PAINT_SHADER_CODE;

		s = z.str();

		return false;
	}

	PaintShaderFactory::PaintShaderFactory()
		: prev_shader_(-1)
	{}

	PaintShaderFactory::PaintShaderFactory(std::vector<vks::VulkanDevice*> &devices)
		: prev_shader_(-1)
	{
		init(devices);
	}

	void PaintShaderFactory::init(std::vector<vks::VulkanDevice*> &devices)
	{
		vdevices_ = devices;
		setupDescriptorSetLayout();
	}

	PaintShaderFactory::~PaintShaderFactory()
	{
		for(unsigned int i=0; i<shader_.size(); i++)
			delete shader_[i];

		for (auto vdev : vdevices_)
		{
			VkDevice device = vdev->logicalDevice;

			vkDestroyPipelineLayout(device, pipeline_[vdev].pipelineLayout, nullptr);
			vkDestroyDescriptorSetLayout(device, pipeline_[vdev].descriptorSetLayout, nullptr);
		}
	}

	ShaderProgram* PaintShaderFactory::shader()
	{
		if(prev_shader_ >= 0)
		{
			if(shader_[prev_shader_]->match()) 
			{
				return shader_[prev_shader_]->program();
			}
		}
		for(unsigned int i=0; i<shader_.size(); i++)
		{
			if(shader_[i]->match()) 
			{
				prev_shader_ = i;
				return shader_[i]->program();
			}
		}

		PaintShader* s = new PaintShader();
		if(s->create())
		{
			delete s;
			return 0;
		}
		shader_.push_back(s);
		prev_shader_ = (int)shader_.size()-1;
		return s->program();
	}

	
	void PaintShaderFactory::setupDescriptorSetLayout()
	{
		for (auto vdev : vdevices_)
		{
			VkDevice device = vdev->logicalDevice;

			std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {};

			int offset = 0;
			for (int i = 0; i < PAINT_SAMPLER_NUM; i++)
			{
				setLayoutBindings.push_back(
					vks::initializers::descriptorSetLayoutBinding(
					VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
					VK_SHADER_STAGE_FRAGMENT_BIT, 
					offset+i)
					);
			}
			
			VkDescriptorSetLayoutCreateInfo descriptorLayout = 
				vks::initializers::descriptorSetLayoutCreateInfo(
				setLayoutBindings.data(),
				static_cast<uint32_t>(setLayoutBindings.size()));

			descriptorLayout.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
			descriptorLayout.pNext = nullptr;

			PaintPipeline pipe;
			VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &pipe.descriptorSetLayout));

			VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
				vks::initializers::pipelineLayoutCreateInfo(
				&pipe.descriptorSetLayout,
				1);

			VkPushConstantRange pushConstantRange = {};
			pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			pushConstantRange.size = sizeof(float) * 4 * 2;
			pushConstantRange.offset = 0;

			// Push constant ranges are part of the pipeline layout
			pPipelineLayoutCreateInfo.pushConstantRangeCount = 1;
			pPipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

			VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipe.pipelineLayout));

			pipeline_[vdev] = pipe;
		}
	}

} // end namespace FLIVR

