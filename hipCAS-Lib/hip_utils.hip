#include "hip_utils.hpp"
#include <hip/hip_runtime.h>
#include <utility>

namespace hip_utils {
// helper method to calculate kernel grid size from given 2D dimensions and blockSize
dim3 gridSizeCalculate(const dim3 blockSize, const int rows, const int cols) { return dim3((cols + blockSize.x - 1) / blockSize.x, (rows + blockSize.y - 1) / blockSize.y); }

// simple wrapper of hipMallocArray to reduce boilerplate
hipArray_t hipMallocArray(const std::size_t cols, const std::size_t rows) {
    hipArray_t arr;
    auto channelDescriptor = hipCreateChannelDesc<uchar4>();
    hipExtent extent = make_hipExtent(cols, rows, 1); // Single-layer array
    hipMalloc3DArray(&arr, &channelDescriptor, extent, 0);
    return arr;
}

// creates a hipResourceDesc from a specified hipArray
hipResourceDesc createResourceDescriptor(hipArray_t hipArray) {
    hipResourceDesc resDesc{};
    resDesc.resType = hipResourceTypeArray;
    resDesc.res.array.array = hipArray;
    return resDesc;
}

// creates a hipTextureDesc with these properties:
// Border addressing mode, point filtering mode,
// sRGB to linear conversion in hardware,
// normalized float [0,1] read mode, non-normalized coords
hipTextureDesc createTextureDescriptor() {
    hipTextureDesc texDesc{};
    texDesc.addressMode[0] = hipAddressModeBorder;
    texDesc.addressMode[1] = hipAddressModeBorder;
    texDesc.addressMode[2] = hipAddressModeClamp;
    texDesc.sRGB = 1;
    texDesc.filterMode = hipFilterModePoint;
    texDesc.readMode = hipReadModeNormalizedFloat; // Read as float in [0,1]
    texDesc.normalizedCoords = 0;
    return texDesc;
}

// creates a texture object from the given hip resource and hip texture descriptors
hipTextureObject_t createTextureObject(const hipResourceDesc& pResDesc, const hipTextureDesc& pTexDesc) {
    hipTextureObject_t texObj = 0;
    hipCreateTextureObject(&texObj, &pResDesc, &pTexDesc, NULL);
    return texObj;
}

// create the hipArray and the textureObject binded to this array.
std::pair<hipTextureObject_t, hipArray_t> createTextureData(const unsigned int rows, const unsigned int cols) {
    hipArray_t arr = hip_utils::hipMallocArray(cols, rows);
    hipResourceDesc resDesc = hip_utils::createResourceDescriptor(arr);
    hipTextureDesc texDesc = hip_utils::createTextureDescriptor();
    hipTextureObject_t texObj = hip_utils::createTextureObject(resDesc, texDesc);
    return std::make_pair(texObj, arr);
}

// copy Host data to Device Array
void copyDataToHipArray(const unsigned char* data, const unsigned int rows, const unsigned int cols, hipArray_t hipArray) {
    hipMemcpy3DParms copyParams{};
    copyParams.kind = hipMemcpyHostToDevice;
    copyParams.dstArray = hipArray;
    // Interleaved data in packed [r, g, b, 0, r, g, b, 0...] format
    copyParams.srcPtr = make_hipPitchedPtr((void*)data, cols * 4 * sizeof(unsigned char), cols, rows);
    copyParams.extent = make_hipExtent(cols, rows, 1); // Depth is 1 because all channels are interleaved per pixel
    hipMemcpy3D(&copyParams);
}
} // namespace hip_utils