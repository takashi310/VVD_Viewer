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
#include <FLIVR/VolSliceShader.h>
#include <FLIVR/ShaderProgram.h>

using std::string;
using std::vector;
using std::ostringstream;

namespace FLIVR
{

#define VOLSLICE_HEAD \
	"#pragma optionNV(inline all)\n" \
	"#pragma optionNV(fastmath on)\n" \
	"#pragma optionNV(ifcvt none)\n" \
	"#pragma optionNV(strict on)\n" \
	"#pragma optionNV(unroll all)\n" \
	"layout (local_size_x = 64) in;\n" \
	"struct Vertex {\n" \
	"	vec3 p;\n" \
	"	vec3 t;\n" \
	"};\n" \
	"struct VertexOut {\n" \
	"	vec4 p;\n" \
	"	vec4 t;\n" \
	"};\n" \
	"layout (std430, binding = 0) buffer Vertbuf {\n" \
	"	VertexOut vertbuf[ ];\n" \
	"};\n" \
	"layout (std430, binding = 1) buffer Idxbuf {\n" \
	"	uint idbuf[ ];\n" \
	"};\n" \
	"layout (push_constant) uniform VolSliceShaderBrickConst {\n" \
	"	vec4 b_min_tmin;\n" \
	"	vec4 b_max_tmax;\n" \
	"	vec4 tb_min;\n" \
	"	vec4 tb_max;\n" \
	"	vec4 view_origin_dt;\n" \
	"	vec4 view_direction;\n" \
	"	vec4 up;\n" \
	"	uvec4 range;\n" \
	"} brk;\n" \
	"\n"

#define VOLSLICE_FORWARD \
	"//VOLSLICE_FORWARD\n" \
	"void main()\n" \
	"{\n" \
	"	uint id = gl_GlobalInvocationID.x;\n" \
	"	if (id >= brk.range.x)\n"\
	"	{\n"\
	"		return;\n"\
	"	}\n"\
	"	\n"\
	"	Vertex vv[12];\n" \
	"	uint ii[6];\n" \
	"	uint degree = 0;\n" \
	"	const vec3 vec = -brk.view_direction.xyz;\n" \
	"	const vec3 pnt = brk.view_origin_dt.xyz + brk.view_direction.xyz * (brk.b_min_tmin.w + id * brk.view_origin_dt.w);\n" \
	"	const vec3 right = cross(brk.view_direction.xyz, brk.up.xyz);\n" \
	"	Vertex edge[12];\n" \
	"	Vertex tedge[12];\n" \
	"	float u[12];\n" \
	"	for (uint j = 0; j < 6; j++)\n" \
	"	{\n" \
	"		vv[j].p = vec3(0.0);\n" \
	"		vv[j].t = vec3(0.0);\n" \
	"		ii[j] = 0;\n" \
	"	}\n" \
	"	\n" \
	"	edge[0].p = vec3(brk.b_min_tmin.x, brk.b_min_tmin.y, brk.b_min_tmin.z);\n" \
	"	edge[0].t = vec3(0.0, 0.0, brk.b_max_tmax.z-brk.b_min_tmin.z);\n" \
	"	edge[1].p = vec3(brk.b_max_tmax.x, brk.b_min_tmin.y, brk.b_min_tmin.z);\n" \
	"	edge[1].t = vec3(0.0, 0.0, brk.b_max_tmax.z-brk.b_min_tmin.z);\n" \
	"	edge[2].p = vec3(brk.b_max_tmax.x, brk.b_max_tmax.y, brk.b_min_tmin.z);\n" \
	"	edge[2].t = vec3(0.0, 0.0, brk.b_max_tmax.z-brk.b_min_tmin.z);\n" \
	"	edge[3].p = vec3(brk.b_min_tmin.x, brk.b_max_tmax.y, brk.b_min_tmin.z);\n" \
	"	edge[3].t = vec3(0.0, 0.0, brk.b_max_tmax.z-brk.b_min_tmin.z);\n" \
	"	edge[4].p = vec3(brk.b_min_tmin.x, brk.b_min_tmin.y, brk.b_min_tmin.z);\n" \
	"	edge[4].t = vec3(brk.b_max_tmax.x-brk.b_min_tmin.x, 0.0, 0.0);\n" \
	"	edge[5].p = vec3(brk.b_max_tmax.x, brk.b_min_tmin.y, brk.b_min_tmin.z);\n" \
	"	edge[5].t = vec3(0.0, brk.b_max_tmax.y-brk.b_min_tmin.y, 0.0);\n" \
	"	edge[6].p = vec3(brk.b_min_tmin.x, brk.b_max_tmax.y, brk.b_min_tmin.z);\n" \
	"	edge[6].t = vec3(brk.b_max_tmax.x-brk.b_min_tmin.x, 0.0, 0.0);\n" \
	"	edge[7].p = vec3(brk.b_min_tmin.x, brk.b_min_tmin.y, brk.b_min_tmin.z);\n" \
	"	edge[7].t = vec3(0.0, brk.b_max_tmax.y-brk.b_min_tmin.y, 0.0);\n" \
	"	edge[8].p = vec3(brk.b_min_tmin.x, brk.b_min_tmin.y, brk.b_max_tmax.z);\n" \
	"	edge[8].t = vec3(brk.b_max_tmax.x-brk.b_min_tmin.x, 0.0, 0.0);\n" \
	"	edge[9].p = vec3(brk.b_max_tmax.x, brk.b_min_tmin.y, brk.b_max_tmax.z);\n" \
	"	edge[9].t = vec3(0.0, brk.b_max_tmax.y-brk.b_min_tmin.y, 0.0);\n" \
	"	edge[10].p = vec3(brk.b_min_tmin.x, brk.b_max_tmax.y, brk.b_max_tmax.z);\n" \
	"	edge[10].t = vec3(brk.b_max_tmax.x-brk.b_min_tmin.x, 0.0, 0.0);\n" \
	"	edge[11].p = vec3(brk.b_min_tmin.x, brk.b_min_tmin.y, brk.b_max_tmax.z);\n" \
	"	edge[11].t = vec3(0.0, brk.b_max_tmax.y-brk.b_min_tmin.y, 0.0);\n" \
	"	\n" \
	"	tedge[0].p = vec3(brk.tb_min.x, brk.tb_min.y, brk.tb_min.z);\n" \
	"	tedge[0].t = vec3(0.0, 0.0, brk.tb_max.z-brk.tb_min.z);\n" \
	"	tedge[1].p = vec3(brk.tb_max.x, brk.tb_min.y, brk.tb_min.z);\n" \
	"	tedge[1].t = vec3(0.0, 0.0, brk.tb_max.z-brk.tb_min.z);\n" \
	"	tedge[2].p = vec3(brk.tb_max.x, brk.tb_max.y, brk.tb_min.z);\n" \
	"	tedge[2].t = vec3(0.0, 0.0, brk.tb_max.z-brk.tb_min.z);\n" \
	"	tedge[3].p = vec3(brk.tb_min.x, brk.tb_max.y, brk.tb_min.z);\n" \
	"	tedge[3].t = vec3(0.0, 0.0, brk.tb_max.z-brk.tb_min.z);\n" \
	"	tedge[4].p = vec3(brk.tb_min.x, brk.tb_min.y, brk.tb_min.z);\n" \
	"	tedge[4].t = vec3(brk.tb_max.x-brk.tb_min.x, 0.0, 0.0);\n" \
	"	tedge[5].p = vec3(brk.tb_max.x, brk.tb_min.y, brk.tb_min.z);\n" \
	"	tedge[5].t = vec3(0.0, brk.tb_max.y-brk.tb_min.y, 0.0);\n" \
	"	tedge[6].p = vec3(brk.tb_min.x, brk.tb_max.y, brk.tb_min.z);\n" \
	"	tedge[6].t = vec3(brk.tb_max.x-brk.tb_min.x, 0.0, 0.0);\n" \
	"	tedge[7].p = vec3(brk.tb_min.x, brk.tb_min.y, brk.tb_min.z);\n" \
	"	tedge[7].t = vec3(0.0, brk.tb_max.y-brk.tb_min.y, 0.0);\n" \
	"	tedge[8].p = vec3(brk.tb_min.x, brk.tb_min.y, brk.tb_max.z);\n" \
	"	tedge[8].t = vec3(brk.tb_max.x-brk.tb_min.x, 0.0, 0.0);\n" \
	"	tedge[9].p = vec3(brk.tb_max.x, brk.tb_min.y, brk.tb_max.z);\n" \
	"	tedge[9].t = vec3(0.0, brk.tb_max.y-brk.tb_min.y, 0.0);\n" \
	"	tedge[10].p = vec3(brk.tb_min.x, brk.tb_max.y, brk.tb_max.z);\n" \
	"	tedge[10].t = vec3(brk.tb_max.x-brk.tb_min.x, 0.0, 0.0);\n" \
	"	tedge[11].p = vec3(brk.tb_min.x, brk.tb_min.y, brk.tb_max.z);\n" \
	"	tedge[11].t = vec3(0.0, brk.tb_max.y-brk.tb_min.y, 0.0);\n" \
	"	\n" \
	"	for (uint j = 0; j <= 11; j++)\n" \
	"	{\n" \
	"		float d = -(vec.x*pnt.x + vec.y*pnt.y + vec.z*pnt.z);\n" \
	"		float no = (vec.x*edge[j].p.x + vec.y*edge[j].p.y + vec.z*edge[j].p.z);\n" \
	"		float nv = dot(vec, edge[j].t);\n" \
	"		u[j] = (abs(nv) > 0.0) ? -(d + no)/nv : -1.0;\n" \
	"	}\n" \
	"	for (uint j = 0; j <= 3; j++)\n" \
	"	{\n" \
	"		if (u[j] >= 0.0 && u[j] <= 1.0)\n" \
	"		{\n" \
	"			vv[degree].p = edge[j].p + u[j]*edge[j].t;\n" \
	"			vv[degree].t = tedge[j].p + u[j]*tedge[j].t;\n" \
	"			degree++;\n" \
	"		}\n" \
	"	}\n" \
	"	for (uint j = 4; j <= 11; j++)\n" \
	"	{\n" \
	"		if (u[j] > 0.0 && u[j] < 1.0)\n" \
	"		{\n" \
	"			vv[degree].p = edge[j].p + u[j]*edge[j].t;\n" \
	"			vv[degree].t = tedge[j].p + u[j]*tedge[j].t;\n" \
	"			degree++;\n" \
	"		}\n" \
	"	}\n" \
	"	\n" \
	"	if (degree < 3 || degree >6)\n" \
	"	{\n" \
	"		for (uint j = 0; j < 6; j++)\n" \
	"		{\n" \
	"			vertbuf[6*id + j].p = vec4(0.0);\n" \
	"			vertbuf[6*id + j].t = vec4(0.0);\n" \
	"			idbuf[12*id + 2*j] = 6*id;\n" \
	"			idbuf[12*id + 2*j+1] = 6*id;\n" \
	"		}\n" \
	"	}\n" \
	"	else \n" \
	"	{\n" \
	"		for (uint j = 0; j < 6; j++)\n" \
	"		{\n" \
	"			ii[j] = j;\n" \
	"		}\n" \
	"		if (degree > 3)\n" \
	"		{\n" \
	"			// compute centroids\n" \
	"			vec3 vc = vec3(0.0);\n" \
	"			for (uint j = 0; j < 6; j++)\n" \
	"			{\n" \
	"				vc += vv[j].p;\n" \
	"			}\n" \
	"			vc = vc / degree;\n" \
	"			// sort vertices\n" \
	"			float pa[6]; \n" \
	"			for (uint i = 0; i < degree; i++)\n" \
	"			{\n" \
	"				float vx = dot(vv[i].p - vc, right);\n" \
	"				float vy = dot(vv[i].p - vc, brk.up.xyz);\n" \
	"				// compute pseudo-angle\n" \
	"				pa[i] = vy / (abs(vx) + abs(vy));\n" \
	"				if (vx < 0.0) pa[i] = 2.0 - pa[i];\n" \
	"				else if (vy < 0.0) pa[i] = 4.0 + pa[i];\n" \
	"			}\n" \
	"			for (uint i = 0; i < degree; i++)\n" \
	"			{\n" \
	"				for (uint j = i+1; j < degree; j++)\n" \
	"				{\n" \
	"					if (pa[j] < pa[i])\n" \
	"					{\n" \
	"						float ftmp = pa[i];\n" \
	"						pa[i] = pa[j];\n" \
	"						pa[j] = ftmp;\n" \
	"						uint tmp = ii[i];\n" \
	"						ii[i] = ii[j];\n" \
	"						ii[j] = tmp;\n" \
	"					}\n" \
	"				}\n" \
	"			}\n" \
	"		}\n" \
	"		for (uint j = degree; j < 6; j++)\n" \
	"		{\n" \
	"			ii[j] = ii[0];\n" \
	"		}\n" \
	"		for (uint j = 0; j < 6; j++)\n" \
	"		{\n" \
	"			vertbuf[6*id + j].p = vec4(vv[j].p, 0.0);\n" \
	"			vertbuf[6*id + j].t = vec4(vv[j].t, 0.0);\n" \
	"		}\n" \
	"		for (uint j = 1; j < 5; j++)\n" \
	"		{\n" \
	"			idbuf[12*id + 3*(j-1)] = 6*id + ii[0];\n" \
	"			idbuf[12*id + 3*(j-1)+1] = 6*id + ii[j];\n" \
	"			idbuf[12*id + 3*(j-1)+2] = 6*id + ii[j+1];\n" \
	"		}\n" \
	"	}\n" \
	"}\n"

#define VOLSLICE_BACKWARD \
	"//VOLSLICE_FORWARD\n" \
	"	const vec3 pnt = brk.view_origin_dt.xyz + brk.view_direction.xyz * (brk.b_max_tmax.w - id * brk.view_origin_dt.w);\n" 

#define VOLSLICE_TEST \
"void main()\n" \
	"{\n" \
	"	uint id = gl_GlobalInvocationID.x;\n" \
	"	if (id >= brk.range.x)\n"\
	"	{\n"\
	"		return;\n"\
	"	}\n"\
	"	\n"\
	"	const vec3 pnt = vec3(0.0, 0.0, 1.0) * id * brk.view_origin_dt.w;\n" \
	"	vertbuf[6*id + 0].p = vec3(0.0, 0.0, pnt.z);\n" \
	"	vertbuf[6*id + 0].t = vec3(0.0, 0.0, pnt.z);\n" \
	"	vertbuf[6*id + 1].p = vec3(1.0, 0.0, pnt.z);\n" \
	"	vertbuf[6*id + 1].t = vec3(1.0, 0.0, pnt.z);\n" \
	"	vertbuf[6*id + 2].p = vec3(1.0, 1.0, pnt.z);\n" \
	"	vertbuf[6*id + 2].t = vec3(1.0, 1.0, pnt.z);\n" \
	"	vertbuf[6*id + 3].p = vec3(0.0, 1.0, pnt.z);\n" \
	"	vertbuf[6*id + 3].t = vec3(0.0, 1.0, pnt.z);\n" \
	"	vertbuf[6*id + 4].p = vec3(1.0, 1.0, pnt.z);\n" \
	"	vertbuf[6*id + 4].t = vec3(1.0, 1.0, pnt.z);\n" \
	"	vertbuf[6*id + 5].p = vec3(1.0, 0.0, pnt.z);\n" \
	"	vertbuf[6*id + 5].t = vec3(1.0, 0.0, pnt.z);\n" \
	"	for (uint j = 1; j < 5; j++)\n" \
	"	{\n" \
	"		idbuf[12*id + 3*(j-1)] = 6*id + 0;\n" \
	"		idbuf[12*id + 3*(j-1)+1] = 6*id + j;\n" \
	"		idbuf[12*id + 3*(j-1)+2] = 6*id + j+1;\n" \
	"	}\n" \
	"}\n" 

#define VOLSLICE_TEST2 \
"void main()\n" \
	"{\n" \
	"	uint id = gl_GlobalInvocationID.x;\n" \
	"	if (id >= brk.range.x)\n"\
	"	{\n"\
	"		return;\n"\
	"	}\n"\
	"	\n"\
	"	const vec3 pnt = vec3(0.0, 0.0, 1.0) * id * brk.view_origin_dt.w;\n" \
	"	vertbuf[4*id + 0].p = vec3(0.0, 0.0, pnt.z);\n" \
	"	vertbuf[4*id + 0].t = vec3(0.0, 0.0, pnt.z);\n" \
	"	vertbuf[4*id + 1].p = vec3(1.0, 0.0, pnt.z);\n" \
	"	vertbuf[4*id + 1].t = vec3(1.0, 0.0, pnt.z);\n" \
	"	vertbuf[4*id + 2].p = vec3(1.0, 1.0, pnt.z);\n" \
	"	vertbuf[4*id + 2].t = vec3(1.0, 1.0, pnt.z);\n" \
	"	vertbuf[4*id + 3].p = vec3(0.0, 1.0, pnt.z);\n" \
	"	vertbuf[4*id + 3].t = vec3(0.0, 1.0, pnt.z);\n" \
	"	for (uint j = 1; j < 3; j++)\n" \
	"	{\n" \
	"		idbuf[6*id + 3*(j-1)] = 6*id + 0;\n" \
	"		idbuf[6*id + 3*(j-1)+1] = 6*id + j;\n" \
	"		idbuf[6*id + 3*(j-1)+2] = 6*id + j+1;\n" \
	"	}\n" \
	"}\n" \

	VolSliceShader::VolSliceShader(VkDevice device, int order) :
	device_(device),
	order_(order)
	{}

	VolSliceShader::~VolSliceShader()
	{
		delete program_;
	}

	bool VolSliceShader::create()
	{
		string cs;
		if (emit(cs)) return true;
		program_ = new ShaderProgram(cs);
		program_->create(device_);
		return false;
	}

	bool VolSliceShader::emit(string& s)
	{
		ostringstream z;

		z << ShaderProgram::glsl_version_;
		z << VOLSLICE_HEAD;

		//uniforms
		switch (order_)
		{
		case VOL_SLICE_ORDER_FORWARD:
			z << VOLSLICE_FORWARD;
			break;
		case VOL_SLICE_ORDER_BACKWARD:
			z << VOLSLICE_BACKWARD;
			break;
		}
		
		s = z.str();
		return false;
	}

	VolSliceShaderFactory::VolSliceShaderFactory()
		: prev_shader_(-1)
	{}

	VolSliceShaderFactory::VolSliceShaderFactory(std::vector<vks::VulkanDevice*> &devices)
		: prev_shader_(-1)
	{
		init(devices);
	}

	void VolSliceShaderFactory::init(std::vector<vks::VulkanDevice*> &devices)
	{
		vdevices_ = devices;
		setupDescriptorSetLayout();
	}

	VolSliceShaderFactory::~VolSliceShaderFactory()
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

	ShaderProgram* VolSliceShaderFactory::shader(VkDevice device, int order)
	{
		if(prev_shader_ >= 0)
		{
			if(shader_[prev_shader_]->match(device, order))
			{
				return shader_[prev_shader_]->program();
			}
		}
		for(unsigned int i=0; i<shader_.size(); i++)
		{
			if(shader_[i]->match(device, order))
			{
				prev_shader_ = i;
				return shader_[i]->program();
			}
		}

		VolSliceShader* s = new VolSliceShader(device, order);
		if(s->create())
		{
			delete s;
			return 0;
		}
		shader_.push_back(s);
		prev_shader_ = (int)shader_.size()-1;
		return s->program();
	}
	
	void VolSliceShaderFactory::setupDescriptorSetLayout()
	{
		for (auto vdev : vdevices_)
		{
			VkDevice device = vdev->logicalDevice;


			std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = 
			{
				//vertex buffer
				vks::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				0),
				
				//index buffer
				vks::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				1),
			};
						
			VkDescriptorSetLayoutCreateInfo descriptorLayout = 
				vks::initializers::descriptorSetLayoutCreateInfo(
				setLayoutBindings.data(),
				static_cast<uint32_t>(setLayoutBindings.size()));

			descriptorLayout.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
			descriptorLayout.pNext = nullptr;

			VolSlicePipeline pipe;
			VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &pipe.descriptorSetLayout));

			VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
				vks::initializers::pipelineLayoutCreateInfo(
				&pipe.descriptorSetLayout,
				1);

			VkPushConstantRange pushConstantRange = {};
			pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			pushConstantRange.size = sizeof(VolSliceShaderFactory::VolSliceShaderBrickConst);
			pushConstantRange.offset = 0;

			// Push constant ranges are part of the pipeline layout
			pPipelineLayoutCreateInfo.pushConstantRangeCount = 1;
			pPipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

			VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipe.pipelineLayout));

			pipeline_[vdev] = pipe;
		}
	}

} // end namespace FLIVR

