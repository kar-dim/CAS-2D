#pragma once
#include "opencl_init.hpp"

enum CASMode 
{
	PLANAR_RGB,
	INTERLEAVED_RGBA
};

struct dim2
{
	unsigned int rows;
	unsigned int cols;
};

//Main class responsible for managing OpenCL memory and calling the CAS kernel to sharpen the input image
class CASImpl
{
private:
	cl::Context context;
	cl::CommandQueue queue;
	cl::Device device;
	cl::Image2D tex;
	cl::Buffer casOutputBuffer;
	cl::Buffer pinnedHostOutputBuffer;
	cl::Program casProgram;
	unsigned char* hostOutputBuffer;
	bool hasAlpha;
	unsigned int rows, cols;
	unsigned long long totalBytes;
	const dim2 localSize { 16, 16 };
	dim2 texKernelDims { 0, 0 };

	void initializeMemory();
	void destroyPinnedMemory();

public:
	CASImpl();
	~CASImpl();

	//delete move/copy ctors/operators, not useful for a DLL class
	CASImpl(const CASImpl& other) = delete;
	CASImpl(CASImpl&& other) noexcept = delete;
	CASImpl& operator=(CASImpl&& other) noexcept = delete;
	CASImpl& operator=(const CASImpl& other) = delete;

	void reinitializeMemory(const bool hasAlpha, const unsigned char* hostRgbPtr, const unsigned int rows, const unsigned int cols);
	const unsigned char* sharpenImage(const int casMode, const float sharpenStrength, const float contrastAdaption);
};