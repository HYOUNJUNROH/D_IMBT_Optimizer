#pragma once


// Panel01
#include "pch.h"
#include "includes/Plan.h"


// For OpenCV LIBS
#include <Windows.h> // gdi plus requires Windows.h
#include <wingdi.h>

#include "includes/opencv2/imgproc.hpp"
#include "includes/opencv2/highgui.hpp"


// For Dicom Image

#include <wingdi.h>
#include "armadillo"
#include "NumCpp.hpp"

using namespace std;
using namespace cv; 
using namespace arma;

struct RTSStruct
{
	StructData* Data;
};

struct DOSEStruct
{

};
 
struct DICOMContourProperty
{
	Scalar m_contourColor;
	bool m_isVisible = true;
};

struct CTStruct
{
public:
	cv::Mat* m_ctMat = 0;
	std::vector<cv::Mat> m_Slices;
	std::vector<float> m_posZ;
	std::vector<float> m_posX;
	std::vector<float> m_posY;
	arma::Cube<float>* m_pCtMatrix;
	arma::Cube<float>* m_pDoseMatrix;
	std::vector<_FPOINT3> m_ImagePositions;

	std::vector<Catheter*> m_vCatheter;
	std::vector<CString> SOPInstanceUID;


	float ImagePosition[3], horiz[3], vertical[3], PixelSpacing[3];


	//vector<vector<CString>> SOPInstanceUID_Struct; // ITEMS - ContSops
	//std::vector<std::vector<std::vector<Point>>> SeqContours;

	vector<vector<CONTOURDATA*>> m_vecContours;
	vector<vector<vector<vector<Point>>>> m_vecContoursOnSlice; // Slice --> Contours --> Segments
	std::vector<Scalar> m_vecContoursColors;

	float width = 40, center = 400;
	float** OriginPixel = nullptr;
	float** OriginPixel_Column_Major = nullptr;
	
	uint8_t** Pixel = nullptr;
	unsigned int page = 0;

public:
	void intialize()
	{
		m_Slices.clear();
		m_posZ.clear();
		m_posX.clear();
		m_posY.clear();
		SOPInstanceUID.clear();
		m_vecContours.clear();
		m_vecContoursOnSlice.clear();
		m_vecContoursColors.clear();
		page = 0;

		if (m_ctMat != 0)
			delete m_ctMat;

		if( OriginPixel != 0 )
			delete[] OriginPixel;

		if (OriginPixel_Column_Major != 0)
			delete[] OriginPixel_Column_Major;

		if( Pixel != 0)
			delete[] Pixel;

		if (m_pCtMatrix != 0)
			delete m_pCtMatrix;

		if(m_pDoseMatrix != 0)
			delete m_pDoseMatrix;

		m_pCtMatrix = 0;
		m_pDoseMatrix = 0;
	}

public:
	CTStruct()
	{
		m_vecContoursColors.push_back(Scalar(0, 0, 255));
		m_vecContoursColors.push_back(Scalar(0, 120, 120));
		m_vecContoursColors.push_back(Scalar(120, 120, 10));
		m_vecContoursColors.push_back(Scalar(60, 60, 120));
		m_vecContoursColors.push_back(Scalar(255, 0, 0));
		m_vecContoursColors.push_back(Scalar(80, 220, 120));
		m_vecContoursColors.push_back(Scalar(120, 0, 120));
		m_vecContoursColors.push_back(Scalar(0, 255, 0));

		m_vecContoursColors.push_back(Scalar(0, 60, 60));
		m_vecContoursColors.push_back(Scalar(60, 60, 10));
		m_vecContoursColors.push_back(Scalar(30, 30, 60));
		m_vecContoursColors.push_back(Scalar(125, 0, 0));
		m_vecContoursColors.push_back(Scalar(40, 110, 60));
		m_vecContoursColors.push_back(Scalar(60, 0, 60));
		m_vecContoursColors.push_back(Scalar(0, 125, 0));
		m_vecContoursColors.push_back(Scalar(0, 125, 0));
		m_vecContoursColors.push_back(Scalar(0, 125, 0));
		m_vecContoursColors.push_back(Scalar(0, 125, 0));
		m_vecContoursColors.push_back(Scalar(0, 125, 0));
		m_vecContoursColors.push_back(Scalar(0, 125, 0));

		m_pCtMatrix = 0;
		m_pDoseMatrix = 0;
	}

	~CTStruct()
	{
		if (m_ctMat != 0)
		{
			m_ctMat->release();
			delete m_ctMat;
		}

		if (OriginPixel != 0)
			delete[] OriginPixel;

		if (Pixel != 0)
			delete[] Pixel;
	}

};


#define CUSTOM_CONTROL_CLASSNAME _T("PanelRTDisplay")

class __declspec(dllexport)  PanelRTDisplay : public CWnd
{
	DECLARE_DYNAMIC(PanelRTDisplay)

public:
	PanelRTDisplay();
	virtual ~PanelRTDisplay();
	static BOOL RegisterWindowClass();

	enum class ImportType {ALL, CT, RTSTRUCT, DOSE};

protected:
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnPaint();
//	afx_msg void OnReadFile();
//	afx_msg void OnReadFolder();
	CTStruct CTs;
	RTSStruct RTs;
	CPlan* m_pCplan;

	CRect SliceRect;

	void ImportCPlan(CPlan* _pPlan, ImportType _type, bool isDrawNow);

	void GetSlices(LPCTSTR path);
	void GetStruct(LPCTSTR path);

	void CreateBitmapInfo(int w, int h, int bpp);
	BITMAPINFO* m_pBitmapInfo;

	void DrawBitmap(cv::Mat mat);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	void GetRTDose(const char* path);

	void DIsplay_Str(int i, int n);
	

	void OverlayImage(cv::Mat* src, cv::Mat* overlay, const Point& location);
	void blendImageToMiddle(cv::Mat background, cv::Mat image);
	COLORREF Scalar_Struct[16] = { RGB(255, 0, 0), RGB(0, 255, 0), RGB(0, 0, 255), RGB(105, 105, 0),
												RGB(0,105, 105), RGB(0,45, 105) , RGB(105,0, 105) , RGB(44,255, 105),
												RGB(88,255, 105), RGB(50,0, 60), RGB(60,70, 105), RGB(90,60, 105),
												RGB(88,55, 105), RGB(50,50, 60), RGB(60,130, 105), RGB(10,60, 105) };
	void test();
	bool WindowCapture(HWND hTargetWnd);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	void SetText();
	void GetRTDose2(const char* path);
	void  LineDisplay(int item, Scalar ScalarN);
	void DrawContoursOnSlice(cv::Mat image, unsigned int _sliceNumber);
	void DrawDoseOnSlice(cv::Mat image, unsigned int _sliceNumber);


private:
	cv::Mat GetImageSlice(int _sliceNum);
	cv::Mat GetImageSlice_arma(int _sliceNum);
	cv::Mat GetDoseSlice_arma(int _sliceNum);
};

