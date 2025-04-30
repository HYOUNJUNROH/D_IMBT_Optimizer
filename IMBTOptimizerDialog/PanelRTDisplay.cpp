// PanelRTDisplay.cpp: 구현 파일
//

#include "PanelRTDisplay.h"
#include "resource.h"
#include <atlimage.h>



// PanelRTDisplay

IMPLEMENT_DYNAMIC(PanelRTDisplay, CWnd)

PanelRTDisplay::PanelRTDisplay()
{

}

PanelRTDisplay::~PanelRTDisplay()
{

	if (m_pBitmapInfo != 0)
	{
		delete m_pBitmapInfo;
	}
}

BOOL PanelRTDisplay::RegisterWindowClass()
{
	WNDCLASS    sttClass;
	HINSTANCE   hInstance = AfxGetInstanceHandle();

	if (GetClassInfo(hInstance, CUSTOM_CONTROL_CLASSNAME, &sttClass) == FALSE)
	{
		sttClass.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		sttClass.lpfnWndProc = ::DefWindowProc;
		sttClass.cbClsExtra = 0;
		sttClass.cbWndExtra = 0;
		sttClass.hInstance = hInstance;
		sttClass.hIcon = NULL;
		sttClass.hCursor = AfxGetApp()->LoadStandardCursor(IDC_ARROW);
		sttClass.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
		sttClass.lpszMenuName = NULL;
		sttClass.lpszClassName = CUSTOM_CONTROL_CLASSNAME;

		if (!AfxRegisterClass(&sttClass))
		{
			AfxThrowResourceException();
			return FALSE;
		}
	}

	return TRUE;
}


BEGIN_MESSAGE_MAP(PanelRTDisplay, CWnd)
	ON_WM_PAINT()
	//	ON_COMMAND(ID_READ_FILE, &Panel01::OnReadFile)
	//	ON_COMMAND(ID_READ_FOLDER, &Panel01::OnReadFolder)
	ON_WM_MOUSEWHEEL()
	//ON_WM_SIZING()
	//ON_WM_ERASEBKGND()
	ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()



// Panel01 메시지 처리기




void PanelRTDisplay::OnPaint()
{
	CPaintDC dc(this);

	CRect r;
	GetClientRect(r);
	dc.FillSolidRect(r, RGB(0, 0, 0));


	if (CTs.m_Slices.size() > 0)
	{
		//DrawBitmap(GetImageSlice(CTs.page));
		DrawBitmap(GetImageSlice_arma(CTs.page));
	}


	//CPaintDC dc(this);

	//CRect rect;
	//GetClientRect(&rect);

	//dc.SetTextColor(RGB(255, 255, 255));
	//dc.SetBkColor(RGB(0, 0, 0));
	//dc.DrawText(CString("test"), &rect, 0);
}




void PanelRTDisplay::CreateBitmapInfo(int w, int h, int bpp)
{
	// TODO: 여기에 구현 코드 추가.
	if (m_pBitmapInfo != 0 )
	{
		delete m_pBitmapInfo;
		m_pBitmapInfo = 0;
	}

	if (bpp == 8)
		m_pBitmapInfo = (BITMAPINFO*) new BYTE[sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD)];
	else // 24 or 32bit
		m_pBitmapInfo = (BITMAPINFO*) new BYTE[sizeof(BITMAPINFO)];

	m_pBitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	m_pBitmapInfo->bmiHeader.biPlanes = 1;
	m_pBitmapInfo->bmiHeader.biBitCount = bpp;
	m_pBitmapInfo->bmiHeader.biCompression = BI_RGB;
	m_pBitmapInfo->bmiHeader.biSizeImage = 0;
	m_pBitmapInfo->bmiHeader.biXPelsPerMeter = 0;
	m_pBitmapInfo->bmiHeader.biYPelsPerMeter = 0;
	m_pBitmapInfo->bmiHeader.biClrUsed = 0;
	m_pBitmapInfo->bmiHeader.biClrImportant = 0;

	if (bpp == 8)
	{
		for (int i = 0; i < 256; i++)
		{
			m_pBitmapInfo->bmiColors[i].rgbBlue = (BYTE)i;
			m_pBitmapInfo->bmiColors[i].rgbGreen = (BYTE)i;
			m_pBitmapInfo->bmiColors[i].rgbRed = (BYTE)i;
			m_pBitmapInfo->bmiColors[i].rgbReserved = 0;
		}
	}

	m_pBitmapInfo->bmiHeader.biWidth = w;
	m_pBitmapInfo->bmiHeader.biHeight = -h;
}


void PanelRTDisplay::DrawBitmap(cv::Mat mat)
{
	float min, max;

	max = CTs.center + (CTs.width / 2);
	min = CTs.center - (CTs.width / 2);

	double min_v, max_v;
	cv::Point min_loc, max_loc;
	cv::minMaxLoc(mat, &min_v, &max_v, &min_loc, &max_loc);

	cvtColor(mat, mat, COLOR_GRAY2BGRA);

	mat.convertTo(mat, CV_8U, 255.0 / CTs.width);

	CreateBitmapInfo(mat.cols, mat.rows, mat.channels() * 8);

	CClientDC dc(this);

	// Picture Control 크기를 얻는다.
	GetClientRect(&SliceRect);
	CDC memDC;
	CBitmap* pOldBitmap, bitmap;

	// Picture Control DC에 호환되는 새로운 CDC를 생성. (임시 버퍼)
	memDC.CreateCompatibleDC(&dc);


	// Picture Control의 크기와 동일한 비트맵을 생성.
	bitmap.CreateCompatibleBitmap(&dc, SliceRect.Width(), SliceRect.Height());

	// 임시 버퍼에서 방금 생성한 비트맵을 선택하면서, 이전 비트맵을 보존.
	pOldBitmap = memDC.SelectObject(&bitmap);

	SetStretchBltMode(memDC.GetSafeHdc(), COLORONCOLOR);

	DrawContoursOnSlice(mat, CTs.page);

	StretchDIBits(memDC.GetSafeHdc(), 0, 0, SliceRect.Width(), SliceRect.Height(), 0, 0, mat.cols, mat.rows, mat.data, m_pBitmapInfo, DIB_RGB_COLORS, SRCCOPY);

	// 임시 버퍼를 Picture Control에 그린다.
	dc.BitBlt(0, 0, SliceRect.Width(), SliceRect.Height(), &memDC, 0, 0, SRCCOPY);

	// 이전 비트맵으로 재설정.
	memDC.SelectObject(pOldBitmap);

	// 생성한 리소스 해제.
	memDC.DeleteDC();
	bitmap.DeleteObject();


	SetText();
}


BOOL PanelRTDisplay::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if (CTs.m_Slices.size() > 0)
	{
		if (zDelta > 0)
		{
			if (CTs.page > 0)
			{
				CTs.page--;
			}
		}
		else if (zDelta < 0)
		{
			if (CTs.page < CTs.m_Slices.size() - 1)
			{
				CTs.page++;
			}
		}

		DrawBitmap(GetImageSlice_arma(CTs.page));
	}

	CPaintDC dc(this);
	CRect rect;
	GetClientRect(&rect);

	dc.SetTextColor(RGB(255, 255, 255));
	dc.SetBkColor(RGB(0, 0, 0));
	dc.DrawText(CString("Slice " + CTs.page), &rect, 0);


	return CWnd::OnMouseWheel(nFlags, zDelta, pt);
}

/*
void PanelRTDisplay::GetRTDose(const char* path)
{
	// TODO: 여기에 구현 코드 추가.

	DRTDose rtdose;
	OFCondition status = rtdose.loadFile(path);
	if (status.good())
	{

		OFVector<Float64> ImagePosition, OffsetVector;
		OFVector<OFVector<Float64>> Images;
		rtdose.getImagePositionPatient(ImagePosition);
		rtdose.getDoseImages(Images);
		rtdose.getGridFrameOffsetVector(OffsetVector);

		int rows = rtdose.getDoseImageHeight();
		int cols = rtdose.getDoseImageWidth();

		OFVector<Float64> PS;

		rtdose.getPixelSpacing(PS);
		//dcmrt_dose->getPixelSpacing(PS);

		Float64 scaling;
		rtdose.getDoseGridScaling(scaling);
		scaling = scaling;




		OFVector<Float64> T;
		T.push_back(CTs.PixelSpacing[0] / PS[0]);
		T.push_back(CTs.PixelSpacing[1] / PS[1]);

		Float64**dosef = new Float64 * [189];

		for (int k = 0; k < 189; k++)
		{
			dosef[k] = new Float64[97 * 253];
		}

		for (int f = 0; f < 189; f++)
		{
			for (int h = 0; h < rows; h++)
			{
				for (int w = 0; w < cols; w++)
				{
					dosef[f][h * cols + w] = Images[f][h * cols + w];
				}
			}
		}



		/// <summary>
		///  좌표변환한 후에 // 정확한 위치에서 디스플레이 하기 위해 Interpolation 할 것
		/// </summary>
		/// <param name="path"></param>

		//int x = int(round((abs(ImagePosition[0]) ) / CTs.PixelSpacing[0]));
		//int y = int(round((abs(ImagePosition[1]) ) / CTs.PixelSpacing[1]));

		// Max 값 찾아주기

		Float64 met = 0;

		for (int i = 0; i<Images.size(); i++)
		{
			Mat mat = Mat(rows, cols, CV_64FC1, dosef[i]);
			Float64 m, n;
			minMaxLoc(mat, &n, &m);
			//Mat gray;
			//mat.convertTo(gray, CV_8U, 255.0 / (m - n), -255.0 * n / (m - n));
			//minMaxLoc(gray, &n, &m);

			if (met < m && m != 255)
			{
				met = m;
			}

		}
		met = met;

		Float64 minn = met * 0.2;

		//Uint32 max = *max_element(pixelv.begin(), pixelv.end());

		for (int f = 0; f < Images.size(); f++) //frames
		{
			for (int p = 0; p < CTs.posZ.size(); p++)
			{
				if (OffsetVector[f] + ImagePosition[2] == CTs.posZ[p])
				{
					Mat mat = Mat(rows, cols, CV_64FC1, dosef[f]);

					resize(mat, mat, Size(cols * T[0], rows * T[1]));

					// Pixel Spacing 값 만큼 다시 늘려주었음.
					resize(mat, mat, Size(cols / T[0], rows / T[1]));

					/// <summary>
					///  Color Table
					/// </summary>
					/// <param name="path"></param>
					///
					Mat gray;

					double Min;
					double Max;
					Point minLoc;
					Point maxLoc;

					minMaxLoc(mat, &Min, &Max, &minLoc, &maxLoc);

					mat.convertTo(gray, CV_8U, 255.0 / (Max - Min), -255.0 * Min / (Max - Min));

					minMaxLoc(gray, &Min, &Max, &minLoc, &maxLoc);

					//Max = met;


					//Max = 255;


					double Line1 = Max * 0.98;
					double Line2 = Max * 0.95;
					double Line3 = Max * 0.9;
					double Line4 = Max * 0.8;
					double Line5 = Max * 0.7;
					double Line6 = Max * 0.5;
					double Line7 = Max * 0.3;
					double Line8 = Max * 0.2;


					unsigned char b[256] = { 0 };
					unsigned char g[256] = { 0 };
					unsigned char r[256] = { 0 };
					unsigned char A[256] = { 0 };

					for (int i = 0; i < 256; i++)
					{
						//if (i > 70)
						//{
						//	b[i] = 55;
						//	g[i] = 55;
						//	r[i] = 55;
						//}
						if (Max < i)
						{
							b[i] = 0;
							g[i] = 0;
							r[i] = 0;
							A[i] = 0;
						}
						if (Max > i && i >= Line1)
						{
							b[i] = 0;
							g[i] = 0;
							r[i] = 150;
							A[i] = 127;
						}
						if (Line1 > i && i >= Line2)
						{
							b[i] = 0;
							g[i] = 0;
							r[i] = 120;
							A[i] = 127;
						}
						if (Line2 > i && i >= Line3)
						{
							b[i] = 0;
							g[i] = 30;
							r[i] = 60;
							A[i] = 127;
						}
						if (Line3 > i && i >= Line4)
						{
							b[i] = 0;
							g[i] = 180;
							r[i] = 0;
							A[i] = 127;
						}
						if (Line4 > i && i >= Line5)
						{
							b[i] = 60;
							g[i] = 120;
							r[i] = 0;
							A[i] = 127;
						}
						if (Line5 > i && i >= Line6)
						{
							b[i] = 60;
							g[i] = 0;
							r[i] = 0;
							A[i] = 127;
						}
						if (Line6 > i && i >= Line7)
						{
							b[i] = 30;
							g[i] = 0;
							r[i] = 0;
							A[i] = 127;
						}
						if (Line7 > i && i >=Line8)
						{
							b[i] = 0;
							g[i] = 0;
							r[i] = 200;
							A[i] = 127;
						}
						if (Line8 > i && i >= 0)
						{
							b[i] = i;
							g[i] = i;
							r[i] = i;
							A[i] = 0;
						}
					}

					/// 10 14

					Mat AGray = Mat(gray.rows, gray.cols, CV_8UC4);
					uchar* ppt = AGray.data;


					cvtColor(gray.clone(), gray, COLOR_GRAY2BGRA);
					uchar* pt = gray.data;

					uchar* mpt = mat.data;

					for (int row = 0; row < gray.rows; row++)
					{
						for (int col = 0; col < gray.cols; col++)
						{
							ppt[row * gray.cols * 4 + col * 4] = pt[row * gray.cols * 4 + col * 4];
							ppt[row * gray.cols * 4 + col * 4 + 1] = pt[row * gray.cols * 4 + col * 4 + 1];
							ppt[row * gray.cols * 4 + col * 4 + 2] = pt[row * gray.cols * 4 + col * 4 + 2];
							ppt[row * gray.cols * 4 + col * 4 + 3] = pt[row * gray.cols * 4 + col * 4 + 3];

							uchar b = pt[row * gray.cols * 4 + col * 4];

							if (Line8 > b && b >= 0)
							{
								ppt[row * gray.cols * 4 + col * 4] = 255;
								ppt[row * gray.cols * 4 + col * 4 + 3] = 0;
							}
							else
							{
								ppt[row * gray.cols * 4 + col * 4 + 3] = 250;
							}
						}
					}

					cv::Mat channels[] = { cv::Mat(256, 1, CV_8U, b), cv::Mat(256, 1, CV_8U, g), cv::Mat(256, 1, CV_8U, r), cv::Mat(256, 1, CV_8U, A)};
					cv::Mat lut; // Create a lookup table
					cv::merge(channels, 4, lut);

					cv::Mat im_color;
					cv::LUT(AGray, lut, im_color);


					// color table end

					//if (f == 101)
					//{
					//	imshow("", im_color);
					//	waitKey(0);
					//}
					// Image Position 다시 잡아주자. CT Image Position Patient를 기반으로


					minMaxLoc(mat, &Min, &Max, &minLoc, &maxLoc);

					int Px, Py;

					Px = int( abs((CTs.ImagePosition[0] - ImagePosition[0]) / CTs.PixelSpacing[0]) );
					Py = int( abs((CTs.ImagePosition[1] - ImagePosition[1]) / CTs.PixelSpacing[1]) );

					if (Max > minn)
					{
						OverlayImage(&CTs.Slices[p], &im_color, Point(Px, Py));
					}
					//OverlayImage(&CTs.Slices[p], &im_color, Point(1, 175));
				}
			}
		}
		MessageBox(_T("Ok"));
	}
}
*/


void PanelRTDisplay::ImportCPlan(CPlan* _pPlan, ImportType _type, bool isCTDrawNow)
{
	m_pCplan = _pPlan;

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// CT Data Processing
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if ((_type == ImportType::CT || _type == ImportType::ALL) &&  m_pCplan != 0 && m_pCplan->m_pPlannedCTData != 0)
	{
		float** ppf = nullptr;
		ppf = m_pCplan->m_pPlannedCTData->m_ppfHU;


		unsigned int rows = m_pCplan->m_pPlannedCTData->m_anDim[0];
		unsigned int cols = m_pCplan->m_pPlannedCTData->m_anDim[1];
		unsigned int SliceSize = m_pCplan->m_pPlannedCTData->m_anDim[2];

		CTs.ImagePosition[0] = m_pCplan->m_pPlannedCTData->m_ptsPatOrg[0].fx; // image position
		CTs.ImagePosition[1] = m_pCplan->m_pPlannedCTData->m_ptsPatOrg[0].fy; // image position
		CTs.ImagePosition[2] = m_pCplan->m_pPlannedCTData->m_ptsPatOrg[0].fz; // image position

		/////////////////////////////////////////////////////////////////////////
		// Reverse in Z Direction
		/////////////////////////////////////////////////////////////////////////
		for (unsigned int i = 0; i < unsigned int(SliceSize / 2 + 0.5); i++)
		{
			for (unsigned int ny = 0; ny < rows; ny++)
			{
				for (unsigned int nx = 0; nx < cols; nx++)
				{
					// Reversed Image in Z direction because of wrong z direction in current CPlan vaersion: 2024-09-12
					float hu_a = ppf[i][ny * cols + nx];
					float hu_b = ppf[SliceSize - i - 1][ny * cols + nx];

					ppf[i][ny * cols + nx] = hu_b;
					ppf[SliceSize - i - 1][ny * cols + nx] = hu_a;

				}
			}

			_FPOINT3 po_a = m_pCplan->m_pPlannedCTData->m_ptsPatOrg[i];
			_FPOINT3 po_b = m_pCplan->m_pPlannedCTData->m_ptsPatOrg[SliceSize - i - 1];

			m_pCplan->m_pPlannedCTData->m_ptsPatOrg[i] = po_b;
			m_pCplan->m_pPlannedCTData->m_ptsPatOrg[SliceSize - i - 1] = po_a;

			CString sop_a = m_pCplan->m_pPlannedCTData->m_arrSOPInstanceUID[i];
			CString sop_b = m_pCplan->m_pPlannedCTData->m_arrSOPInstanceUID[SliceSize - i - 1];

			m_pCplan->m_pPlannedCTData->m_arrSOPInstanceUID[i] = sop_b;
			m_pCplan->m_pPlannedCTData->m_arrSOPInstanceUID[SliceSize - i - 1] = sop_a;
		}

		for (unsigned int i = 0; i < SliceSize; i++)
		{
			CTs.m_ImagePositions.push_back(m_pCplan->m_pPlannedCTData->m_ptsPatOrg[i]);
		}

		m_pCplan->m_pPlannedCTData->m_afPatOrg[0] = m_pCplan->m_pPlannedCTData->m_ptsPatOrg[0].fx; // image position
		m_pCplan->m_pPlannedCTData->m_afPatOrg[1] = m_pCplan->m_pPlannedCTData->m_ptsPatOrg[0].fy; // image position
		m_pCplan->m_pPlannedCTData->m_afPatOrg[2] = m_pCplan->m_pPlannedCTData->m_ptsPatOrg[0].fz; // image position
		/////////////////////////////////////////////////////////////////////////
		// End
		/////////////////////////////////////////////////////////////////////////


		for (int i = 0; i < SliceSize; i++)
		{
			CTs.SOPInstanceUID.push_back(m_pCplan->m_pPlannedCTData->m_arrSOPInstanceUID[i]);
			CTs.m_posZ.push_back(m_pCplan->m_pPlannedCTData->m_ptsPatOrg[i].fz);
		}


		for (int i = 0; i < 3; i++)
		{
			CTs.horiz[i] = m_pCplan->m_pPlannedCTData->m_afPatOrientHor[i];
			CTs.vertical[i] = m_pCplan->m_pPlannedCTData->m_afPatOrientVer[i];
			//CTs.ImagePosition[i] = m_pCplan->m_pPlannedCTData->m_afPatOrg[i]; // image position
			CTs.PixelSpacing[i] = m_pCplan->m_pPlannedCTData->m_afRes[i]; // pixel spacing
		}

		CTs.center = m_pCplan->m_pPlannedCTData->m_nWndCenter;
		CTs.width = m_pCplan->m_pPlannedCTData->m_nWndWidth;

		CTs.OriginPixel = new float* [SliceSize];
		CTs.OriginPixel_Column_Major = new float* [SliceSize];

		// new pixeldata
		CString str;

		unsigned int dataSize = rows * cols;

		for (int i = 0; i < SliceSize; i++)
		{
			CTs.OriginPixel[i] = new float[dataSize];
		}

		for (int i = 0; i < SliceSize; i++)
		{
			for (int ny = 0; ny < rows; ny++)
			{
				for (int nx = 0; nx < cols; nx++)
				{
					CTs.OriginPixel[i][ny * cols + nx] = ppf[i][ny * cols + nx];
				}
			}
		}

		// Arma Cube based CT data structures
		CTs.m_pCtMatrix = new Cube<float>(rows, cols, SliceSize);

		for (size_t z = 0; z < SliceSize; z++) // Slices
		{
			arma::Mat<float> tmpSlice(CTs.OriginPixel[z], rows, cols, false, true);
			CTs.m_pCtMatrix->slice(z) = tmpSlice; // for 3d matrix processing

			cv::Mat slice(2, new int[] {(int)rows, (int)cols}, CV_32F, tmpSlice.memptr());
			CTs.m_Slices.push_back(slice); // for display
		}

		// Coordinate of X axis
		for (int i = 0; i < cols; i++)
		{
			CTs.m_posX.push_back(CTs.ImagePosition[0] + CTs.PixelSpacing[0] * i);
		}

		// Coordinate of Y axis
		for (int i = 0; i < cols; i++)
		{
			CTs.m_posY.push_back(CTs.ImagePosition[1] + CTs.PixelSpacing[1] * i);
		}

		CTs.m_vCatheter = m_pCplan->m_vecCatheters;

		CTs.page = (int)(SliceSize / 2.0);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Dose Data Processing
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if ((_type == ImportType::DOSE) && m_pCplan != 0 && m_pCplan->m_pCalculatedDose != 0)
	{
		float** ppw = nullptr;
		ppw = m_pCplan->m_pCalculatedDose->m_ppwDen;

		unsigned int rows = m_pCplan->m_pCalculatedDose->m_anDim[0];
		unsigned int cols = m_pCplan->m_pCalculatedDose->m_anDim[1];
		unsigned int SliceSize = m_pCplan->m_pCalculatedDose->m_anDim[2];


		// Arma Cube based CT data structures
		CTs.m_pDoseMatrix = new Cube<float>(rows, cols, SliceSize);

		for (size_t z = 0; z < SliceSize; z++) // Slices
		{
			arma::Mat<float> tmpSlice(ppw[z], rows, cols, false, true);
			CTs.m_pCtMatrix->slice(z) = tmpSlice; // for 3d matrix processing
		}

		cv::Mat slice = GetDoseSlice_arma(20);
		//cvtColor(slice.clone(), slice, COLOR_GRAY2BGRA);

		cv::Mat slice2;
		slice.convertTo(slice2, CV_8U);

		cv::Mat slice_color;
		applyColorMap(slice2, slice_color, COLORMAP_JET);

		//cv::imshow("colorMap", slice_color);
		cv::imshow("colorMap", slice_color);

		waitKey(0);

	}


	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// RTStruct Data Processing
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if ((_type == ImportType::RTSTRUCT || _type == ImportType::ALL) && m_pCplan != 0 && m_pCplan->m_pPlannedStructData != 0)
	{
		CTs.m_vecContours.clear();
		CTs.m_vecContoursOnSlice.clear();

		std::vector<CROIITEM*> items = m_pCplan->m_pPlannedStructData->m_vecROIItems;
		RTs.Data = m_pCplan->m_pPlannedStructData;

		int ItemSize = m_pCplan->m_pPlannedStructData->m_vecROIItems.size();
		for (int i = 0; i < ItemSize; i++)
		{
			CROIITEM* roi = items[i];
			//int ContSize =  roi->m_vecContours.size();
			//int SliceSize = CTs.Slices.size();

			vector<CString> UID;

			vector<CONTOURDATA*> ContData;//= new CONTOURDATA[ContSize];
			ContData = roi->m_vecContours;
			CTs.m_vecContours.push_back(ContData);
		}


		// Build Points of contours per ct slice
		for (int p = 0; p < CTs.m_Slices.size(); p++)
		//for (int p = CTs.m_Slices.size() - 1; p >= 0 ; p--)
		{
			vector<vector<vector<Point>>> slice;
			for (int item = 0; item < CTs.m_vecContours.size(); item++)
			{
				vector<vector<Point>> contour;
				for (int s = 0; s < CTs.m_vecContours[item].size(); s++)
				{

					if (CTs.SOPInstanceUID[p] == CTs.m_vecContours[item][s]->strReferencedSOPInstanceUID)
					{
						int ptsSize = CTs.m_vecContours[item][s]->m_arrayPts.size();
						vector<Point> Pts;

						for (int i = 0; i < ptsSize; i++) // items contours
						{
							Point P;

							//  CT의 Image Position Patient를 기반으로 Structure set의 상대 위치를 지정한다.

							//int c = int(round((abs(CTs.ImagePosition[0]) + CTs.m_vecContours[item][s]->m_arrayPts[i].fx) / CTs.PixelSpacing[0]));
							//int r = int(round((abs(CTs.ImagePosition[1]) + CTs.m_vecContours[item][s]->m_arrayPts[i].fy) / CTs.PixelSpacing[1]));
							int c = int(round((abs(CTs.m_ImagePositions[p].fx) + CTs.m_vecContours[item][s]->m_arrayPts[i].fx) / CTs.PixelSpacing[0]));
							int r = int(round((abs(CTs.m_ImagePositions[p].fy) + CTs.m_vecContours[item][s]->m_arrayPts[i].fy) / CTs.PixelSpacing[1]));

							P.x = c;
							P.y = r;

							//Pts[i] = P;
							Pts.push_back(P);
						}

						//polylines(CTs.Slices[p], Pts, TRUE, ScalarN, 1);
						contour.push_back(Pts);
					}

				}

				slice.push_back(contour);
			}

			CTs.m_vecContoursOnSlice.push_back(slice);
		}
	}

	if (isCTDrawNow)
		//DrawBitmap(GetImageSlice(CTs.page));
		DrawBitmap(GetImageSlice_arma(CTs.page));
}


cv::Mat PanelRTDisplay::GetImageSlice(int _sliceNum)
{
	int size_row = CTs.m_ctMat->size[0];
	int size_col = CTs.m_ctMat->size[1];

	float* ind = (float*)CTs.m_ctMat->data + _sliceNum * size_row * size_col;
	cv::Mat slice(2, new int[] {(int)size_row, (int)size_col}, CTs.m_ctMat->type(), ind);

	return slice;
}

cv::Mat PanelRTDisplay::GetImageSlice_arma(int _sliceNum)
{
	cv::Mat slice(2, new int[] {(int)CTs.m_pCtMatrix->n_rows, (int)CTs.m_pCtMatrix->n_cols}, CV_32F, CTs.m_pCtMatrix->slice(_sliceNum).memptr());

	return slice;
}

cv::Mat PanelRTDisplay::GetDoseSlice_arma(int _sliceNum)
{
	cv::Mat slice(2, new int[] {(int)CTs.m_pDoseMatrix->n_rows, (int)CTs.m_pDoseMatrix->n_cols}, CV_32F, CTs.m_pDoseMatrix->slice(_sliceNum).memptr());

	return slice;
}

// 2023-08-21 not used
void PanelRTDisplay::GetSlices(LPCTSTR path)
{
	m_pCplan = new CPlan();

	bool bLoad = m_pCplan->LoadCTData(path);

	if (bLoad)
	{
		float** ppf = nullptr;
		ppf = m_pCplan->m_pPlannedCTData->m_ppfHU;


		unsigned int rows = m_pCplan->m_pPlannedCTData->m_anDim[0];
		unsigned int cols = m_pCplan->m_pPlannedCTData->m_anDim[1];
		unsigned int SliceSize = m_pCplan->m_pPlannedCTData->m_anDim[2];

		for (int i = 0; i < 3; i++)
		{
			CTs.horiz[i] = m_pCplan->m_pPlannedCTData->m_afPatOrientHor[i];
			CTs.vertical[i] = m_pCplan->m_pPlannedCTData->m_afPatOrientVer[i];
			CTs.ImagePosition[i] = m_pCplan->m_pPlannedCTData->m_afPatOrg[i]; // image position
			CTs.PixelSpacing[i] = m_pCplan->m_pPlannedCTData->m_afRes[i]; // pixel spacing
		}

		//SliceRect = CRect(0, 0, cols, rows);

		for (int i = 0; i < SliceSize; i++)
		{
			CTs.SOPInstanceUID.push_back(m_pCplan->m_pPlannedCTData->m_arrSOPInstanceUID[i]);
			CTs.m_posZ.push_back(m_pCplan->m_pPlannedCTData->m_ptsPatOrg[i].fz);
		}

		CTs.center = m_pCplan->m_pPlannedCTData->m_nWndCenter;
		CTs.width = m_pCplan->m_pPlannedCTData->m_nWndWidth;

		float min, max;
		max = CTs.center + (CTs.width / 2);
		min = CTs.center - (CTs.width / 2);

		CTs.OriginPixel = new float* [SliceSize];

		// new pixeldata
		CString str;

		unsigned int dataSize = rows * cols;

		for (int i = 0; i < SliceSize; i++)
		{
			//CTs.Pixel[i] = new uint8_t[dataSize];
			CTs.OriginPixel[i] = new float[dataSize];
		}

		for (int i = 0; i < SliceSize; i++)
		{
			for (int ny = 0; ny < rows; ny++)
			{
				for (int nx = 0; nx < cols; nx++)
				{
					float hu = ppf[i][ny * cols + nx];
					CTs.OriginPixel[i][ny * cols + nx] = hu;

					/*
					if (hu < min)
						CTs.Pixel[i][ny * cols + nx] = 0;
					else if (hu > max)
						CTs.Pixel[i][ny * cols + nx] = 255;
					else
					{
						CTs.Pixel[i][ny * cols + nx] = ((hu - (CTs.center - 0.5)) / (CTs.width - 1) + 0.5) * 255;
					}
					*/
				}
			}
		}


		CTs.m_ctMat = new cv::Mat(3, new int[] {(int)rows, (int)cols, (int)SliceSize}, CV_32F, cv::Scalar(0));

		int size_row = CTs.m_ctMat->size[0];
		int size_cols = CTs.m_ctMat->size[1];
		int size_slices = CTs.m_ctMat->size[2];

		for (size_t z = 0; z < size_slices; z++) // Slices
		{
			cv::Mat mat = GetImageSlice(z);

			memcpy(mat.data, (void*)CTs.OriginPixel[z], sizeof(float) * size_row * size_cols);

		}

		for (size_t z = 0; z < size_slices; z++) // Slices
		{
			cv::Mat slice = GetImageSlice(z);


			//cvtColor(slice, slice, COLOR_GRAY2BGRA);
			//slice.convertTo(slice, CV_8U, 255.0 / (max - min), -min * 255.0 / (max - min));

			CTs.m_Slices.push_back(slice);
		}

		//DrawBitmap(GetImageSlice(CTs.page));
		DrawBitmap(GetImageSlice_arma(CTs.page));
	}
}




void PanelRTDisplay::GetStruct(LPCTSTR path)
{
	if (m_pCplan == 0)
		return;
	//CPlan* pCplan = new CPlan();

	if (m_pCplan->LoadStructData(path) == TRUE)
	{
		std::vector<CROIITEM*> items = m_pCplan->m_pPlannedStructData->m_vecROIItems;
		RTs.Data = m_pCplan->m_pPlannedStructData;


		int ItemSize = m_pCplan->m_pPlannedStructData->m_vecROIItems.size();
		for (int i = 0; i < ItemSize; i++)
		{
			CROIITEM* roi = items[i];
			//int ContSize =  roi->m_vecContours.size();
			//int SliceSize = CTs.Slices.size();

			vector<CString> UID;

			vector<CONTOURDATA*> ContData;//= new CONTOURDATA[ContSize];
			ContData = roi->m_vecContours;
			CTs.m_vecContours.push_back(ContData);
		}


		// Build Points of contours per ct slice
		for (int p = 0; p < CTs.m_Slices.size(); p++)
		{
			vector<vector<vector<Point>>> slice;
			for (int item = 0; item < CTs.m_vecContours.size(); item++)
			{
				vector<vector<Point>> contour;
				for (int s = 0; s < CTs.m_vecContours[item].size(); s++)
				{

					if (CTs.SOPInstanceUID[p] == CTs.m_vecContours[item][s]->strReferencedSOPInstanceUID)
					{
						int ptsSize = CTs.m_vecContours[item][s]->m_arrayPts.size();
						vector<Point> Pts;

						for (int i = 0; i < ptsSize; i++) // items contours
						{
							Point P;

							//  CT의 Image Position Patient를 기반으로 Structure set의 상대 위치를 지정한다.

							int c = int(round((abs(CTs.ImagePosition[0]) + CTs.m_vecContours[item][s]->m_arrayPts[i].fx) / CTs.PixelSpacing[0]));
							int r = int(round((abs(CTs.ImagePosition[1]) + CTs.m_vecContours[item][s]->m_arrayPts[i].fy) / CTs.PixelSpacing[1]));

							P.x = c;
							P.y = r;

							//Pts[i] = P;
							Pts.push_back(P);
						}

						//polylines(CTs.Slices[p], Pts, TRUE, ScalarN, 1);
						contour.push_back(Pts);
					}

				}

				slice.push_back(contour);
			}

			CTs.m_vecContoursOnSlice.push_back(slice);
		}
	}

	//DrawBitmap(GetImageSlice(CTs.page));
	DrawBitmap(GetImageSlice_arma(CTs.page));
}


void PanelRTDisplay::DIsplay_Str(int i, int n)
{
	// TODO: 여기에 구현 코드 추가.
	CDC* pDC = GetDC();

	//CPen penblue(PS_SOLID,)
	CPen penBlue(PS_SOLID, 1, Scalar_Struct[n]);
	CPen* pOldPen = pDC->SelectObject(&penBlue);

	// and a solid red brush
	CBrush brushRed(Scalar_Struct[n]);
	brushRed.CreateStockObject(NULL_BRUSH);

	CBrush* pOldBrush = pDC->SelectObject(&brushRed);

	for (int j = 0; j < CTs.m_vecContours[i].size(); j++) // contours
	{
		if (CTs.SOPInstanceUID[CTs.page] == CTs.m_vecContours[i][j]->strReferencedSOPInstanceUID)
		{
			if (i == 3)
			{
				int test = 1;
			}
			int ptsSize = CTs.m_vecContours[i][j]->m_arrayPts.size();
			POINT* Pts = new POINT[ptsSize];
			for (int p = 0; p < ptsSize; p++)
			{
				POINT J;

				J.x = int(round((abs(CTs.ImagePosition[0]) + CTs.m_vecContours[i][j]->m_arrayPts[p].fx) / CTs.PixelSpacing[0]));
				J.y = int(round((abs(CTs.ImagePosition[1]) + CTs.m_vecContours[i][j]->m_arrayPts[p].fy) / CTs.PixelSpacing[1]));

				Pts[p] = J;
			}

			pDC->Polygon(Pts, ptsSize);

			pDC->SelectObject(pOldPen);
			pDC->SelectObject(pOldBrush);

			ReleaseDC(pDC);
			delete Pts;
			//break;
		}
	}
}

void PanelRTDisplay::OverlayImage(cv::Mat* src, cv::Mat* overlay, const Point& location)
{
	for (int y = max(location.y, 0); y < src->rows; ++y)
	{
		int fY = y - location.y;

		if (fY >= overlay->rows)
			break;

		for (int x = max(location.x, 0); x < src->cols; ++x)
		{
			int fX = x - location.x;

			if (fX >= overlay->cols)
				break;

			double opacity = ((double)overlay->data[fY * overlay->step + fX * overlay->channels() + 3]) / 255;

			for (int c = 0; opacity > 0 && c < src->channels(); ++c)
			{
				unsigned char overlayPx = overlay->data[fY * overlay->step + fX * overlay->channels() + c];
				unsigned char srcPx = src->data[y * src->step + x * src->channels() + c];
				src->data[y * src->step + src->channels() * x + c] = srcPx * (1. - opacity) + overlayPx * opacity;
			}
		}
	}
}

void PanelRTDisplay::blendImageToMiddle(cv::Mat background, cv::Mat image) {
	// blend image into background (image over background)
	// background is fully opaque (don't have alpha channel, so must have 3 channels)

	int bgStartX = 0;
	int bgStartY = 0;

	// get the sub-area for blending in background
	cv::Mat blendMat = background(cv::Range(bgStartY, bgStartY + image.rows),
		cv::Range(bgStartX, bgStartX + image.cols));

	for (int y = 0; y < image.rows; ++y)
	{
		for (int x = 0; x < image.cols; ++x)
		{
			cv::Vec3b& backgroundPixel = blendMat.at<cv::Vec3b>(y, x);
			cv::Vec4b& imagePixel = image.at<cv::Vec4b>(y, x);

			// get alpha value (divide 255 since the image of alpha value is 0 to 255. Change it to 0.0-1.0)
			float alpha = imagePixel[3] / 255;

			backgroundPixel[0] = cv::saturate_cast<uchar>(alpha * imagePixel[0] + (1.0 - alpha) * backgroundPixel[0]);
			backgroundPixel[1] = cv::saturate_cast<uchar>(alpha * imagePixel[1] + (1.0 - alpha) * backgroundPixel[1]);
			backgroundPixel[2] = cv::saturate_cast<uchar>(alpha * imagePixel[2] + (1.0 - alpha) * backgroundPixel[2]);


			//imagePixel[3] = 10;
		}
	}
}


void PanelRTDisplay::test()
{
	// TODO: 여기에 구현 코드 추가.
	//CRect rect;
	//GetClientRect(rect);

	//Float32* img = new Float32[rect.Height() * rect.Width()];
	//
	//for (int y = 0; y < rect.Height(); y++)
	//{
	//	for (int x = 0; x < rect.Width(); x++)
	//	{
	//		
	//	}
	//}
	///////////////////////////////// rect dc

	CWnd* pWndDesktop = this;
	// 바탕 화면 윈도우 DC
	CWindowDC ScrDC(pWndDesktop);
	// 뷰 윈도우 DC
	CClientDC dc(this);

	// Rect를 사용해서 작업 영역에 대한 좌표를 얻고,
	CRect Rect;
	GetClientRect(&Rect);

	// 그 위치를 현재 윈도우의 절대 좌표로 변경해 주자.
	//CWnd::GetWindowRect(&Rect);

	// CImage를 하나 만들고
	CImage Image;
	// 스캔을 시작할 x, y 위치와
	int sx = Rect.left;
	int sy = Rect.top;

	// 작업 영역에 대한 크기를 각각 임시로 저장해 두자.
	int cx = Rect.Width();
	int cy = Rect.Height();

	// 작업 영역의 크기만큼, 현재 바탕화면의 속성과 동일한 image를 생성한다.
	Image.Create(cx, cy, ScrDC.GetDeviceCaps(BITSPIXEL));

	// 이미지 DC에 현재 작업 영역의 절대 좌표를 사용해 그 크기만큼 저장하게 한다.
	CDC* pDC = CDC::FromHandle(Image.GetDC());
	pDC->BitBlt(0, 0, cx, cy, &ScrDC, sx + 1, sy + 1, SRCCOPY);
	Image.ReleaseDC();

	// 저장된 이미지를 원하는 파일명, 포멧타입을 지정해서 저장한다.
	Image.Save(TEXT("C:\\test\\test.jpg"), Gdiplus::ImageFormatJPEG);


	//CWnd* pWnd = this;

	//WindowCapture(pWnd->GetSafeHwnd());


	//CMemDCDoc* pDoc = GetDocument();
	//ASSERT_VALID(pDoc);


	////보통은 pDC 에 그림을 그리는데 이번엔 가상화면에 선을 가로로 100개 세로로 100개 그려서
	////가상화면을 pDC 에 전체 복사하는 방식으로 해보겠습니다.

	//////////////////// 가상화면 생성 하는 부분입니다. ////////////////////
	//CDC memDC;
	//CBitmap bit;
	//memDC.CreateCompatibleDC(pDC);     //화면과 호환이 되는 메모리 DC 생성
	//bit.CreateCompatibleBitmap(pDC, 640, 480);  //640x480의 가상화면을 생성합니다.
	//memDC.SelectObject(&bit);      //메모리 DC 가 비트맵의 공간을 갖게됨

	//// 처음에 가상화면에 브러쉬및 펜이 설정되어있지않으므로 설정해줍니다.

	//// ( 비트맵 작업할경우 불필요 )
	//CPen pen(PS_SOLID, 1, RGB(255, 0, 0));
	//CBrush brush;
	//brush.CreateSolidBrush(RGB(0, 0, 255));
	//memDC.SelectObject(&pen);
	//memDC.SelectObject(&brush);

	////// 여기서 부터는 평소에 pDC에 그리던 작업을 memDC에 해보겠습니다. ///////
 //  ////  모든 행동은 pDC 에 하던것과 똑같습니다.  //////////////////////////////


	//int x = 0, y = 0;
	//for (x = 0; x < 640; x += 5) {  //x 0 부터 640 까지 5픽셀씩 증가하면서 세로줄을 그립니다.
	//	memDC.MoveTo(x, 0);
	//	memDC.LineTo(x, 480);
	//}
	//for (y = 0; y < 480; y += 5) {  //y 0 부터 480 까지 5픽셀씩 증가하면서 가로줄을 그립니다.
	//	memDC.MoveTo(0, y);
	//	memDC.LineTo(640, y);
	//}

	/////////////////  정말중요한 작업부분입니다. 그린것을 한번에 화면에 복사 //////////////////
	//pDC->BitBlt(0, 0, 640, 480, &memDC, 0, 0, SRCCOPY);



	/////////////// 썼으니까 지워주는 부분 ////////////////////////////////////////////////////
	//memDC.DeleteDC();
	//bit.DeleteObject();
	//pen.DeleteObject();
	//brush.DeleteObject();

/*	Mat mat;

	imshow("", mat);
	waitKey(0)*/;
}


bool PanelRTDisplay::WindowCapture(HWND hTargetWnd)
{
	// TODO: 여기에 구현 코드 추가.
	CRect rct;
	if (hTargetWnd)
		::GetClientRect(hTargetWnd, &rct);
	else
		return FALSE;

	CRect rcClient;
	GetClientRect(&rcClient);

	HBITMAP hBitmap = NULL;
	HBITMAP hOldBitmap = NULL;
	BOOL bSuccess = FALSE;

	HDC hDC = ::GetDC(hTargetWnd);
	HDC hMemDC = ::CreateCompatibleDC(hDC);
	hBitmap = ::CreateCompatibleBitmap(hDC, rct.Width(), rct.Height());

	if (!hBitmap)
		return FALSE;

	hOldBitmap = (HBITMAP)::SelectObject(hMemDC, hBitmap);

	// 캡쳐한 윈도우 현재 클라이언트 화면 크기에 맞춰 그리기
	CClientDC dc(this);

	dc.SetStretchBltMode(COLORONCOLOR);
	dc.StretchBlt(0, 0, rcClient.Width(), rcClient.Height(), CDC::FromHandle(hDC),
		0, 0, rct.Width(), rct.Height(), SRCCOPY);

	::SelectObject(hMemDC, hOldBitmap);
	DeleteObject(hBitmap);
	::DeleteDC(hMemDC);
	::ReleaseDC(hTargetWnd, hDC);

	return bSuccess;
}


void PanelRTDisplay::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	//CPaintDC dc(this);

	//CRect rect;
	//GetClientRect(&rect);

	//dc.SetTextColor(RGB(255, 255, 255));
	//dc.SetBkColor(RGB(0, 0, 0));
	//dc.DrawText(CString("test"), &rect, 0);


	CWnd::OnMouseMove(nFlags, point);
}


void PanelRTDisplay::SetText()
{
	CPaintDC dc(this);

	CRect rect;
	GetClientRect(&rect);

	dc.SetTextColor(RGB(255, 255, 255));
	dc.SetBkColor(RGB(0, 0, 0));
	dc.DrawText(CString("test"), &rect, 0);
}


/*
void PanelRTDisplay::GetRTDose2(const char* path)
{
	DRTDose rtdose;
	OFCondition status = rtdose.loadFile(path);
	if (status.good())
	{

		OFVector<Float64> ImagePosition, OffsetVector;
		OFVector<OFVector<Float64>> Images;
		rtdose.getImagePositionPatient(ImagePosition);
		rtdose.getDoseImages(Images);
		rtdose.getGridFrameOffsetVector(OffsetVector);

		int rows = rtdose.getDoseImageHeight();
		int cols = rtdose.getDoseImageWidth();

		OFVector<Float64> PS;

		rtdose.getPixelSpacing(PS);
		//dcmrt_dose->getPixelSpacing(PS);

		OFVector<Float64> T;
		T.push_back(CTs.PixelSpacing[0] / PS[0]);
		T.push_back(CTs.PixelSpacing[1] / PS[1]);

		Float64** dosef = new Float64 * [189];

		for (int k = 0; k < 189; k++)
		{
			dosef[k] = new Float64[97 * 253];
		}

		for (int f = 0; f < 189; f++)
		{
			for (int h = 0; h < rows; h++)
			{
				for (int w = 0; w < cols; w++)
				{
					dosef[f][h * cols + w] = Images[f][h * cols + w];
				}
			}
		}



		/// <summary>
		///  좌표변환한 후에 // 정확한 위치에서 디스플레이 하기 위해 Interpolation 할 것
		/// </summary>
		/// <param name="path"></param>

		//int x = int(round((abs(ImagePosition[0]) ) / CTs.PixelSpacing[0]));
		//int y = int(round((abs(ImagePosition[1]) ) / CTs.PixelSpacing[1]));

		// Max 값 찾아주기

		Float64 Max = 0;

		for (int i = 0; i < Images.size(); i++)
		{
			Mat mat = Mat(rows, cols, CV_64FC1, dosef[i]);
			Float64 m, n;
			minMaxLoc(mat, &n, &m);
			//Mat gray;
			//mat.convertTo(gray, CV_8U, 255.0 / (m - n), -255.0 * n / (m - n));
			//minMaxLoc(gray, &n, &m);

			Max = m;

		}

		double Line1 = Max * 0.98;
		double Line2 = Max * 0.95;
		double Line3 = Max * 0.9;
		double Line4 = Max * 0.8;
		double Line5 = Max * 0.7;
		double Line6 = Max * 0.5;
		double Line7 = Max * 0.3;

		//Uint32 max = *max_element(pixelv.begin(), pixelv.end());

		for (int f = 0; f < Images.size(); f++) //frames
		{
			for (int p = 0; p < CTs.posZ.size(); p++)
			{
				if (OffsetVector[f] + ImagePosition[2] == CTs.posZ[p])
				{
					Mat mat = Mat(rows, cols, CV_64FC1, dosef[f]);

					resize(mat, mat, Size(cols * T[0], rows * T[1]));
					resize(mat, mat, Size(cols * 2, rows * 2));


					Mat gray;

					double Min;
					double Max;
					Point minLoc;
					Point maxLoc;

					minMaxLoc(mat, &Min, &Max, &minLoc, &maxLoc);

					mat.convertTo(gray, CV_8U, 255.0 / (Max - Min), -255.0 * Min / (Max - Min));
					OverlayImage(&CTs.Slices[p], &gray, Point(1, 175));

					/// <summary>
					///  Color Table
					/// </summary>
					/// <param name="path"></param>
					///
					///

					//double Max = met;

					//Mat Color;
					//cvtColor(Color.clone(), Color, COLOR_GRAY2BGRA);
					//uchar* pt = Color.data;

					//for (int r = 0; r < mat.rows; r++)
					//{
					//	for (int c = 0; c < mat.cols; c++)
					//	{

					//	}
					//}
				}
			}
		}
	}
}
*/


/// <summary>
/// Draw lines on CT images
/// </summary>
/// <param name="item">Contour Number</param>
/// <param name="ScalarN">Contour Color</param>
void  PanelRTDisplay::LineDisplay(int item, Scalar ScalarN)
{
	for (int p = 0; p < CTs.m_Slices.size(); p++)
	{
		for (int s = 0; s < CTs.m_vecContours[item].size(); s++)
		{
			if (CTs.SOPInstanceUID[p] == CTs.m_vecContours[item][s]->strReferencedSOPInstanceUID)
			{
				int ptsSize = CTs.m_vecContours[item][s]->m_arrayPts.size();
				vector<Point> Pts;
				for (int i = 0; i < ptsSize; i++) // items contours
				{
					Point P;

					//  CT의 Image Position Patient를 기반으로 Structure set의 상대 위치를 지정한다.

					int c = int(round((abs(CTs.ImagePosition[0]) + CTs.m_vecContours[item][s]->m_arrayPts[i].fx) / CTs.PixelSpacing[0]));
					int r = int(round((abs(CTs.ImagePosition[1]) + CTs.m_vecContours[item][s]->m_arrayPts[i].fy) / CTs.PixelSpacing[1]));

					P.x = c;
					P.y = r;
					//Pts[i] = P;
					Pts.push_back(P);
				}
				polylines(CTs.m_Slices[p], Pts, TRUE, ScalarN, 1);
			}
		}
	}
}


/// <summary>
/// Draw contours on Slice Image
/// </summary>
/// <param name="image"></param>
/// <param name="_sliceNumber"></param>
void PanelRTDisplay::DrawContoursOnSlice(cv::Mat image, unsigned int _sliceNumber)
{
	if (CTs.m_vecContoursOnSlice.size() <= 0)
		return;

	vector<vector<vector<Point>>> contours = CTs.m_vecContoursOnSlice[_sliceNumber];

	for (int s = 0; s < contours.size(); s++)
	{
		if (contours[s].size() == 0)
			continue;

		for( int segment = 0; segment < contours[s].size(); segment++ )
			polylines(image, contours[s][segment], TRUE, CTs.m_vecContoursColors[s], 1);
	}
}

/// <summary>
/// Draw Dose distribution on Slice Image
/// </summary>
/// <param name="image"></param>
/// <param name="_sliceNumber"></param>
void PanelRTDisplay::DrawDoseOnSlice(cv::Mat image, unsigned int _sliceNumber)
{
	if (CTs.m_pDoseMatrix == 0)
		return;

	cv::Mat doseMat = GetDoseSlice_arma(_sliceNumber);
	cv::Mat doseMat_color;

	applyColorMap(doseMat, doseMat_color, COLORMAP_JET);



	/*
	if (Max > minn)
	{
		OverlayImage(&CTs.Slices[p], &im_color, Point(Px, Py));
	}
	*/
}
