#include "cas.hpp"
#include "opencl_init.hpp"
#include "opencl_utils.hpp"
#include <exception>
#include <stdexcept>
#include <string>
#include <vector>

using std::string;
#define ULONG(x) static_cast<unsigned long>(x)

cl_utils::KernelBuilder::KernelBuilder(const cl::Program& program, const char* name)
{
    kernel = cl::Kernel(program, name);
    argsCounter = 0;
}

cl::Kernel cl_utils::KernelBuilder::build() const
{
    return kernel;
}

void cl_utils::copyBufferToImage(const cl::CommandQueue& queue, const cl::Image2D& image2d, const unsigned char* hostRgbPtr, const long long rows, const long long cols)
{
    const cl::array<size_t, 3> orig { 0,0,0 };
    const cl::array<size_t, 3> des { static_cast<size_t>(cols), static_cast<size_t>(rows), 1 };
    queue.enqueueWriteImage(image2d, CL_TRUE, orig, des, 0, 0, hostRgbPtr);
}

unsigned long cl_utils::calculateDeviceScore(const cl::Device& device)
{
    unsigned long score = 0;
    //Core hardware properties
    score += ULONG(device.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() * 200);
    score += ULONG(device.getInfo<CL_DEVICE_MAX_CLOCK_FREQUENCY>() * 10);
    score += ULONG(device.getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>() / (1024 * 1024 * 256)); //256 MB chunks
    score += ULONG(device.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>() / 1024); //Local memory in KB
    // Image2D (texture) support
    score += ULONG(device.getInfo<CL_DEVICE_IMAGE2D_MAX_WIDTH>() / 512);
    score += ULONG(device.getInfo<CL_DEVICE_IMAGE2D_MAX_HEIGHT>() / 512);
    // Memory cache
    score += ULONG(device.getInfo<CL_DEVICE_GLOBAL_MEM_CACHE_SIZE>() / (1024 * 64)); // 64 KB chunks
    score += ULONG(device.getInfo<CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE>());
    return score;
}

cl::Context cl_utils::createOpenCLContext(cl::CommandQueue &queue, cl::Device &device) 
{
    try {
        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);
        if (platforms.empty()) 
            throw std::runtime_error("No OpenCL platforms found.");

        cl::Device bestDevice;
        unsigned long bestScore = 0;

        // Iterate through all platforms and devices
        for (const auto& platform : platforms) 
        {
            std::vector<cl::Device> devices;
            platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);

            for (const auto& device : devices) 
            {
                auto score = cl_utils::calculateDeviceScore(device);
                if (score > bestScore) 
                {
                    bestScore = score;
                    bestDevice = device;
                }
            }
        }

        if (bestScore == 0) 
            throw std::runtime_error("No suitable OpenCL devices found.");

        // Create context and queue for the fastest device
        cl::Context context(bestDevice);
        queue = cl::CommandQueue(context, bestDevice, CL_QUEUE_PROFILING_ENABLE);
        device = bestDevice;
        return context;
    }
    catch (const cl::Error& ex) 
    {
        throw ex;
    }
}

cl::Program cl_utils::buildCasKernel(cl::Context& context, cl::CommandQueue& queue, cl::Device& device)
{
    //compile opencl kernel
    cl::Program casKernel;
    try {
        casKernel = cl::Program(context, cas);
        casKernel.build(device, "-cl-mad-enable");
    }
    catch (const std::exception& ex) {
        throw ex;
    }
    return casKernel;
}
