__constant sampler_t MySampler = CLK_NORMALIZED_COORDS_FALSE |
							CLK_ADDRESS_CLAMP_TO_EDGE |
							CLK_FILTER_LINEAR;

__constant sampler_t MySampler2 = CLK_NORMALIZED_COORDS_FALSE |
							CLK_ADDRESS_CLAMP_TO_EDGE |
							CLK_FILTER_NEAREST;

__kernel void dslt_min(__read_only image3d_t in,
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

    if (out[id] > sum) {
		out[id] = sum;
		n_out[id] = n;
	}
}

__kernel void dslt_max(__read_only image3d_t in,
				   __read_only image3d_t mask,
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
	
	float sum = 0.0f;
	
	float4 coord = (float4)(x+0.5, y+0.5, z+0.5, 0);

	if (read_imagef(mask, MySampler2, coord).x > 0.0)
	{
		const int id = z*zpitch + y*ypitch + x;
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
}

__kernel void dslt_l2(__read_only image3d_t in,
					  __read_only image3d_t mask,
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

	float4 coord = (float4)(x+0.5, y+0.5, z+0.5, 0);

	if (read_imagef(mask, MySampler2, coord).x > 0.0)
	{

		const int nid = n_in[id];
		const float sinlati  = sincostable[nid*4];
		const float coslati  = sincostable[nid*4+1];
		const float sinlongi = sincostable[nid*4+2];
		const float coslongi = sincostable[nid*4+3];

		const float drx = bx*coslati*coslongi - by*sinlongi;
		const float dry = bx*coslati*sinlongi + by*coslongi;
		const float drz = -bx*sinlati;

		float sum = 0.0f;

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
}

__kernel void dslt_binarize(__read_only image3d_t in,
							__read_only image3d_t mask,
							__global float* dslt,
							__global unsigned char* out,
							float constC,
							int ypitch,
							int zpitch)
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    const int z = get_global_id(2);
	
	float4 coord = (float4)(x+0.5, y+0.5, z+0.5, 0);

	if (read_imagef(mask, MySampler2, coord).x > 0.0)
	{
		const int id = z*zpitch + y*ypitch + x;
		float4 srcval = read_imagef(in, MySampler, coord);

		out[id] = (srcval.x > dslt[id] + constC) ? 255 : 0;
	}
}

__kernel void dslt_elem_min(__global float* in,
							__global float* out,
							__read_only image3d_t mask,
							int ypitch,
							int zpitch)
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    const int z = get_global_id(2);

	float4 coord = (float4)(x+0.5, y+0.5, z+0.5, 0);

	if (read_imagef(mask, MySampler, coord).x > 0.0)
	{
		const int id = z*zpitch + y*ypitch + x;

		out[id] = (out[id] > in[id]) ? in[id] : out[id];
	}
}

__kernel void dslt_ap_mask(__global unsigned char* src,
						   __read_only image3d_t mask,
							__global unsigned char* dst,
							int ypitch,
							int zpitch)
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    const int z = get_global_id(2);

	float4 coord = (float4)(x+0.5, y+0.5, z+0.5, 0);

	if (read_imagef(mask, MySampler2, coord).x > 0.0)
	{
		const int id = z*zpitch + y*ypitch + x;
		dst[id] = src[id] | dst[id];
	}
}

