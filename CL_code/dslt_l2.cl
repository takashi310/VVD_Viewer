__constant sampler_t MySampler = CLK_NORMALIZED_COORDS_FALSE |
							CLK_ADDRESS_CLAMP_TO_EDGE |
							CLK_FILTER_LINEAR;

__kernel void kernel_main(__read_only image3d_t in,
					  __global float* out,
					  __global int* n_in,
					  __constant float* filter,
					  __constant float* sincostable,
					  int r,
					  int knum,
					  float bx,
					  float by,
					  int ypitch,
					  int zpitch)
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    const int z = get_global_id(2);
	const int id = z*zpitch + y*ypitch + x;
	
	const int nid = n_in[id];
	const float sinlati  = sincostable[nid*4];
	const float coslati  = sincostable[nid*4+1];
	const float sinlongi = sincostable[nid*4+2];
	const float coslongi = sincostable[nid*4+3];

	const float drx = bx*coslati*coslongi - by*sinlongi;
	const float dry = bx*coslati*sinlongi + by*coslongi;
	const float drz = -bx*sinlati;

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

   out[id] += sum / (float)knum;

}
