#pragma once
#include "opencl_init.hpp"

namespace cl_utils
{
    class KernelBuilder
    {
    private:
        cl::Kernel kernel;
        int argsCounter;
    public:
        KernelBuilder(const cl::Program& program, const char* name);

        /*! \brief setArg overload taking a POD type */
        template <typename... T>
        KernelBuilder& args(const T&... values)
        {
            (kernel.setArg<T>(argsCounter++, values), ...);
            return *this;
        }

        /*! \brief build the cl::Kernel object */
        cl::Kernel build() const;
    };
    
    //helper method to copy host pinned memory into an OpenCL Image
    void copyBufferToImage(const cl::CommandQueue& queue, const cl::Image2D& image2d, const unsigned char* hostRgbPtr, const long long rows, const long long cols);

    //find the fastes opencl device on the system based on various metrics
    unsigned long calculateDeviceScore(const cl::Device& device);

    //create OpenCL Queue, Device and Context
    cl::Context createOpenCLContext(cl::CommandQueue& queue, cl::Device& device);

    //builds the CAS kernel
    cl::Program buildCasKernel(cl::Context& context, cl::CommandQueue& queue, cl::Device& device);
}