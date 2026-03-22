#include "hip_utils.hpp"
#include <hip/hip_runtime.h>
#include <utility>

namespace hip_utils {
// helper method to calculate kernel grid size from given 2D dimensions and blockSize
dim3 gridSizeCalculate(const dim3 blockSize, const int rows, const int cols) { return dim3((cols + blockSize.x - 1) / blockSize.x, (rows + blockSize.y - 1) / blockSize.y); }

// simple wrapper of hipMallocArray to reduce boilerplate
hipArray_t hipMallocArray(const std::size_t cols, const std::size_t rows) {
    hipArray_t arr;
    const auto channelDescriptor = hipCreateChannelDesc<uchar4>();
    hipMallocArray(&arr, &channelDescriptor, cols, rows, hipArrayDefault);
    return arr;
}

// creates a hipResourceDesc from a specified hipArray
hipResourceDesc createResourceDescriptor(const hipArray_t hipArray) {
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
    texDesc.sRGB = 1; // do automatic sRGB to linear
    texDesc.filterMode = hipFilterModePoint;
    texDesc.readMode = hipReadModeNormalizedFloat; // read as float in [0,1]
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
    const hipArray_t arr = hipMallocArray(cols, rows);
    return std::make_pair(createTextureObject(createResourceDescriptor(arr), createTextureDescriptor()), arr);
}

// copy Host data to Device Array
void copyDataToHipArray(const unsigned char* data, const unsigned int rows, const unsigned int cols, hipArray_t hipArray) {
    const size_t widthBytes = cols * 4 * sizeof(unsigned char);
    hipMemcpy2DToArray(hipArray, 0, 0, data, widthBytes, widthBytes, rows, hipMemcpyHostToDevice);
}
} // namespace hip_utils