#pragma once
#include <hip/hip_runtime.h>
#include <utility>

// Helper functions related to HIP
namespace hip_utils {
dim3 gridSizeCalculate(const dim3 blockSize, const int rows, const int cols);
hipArray_t hipMallocArray(const std::size_t cols, const std::size_t rows);
hipResourceDesc createResourceDescriptor(const hipArray_t hipArray);
hipTextureDesc createTextureDescriptor();
hipTextureObject_t createTextureObject(const hipResourceDesc& pResDesc, const hipTextureDesc& pTexDesc);
std::pair<hipTextureObject_t, hipArray_t> createTextureData(const unsigned int rows, const unsigned int cols);
void copyDataToHipArray(const unsigned char* data, const unsigned int rows, const unsigned int cols, hipArray_t hipArray);
} // namespace hip_utils