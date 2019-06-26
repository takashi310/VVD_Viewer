/*
For more information, please see: http://software.sci.utah.edu

The MIT License

Copyright (c) 2004 Scientific Computing and Imaging Institute,
University of Utah.


Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#include <string>
#include <sstream>
#include <iostream>
#include <FLIVR/VolCalShader.h>
#include <FLIVR/ShaderProgram.h>
#include <FLIVR/VolShaderCode.h>

using std::string;
using std::vector;
using std::ostringstream;

namespace FLIVR
{
#define CAL_INPUTS \
	"#pragma optionNV(inline all)\n" \
	"#pragma optionNV(fastmath on)\n" \
	"#pragma optionNV(ifcvt none)\n" \
	"#pragma optionNV(strict on)\n" \
	"#pragma optionNV(unroll all)\n" \
	"layout (local_size_x = 4, local_size_y = 4, local_size_z = 4) in;\n" \

#define CAL_OUTPUTS \
	"//CAL_INOUT\n" \
	"layout (binding = 5, r8) uniform image3D outimg;\n"

#define CAL_OUTPUTS_16BIT \
	"//CAL_OUTPUTS_16BIT\n" \
	"layout (binding = 5, r16) uniform image3D outimg;\n"

#define CAL_UNIFORMS_COMMON \
	"//CAL_UNIFORMS_COMMON\n" \
	"layout (binding = 1) uniform sampler3D tex1;//operand A\n" \
	"layout (binding = 2) uniform sampler3D tex2;//operand B\n" \
	"layout (push_constant) uniform PushConsts {\n" \
	"	vec4 loc0;//(scale_A, scale_B, 0.0, 0.0)\n" \
	"} ct;" \
	"\n"

#define CAL_UNIFORMS_WITH_MASK \
	"//CAL_UNIFORMS_WITH_MASK\n" \
	"layout (binding = 1) uniform sampler3D tex1;//operand A\n" \
	"layout (binding = 3) uniform sampler3D tex3;//mask of A\n" \
	"layout (binding = 2) uniform sampler3D tex2;//operand B\n" \
	"layout (binding = 4) uniform sampler3D tex4;//mask of B\n" \
	"layout (push_constant) uniform PushConsts {\n" \
	"	vec4 loc0;//(scale_a, scale_b, use_mask_a, use_mask_b)\n" \
	"} ct;" \
	"\n"

#define CAL_HEAD \
	"//SEG_HEAD\n" \
	"void main()\n" \
	"{\n" \
	"	vec4 t = vec4((gl_GlobalInvocationID.x+0.5)*brk.loc4.x, (gl_GlobalInvocationID.y+0.5)*brk.loc4.y, (gl_GlobalInvocationID.z+0.5)*brk.loc4.z, 1.0);\n" \
	"	vec4 t1 = t;//position in the operand A\n" \
	"	vec4 t2 = t;//position in the operand B\n" \
	"\n"

#define CAL_TEX_LOOKUP \
	"	//CAL_TEX_LOOKUP\n" \
	"	vec4 c1 = texture(tex1, t1.stp)*ct.loc0.x;\n" \
	"	vec4 c2 = texture(tex2, t2.stp)*ct.loc0.y;\n" \
	"\n"

#define CAL_TEX_LOOKUP_WITH_MASK \
	"	//CAL_TEX_LOOKUP_WITH_MASK\n" \
	"	vec4 c1 = texture3D(tex1, t1.stp)*ct.loc0.x;\n" \
	"	vec4 m1 = ct.loc0.z>0.0?texture(tex3, t1.stp):vec4(1.0);\n" \
	"	vec4 c2 = texture(tex2, t2.stp)*ct.loc0.y;\n" \
	"	vec4 m2 = ct.loc0.w>0.0?texture(tex4, t2.stp):vec4(1.0);\n" \
	"\n"

#define CAL_BODY_SUBSTRACTION \
	"	//CAL_BODY_SUBSTRACTION\n" \
	"	vec4 c = vec4(clamp(c1.x-c2.x, 0.0, 1.0));\n" \
	"\n"

#define CAL_BODY_ADDITION \
	"	//CAL_BODY_ADDITION\n" \
	"	vec4 c = vec4(max(c1.x, c2.x));\n" \
	"\n"

#define CAL_BODY_DIVISION \
	"	//CAL_BODY_DIVISION\n" \
	"	vec4 c = vec4(0.0);\n" \
	"	if (c1.x>1e-5 && c2.x>1e-5)\n" \
	"		c = vec4(clamp(c1.x/c2.x, 0.0, 1.0));\n" \
	"\n"

#define CAL_BODY_INTERSECTION \
	"	//CAL_BODY_INTERSECTION\n" \
	"	vec4 c = vec4(min(c1.x, c2.x));\n" \
	"\n"

#define CAL_BODY_INTERSECTION_WITH_MASK \
	"	//CAL_BODY_INTERSECTION_WITH_MASK\n" \
	"	vec4 c = vec4(min(c1.x*m1.x, c2.x*m2.x));\n" \
	"\n"

#define CAL_BODY_APPLYMASK \
	"	//CAL_BODY_APPLYMASK\n" \
	"	vec4 c = vec4((ct.loc0.z<0.0?(1.0-c1.x):c1.x)*c2.x);\n" \
	"\n"

#define CAL_BODY_APPLYMASKINV \
	"	//CAL_BODY_APPLYMASKINV\n" \
	"	vec4 c = vec4(c1.x*(1.0-c2.x));\n" \
	"\n"

#define CAL_RESULT \
	"	//CAL_RESULT\n" \
	"	imageStore(outimg, ivec3(gl_GlobalInvocationID.xyz), c);\n" \
	"\n"

#define CAL_TAIL \
	"//CAL_TAIL\n" \
	"}\n" \
	"\n"

	VolCalShader::VolCalShader(VkDevice device, int type, int out_bytes) :
	device_(device),
	type_(type),
	out_bytes_(out_bytes),
	program_(0)
	{}

	VolCalShader::~VolCalShader()
	{
		delete program_;
	}

	bool VolCalShader::create()
	{
		string cs;
		if (emit(cs)) return true;
		program_ = new ShaderProgram(cs);
		program_->create(device_);
		return false;
	}

	bool VolCalShader::emit(string& s)
	{
		ostringstream z;

		z << ShaderProgram::glsl_version_;
		z << CAL_INPUTS;
		if (out_bytes_ == 2) z << CAL_OUTPUTS_16BIT;
		else z << CAL_OUTPUTS;

		switch (type_)
		{
		case CAL_SUBSTRACTION:
		case CAL_ADDITION:
		case CAL_DIVISION:
		case CAL_INTERSECTION:
		case CAL_APPLYMASK:
		case CAL_APPLYMASKINV:
		case CAL_APPLYMASKINV2:
			z << CAL_UNIFORMS_COMMON;
			break;
		case CAL_INTERSECTION_WITH_MASK:
			z << CAL_UNIFORMS_WITH_MASK;
			break;
		}

		z << CAL_HEAD;

		if (type_ == CAL_INTERSECTION_WITH_MASK)
			z << CAL_TEX_LOOKUP_WITH_MASK;
		else
			z << CAL_TEX_LOOKUP;

		switch (type_)
		{
		case CAL_SUBSTRACTION:
			z << CAL_BODY_SUBSTRACTION;
			break;
		case CAL_ADDITION:
			z << CAL_BODY_ADDITION;
			break;
		case CAL_DIVISION:
			z << CAL_BODY_DIVISION;
			break;
		case CAL_INTERSECTION:
			z << CAL_BODY_INTERSECTION;
			break;
		case CAL_APPLYMASK:
			z << CAL_BODY_APPLYMASK;
			break;
		case CAL_APPLYMASKINV:
		case CAL_APPLYMASKINV2:
			z << CAL_BODY_APPLYMASKINV;
			break;
		case CAL_INTERSECTION_WITH_MASK:
			z << CAL_BODY_INTERSECTION_WITH_MASK;
			break;
		}

		z << CAL_RESULT;
		z << CAL_TAIL;

		s = z.str();
		return false;
	}


	VolCalShaderFactory::VolCalShaderFactory()
		: prev_shader_(-1)
	{}

	VolCalShaderFactory::VolCalShaderFactory(std::vector<vks::VulkanDevice*> &devices)
		: prev_shader_(-1)
	{
		init(devices);
	}

	void VolCalShaderFactory::init(std::vector<vks::VulkanDevice*> &devices)
	{
		vdevices_ = devices;
		setupDescriptorSetLayout();
	}

	VolCalShaderFactory::~VolCalShaderFactory()
	{
		for(unsigned int i=0; i<shader_.size(); i++)
		{
			delete shader_[i];
		}
		
		for (auto vdev : vdevices_)
		{
			VkDevice device = vdev->logicalDevice;

			vkDestroyPipelineLayout(device, pipeline_[vdev].pipelineLayout, nullptr);
			vkDestroyDescriptorSetLayout(device, pipeline_[vdev].descriptorSetLayout, nullptr);
		}
	}

	ShaderProgram* VolCalShaderFactory::shader(VkDevice device, int type, int out_bytes)
	{
		if(prev_shader_ >= 0)
		{
			if(shader_[prev_shader_]->match(device, type, out_bytes)) 
			{
				return shader_[prev_shader_]->program();
			}
		}
		for(unsigned int i=0; i<shader_.size(); i++)
		{
			if(shader_[i]->match(device, type, out_bytes)) 
			{
				prev_shader_ = i;
				return shader_[i]->program();
			}
		}

		VolCalShader* s = new VolCalShader(device, type, out_bytes);
		if(s->create())
		{
			delete s;
			return 0;
		}
		shader_.push_back(s);
		prev_shader_ = int(shader_.size())-1;
		return s->program();
	}

	
	void VolCalShaderFactory::setupDescriptorSetLayout()
	{
		for (auto vdev : vdevices_)
		{
			VkDevice device = vdev->logicalDevice;

			std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {};

			int offset = 0;
			for (int i = 0; i < CAL_SAMPLER_NUM; i++)
			{
				setLayoutBindings.push_back(
					vks::initializers::descriptorSetLayoutBinding(
					VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
					VK_SHADER_STAGE_COMPUTE_BIT,
					offset+i)
					);
			}
			offset += CAL_SAMPLER_NUM;

			//output image
			setLayoutBindings.push_back(
				vks::initializers::descriptorSetLayoutBinding(
					VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
					VK_SHADER_STAGE_COMPUTE_BIT,
					offset)
			);
			
			VkDescriptorSetLayoutCreateInfo descriptorLayout = 
				vks::initializers::descriptorSetLayoutCreateInfo(
				setLayoutBindings.data(),
				static_cast<uint32_t>(setLayoutBindings.size()));
			
			descriptorLayout.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
			descriptorLayout.pNext = nullptr;

			VolCalPipeline pipe;
			VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &pipe.descriptorSetLayout));

			VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
				vks::initializers::pipelineLayoutCreateInfo(
				&pipe.descriptorSetLayout,
				1);

			VkPushConstantRange pushConstantRange = {};
			pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			pushConstantRange.size = sizeof(CalCompShaderBrickConst);
			pushConstantRange.offset = 0;

			// Push constant ranges are part of the pipeline layout
			pPipelineLayoutCreateInfo.pushConstantRangeCount = 1;
			pPipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

			VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipe.pipelineLayout));

			pipeline_[vdev] = pipe;
		}
	}

} // end namespace FLIVR
