__constant sampler_t MySampler = CLK_NORMALIZED_COORDS_FALSE |
							CLK_ADDRESS_CLAMP_TO_EDGE |
							CLK_FILTER_LINEAR;

__kernel void kernel_main(__global float* in,
							__global float* out,
							int ypitch,
							int zpitch)
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    const int z = get_global_id(2);
	const int id = z*zpitch + y*ypitch + x;

	out[id] = (out[id] > in[id]) ? in[id] : out[id];
}
