#include "OpenCLHandler.h"

OpenCLHandler::OpenCLHandler()
{
	this->openCLHandlerInit();
}

OpenCLHandler::~OpenCLHandler()
{

}

void OpenCLHandler::openCLHandlerInit()
{
	this->OpenCLKernelScript = R"(

	typedef struct _CLPosition2D
	{
		double x; 
		double y;
	} CLPosition2D;

	typedef struct _CLPosition3D
	{
		double x; 
		double y;
		double z;
	} CLPosition3D;

	typedef struct _CLSizeInt3D
	{
		int x;
		int y;
		int z;
	} CLSizeInt3D;

	typedef struct _CLPixelSpacing3D
	{
		double x;
		double y;
		double z;
	} CLPixelSpacing3D;

	__kernel void calDVHDeviceCL (__global CLPosition3D* imagePositionPatient, __global CLPixelSpacing3D* pixelSpacing, __global CLSizeInt3D* resolution, __global int* beamGyCount, __global CLPosition2D* contourData, __global int* contourDataSize, __global double* doseData, __global int* ROIVoxelCount) 
	{ 
		int xPos = get_global_id(0);
		int yPos = get_global_id(1);
		int index = (yPos * resolution->x) + xPos;
		CLPosition2D currentPos;
		currentPos.x = imagePositionPatient->x + xPos * pixelSpacing->x;
		currentPos.y = imagePositionPatient->y + yPos * pixelSpacing->y;
		int isVoxelInsideContour = 0;
		for (int i = 0; i < (*contourDataSize); i++)
		{
			int j = (i + 1) % (*contourDataSize);
			if ((contourData[i].y > currentPos.y) != (contourData[j].y > currentPos.y))
			{
				float atX = (contourData[j].x - contourData[i].x) * (currentPos.y - contourData[i].y) / (contourData[j].y - contourData[i].y) + contourData[i].x;
				if (currentPos.x < atX) { isVoxelInsideContour++; }
			}
		}
		if (isVoxelInsideContour % 2 > 0 && doseData[index] > 0.f)
		{
			int DVHcGyIndex = doseData[index] / 100.f; 
			if (DVHcGyIndex > 0) { DVHcGyIndex = DVHcGyIndex - 1; }
			atomic_inc(&beamGyCount[DVHcGyIndex]);
		}
		if (isVoxelInsideContour % 2 > 0)
		{
			atomic_inc(ROIVoxelCount);
		}
	}

	)";

	cl::Platform::get(&this->openCLObject.platforms);

	// Check platform vector is empty
	if (this->openCLObject.platforms.empty())
	{

	}

	// Get device and allocate it to platform
	auto platform = this->openCLObject.platforms.front();
	std::vector<cl::Device> devices;
	platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);

	// Check device vector is empty
	if (devices.empty()) 
	{

	}

	// Use first device found
	this->openCLObject.device = devices.front();
	this->openCLObject.context = cl::Context(this->openCLObject.device);

	// Get source strings from openclacc.h
	cl::Program::Sources sources(1, this->OpenCLKernelScript);
	this->openCLObject.program = cl::Program(this->openCLObject.context, sources);

	auto err = this->openCLObject.program.build();
	if (err != CL_BUILD_SUCCESS) 
	{

	}
}

void OpenCLHandler::calDVHHost(CLSizeInt3D* resolution, std::vector<std::vector<CLPosition2D>>* sliceContourData, std::vector<std::vector<double>>* dosecGyData, int* beamGyCountVectorSize, std::vector<std::vector<double>>* beamDVHcGyData, std::vector<std::vector<double>>* beamDVHPercentData, int* ROIVoxelCount)
{
	// Create beam Gy count vector
	std::vector<int> beamGyCountVector(*beamGyCountVectorSize);

	// Output
	cl::Buffer beamGyCountBuf(this->openCLObject.context, CL_MEM_READ_WRITE, (*beamGyCountVectorSize) * sizeof(int));
	this->openCLObject.queue.enqueueWriteBuffer(beamGyCountBuf, CL_TRUE, 0, (*beamGyCountVectorSize) * sizeof(int), beamGyCountVector.data());
	this->openCLObject.kernel.setArg(3, beamGyCountBuf);

	// Count ROI voxel
	cl::Buffer ROIVoxelCountBuf(this->openCLObject.context, CL_MEM_READ_WRITE, sizeof(int));
	this->openCLObject.queue.enqueueWriteBuffer(beamGyCountBuf, CL_TRUE, 0, sizeof(int), ROIVoxelCount);
	this->openCLObject.kernel.setArg(7, ROIVoxelCountBuf);

	// GPU calculation by each RT dose slices
	for (int i = 0; i < resolution->z; i++)
	{
		// Get contour data size
		cl_int currentSliceContourDataSize = cl_int((*sliceContourData)[i].size());

		// Create current slice data buffer
		cl::Buffer contourDataBuf(this->openCLObject.context, CL_MEM_READ_ONLY | CL_MEM_HOST_NO_ACCESS | CL_MEM_COPY_HOST_PTR, currentSliceContourDataSize * sizeof(CLPosition2D), (*sliceContourData)[i].data());
		cl::Buffer contourDataSizeBuf(this->openCLObject.context, CL_MEM_READ_ONLY | CL_MEM_HOST_NO_ACCESS | CL_MEM_COPY_HOST_PTR, sizeof(cl_int), &currentSliceContourDataSize);
		cl::Buffer doseDataBuf(this->openCLObject.context, CL_MEM_READ_ONLY | CL_MEM_HOST_NO_ACCESS | CL_MEM_COPY_HOST_PTR, size_t(resolution->x) * size_t(resolution->y) * sizeof(double), (*dosecGyData)[i].data());

		// Set current slice data to argument
		this->openCLObject.kernel.setArg(4, contourDataBuf);
		this->openCLObject.kernel.setArg(5, contourDataSizeBuf);
		this->openCLObject.kernel.setArg(6, doseDataBuf);

		// Add queue
		this->openCLObject.queue.enqueueNDRangeKernel(this->openCLObject.kernel, cl::NullRange, cl::NDRange(resolution->x, resolution->y));
	}

	this->openCLObject.queue.enqueueReadBuffer(beamGyCountBuf, CL_TRUE, 0, sizeof(int) * (*beamGyCountVectorSize), beamGyCountVector.data());
	this->openCLObject.queue.enqueueReadBuffer(ROIVoxelCountBuf, CL_TRUE, 0, sizeof(int), ROIVoxelCount);

	// Interpret dose data from GPU calculation
	std::vector<double> dvhcGyData;
	std::vector<double> dvhPercentData;
	dvhcGyData.reserve(*beamGyCountVectorSize);
	dvhPercentData.resize(*beamGyCountVectorSize);

	int currentContourTotalVolumeCount = std::accumulate(beamGyCountVector.begin(), beamGyCountVector.end(), 0);

	for (int i = 0; i < *beamGyCountVectorSize; i++)
	{
		dvhcGyData.push_back(double(i) * 100); // Set cGy data (100 cGy (1 Gy term))
		int accumulatedVolumeCount{}; // Define accumulated volume count
		double accumulatedVolumePercent{}; // Define accumulated volume percent

		// Calculate accumulated count from high dose, and divided by total volumes in contour
		for (int j = int(*beamGyCountVectorSize - 1); j > *beamGyCountVectorSize - 2 - i; j--) accumulatedVolumeCount += beamGyCountVector[j];

		accumulatedVolumePercent = (double(accumulatedVolumeCount) / double(currentContourTotalVolumeCount)) * 100.f;
		dvhPercentData[*beamGyCountVectorSize - 1 - i] = accumulatedVolumePercent;
	}

	// Add DVH data to vector
	beamDVHcGyData->push_back(dvhcGyData);
	beamDVHPercentData->push_back(dvhPercentData);
}

OpenCLObject* OpenCLHandler::getOpenCLObject()
{
	return &(this->openCLObject);
}