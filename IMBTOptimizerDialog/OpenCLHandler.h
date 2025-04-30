#pragma once

#include "CL/opencl.hpp"
#include <numeric>

struct CLPosition2D
{
	cl_double x{};
	cl_double y{};
};

struct CLPosition3D
{
	cl_double x{};
	cl_double y{};
	cl_double z{};
};

struct CLSizeInt3D
{
	cl_int x{};
	cl_int y{};
	cl_int z{};
};

struct CLPixelSpacing3D
{
	cl_double x{};
	cl_double y{};
	cl_double z{};
};

struct OpenCLObject
{
	cl::Program program;
	cl::Context context;
	cl::Device device;
	std::vector<cl::Platform> platforms;
	cl::Kernel kernel;
	cl::CommandQueue queue;
};

class OpenCLHandler
{
public:
	OpenCLHandler();
	~OpenCLHandler();

	void openCLHandlerInit();
	void calDVHHost(CLSizeInt3D* resolution, std::vector<std::vector<CLPosition2D>>* sliceContourData, std::vector<std::vector<double>>* currentBeamData, int* beamGyCountVectorSize, std::vector<std::vector<double>>* beamDVHGyData, std::vector<std::vector<double>>* beamDVHPercentData, int* ROIVoxelCount);
	OpenCLObject* getOpenCLObject();

private:
	OpenCLObject openCLObject;
	std::string OpenCLKernelScript;
};