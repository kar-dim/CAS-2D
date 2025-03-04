#include "opencl_utils.hpp"
#include <exception>
#include <fstream>

#include <iterator>
#include <stdexcept>
#include <string>

using std::string;

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

int cl_utils::calculateDeviceScore(const cl::Device& device)
{
    int score = 0;
    score += device.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() * 100;
    score += device.getInfo<CL_DEVICE_MAX_CLOCK_FREQUENCY>() * 10;
    score += static_cast<int>(device.getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>() / (1024 * 1024 * 1024)); // GB
    score += static_cast<int>(device.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>() / 1024);  // KB
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
        int bestScore = -1;

        // Iterate through all platforms and devices
        for (const auto& platform : platforms) 
        {
            std::vector<cl::Device> devices;
            platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);

            for (const auto& device : devices) 
            {
                int score = cl_utils::calculateDeviceScore(device);
                if (score > bestScore) 
                {
                    bestScore = score;
                    bestDevice = device;
                }
            }
        }

        if (bestScore == -1) 
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

string cl_utils::loadFileString(const string& input)
{
    std::ifstream stream(input.c_str());
    if (!stream.is_open())
        throw std::runtime_error("Could not load Program: " + input + "\n");
    return string(std::istreambuf_iterator<char>(stream), (std::istreambuf_iterator<char>()));
}

cl::Program cl_utils::buildCasKernel(cl::Context& context, cl::CommandQueue& queue, cl::Device& device)
{
    //compile opencl kernel
    cl::Program casKernel;
    try {
        const string options = "-cl-mad-enable";
        casKernel = cl::Program(context, cl_utils::loadFileString("../FidelityFX-CAS-CUDA/kernels/cas.cl"));
        casKernel.build(device, options.c_str());
    }
    catch (const std::exception& ex) {
        throw ex;
    }
    return casKernel;
}
