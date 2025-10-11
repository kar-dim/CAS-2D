#include "include/CASLibWrapper.h"
#include <exception>
#if defined (_USE_CUDA_)
#include "CASImpl.cuh"
#elif defined(_USE_OPENCL_)
#include "CASImpl.hpp"
#endif
//Implementation of the CAS DLL API
extern "C" {

    CAS_API void* CAS_initialize()
    {
        try {
            return new CASImpl();
        }
        catch (const std::exception&) {
            return nullptr;
        }
    }

    CAS_API void CAS_supplyImage(void* casImpl, const unsigned char* inputImage, const int hasAlpha, const unsigned int rows, const unsigned int cols)
    {
        CASImpl* cas = static_cast<CASImpl*>(casImpl);
        cas->reinitializeMemory(hasAlpha, inputImage, rows, cols);
    }

    CAS_API const unsigned char* CAS_sharpenImage(void* casImpl, const int casMode, const float sharpenStrength, const float contrastAdaption)
    {
        CASImpl* cas = static_cast<CASImpl*>(casImpl);
        return cas->sharpenImage(casMode, sharpenStrength, contrastAdaption);
    }

    CAS_API void CAS_destroy(void* casImpl)
    {
        CASImpl* cas = static_cast<CASImpl*>(casImpl);
        delete cas;
    }
}