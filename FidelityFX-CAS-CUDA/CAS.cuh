#pragma once
#include <cuda_runtime.h>
#include "helper_math.h"

//Main CAS kernel
//Template: hasAlpha: whether the input image has an alpha channel
//Input: texObj: input (sRGB) texture object
//		 sharpenStrength: sharpening strength
//		 contrastAdaption: contrast adaption
//		 casOutput: output RGB(A) interleaved
//		 height: height of the input texture
//		 width: width of the input texture
//Output: None
template <const bool hasAlpha>
__global__ void cas(cudaTextureObject_t texObj, const float sharpenStrength, const float contrastAdaption, unsigned char* casOutput, const unsigned int height, const unsigned int width)
{
	const int x = blockIdx.x * blockDim.x + threadIdx.x;
	const int y = blockIdx.y * blockDim.y + threadIdx.y;
	const int outputIndex = (y * width) + x;

	if (x >= width || y >= height)
		return;

	// fetch a 3x3 neighborhood around the pixel 'e', a = a, d = d, ....i = i 
	//  a b c
	//  d(e)f
	//  g h i
	const float4 currentPixel = tex3D<float4>(texObj, x, y, 0);
	//speedup if alpha is zero -> just write the alpha value only and return
	if constexpr (hasAlpha)
	{
		if (currentPixel.w == 0)
		{
			casOutput[4 * outputIndex + 3] = 0;
			return;
		}	
	}
	const float3 e = make_float3(currentPixel);
	const float3 a = make_float3(tex3D<float4>(texObj, x - 1, y - 1, 0));
	const float3 b = make_float3(tex3D<float4>(texObj, x, y - 1, 0));
	const float3 c = make_float3(tex3D<float4>(texObj, x + 1, y - 1, 0));
	const float3 d = make_float3(tex3D<float4>(texObj, x - 1, y, 0));
	const float3 f = make_float3(tex3D<float4>(texObj, x + 1, y, 0));
	const float3 g = make_float3(tex3D<float4>(texObj, x - 1, y + 1, 0));
	const float3 h = make_float3(tex3D<float4>(texObj, x, y + 1, 0));
	const float3 i = make_float3(tex3D<float4>(texObj, x + 1, y + 1, 0));

	// Soft min and max.
	//  a b c             b
	//  d e f * 0.5  +  d e f * 0.5
	//  g h i             h
	// These are 2.0x bigger (factored out the extra multiply).
	float3 mnRGB = fminf(fminf(fminf(d, e), fminf(f, b)), h);
	const float3 mnRGB2 = fminf(mnRGB, fminf(fminf(a, c), fminf(g, i)));
	mnRGB += mnRGB2;

	float3 mxRGB = fmaxf(fmaxf(fmaxf(d, e), fmaxf(f, b)), h);
	const float3 mxRGB2 = fmaxf(mxRGB, fmaxf(fmaxf(a, c), fmaxf(g, i)));
	mxRGB += mxRGB2;

	// Smooth minimum distance to signal limit divided by smooth max.
	const float3 ampRGB = rsqrtf(saturate(fminf(mnRGB, 2.0f - mxRGB) * rcp(mxRGB)));

	// Shaping amount of sharpening.
	const float3 wRGB = -rcp(ampRGB * (-3.0f * contrastAdaption + 8.0f));
	const float3 rcpWeightRGB = rcp(4.0f * wRGB + 1.0f);

	//						  0 w 0
	//  Filter shape:		  w 1 w
	//						  0 w 0  
	const float3 filterWindow = (b + d) + (f + h);
	const float3 outColor = saturate((filterWindow * wRGB + e) * rcpWeightRGB);
	const float3 sharpenedValues = fastLerp(e, outColor, sharpenStrength);

	//write to global memory
	const unsigned char colorR = normalizedFloatToUchar(linearToSRGB(sharpenedValues.x));
	const unsigned char colorG = normalizedFloatToUchar(linearToSRGB(sharpenedValues.y));
	const unsigned char colorB = normalizedFloatToUchar(linearToSRGB(sharpenedValues.z));
	//interleaved mode, write RGB(A) consecutively (uncoalesced writes, low performance hit, may optimize)
	if constexpr (hasAlpha)
	{
		casOutput[4 * outputIndex] = colorR;
		casOutput[4 * outputIndex + 1] = colorG;
		casOutput[4 * outputIndex + 2] = colorB;
		casOutput[4 * outputIndex + 3] = normalizedFloatToUchar(currentPixel.w);
	}
	else
	{
		casOutput[3 * outputIndex] = colorR;
		casOutput[3 * outputIndex + 1] = colorG;
		casOutput[3 * outputIndex + 2] = colorB;
	}	
}
