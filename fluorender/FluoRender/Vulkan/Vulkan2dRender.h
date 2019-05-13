#include <VVulkan.h>
#include <FLIVR/ImgShader.h>

#ifndef _Vulkan2dRender_H_
#define _Vulkan2dRender_H_

class Vulkan2dRender
{
public:
	vks::Buffer vertexBuffer;
	vks::Buffer indexBuffer;
	uint32_t indexCount;

};

#endif