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
#include <regex>
#include <FLIVR/VRayShader.h>
#include <FLIVR/ShaderProgram.h>
#include <FLIVR/VolShaderCode.h>

using std::string;
using std::vector;
using std::ostringstream;

namespace FLIVR
{
#define CORE_PROFILE_VTX_SHADER 1

#define VRAY_VTX_SHADER_CODE_CORE_PROFILE \
	"//VTX_SHADER_CODE_CORE_PROFILE\n" \
	"layout (binding = 0) uniform VolVertShaderUBO {\n" \
	"	mat4 matrix0; //projection matrix\n" \
	"	mat4 matrix1; //modelview matrix\n" \
	"} vubo;\n" \
	"// VOL_UNIFORMS_BRICK\n" \
	"layout(push_constant) uniform VolFragShaderBrickConst {\n" \
	"	vec4 brkscale;//tex transform for bricking\n" \
	"	vec4 brktrans;//tex transform for bricking\n" \
	"	vec4 mskbrkscale;//tex transform for mask bricks\n" \
	"	vec4 mskbrktrans;//tex transform for mask bricks\n" \
	"	vec3 loc4;//(zmin, zmax, dz)\n" \
	"	uint stnum;\n" \
	"} brk;\n" \
	"\n" \
	"layout(location = 0) in vec3 InVertex;  //w will be set to 1.0 automatically\n" \
	"layout(location = 1) in vec3 InTexture;\n" \
	"layout(location = 0) out vec4 OutVertex;\n" \
	"layout(location = 1) out vec3 OutTexture;\n" \
	"//-------------------\n" \
	"void main()\n" \
	"{\n" \
	"	gl_Position = vubo.matrix0 * vubo.matrix1 * (vec4(InVertex,1.)*brk.brkscale + brk.brktrans);\n" \
	"	OutTexture = InTexture;\n" \
	"	OutVertex  = gl_Position;\n" \
	"}\n" 

#define VRAY_FRG_SHADER_CODE_TEST \
	"#version 450\n" \
	"#pragma optionNV(inline all)\n" \
	"#pragma optionNV(fastmath on)\n" \
	"#pragma optionNV(ifcvt none)\n" \
	"#pragma optionNV(strict on)\n" \
	"#pragma optionNV(unroll all)\n" \
	"layout(location = 0) in vec4 OutVertex;\n" \
	"layout(location = 1) in vec3 OutTexture;\n" \
	"layout(location = 0) out vec4 FragColor;\n" \
	"// VOL_UNIFORMS_BASE\n" \
	"layout(binding = 1) uniform VolFragShaderBaseUBO {\n" \
	"	vec4 loc0;//(lx, ly, lz, alpha)\n" \
	"	vec4 loc1;//(ka, kd, ks, ns)\n" \
	"	vec4 loc2;//(scalar_scale, gm_scale, left_thresh, right_thresh)\n" \
	"	vec4 loc3;//(gamma, gm_thresh, offset, sw)\n" \
	"	vec4 loc5;//(spcx, spcy, spcz, max_id)\n" \
	"	vec4 loc6;//(r, g, b, 0.0) or (1/vx, 1/vy, luminance, depth_mode)\n" \
	"	vec4 loc7;//(1/vx, 1/vy, 1/sample_rate, 0.0)\n" \
	"	vec4 loc8;//(int, start, end, 0.0)\n" \
	"	vec4 loc10; //plane0\n" \
	"	vec4 loc11; //plane1\n" \
	"	vec4 loc12; //plane2\n" \
	"	vec4 loc13; //plane3\n" \
	"	vec4 loc14; //plane4\n" \
	"	vec4 loc15; //plane5\n" \
	"	mat4 matrix0; //inverted projection matrix\n" \
	"	mat4 matrix1; //inverted modelview matrix\n" \
	"} base;\n" \
	"\n" \
	"layout(binding = 2) uniform sampler3D tex0;//data volume\n" \
	"layout(binding = 3) uniform sampler3D tex1;//gm volume\n" \
	"\n" \
	"// VOL_UNIFORMS_BRICK\n" \
	"layout(push_constant) uniform VolFragShaderBrickConst {\n" \
	"	vec4 brkscale;//tex transform for bricking\n" \
	"	vec4 brktrans;//tex transform for bricking\n" \
	"	vec4 mskbrkscale;//tex transform for mask bricks\n" \
	"	vec4 mskbrktrans;//tex transform for mask bricks\n" \
	"	vec3 loc4;//(zmin, zmax, dz)\n" \
	"	uint stnum;\n" \
	"} brk;\n" \
	"\n" \
	"//VOL_CLIP_FUNC\n" \
	"bool vol_clip_func(vec4 t)\n" \
	"{\n" \
	"	vec4 brickt = (t * brk.brkscale + brk.brktrans);\n" \
	"	if (dot(brickt.xyz, base.loc10.xyz) + base.loc10.w < 0.0 ||\n" \
	"		dot(brickt.xyz, base.loc11.xyz) + base.loc11.w < 0.0 ||\n" \
	"		dot(brickt.xyz, base.loc12.xyz) + base.loc12.w < 0.0 ||\n" \
	"		dot(brickt.xyz, base.loc13.xyz) + base.loc13.w < 0.0 ||\n" \
	"		dot(brickt.xyz, base.loc14.xyz) + base.loc14.w < 0.0 ||\n" \
	"		dot(brickt.xyz, base.loc15.xyz) + base.loc15.w < 0.0)\n" \
	"		return true;\n" \
	"	else\n" \
	"		return false;\n" \
	"}\n" \
	"\n" \
	"//VOL_HEAD\n" \
	"void main()\n" \
	"{\n" \
	"	vec4 view = vec4(0.0, 0.0, brk.loc4.x, 1.0);\n" \
	"	vec4 ray = normalize(base.matrix0 * OutVertex);\n" \
	"	vec4 st = (brk.loc4.x / ray.z) * ray;\n" \
	"	const float step = brk.loc4.z / ray.z;\n" \
	"	vec4 outcol = vec4(0.0);\n" \
	"	vec4 dir = vec4(1.0/1200.0, 1.0/566.0, 1.0/180.0, 0.0);\n" \
	"\n" \
	"	for (uint i = 0; i < brk.stnum; i++)\n" \
	"	{\n" \
	"		vec4 TexCoord = ((base.matrix1 * (st + ray*step*float(i))) - brk.brktrans) / brk.brkscale;\n" \
	"		vec4 t = vec4(TexCoord.xyz, 1.0);\n" \
	"\n" \
	"		if (!vol_clip_func(t) && t.x >= 0.0 && t.x <= 1.0 && t.y >= 0.0 && t.y <= 1.0 && t.z >= 0.0 && t.z <= 1.0 && outcol.w <= 0.99995)\n" \
	"		{\n" \
	"			//VOL_HEAD_LIT\n" \
	"			vec4 l = base.loc0; // {lx, ly, lz, alpha}\n" \
	"			vec4 k = base.loc1; // {ka, kd, ks, ns}\n" \
	"			k.x = k.x > 1.0 ? log2(3.0 - k.x) : k.x;\n" \
	"			vec4 n, w;\n" \
	"			if (l.w == 0.0) { discard; return; }\n" \
	"\n" \
	"			//VOL_DATA_VOLUME_LOOKUP\n" \
	"			vec4 v = texture(tex0, t.stp);\n" \
	"\n" \
	"			// VOL_GRAD_COMPUTE_HI\n" \
	"			vec4 r, p;\n" \
	"			v = vec4(v.x);\n" \
	"			n = vec4(0.0);\n" \
	"			w = vec4(0.0);\n" \
	"			w.x = dir.x;\n" \
	"			p = clamp(TexCoord + w, 0.0, 1.0);\n" \
	"			r = texture(tex0, p.stp);\n" \
	"			n.x = r.x + n.x;\n" \
	"			p = clamp(TexCoord - w, 0.0, 1.0);\n" \
	"			r = texture(tex0, p.stp);\n" \
	"			n.x = r.x - n.x;\n" \
	"			w = vec4(0.0);\n" \
	"			w.y = dir.y;\n" \
	"			p = clamp(TexCoord + w, 0.0, 1.0);\n" \
	"			r = texture(tex0, p.stp);\n" \
	"			n.y = r.x + n.y;\n" \
	"			p = clamp(TexCoord - w, 0.0, 1.0);\n" \
	"			r = texture(tex0, p.stp);\n" \
	"			n.y = r.x - n.y;\n" \
	"			w = vec4(0.0);\n" \
	"			w.z = dir.z;\n" \
	"			p = clamp(TexCoord + w, 0.0, 1.0);\n" \
	"			r = texture(tex0, p.stp);\n" \
	"			n.z = r.x + n.z;\n" \
	"			p = clamp(TexCoord - w, 0.0, 1.0);\n" \
	"			r = texture(tex0, p.stp);\n" \
	"			n.z = r.x - n.z;\n" \
	"			p.y = length(n.xyz);\n" \
	"			p.y = 0.5 * (base.loc2.x < 0.0 ? (1.0 + p.y * base.loc2.x) : p.y * base.loc2.x);\n" \
	"\n" \
	"			//VOL_BODY_SHADING\n" \
	"			n.xyz = normalize(n.xyz);\n" \
	"			n.w = dot(l.xyz, n.xyz); // calculate angle between light and normal. \n" \
	"			n.w = clamp(abs(n.w), 0.0, 1.0); // two-sided lighting, n.w = abs(cos(angle))  \n" \
	"			w = k; // w.x = weight*ka, w.y = weight*kd, w.z = weight*ks \n" \
	"			w.x = k.x - w.y; // w.x = ka - kd*weight \n" \
	"			w.x = w.x + k.y; // w.x = ka + kd - kd*weight \n" \
	"			n.z = pow(n.w, k.w); // n.z = abs(cos(angle))^ns \n" \
	"			n.w = (n.w * w.y) + w.x; // n.w = abs(cos(angle))*kd+ka\n" \
	"			n.z = w.z * n.z; // n.z = weight*ks*abs(cos(angle))^ns \n" \
	"\n" \
	"			//VOL_COMPUTED_GM_LOOKUP\n" \
	"			v.y = p.y;\n" \
	"\n" \
	"			//VOL_TRANSFER_FUNCTION_SIN_COLOR\n" \
	"			vec4 c;\n" \
	"			float tf_alp = 0.0;\n" \
	"			float alpha = 0.0;\n" \
	"			v.x = base.loc2.x < 0.0 ? (1.0 + v.x * base.loc2.x) : v.x * base.loc2.x;\n" \
	"			if (v.x<base.loc2.z - base.loc3.w || v.x>base.loc2.w + base.loc3.w || v.y < base.loc3.y - base.loc3.w)\n" \
	"				c = vec4(0.0);\n" \
	"			else\n" \
	"			{\n" \
	"				v.x = (v.x < base.loc2.z ? (base.loc3.w - base.loc2.z + v.x) / base.loc3.w : (v.x > base.loc2.w ? (base.loc3.w - v.x + base.loc2.w) / base.loc3.w : 1.0)) * v.x;\n" \
	"				v.x = (v.y < base.loc3.y ? (base.loc3.w - base.loc3.y + v.y) / base.loc3.w : 1.0) * v.x;\n" \
	"				tf_alp = pow(clamp(v.x / base.loc3.z,\n" \
	"					base.loc3.x < 1.0 ? -(base.loc3.x - 1.0) * 0.00001 : 0.0,\n" \
	"					base.loc3.x>1.0 ? 0.9999 : 1.0), base.loc3.x);\n" \
	"				alpha = (1.0 - pow(clamp(1.0 - tf_alp * l.w, 0.0, 1.0), base.loc7.z)) / l.w;\n" \
	"				c = vec4(base.loc6.rgb * alpha * tf_alp, alpha);\n" \
	"			}\n" \
	"\n" \
	"			//VOL_COLOR_OUTPUT\n" \
	"			c.xyz = c.xyz * clamp(1.0 - base.loc1.x, 0.0, 1.0) + base.loc1.x * c.xyz * (base.loc1.y > 0.0 ? (n.w + n.z) : 1.0);\n" \
	"			c.xyz *= pow(1.0 - base.loc1.x / 2.0, 2.0) + 1.0;\n" \
	"\n" \
	"			//VOL_RASTER_BLEND\n" \
	"			c = c * l.w;\n" \
	"			outcol = outcol + (1.0 - outcol.w) * c; // VOL_RASTER_BLEND\n" \
	"		}\n" \
	"	}\n" \
	"\n" \
	"	FragColor = outcol;\n" \
	"\n" \
	"	//VOL_TAIL\n" \
	"}\n" 


#define VRAY_FRG_SHADER_CODE_TEST_ORTHO \
	"#version 450\n" \
	"#pragma optionNV(inline all)\n" \
	"#pragma optionNV(fastmath on)\n" \
	"#pragma optionNV(ifcvt none)\n" \
	"#pragma optionNV(strict on)\n" \
	"#pragma optionNV(unroll all)\n" \
	"layout(location = 0) in vec4 OutVertex;\n" \
	"layout(location = 1) in vec3 OutTexture;\n" \
	"layout(location = 0) out vec4 FragColor;\n" \
	"// VOL_UNIFORMS_BASE\n" \
	"layout(binding = 1) uniform VolFragShaderBaseUBO {\n" \
	"	vec4 loc0;//(lx, ly, lz, alpha)\n" \
	"	vec4 loc1;//(ka, kd, ks, ns)\n" \
	"	vec4 loc2;//(scalar_scale, gm_scale, left_thresh, right_thresh)\n" \
	"	vec4 loc3;//(gamma, gm_thresh, offset, sw)\n" \
	"	vec4 loc5;//(spcx, spcy, spcz, max_id)\n" \
	"	vec4 loc6;//(r, g, b, 0.0) or (1/vx, 1/vy, luminance, depth_mode)\n" \
	"	vec4 loc7;//(1/vx, 1/vy, 1/sample_rate, 0.0)\n" \
	"	vec4 loc8;//(int, start, end, 0.0)\n" \
	"	vec4 loc10; //plane0\n" \
	"	vec4 loc11; //plane1\n" \
	"	vec4 loc12; //plane2\n" \
	"	vec4 loc13; //plane3\n" \
	"	vec4 loc14; //plane4\n" \
	"	vec4 loc15; //plane5\n" \
	"	mat4 matrix0; //inverted projection matrix\n" \
	"	mat4 matrix1; //inverted modelview matrix\n" \
	"} base;\n" \
	"\n" \
	"layout(binding = 2) uniform sampler3D tex0;//data volume\n" \
	"layout(binding = 3) uniform sampler3D tex1;//gm volume\n" \
	"\n" \
	"// VOL_UNIFORMS_BRICK\n" \
	"layout(push_constant) uniform VolFragShaderBrickConst {\n" \
	"	vec4 brkscale;//tex transform for bricking\n" \
	"	vec4 brktrans;//tex transform for bricking\n" \
	"	vec4 mskbrkscale;//tex transform for mask bricks\n" \
	"	vec4 mskbrktrans;//tex transform for mask bricks\n" \
	"	vec3 loc4;//(zmin, zmax, dz)\n" \
	"	uint stnum;\n" \
	"} brk;\n" \
	"\n" \
	"//VOL_CLIP_FUNC\n" \
	"bool vol_clip_func(vec4 t)\n" \
	"{\n" \
	"	vec4 brickt = (t * brk.brkscale + brk.brktrans);\n" \
	"	if (dot(brickt.xyz, base.loc10.xyz) + base.loc10.w < 0.0 ||\n" \
	"		dot(brickt.xyz, base.loc11.xyz) + base.loc11.w < 0.0 ||\n" \
	"		dot(brickt.xyz, base.loc12.xyz) + base.loc12.w < 0.0 ||\n" \
	"		dot(brickt.xyz, base.loc13.xyz) + base.loc13.w < 0.0 ||\n" \
	"		dot(brickt.xyz, base.loc14.xyz) + base.loc14.w < 0.0 ||\n" \
	"		dot(brickt.xyz, base.loc15.xyz) + base.loc15.w < 0.0)\n" \
	"		return true;\n" \
	"	else\n" \
	"		return false;\n" \
	"}\n" \
	"\n" \
	"//VOL_HEAD\n" \
	"void main()\n" \
	"{\n" \
	"	vec4 ray = vec4(0.0, 0.0, 1.0, 0.0);\n" \
	"	vec4 o = vec4((base.matrix0 * OutVertex).xy, 0.0, 1.0);\n" \
	"	vec4 st = o + (brk.loc4.x / ray.z) * ray;\n" \
	"	const float step = brk.loc4.z / ray.z;\n" \
	"	vec4 outcol = vec4(0.0);\n" \
	"	vec4 dir = vec4(1.0/1200.0, 1.0/566.0, 1.0/180.0, 0.0);\n" \
	"\n" \
	"	//VOL_HEAD_LIT\n" \
	"	vec4 l = base.loc0; // {lx, ly, lz, alpha}\n" \
	"	vec4 k = base.loc1; // {ka, kd, ks, ns}\n" \
	"	k.x = k.x > 1.0 ? log2(3.0 - k.x) : k.x;\n" \
	"	vec4 n, w;\n" \
	"	if (l.w == 0.0) { discard; return; }\n" \
	"\n" \
	"	for (uint i = 0; i < brk.stnum; i++)\n" \
	"	{\n" \
	"		vec4 TexCoord = ((base.matrix1 * (st + ray*step*float(i))) - brk.brktrans) / brk.brkscale;\n" \
	"		vec4 t = vec4(TexCoord.xyz, 1.0);\n" \
	"\n" \
	"		if (!vol_clip_func(t) && t.x >= 0.0 && t.x <= 1.0 && t.y >= 0.0 && t.y <= 1.0 && t.z >= 0.0 && t.z <= 1.0 && outcol.w <= 0.99995)\n" \
	"		{\n" \
	"			//VOL_DATA_VOLUME_LOOKUP\n" \
	"			vec4 v = texture(tex0, t.stp);\n" \
	"\n" \
	"			// VOL_GRAD_COMPUTE_HI\n" \
	"			vec4 r, p;\n" \
	"			v = vec4(v.x);\n" \
	"			n = vec4(0.0);\n" \
	"			w = vec4(0.0);\n" \
	"			w.x = dir.x;\n" \
	"			p = clamp(TexCoord + w, 0.0, 1.0);\n" \
	"			r = texture(tex0, p.stp);\n" \
	"			n.x = r.x + n.x;\n" \
	"			p = clamp(TexCoord - w, 0.0, 1.0);\n" \
	"			r = texture(tex0, p.stp);\n" \
	"			n.x = r.x - n.x;\n" \
	"			w = vec4(0.0);\n" \
	"			w.y = dir.y;\n" \
	"			p = clamp(TexCoord + w, 0.0, 1.0);\n" \
	"			r = texture(tex0, p.stp);\n" \
	"			n.y = r.x + n.y;\n" \
	"			p = clamp(TexCoord - w, 0.0, 1.0);\n" \
	"			r = texture(tex0, p.stp);\n" \
	"			n.y = r.x - n.y;\n" \
	"			w = vec4(0.0);\n" \
	"			w.z = dir.z;\n" \
	"			p = clamp(TexCoord + w, 0.0, 1.0);\n" \
	"			r = texture(tex0, p.stp);\n" \
	"			n.z = r.x + n.z;\n" \
	"			p = clamp(TexCoord - w, 0.0, 1.0);\n" \
	"			r = texture(tex0, p.stp);\n" \
	"			n.z = r.x - n.z;\n" \
	"			p.y = length(n.xyz);\n" \
	"			p.y = 0.5 * (base.loc2.x < 0.0 ? (1.0 + p.y * base.loc2.x) : p.y * base.loc2.x);\n" \
	"\n" \
	"			//VOL_BODY_SHADING\n" \
	"			n.xyz = normalize(n.xyz);\n" \
	"			n.w = dot(l.xyz, n.xyz); // calculate angle between light and normal. \n" \
	"			n.w = clamp(abs(n.w), 0.0, 1.0); // two-sided lighting, n.w = abs(cos(angle))  \n" \
	"			w = k; // w.x = weight*ka, w.y = weight*kd, w.z = weight*ks \n" \
	"			w.x = k.x - w.y; // w.x = ka - kd*weight \n" \
	"			w.x = w.x + k.y; // w.x = ka + kd - kd*weight \n" \
	"			n.z = pow(n.w, k.w); // n.z = abs(cos(angle))^ns \n" \
	"			n.w = (n.w * w.y) + w.x; // n.w = abs(cos(angle))*kd+ka\n" \
	"			n.z = w.z * n.z; // n.z = weight*ks*abs(cos(angle))^ns \n" \
	"\n" \
	"			//VOL_COMPUTED_GM_LOOKUP\n" \
	"			v.y = p.y;\n" \
	"\n" \
	"			//VOL_TRANSFER_FUNCTION_SIN_COLOR\n" \
	"			vec4 c;\n" \
	"			float tf_alp = 0.0;\n" \
	"			float alpha = 0.0;\n" \
	"			v.x = base.loc2.x < 0.0 ? (1.0 + v.x * base.loc2.x) : v.x * base.loc2.x;\n" \
	"			if (v.x<base.loc2.z - base.loc3.w || v.x>base.loc2.w + base.loc3.w || v.y < base.loc3.y - base.loc3.w)\n" \
	"				c = vec4(0.0);\n" \
	"			else\n" \
	"			{\n" \
	"				v.x = (v.x < base.loc2.z ? (base.loc3.w - base.loc2.z + v.x) / base.loc3.w : (v.x > base.loc2.w ? (base.loc3.w - v.x + base.loc2.w) / base.loc3.w : 1.0)) * v.x;\n" \
	"				v.x = (v.y < base.loc3.y ? (base.loc3.w - base.loc3.y + v.y) / base.loc3.w : 1.0) * v.x;\n" \
	"				tf_alp = pow(clamp(v.x / base.loc3.z,\n" \
	"					base.loc3.x < 1.0 ? -(base.loc3.x - 1.0) * 0.00001 : 0.0,\n" \
	"					base.loc3.x>1.0 ? 0.9999 : 1.0), base.loc3.x);\n" \
	"				alpha = (1.0 - pow(clamp(1.0 - tf_alp * l.w, 0.0, 1.0), base.loc7.z)) / l.w;\n" \
	"				c = vec4(base.loc6.rgb * alpha * tf_alp, alpha);\n" \
	"			}\n" \
	"\n" \
	"			//VOL_COLOR_OUTPUT\n" \
	"			c.xyz = c.xyz * clamp(1.0 - base.loc1.x, 0.0, 1.0) + base.loc1.x * c.xyz * (base.loc1.y > 0.0 ? (n.w + n.z) : 1.0);\n" \
	"			c.xyz *= pow(1.0 - base.loc1.x / 2.0, 2.0) + 1.0;\n" \
	"\n" \
	"			//VOL_RASTER_BLEND\n" \
	"			c = c * l.w;\n" \
	"			outcol = outcol + (1.0 - outcol.w) * c; // VOL_RASTER_BLEND\n" \
	"		}\n" \
	"	}\n" \
	"\n" \
	"	FragColor = outcol;\n" \
	"\n" \
	"	//VOL_TAIL\n" \
	"}\n" 

#define VRAY_FRG_SHADER_CODE_TEST2 \
	"#version 450\n" \
	"#pragma optionNV(inline all)\n" \
	"#pragma optionNV(fastmath on)\n" \
	"#pragma optionNV(ifcvt none)\n" \
	"#pragma optionNV(strict on)\n" \
	"#pragma optionNV(unroll all)\n" \
	"layout(location = 0) in vec4 OutVertex;\n" \
	"layout(location = 1) in vec3 OutTexture;\n" \
	"layout(location = 0) out vec4 FragColor;\n" \
	"// VOL_UNIFORMS_BASE\n" \
	"layout(binding = 1) uniform VolFragShaderBaseUBO {\n" \
	"	vec4 loc0;//(lx, ly, lz, alpha)\n" \
	"	vec4 loc1;//(ka, kd, ks, ns)\n" \
	"	vec4 loc2;//(scalar_scale, gm_scale, left_thresh, right_thresh)\n" \
	"	vec4 loc3;//(gamma, gm_thresh, offset, sw)\n" \
	"	vec4 loc5;//(spcx, spcy, spcz, max_id)\n" \
	"	vec4 loc6;//(r, g, b, 0.0) or (1/vx, 1/vy, luminance, depth_mode)\n" \
	"	vec4 loc7;//(1/vx, 1/vy, 0.0, 0.0)\n" \
	"	vec4 loc8;//(int, start, end, 0.0)\n" \
	"	vec4 loc10; //plane0\n" \
	"	vec4 loc11; //plane1\n" \
	"	vec4 loc12; //plane2\n" \
	"	vec4 loc13; //plane3\n" \
	"	vec4 loc14; //plane4\n" \
	"	vec4 loc15; //plane5\n" \
	"	mat4 matrix0; //inverted projection matrix\n" \
	"	mat4 matrix1; //inverted modelview matrix\n" \
	"} base;\n" \
	"\n" \
	"layout(binding = 2) uniform sampler3D tex0;//data volume\n" \
	"layout(binding = 3) uniform sampler3D tex1;//gm volume\n" \
	"\n" \
	"// VOL_UNIFORMS_BRICK\n" \
	"layout(push_constant) uniform VolFragShaderBrickConst {\n" \
	"	vec4 brkscale;//tex transform for bricking\n" \
	"	vec4 brktrans;//tex transform for bricking\n" \
	"	vec4 mskbrkscale;//tex transform for mask bricks\n" \
	"	vec4 mskbrktrans;//tex transform for mask bricks\n" \
	"	vec3 loc4;//(zmin, zmax, dz)\n" \
	"	uint stnum;\n" \
	"} brk;\n" \
	"\n" \
	"//VOL_CLIP_FUNC\n" \
	"bool vol_clip_func(vec4 t)\n" \
	"{\n" \
	"	vec4 brickt = (t * brk.brkscale + brk.brktrans);\n" \
	"	if (dot(brickt.xyz, base.loc10.xyz) + base.loc10.w < 0.0 ||\n" \
	"		dot(brickt.xyz, base.loc11.xyz) + base.loc11.w < 0.0 ||\n" \
	"		dot(brickt.xyz, base.loc12.xyz) + base.loc12.w < 0.0 ||\n" \
	"		dot(brickt.xyz, base.loc13.xyz) + base.loc13.w < 0.0 ||\n" \
	"		dot(brickt.xyz, base.loc14.xyz) + base.loc14.w < 0.0 ||\n" \
	"		dot(brickt.xyz, base.loc15.xyz) + base.loc15.w < 0.0)\n" \
	"		return true;\n" \
	"	else\n" \
	"		return false;\n" \
	"}\n" \
	"\n" \
	"//VOL_HEAD\n" \
	"void main()\n" \
	"{\n" \
	"	vec4 view = vec4(0.0, 0.0, brk.loc4.x, 0.0);\n" \
	"	vec4 ray = normalize(view);//normalize(base.matrix0 * OutVertex);\n" \
	"	vec4 o = vec4((base.matrix0 * OutVertex).xy, 0.0, 1.0);\n" \
	"	vec4 st = o + (brk.loc4.x / ray.z) * ray;\n" \
	"	st.w = 1.0;\n" \
	"	const float step = brk.loc4.z / ray.z;\n" \
	"	const float invstnum = 1.0 / float(brk.stnum);\n" \
	"	vec4 outcol = vec4(0.0);\n" \
	"	vec4 dir = vec4(1.0/brk.brkscale.x, 1.0/brk.brkscale.y, 1.0/brk.brkscale.z, 0.0);\n" \
	"\n" \
	"	vec4 TexCoord = ((base.matrix1 * (st + ray*step*float(0))) - brk.brktrans) / brk.brkscale;\n" \
	"	vec4 t = vec4(TexCoord.xyz, 1.0);\n" \
	"	FragColor = t;\n" \
	"\n" \
	"	//VOL_TAIL\n" \
	"}\n" 

VRayShader::VRayShader(
	VkDevice device, bool poly, int channels,
	bool shading, bool fog,
	int peel, bool clip,
	bool hiqual, int mask,
	int color_mode, int colormap, int colormap_proj,
	bool solid, int vertex_shader, int mask_hide_mode)
	: device_(device),
	poly_(poly),
	channels_(channels),
	shading_(shading),
	fog_(fog),
	peel_(peel),
	clip_(clip),
	hiqual_(hiqual),
	mask_(mask),
	color_mode_(color_mode),
	colormap_(colormap),
	colormap_proj_(colormap_proj),
	solid_(solid),
	vertex_type_(vertex_shader),
	mask_hide_mode_(mask_hide_mode),
	program_(0)
	{
	}

	VRayShader::~VRayShader()
	{
		delete program_;
	}

	bool VRayShader::create()
	{
		string fs,vs;
		if (emit_f(fs)) return true;
		if (emit_v(vs)) return true;
		program_ = new ShaderProgram(vs,fs);
		program_->create(device_);
		return false;
	}

	bool VRayShader::emit_v(string& s)
	{
		ostringstream z;
		z << ShaderProgram::glsl_version_;
		/*if (fog_)
			z << VTX_SHADER_CODE_FOG;
		else*/
			z << VRAY_VTX_SHADER_CODE_CORE_PROFILE;

		s = z.str();
		return false;
	}

	string VRayShader::get_colormap_code()
	{
		switch (colormap_)
		{
		case 0:
			return string(VOL_COLORMAP_CALC0);
		case 1:
			return string(VOL_COLORMAP_CALC1);
		case 2:
			return string(VOL_COLORMAP_CALC2);
		case 3:
			return string(VOL_COLORMAP_CALC3);
		case 4:
			return string(VOL_COLORMAP_CALC4);
		}
		return string(VOL_COLORMAP_CALC0);
	}

	string VRayShader::get_colormap_proj()
	{
		switch (colormap_proj_)
		{
		case 0:
			return string(VOL_TRANSFER_FUNCTION_COLORMAP_VALU0);
		case 1:
			return string(VOL_TRANSFER_FUNCTION_COLORMAP_VALU1);
		case 2:
			return string(VOL_TRANSFER_FUNCTION_COLORMAP_VALU2);
		case 3:
			return string(VOL_TRANSFER_FUNCTION_COLORMAP_VALU3);
		}
		return string(VOL_TRANSFER_FUNCTION_COLORMAP_VALU0);
	}

	bool VRayShader::emit_f(string& s)
	{
		ostringstream z;

		z << VRAY_FRG_SHADER_CODE_TEST_ORTHO;

		//if (poly_)
		//{
		//	z << ShaderProgram::glsl_version_;
		//	z << FRG_SHADER_CODE_CORE_PROFILE;
		//	//output
		//	s = z.str();
		//	return false;
		//}

		////version info
		//z << ShaderProgram::glsl_version_;
		//z << VOL_INPUTS;
		//if (fog_)
		//	z << VOL_INPUTS_FOG;
		//z << VOL_OUTPUTS;
		//if (color_mode_ == 3) z << VOL_ID_COLOR_OUTPUTS;

		////the common uniforms
		//z << VOL_UNIFORMS_BASE;

		////the brick uniforms
		//z << VOL_UNIFORMS_BRICK;

		////color modes
		//switch (color_mode_)
		//{
		//case 2://depth map
		//	z << VOL_UNIFORMS_DEPTHMAP;
		//	break;
		//case 3://index color
		//	z << VOL_UNIFORMS_INDEX_COLOR;
		//	break;
  //      case 255://index color (depth mode)
  //          z << VOL_UNIFORMS_INDEX_COLOR_D;
  //          break;
  //              
		//}

		//// add uniform for depth peeling
		//if (peel_ != 0)
		//	z << VOL_UNIFORMS_DP;

		//// add uniforms for masking
		//switch (mask_)
		//{
		//case 1:
		//case 2:
		//	z << VOL_UNIFORMS_MASK;
		//	break;
		//case 3:
		//	z << VOL_UNIFORMS_LABEL;
		//	break;
		//case 4:
		//	z << VOL_UNIFORMS_MASK;
		//	z << VOL_UNIFORMS_LABEL;
		//	break;
		//}

		//if ( (mask_ <= 0 || mask_ == 3)  && mask_hide_mode_ > 0)
		//	z << VOL_UNIFORMS_MASK;

		////functions
		//if (clip_)
		//	z << VOL_CLIP_FUNC;
		////the common head
		//z << VOL_HEAD;

		//if (peel_ != 0 || color_mode_ == 2)
		//	z << VOL_HEAD_2DMAP_LOC;

		////head for depth peeling
		//if (peel_ == 1)//draw volume before 15
		//	z << VOL_HEAD_DP_1;
		//else if (peel_ == 2 || peel_ == 5)//draw volume after 14
		//	z << VOL_HEAD_DP_2;
		//else if (peel_ == 3 || peel_ == 4)//draw volume after 14 and before 15
		//	z << VOL_HEAD_DP_3;

		////head for clipping planes
		//if (clip_)
		//	z << VOL_HEAD_CLIP_FUNC;

		//// Set up light variables and input parameters.
		//z << VOL_HEAD_LIT;

		//if ( mask_hide_mode_ == 1)
		//	z << VOL_HEAD_HIDE_OUTSIDE_MASK;
		//else if ( mask_hide_mode_ == 2)
		//	z << VOL_HEAD_HIDE_INSIDE_MASK;

		//// Set up fog variables and input parameters.
		//if (fog_)
		//	z << VOL_HEAD_FOG;

		////bodies
		//if (color_mode_ == 3)
		//{
		//	if (shading_)
		//		z << VOL_INDEX_COLOR_BODY_SHADE;
		//	else
		//		z << VOL_INDEX_COLOR_BODY;
		//}
  //      else if (color_mode_ == 255)
  //      {
  //          z << VOL_INDEX_COLOR_D_BODY;
  //      }
		//else if (shading_)
		//{
		//	//no gradient volume, need to calculate in real-time
		//	z << VOL_DATA_VOLUME_LOOKUP;

		//	if (hiqual_)
		//		z << VOL_GRAD_COMPUTE_HI;
		//	else
		//		z << VOL_GRAD_COMPUTE_LO;

		//	z << VOL_BODY_SHADING;

		//	if (channels_ == 1)
		//		z << VOL_COMPUTED_GM_LOOKUP;
		//	else
		//		z << VOL_TEXTURE_GM_LOOKUP;

		//	switch (color_mode_)
		//	{
		//	case 0://normal
		//		if (solid_)
		//			z << VOL_TRANSFER_FUNCTION_SIN_COLOR_SOLID;
		//		else
		//			z << VOL_TRANSFER_FUNCTION_SIN_COLOR;
		//		break;
		//	case 1://colormap
		//		if (solid_)
		//		{
		//			z << VOL_TRANSFER_FUNCTION_COLORMAP_SOLID;
		//			z << get_colormap_proj();
		//			z << get_colormap_code();
		//			z << VOL_COMMON_TRANSFER_FUNCTION_CALC;
		//			z << VOL_TRANSFER_FUNCTION_COLORMAP_SOLID_RESULT;
		//		}
		//		else
		//		{
		//			z << VOL_TRANSFER_FUNCTION_COLORMAP;
		//			z << get_colormap_proj();
		//			z << get_colormap_code();
		//			z << VOL_COMMON_TRANSFER_FUNCTION_CALC;
		//			z << VOL_TRANSFER_FUNCTION_COLORMAP_RESULT;
		//		}
		//		break;
		//	}

		//	z << VOL_COLOR_OUTPUT;
		//}
		//else // No shading
		//{
		//	z << VOL_DATA_VOLUME_LOOKUP;

		//	if (channels_ == 1)
		//	{
		//		// Compute Gradient magnitude and use it.
		//		if (hiqual_)
		//			z << VOL_GRAD_COMPUTE_HI;
		//		else
		//			z << VOL_GRAD_COMPUTE_LO;

		//		z << VOL_COMPUTED_GM_LOOKUP;
		//	}
		//	else
		//	{
		//		z << VOL_TEXTURE_GM_LOOKUP;
		//	}

		//	switch (color_mode_)
		//	{
		//	case 0://normal
		//		if (solid_)
		//			z << VOL_TRANSFER_FUNCTION_SIN_COLOR_SOLID;
		//		else
		//			z << VOL_TRANSFER_FUNCTION_SIN_COLOR;
		//		break;
		//	case 1://colormap
		//		if (solid_)
		//		{
		//			z << VOL_TRANSFER_FUNCTION_COLORMAP_SOLID;
		//			z << get_colormap_proj();
		//			z << get_colormap_code();
		//			z << VOL_COMMON_TRANSFER_FUNCTION_CALC;
		//			z << VOL_TRANSFER_FUNCTION_COLORMAP_SOLID_RESULT;
		//		}
		//		else
		//		{
		//			z << VOL_TRANSFER_FUNCTION_COLORMAP;
		//			z << get_colormap_proj();
		//			z << get_colormap_code();
		//			z << VOL_COMMON_TRANSFER_FUNCTION_CALC;
		//			z << VOL_TRANSFER_FUNCTION_COLORMAP_RESULT;
		//		}
		//		break;
		//	case 2://depth map
		//		z << VOL_TRANSFER_FUNCTION_DEPTHMAP;
		//		break;
		//	}
		//}

		//// fog
		//if (fog_)
		//{
		//	z << VOL_FOG_BODY;
		//}

		////final blend
		//switch (mask_)
		//{
		//case 0:
		//	if (color_mode_ == 3 || color_mode_ == 255)
		//		z << VOL_RASTER_BLEND_ID;
		//	else if (color_mode_ == 2)
		//		z << VOL_RASTER_BLEND_DMAP;
		//	else
		//	{
		//		if (solid_)
		//			z << VOL_RASTER_BLEND_SOLID;
		//		else
		//			z << VOL_RASTER_BLEND;
		//	}
		//	break;
		//case 1:
		//	if (color_mode_ == 3 || color_mode_ == 255)
		//		z << VOL_RASTER_BLEND_MASK_ID;
		//	else if (color_mode_ == 2)
		//		z << VOL_RASTER_BLEND_MASK_DMAP;
		//	else
		//	{
		//		if (solid_)
		//			z << VOL_RASTER_BLEND_MASK_SOLID;
		//		else
		//			z << VOL_RASTER_BLEND_MASK;
		//	}
		//	break;
		//case 2:
		//	if (color_mode_ == 3 || color_mode_ == 255)
		//		z << VOL_RASTER_BLEND_NOMASK_ID;
		//	else if (color_mode_ == 2)
		//		z << VOL_RASTER_BLEND_NOMASK_DMAP;
		//	else
		//	{
		//		if (solid_)
		//			z << VOL_RASTER_BLEND_NOMASK_SOLID;
		//		else
		//			z << VOL_RASTER_BLEND_NOMASK;
		//	}
		//	break;
		//case 3:
		//	z << VOL_RASTER_BLEND_LABEL;
		//	break;
		//case 4:
		//	z << VOL_RASTER_BLEND_LABEL_MASK;
		//	break;
		//}

		////the common tail
		//z << VOL_TAIL;

		//output
		s = z.str();
		return false;
	}

	VRayShaderFactory::VRayShaderFactory()
		: prev_shader_(-1)
	{

	}

	VRayShaderFactory::VRayShaderFactory(std::vector<vks::VulkanDevice*> &devices)
		: prev_shader_(-1)
	{
		init(devices);
	}

	void VRayShaderFactory::init(std::vector<vks::VulkanDevice*> &devices)
	{
		vdevices_ = devices;
		setupDescriptorSetLayout();
	}

	VRayShaderFactory::~VRayShaderFactory()
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

	ShaderProgram* VRayShaderFactory::shader(
		VkDevice device,
		bool poly, int channels,
		bool shading, bool fog,
		int peel, bool clip,
		bool hiqual, int mask,
		int color_mode, int colormap, int colormap_proj,
		bool solid, int vertex_shader, int mask_hide_mode)
	{
		VRayShader*ret = nullptr;
		if(prev_shader_ >= 0)
		{
			if(shader_[prev_shader_]->match(
				device, poly, channels,
				shading, fog,
				peel, clip,
				hiqual, mask,
				color_mode, colormap, colormap_proj,
				solid, vertex_shader, mask_hide_mode))
			{
				ret = shader_[prev_shader_];
			}
		}
		if (!ret)
		{
			for(unsigned int i=0; i<shader_.size(); i++)
			{
				if(shader_[i]->match(
					device, poly, channels,
					shading, fog,
					peel, clip,
					hiqual, mask,
					color_mode, colormap, colormap_proj,
					solid, vertex_shader, mask_hide_mode))
				{
					prev_shader_ = i;
					ret = shader_[i];
					break;
				}
			}
		}

		if (!ret)
		{
			VRayShader* s = new VRayShader(device, poly, channels,
				shading, fog,
				peel, clip,
				hiqual, mask,
				color_mode, colormap, colormap_proj,
				solid, vertex_shader, mask_hide_mode);
			if(s->create())
				delete s;
			else
			{
				shader_.push_back(s);
				prev_shader_ = int(shader_.size())-1;
				ret = s;
			}
		}

		return ret ? ret->program() : NULL;
	}

	void VRayShaderFactory::setupDescriptorSetLayout()
	{
		for (auto vdev : vdevices_)
		{
			VkDevice device = vdev->logicalDevice;

			std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = 
			{
				// Binding 0 : Uniform buffer for vertex shader
				vks::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
				VK_SHADER_STAGE_VERTEX_BIT, 
				0),
				// Binding 1 : Base uniform buffer for fragment shader
				vks::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
				VK_SHADER_STAGE_FRAGMENT_BIT, 
				1),
			};

			int offset = 2;
			for (int i = 0; i < ShaderProgram::MAX_SHADER_UNIFORMS; i++)
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

			VRayPipelineSettings pipe;
			VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &pipe.descriptorSetLayout));

			VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
				vks::initializers::pipelineLayoutCreateInfo(
				&pipe.descriptorSetLayout,
				1);

			VkPushConstantRange pushConstantRange = {};
			pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;
			pushConstantRange.size = sizeof(VRayShaderFactory::VRayFragShaderBrickConst);
			pushConstantRange.offset = 0;

			// Push constant ranges are part of the pipeline layout
			pPipelineLayoutCreateInfo.pushConstantRangeCount = 1;
			pPipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

			VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipe.pipelineLayout));

			pipeline_[vdev] = pipe;
		}
	}
	
	void VRayShaderFactory::getDescriptorSetWriteUniforms(vks::VulkanDevice *vdev, VRayUniformBufs& uniformBuffers, std::vector<VkWriteDescriptorSet>& writeDescriptorSets)
	{
		VkDevice device = vdev->logicalDevice;
		
		writeDescriptorSets.push_back(
			vks::initializers::writeDescriptorSet(
				VK_NULL_HANDLE,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&uniformBuffers.vert.descriptor)
		);
		writeDescriptorSets.push_back(
			vks::initializers::writeDescriptorSet(
				VK_NULL_HANDLE,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				1,
				&uniformBuffers.frag_base.descriptor)
		);
	}

	void VRayShaderFactory::getDescriptorSetWriteUniforms(vks::VulkanDevice* vdev, vks::Buffer& vert, vks::Buffer& frag, std::vector<VkWriteDescriptorSet>& writeDescriptorSets)
	{
		VkDevice device = vdev->logicalDevice;

		writeDescriptorSets.push_back(
			vks::initializers::writeDescriptorSet(
				VK_NULL_HANDLE,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&vert.descriptor)
		);
		writeDescriptorSets.push_back(
			vks::initializers::writeDescriptorSet(
				VK_NULL_HANDLE,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				1,
				&frag.descriptor)
		);
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void VRayShaderFactory::prepareUniformBuffers(std::map<vks::VulkanDevice*, VRayUniformBufs>& uniformBuffers)
	{
		for (auto vulkanDev : vdevices_)
		{
			VRayUniformBufs uniformbufs;

			VK_CHECK_RESULT(vulkanDev->createBuffer(
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
				&uniformbufs.vert,
				sizeof(VRayVertShaderUBO)));

			VK_CHECK_RESULT(vulkanDev->createBuffer(
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
				&uniformbufs.frag_base,
				sizeof(VRayFragShaderBaseUBO)));

			// Map persistent
			VK_CHECK_RESULT(uniformbufs.vert.map());
			VK_CHECK_RESULT(uniformbufs.frag_base.map());

			uniformBuffers[vulkanDev] = uniformbufs;
		}
	}

	void VRayShaderFactory::updateUniformBuffers(VRayUniformBufs& uniformBuffers, VRayVertShaderUBO vubo, VRayFragShaderBaseUBO fubo)
	{
		memcpy(uniformBuffers.vert.mapped, &vubo, sizeof(vubo));
		memcpy(uniformBuffers.frag_base.mapped, &fubo, sizeof(fubo));
	}

	
} // end namespace FLIVR
