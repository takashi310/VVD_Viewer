__constant sampler_t MySampler = CLK_NORMALIZED_COORDS_FALSE |
							CLK_ADDRESS_CLAMP_TO_EDGE |
							CLK_FILTER_LINEAR;

__kernel void kernel_main(__read_only image3d_t in,
							__global float* dslt,
							__global unsigned char* out,
							float constC,
							int ypitch,
							int zpitch)
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    const int z = get_global_id(2);
	const int id = z*zpitch + y*ypitch + x;

	float4 coord = (float4)(x+0.5, y+0.5, z+0.5, 0);
	float4 srcval = read_imagef(in, MySampler, coord);
	
	out[id] = (srcval.x > dslt[id] + constC) ? 255 : 0;
	//out[id] = (srcval.x > dslt[id]+0.00001) ? 255 : 0;
}

/*
__kernel void kernel_main(__global float* out,
							int ypitch,
							int zpitch)
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    const int z = get_global_id(2);
	const int id = z*zpitch + y*ypitch + x;

	out[id] = 1.0;
}
*/