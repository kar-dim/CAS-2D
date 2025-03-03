__kernel void cas(
    __read_only image2d_t image,
    const float sharpenStrength, 
    const float contrastAdaption
    __global unsigned char* casOutput)
{
    const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

    int2 pixelCoord = (int2)(get_global_id(0), get_global_id(1));
    const int width = get_image_height(image);
    const int height = get_image_width(image);

    if (x >= width || x >= height)
        return;

    // Read RGBA pixel from the image (sRGB encoded)
    float4 pixel = read_imagef(image, sampler, pixelCoord);

    // Apply sRGB to linear conversion (standard formula)
    float r = (r <= 0.04045f) ? (r / 12.92f) : pow((r + 0.055f) / 1.055f, 2.4f);
    float g = (g <= 0.04045f) ? (g / 12.92f) : pow((g + 0.055f) / 1.055f, 2.4f);
    float b = (b <= 0.04045f) ? (b / 12.92f) : pow((b + 0.055f) / 1.055f, 2.4f);
}