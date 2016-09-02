__constant sampler_t MySampler = CLK_NORMALIZED_COORDS_FALSE |
							CLK_ADDRESS_CLAMP_TO_EDGE |
							CLK_FILTER_LINEAR;

__kernel void kernel_main(__read_only image3d_t in,
				   __global float* out,
				   __global int* n_out,
				   __constant float* filter,
				   int r,
				   int n,
				   float drx,
				   float dry,
				   float drz,
				   int ypitch,
				   int zpitch)
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    const int z = get_global_id(2);
	const int id = z*zpitch + y*ypitch + x;

	float sum = 0.0f;
	
	float4 coord = (float4)(x+0.5, y+0.5, z+0.5, 0);
	float4 dstval = read_imagef(in, MySampler, coord) * filter[0];
	sum += dstval.x;
	#pragma unroll
	for(int i = 1; i <= r; i++){
		coord = (float4)(x+drx*i+0.5, y+dry*i+0.5, z+drz*i+0.5, 0);
		dstval = read_imagef(in, MySampler, coord) * filter[i];
		sum += dstval.x;
		coord = (float4)(x-drx*i+0.5, y-dry*i+0.5, z-drz*i+0.5, 0);
		dstval = read_imagef(in, MySampler, coord) * filter[i];
		sum += dstval.x;
	}

    if (out[id] < sum) {
		out[id] = sum;
		n_out[id] = n;
	}
}
