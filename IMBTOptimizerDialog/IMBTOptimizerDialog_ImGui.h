#pragma once

#define STB_IMAGE_IMPLEMENTATION
#define NOMINMAX

#ifdef _HAS_STD_BYTE
#undef _HAS_STD_BYTE
#define _HAS_STD_BYTE 1
#endif

#define USE_MATLAB_ENGINE

#ifdef USE_MATLAB_ENGINE
// Matlab Engine
#include "MatlabDataArray.hpp"
#include "MatlabEngine.hpp"
#endif

#include "Plan.h"

// Graphic library
#include "GL/gl3w.h"	// gl3w
#include "GLFW/glfw3.h" // glfw

// GUI library (ImGui, Immediate Mode GUI)
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"
#include "implot.h"
#include "implot_internal.h"

// Image loading library

// XTL library for voxel
#include "xtensor.hpp"

// RTToolbox for calculating DVH
#include "boost/make_shared.hpp"
#include "boost/shared_ptr.hpp"

#include "rttbBaseType.h"
#include "rttbDVHCalculator.h"
#include "rttbGenericMaskedDoseIterator.h"
#include "rttbGeometricInfo.h"
#include "rttbGenericDoseIterator.h"
#include "rttbBoostMaskAccessor.h"
#include "DummyDoseAccessor.h"
#include "rttbStrVectorStructureSetGenerator.h"
#include "rttbStructure.h"

// Optimization
#include "armadillo"
#include "NumCpp.hpp"

// OpenCL for GPU-based DVH calculation
#include "OpenCLHandler.h"

// C++ original library
#include <vector>
#include <array>
#include <iostream>
#include <string>
#include <thread>
#include <deque>
#include <random>
#include <ctime>
#include <functional>

using namespace arma;

#ifdef USE_MATLAB_ENGINE
// Matlab Engine
class MatlabEngine
{
private:
	MatlabEngine()
		: m_pMatlab{matlab::engine::startMATLAB()} {};

public:
	std::unique_ptr<matlab::engine::MATLABEngine> m_pMatlab;
	static MatlabEngine &instance()
	{
		static MatlabEngine INSTANCE;
		return INSTANCE;
	}
	static void startMatlabEngine()
	{
		auto& inst = MatlabEngine::instance();
		if (!inst.m_pMatlab) {
			// 만약 아직 엔진이 없다면, 다시 startMATLAB()
			inst.m_pMatlab = matlab::engine::startMATLAB();
		}
	}

	// ★ 추가된 부분: 엔진 종료 함수
	void stopMatlabEngine() {
		if (m_pMatlab) {
			// unique_ptr를 해제하면, ~MATLABEngine()이 호출됨
			// => 그 내부에서 cpp_engine_terminate_out_of_process_matlab() 실행
			m_pMatlab.reset();
		}
	}
};
#endif // USE_MATLAB_ENGINE

/* Basic data type struct -------------------------------------------------------------- */
struct PointFloat3D
{
	float x{};
	float y{};
	float z{};
	CString strReferencedSOPInstanceUID;
	float z_sliceIndex{};
};

struct PointFloat2D
{
	float x{};
	float y{};
};

struct PointInt2D
{
	int x{};
	int y{};
};

struct PointInt3D
{
	int x{};
	int y{};
	int z{};
};

struct SizeInt3D
{
	size_t x{};
	size_t y{};
	size_t z{};
};

struct SizeInt2D
{
	size_t x{};
	size_t y{};
};

struct SizeFloat3D
{
	float x{};
	float y{};
	float z{};
};

struct Color
{
	float R{};
	float G{};
	float B{};
	float A{};
};

/* RT information struct -------------------------------------------------------------- */
struct ConstraintsSubInfo
{
	int idType{};
	double constraintVolumePercent{};
	int constraintDose{};
	int constraintActualDose{};
	int constraintPriority{};
};

struct Constraints
{
	int index{};
	int ROINameIndex{};
	double ROIVolume{};
	std::vector<ConstraintsSubInfo> constraintSubInfo;
};

struct RTPlan
{
	float prescriptionDose{}; // cGy
	float fractions;
	float normalization; // Percentages
	int angleResolution{10};
	int numOfDwellPositionTandem{27};
	int numOfDwellPositionOvoid{5};
};

struct RTStruct
{
	std::vector<std::string> ROINames;
	std::vector<Color> ROIColor;
	std::vector<std::vector<std::vector<PointFloat3D>>> ROIContour;
	std::vector<std::vector<std::vector<std::vector<PointFloat3D>>>> CTImageAxialContourData;
	std::vector<std::vector<std::uint16_t>> CTImageAxialContourIndexData;
	std::vector<std::vector<std::vector<ImVec2>>> CTImageAxialContourOriginPos;
	std::vector<std::vector<std::vector<ImVec2>>> CTImageAxialContourScreenPos;
	std::vector<std::vector<std::vector<ImVec2>>> currentCTImageContourDrawData;
	std::deque<bool> ROIPresentation;
	bool currentCTImageContourDataLoading{};
	bool contourDataRemappingForViewRegion{};
};

struct CalculatedDose
{
	CalculatedDose()
	{
		InitializedDoseKernel = false;
		InitializationInProgress = false;
		calculationComplete = false;
		calculationInProcess = false;
		calculatedDoseConfirm = false;
		refreshDoseTexture = false;
	}

	bool InitializedDoseKernel{};
	bool InitializationInProgress{};
	bool calculationComplete{};
	bool calculationInProcess{};
	bool calculatedDoseConfirm{};
	bool refreshDoseTexture{};
	double maxDoseValue{};

	SizeInt3D dimension;
	SizeFloat3D pixelSpacing;
	xt::xarray<float> doseVoxel;
	float doseScalingFactor;

#ifndef USE_MATLAB_ENGINE
	std::vector<double> dwellPositions; // Changed from xt::xarray to std::vector
#endif
};

struct CT
{
	int viewIndex{0};
	bool refreshCTImageTexture{false};
	SizeInt3D dimension;
	SizeFloat3D pixelSpacing;
	std::vector<PointFloat3D> imagePositionPatients;
	PointFloat3D imagePositionPatient;
	PointInt3D imageOrientationPatientHor;
	PointInt3D imageOrientationPatientVer;
	xt::xarray<int> originalCTVoxel;
	xt::xarray<uint8_t> textureVoxel;
	float m_huCenter, m_huWidth;
};

struct RTDVH
{
	bool DVHCalculated{};
	bool calculatingDVH{};
	double maxcGyValue{};
	std::vector<Color> calculatedDVHROIColor;
	std::vector<std::string> calculatedDVHROIName;
	std::vector<double> calculatedVolumeSize;
	std::vector<std::string> calculatedVolumeSizeString;
	std::vector<std::vector<double>> calculatedDVHcGyData;
	std::vector<std::vector<double>> calculatedDVHPercentData;
	std::deque<bool> DVHCalculationSelection;
};

class CDialogIMBTOptimizer_ImGUI;
class __declspec(dllexport) IMBTOptimizerDialog_ImGui
{
public:
	IMBTOptimizerDialog_ImGui(CPlan *pPlan, 
		CDialogIMBTOptimizer_ImGUI* _pDialog, 
		bool bPlanModified,
		bool* _pCalculatedDoseConfirm
	);
	~IMBTOptimizerDialog_ImGui();

	//bool showDialog();
	bool initGUI(HWND parentWindow);
	bool closeGUI();

private:
	CDialogIMBTOptimizer_ImGUI *m_pDialog;
	// Optimization
	arma::Cube<float> m_pCtMatrix;
	std::vector<std::string> m_sOptimizationLog;

	// GUI
	GLFWwindow *m_pGUIWindow;
	SizeInt2D windowSize{1920, 1080};
	CPlan *m_pCPlan;

	static RTStruct RTStructInfoContainer;
	static CT CTInfoContainer;
	static CalculatedDose calculatedDoseContainer;
	static RTDVH DVHInfoContainer;
	static RTPlan RTPlanContainer;
	//static std::vector<Constraints> constraintsContainer;
	std::vector<Constraints> constraintsContainer;
	bool originalDataConverted{};

	int dvhCalculationIndex{};

#ifndef USE_MATLAB_ENGINE
	// Add member variables for dose matrix result
	mat m_PTV;
	mat m_bladder;
	mat m_rectum;
	mat m_bowel;
	mat m_sigmoid;
	mat m_body;
	uvec m_ind_bodyd;
#endif

	//---------------------------------------------------
	// Data handling function
	//---------------------------------------------------
	void convertDataFromTPS(CPlan *planData);
	void calculateDVH(OpenCLHandler* openCLHandler);

	// Patient information window
	void showPatientInfo(float positionX, float positionY, float wWidth, float wHeight);

	// CT voxel and dose information window
	void showCTVoxelAndDose(GLuint *CTVoxelTexture, GLuint *doseVoxelTexture, CPlan *planData, float positionX, float positionY, float wWidth, float wHeight);

	// ROI information window
	void showROIandDVHInfo(float positionX, float positionY, float wWidth, float wHeight, OpenCLHandler* openCLHandler);

	// Constraint information window
	void showConstraintInfo(float positionX, float positionY, float wWidth, float wHeight);
	void showDoseCalculationControlPanel(CPlan *planData, float positionX, float positionY, float wWidth, float wHeight);
	void drawGUIWindow(GLuint* CTVoxelTexture, GLuint* doseVoxelTexture, CPlan* planData, OpenCLHandler* openCLHandler);
	void showOptimizationOptions(float positionX, float positionY, float wWidth, float wHeight);

	static uint8_t HUtoTextureMapping(int HU, CT &ctInfoContainer);
	static Color getJetColorMap(double v, double vmin, double vmax);
	static ImVec2 WorldToScreen(const ImVec2& worldPos, const ImVec2& canvasP0, const ImVec2& imageDimension, const ImVec2& imageSize, float zoom, const ImVec2& pan);
	static void showMessageBox(std::string sTitle, std::string sText);

	// Optimization
	bool initializeDoseKernel(CPlan *_plan);
	bool optimizeIMBT_Matlab(CPlan *_plan);
	void ConfirmOptimization(CPlan *_plan);
	void initialize();
protected:
	bool m_bPlanModified = true;  
	bool* m_CalculatedDoseConfirm;
#ifndef USE_MATLAB_ENGINE
	// DoseMatrixResult struct for non-MATLAB engine version
	struct DoseMatrixResult
	{
		arma::mat PTV;
		arma::mat bladder;
		arma::mat rectum;
		arma::mat bowel;
		arma::mat sigmoid;
		arma::mat body;
		arma::uvec ind_bodyd;

		// Default constructor
		DoseMatrixResult() = default;

		// Copy constructor
		DoseMatrixResult(const DoseMatrixResult &other)
			: PTV(other.PTV), bladder(other.bladder), rectum(other.rectum), bowel(other.bowel), sigmoid(other.sigmoid), body(other.body), ind_bodyd(other.ind_bodyd)
		{
		}

		// Move constructor
		DoseMatrixResult(DoseMatrixResult &&other) noexcept
			: PTV(std::move(other.PTV)), bladder(std::move(other.bladder)), rectum(std::move(other.rectum)), bowel(std::move(other.bowel)), sigmoid(std::move(other.sigmoid)), body(std::move(other.body)), ind_bodyd(std::move(other.ind_bodyd))
		{
		}

		// Copy assignment operator
		DoseMatrixResult &operator=(const DoseMatrixResult &other)
		{
			if (this != &other)
			{
				PTV = other.PTV;
				bladder = other.bladder;
				rectum = other.rectum;
				bowel = other.bowel;
				sigmoid = other.sigmoid;
				body = other.body;
				ind_bodyd = other.ind_bodyd;
			}
			return *this;
		}

		// Move assignment operator
		DoseMatrixResult &operator=(DoseMatrixResult &&other) noexcept
		{
			if (this != &other)
			{
				PTV = std::move(other.PTV);
				bladder = std::move(other.bladder);
				rectum = std::move(other.rectum);
				bowel = std::move(other.bowel);
				sigmoid = std::move(other.sigmoid);
				body = std::move(other.body);
				ind_bodyd = std::move(other.ind_bodyd);
			}
			return *this;
		}
	};

	// Store DoseMatrixResult as member variable
	DoseMatrixResult m_doseMatrixResult;
#endif
};
