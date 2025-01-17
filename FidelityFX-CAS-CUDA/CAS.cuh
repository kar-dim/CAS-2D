#pragma once
#include <cuda_runtime.h>
#include <cuda_fp16.h>
#include "helper_math.h"

//Main CAS kernel
//Template: hasAlpha: whether the input image has an alpha channel
//			casMode: whether the output image should be written as interleaved RGBA or planar RGB
//Input: texObj: input (sRGB) texture object
//		 sharpenStrength: sharpening strength
//		 contrastAdaption: contrast adaption
//		 casOutput: output RGB(A) interleaved
//		 height: height of the input texture
//		 width: width of the input texture
//Output: None
template <const bool hasAlpha, const int casMode>
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
	const half4 currentPixel = make_half4(tex3D<float4>(texObj, x, y, 0));
	//speedup if alpha is zero -> just write the alpha value only and return
	if constexpr (hasAlpha)
	{
		if (__high2half(currentPixel.y) == __float2half(0.0f))
		{
			if constexpr (casMode == 0)
				casOutput[width * height * 3 + outputIndex] = 0;
			else
				casOutput[4 * outputIndex + 3] = 0;
			return;
		}	
	}
	const half3 e = make_half3(currentPixel);
	const half3 a = make_half3(tex3D<float4>(texObj, x - 1, y - 1, 0));
	const half3 b = make_half3(tex3D<float4>(texObj, x, y - 1, 0));
	const half3 c = make_half3(tex3D<float4>(texObj, x + 1, y - 1, 0));
	const half3 d = make_half3(tex3D<float4>(texObj, x - 1, y, 0));
	const half3 f = make_half3(tex3D<float4>(texObj, x + 1, y, 0));
	const half3 g = make_half3(tex3D<float4>(texObj, x - 1, y + 1, 0));
	const half3 h = make_half3(tex3D<float4>(texObj, x, y + 1, 0));
	const half3 i = make_half3(tex3D<float4>(texObj, x + 1, y + 1, 0));

	// Soft min and max.
	//  a b c             b
	//  d e f * 0.5  +  d e f * 0.5
	//  g h i             h
	// These are 2.0x bigger (factored out the extra multiply).
	half3 mnRGB = hmin3(hmin3(hmin3(d, e), hmin3(f, b)), h);
	const half3 mnRGB2 = hmin3(mnRGB, hmin3(hmin3(a, c), hmin3(g, i)));
	mnRGB += mnRGB2;

	half3 mxRGB = hmax3(hmax3(hmax3(d, e), hmax3(f, b)), h);
	const half3 mxRGB2 = hmax3(mxRGB, hmax3(hmax3(a, c), hmax3(g, i)));
	mxRGB += mxRGB2;

	// Smooth minimum distance to signal limit divided by smooth max.
	const half3 ampRGB = rsqrtf(saturate(hmin3(mnRGB, __float2half(2.0f) - mxRGB) * rcp(mxRGB)));

	// Shaping amount of sharpening.
	const half3 wRGB = -rcp(ampRGB * (__float2half(-3.0f) * __float2half(contrastAdaption) + __float2half(8.0f)));
	const half3 rcpWeightRGB = rcp(__float2half(4.0f) * wRGB + __float2half(1.0f));

	//						  0 w 0
	//  Filter shape:		  w 1 w
	//						  0 w 0  
	const half3 filterWindow = (b + d) + (f + h);
	const half3 outColor = saturate((filterWindow * wRGB + e) * rcpWeightRGB);
	const half3 sharpenedValues = fastLerp(e, outColor, __float2half(sharpenStrength));

	//write to global memory
	const unsigned char colorR = normalizedHalfToUchar(linearToSRGB(__low2half(sharpenedValues.x)));
	const unsigned char colorG = normalizedHalfToUchar(linearToSRGB(__high2half(sharpenedValues.x)));
	const unsigned char colorB = normalizedHalfToUchar(linearToSRGB(sharpenedValues.y));
	
	//Write to global memory based on template params
	//If hasAlpha is true, write the alpha channel as well
	//if casMode == 0 -> write planar RGB
	if constexpr (casMode == 0)
	{
		casOutput[outputIndex] = colorR;
		casOutput[width * height + outputIndex] = colorG;
		casOutput[width * height * 2 + outputIndex] = colorB;
		if constexpr (hasAlpha)
			casOutput[width * height * 3 + outputIndex] = normalizedHalfToUchar(__high2half(currentPixel.y));
	}
	else //write interleaved RGBA
	{
		const int baseIndex = (hasAlpha ? 4 : 3) * outputIndex; //ternary check will be optimized out
		casOutput[baseIndex] = colorR;
		casOutput[baseIndex + 1] = colorG;
		casOutput[baseIndex + 2] = colorB;
		if constexpr (hasAlpha)
			casOutput[baseIndex + 3] = normalizedHalfToUchar(__high2half(currentPixel.y));
	}
}
