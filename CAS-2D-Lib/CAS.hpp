#pragma once
#include <string>
inline const std::string cas = R"CLC(

#pragma OPENCL EXTENSION cl_khr_fp16 : enable

/* HELPER FUNCTIONS */
// *******************************************************************************************************
// *******************************************************************************************************

//faster linear interpolation by using FMA operations
inline float3 flerp3(const float3 v0, const float3 v1, const float t)
{
	return (float3)(fma(t, v1.x, fma(-t, v0.x, v0.x)), fma(t, v1.y, fma(-t, v0.y, v0.y)), fma(t, v1.z, fma(-t, v0.z, v0.z)));
}

inline float3 fmax3(float3 a, float3 b) 
{
	return (float3)(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z));
}

inline float3 fmin3(float3 a, float3 b)
{
	return (float3)(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z));
}

inline float3 make_float3(const float4 values)
{
	return (float3)(values.x, values.y, values.z);
}

//converts a float in the range [0,1] to an unsigned char in the range [0,255]
inline unsigned char floatToUchar(const float value)
{
    return convert_uchar_sat_rte(value * 255.0f);
}

//Convert a linear RGB value to sRGB value
inline float sRGB(const float linearColor)
{
    return linearColor <= 0.0031308f ? linearColor * 12.92f : 1.055f * pow(linearColor, 0.416667f) - 0.055f;
}

/* HELPER FUNCTIONS END */
// *******************************************************************************************************
// *******************************************************************************************************

#define RGB 0
#define RGBA 1

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
__kernel void cas(
    __read_only image2d_t image,
	const int hasAlpha, 
	const int casMode,
    const float sharpenStrength, 
    const float contrastAdaption,
    __global unsigned char* casOutput)
{
    const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;
    const int2 pixelCoord = (int2)(get_global_id(0), get_global_id(1));
    const int x = pixelCoord.x;
    const int y = pixelCoord.y;
    const int width = get_image_width(image);
    const int height = get_image_height(image);
    const int outputIndex = (y * width) + x;

    if (x >= width || y >= height)
        return;

	// fetch a 3x3 neighborhood around the pixel 'e', a = a, d = d, ....i = i 
	//  a b c
	//  d(e)f
	//  g h i
	const float4 currentPixel = read_imagef(image, sampler, pixelCoord);
	//speedup if alpha is zero -> just write the alpha value only and return
	if (hasAlpha)
	{
		if (currentPixel.w == 0.0f)
		{
			if (casMode == RGB)
				casOutput[width * height * 3 + outputIndex] = 0;
			else 
				((__global uchar4*)casOutput)[outputIndex] = (uchar4)(0, 0, 0, 0);
			return;
		}
	}
	const float3 e = make_float3(currentPixel);
	const float3 a = make_float3(read_imagef(image, sampler, (int2)(x - 1, y - 1)));
	const float3 b = make_float3(read_imagef(image, sampler, (int2)(x, y - 1)));
	const float3 c = make_float3(read_imagef(image, sampler, (int2)(x + 1, y - 1)));
	const float3 d = make_float3(read_imagef(image, sampler, (int2)(x - 1, y)));
	const float3 f = make_float3(read_imagef(image, sampler, (int2)(x + 1, y)));
	const float3 g = make_float3(read_imagef(image, sampler, (int2)(x - 1, y + 1)));
	const float3 h = make_float3(read_imagef(image, sampler, (int2)(x, y + 1)));
	const float3 i = make_float3(read_imagef(image, sampler, (int2)(x + 1, y + 1)));

	// Soft min and max.
	//  a b c             b
	//  d e f * 0.5  +  d e f * 0.5
	//  g h i             h
	// These are 2.0x bigger (factored out the extra multiply).
	float3 mnRGB = fmin3(fmin3(fmin3(d, e), fmin3(f, b)), h);
	const float3 mnRGB2 = fmin3(mnRGB, fmin3(fmin3(a, c), fmin3(g, i)));
	mnRGB += mnRGB2;

	float3 mxRGB = fmax3(fmax3(fmax3(d, e), fmax3(f, b)), h);
	const float3 mxRGB2 = fmax3(mxRGB, fmax3(fmax3(a, c), fmax3(g, i)));
	mxRGB += mxRGB2;

	// Smooth minimum distance to signal limit divided by smooth max.
	const float3 ampRGB = native_rsqrt(clamp(fmin3(mnRGB, 2.0f - mxRGB) * native_recip(mxRGB), 0.0f, 1.0f));

	// Shaping amount of sharpening.
	const float3 wRGB = -native_recip(ampRGB * (-3.0f * contrastAdaption + 8.0f));
	const float3 rcpWeightRGB = native_recip(4.0f * wRGB + 1.0f);

	//						  0 w 0
	//  Filter shape:		  w 1 w
	//						  0 w 0  
	const float3 filterWindow = (b + d) + (f + h);
	const float3 outColor = clamp((filterWindow * wRGB + e) * rcpWeightRGB, 0.0f, 1.0f);
	const float3 sharpenedValues = flerp3(e, outColor, sharpenStrength);

	//convert to uchar sRGB
	const unsigned char colorR = floatToUchar(sRGB(sharpenedValues.x));
	const unsigned char colorG = floatToUchar(sRGB(sharpenedValues.y));
	const unsigned char colorB = floatToUchar(sRGB(sharpenedValues.z));

	//Write to global memory based on template params
	//If hasAlpha is true, write the alpha channel as well
	//if casMode == 0 -> write planar RGB (slower, strided writes)
	if (casMode == RGB)
	{
		casOutput[outputIndex] = colorR;
		casOutput[width * height + outputIndex] = colorG;
		casOutput[width * height * 2 + outputIndex] = colorB;
		if (hasAlpha)
			casOutput[width * height * 3 + outputIndex] = floatToUchar(currentPixel.w);
	}
	//write interleaved RGBA
	else
	{
		//if alpha is needed, we fully utilize the memory coalescing by writing 4 bytes at once (1 memory transaction)
		if (hasAlpha) 
			((__global uchar4*)casOutput)[outputIndex] = (uchar4)(colorR, colorG, colorB, floatToUchar(currentPixel.w));
		else
		{
			// can't coalesce, write 3 bytes per thread (still far better than fully coalescing but transfering an extra channel back to host)
			int outIndex = outputIndex * 3;
			casOutput[outIndex + 0] = colorR;
			casOutput[outIndex + 1] = colorG;
			casOutput[outIndex + 2] = colorB;
		}
	}
}

)CLC";