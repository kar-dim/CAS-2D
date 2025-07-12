#include "CASImpl.hpp"
#include "opencl_init.hpp"
#include "opencl_utils.hpp"
#include <stdexcept>
#include <string>

//initialize empty CAS instance
CASImpl::CASImpl() : casOutputBuffer(nullptr), hostOutputBuffer(nullptr), pinnedHostOutputBuffer(nullptr), hasAlpha(false), 
rows(0), cols(0), totalBytes(0), context(cl_utils::createOpenCLContext(queue, device)), casProgram(cl_utils::buildCasKernel(context, queue, device))
{ }

//destructor, unmaps pinned memory, everything else is handled with RAII (cl::Buffers etc)
CASImpl::~CASImpl()
{
	destroyPinnedMemory();
}

//initialize buffers and texture data based on the provided image dimensions
void CASImpl::initializeMemory()
{
	//note: sizeof(cl_uchar) * 3 is not equal to sizeof(cl_uchar3), cl_uchar3 is 4 bytes aligned like cl_uchar4
	totalBytes = rows * cols * (hasAlpha ? sizeof(cl_uchar4) : sizeof(cl_uchar) * 3);
	//initialize CAS output buffers and pinned memory for output
	casOutputBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, totalBytes);
	pinnedHostOutputBuffer = cl::Buffer(context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, totalBytes);
	hostOutputBuffer = (unsigned char *)queue.enqueueMapBuffer(pinnedHostOutputBuffer, CL_TRUE, CL_MAP_WRITE | CL_MAP_READ, 0, totalBytes);
	//initialize texture
	tex = cl::Image2D(context, CL_MEM_READ_ONLY, cl::ImageFormat(CL_sRGBA, CL_UNORM_INT8), cols, rows, 0, NULL);
}

//destory and re-initialize memory objects
void CASImpl::reinitializeMemory(const bool hasAlpha, const unsigned char* hostRgbPtr, const unsigned int rows, const unsigned int cols)
{
	
	this->rows = rows;
	this->cols = cols;
	this->hasAlpha = hasAlpha;
	texKernelDims = { align<16>(rows), align<16>(cols) };
	destroyPinnedMemory();
	initializeMemory();
	cl_utils::copyBufferToImage(queue, tex, hostRgbPtr, rows, cols);
}

//destroys the host pinned buffer only if it is not already mapped
void CASImpl::destroyPinnedMemory() 
{
	if (hostOutputBuffer != nullptr) 
	{
		queue.enqueueUnmapMemObject(pinnedHostOutputBuffer, hostOutputBuffer);
		queue.finish();
		hostOutputBuffer = nullptr;
	}
}

//calls CAS kernel on the texture data, return sharpened image as unsigned char buffer (pinned memory of this CAS instance)
//overloaded method to be used when the texture data is already set (get away with one Host to Device copy if we want to sharpen the same image)
const unsigned char* CASImpl::sharpenImage(const int casMode, const float sharpenStrength, const float contrastAdaption)
{
	//execute kernel
	try {
		queue.enqueueNDRangeKernel(
			cl_utils::KernelBuilder(casProgram, "cas").args(tex, (int)hasAlpha, casMode, sharpenStrength, contrastAdaption, casOutputBuffer).build(),
			cl::NDRange(), cl::NDRange(texKernelDims.cols, texKernelDims.rows), cl::NDRange(16, 16));
		queue.finish();
		//copy from GPU to HOST
		queue.enqueueReadBuffer(casOutputBuffer, CL_TRUE, 0, totalBytes, hostOutputBuffer);
		return hostOutputBuffer;
	}
	catch (const cl::Error& ex) {
		throw std::runtime_error("ERROR in cas: " + std::string(ex.what()) + " Error code: " + std::to_string(ex.err()) + "\n");
	}
}
