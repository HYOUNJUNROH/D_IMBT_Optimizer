#ifdef USE_MATLAB_ENGINE
// CDialogIMBTOptimizer.cpp : implementation file
//
#include "pch.h"
#include "afxdialogex.h"
#include "CDialogIMBTOptimizer.h"
#include "resource.h"
#include "StyleSheet.h"
#include "UniButton.h"
#include "includes/Plan.h"

#include "../IMBT_Optimizer/IMBT_Optimizer.h"

#include "CRunningDlg.h"

// CDialogIMBTOptimizer dialog

IMPLEMENT_DYNAMIC(CDialogIMBTOptimizer, CDialog)

BEGIN_MESSAGE_MAP(CDialogIMBTOptimizer, CDialog)
ON_WM_ERASEBKGND()
ON_WM_GETMINMAXINFO()
ON_WM_SIZE()
ON_BN_CLICKED(IDOK, &CDialogIMBTOptimizer::OnBnClickedOk)
ON_BN_CLICKED(IDC_BUTTON_INITIALIZE, &CDialogIMBTOptimizer::OnBnClickedButtonInitialize)
ON_BN_CLICKED(IDC_BUTTON_START_OPTIMIZER, &CDialogIMBTOptimizer::OnBnClickedButtonStartOptimizer)
END_MESSAGE_MAP()

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
};

/// <summary>
///
/// </summary>
/// <param name="pParent"></param>
CDialogIMBTOptimizer::CDialogIMBTOptimizer(CWnd *pParent = nullptr, CPlan *_plan = nullptr)
	: CDialog(IDD_DIALOG_Optimizer, pParent)
{
	m_pPlan = 0;

	if (_plan != 0)
	{
		m_pPlan = _plan;
	}

	m_ButtonOK = new CUniButton();
	m_ButtonCancel = new CUniButton();
	m_ButtonInitialize = new CUniButton();
	m_ButtonStartOptimizer = new CUniButton();

	m_PanelCT_top = new PanelRTDisplay();
	m_PanelCT_bottom = new PanelRTDisplay();

	m_fDoseResolution = 2.5;  // Default 2.5 mm Grid Size;
	float m_fAngleResolution; // Default 10 deg Angle Step
	m_vAnglePositions.clear();

	m_bInitializeDone = false;
	m_bOptimizedDone = false;
}

CDialogIMBTOptimizer::~CDialogIMBTOptimizer()
{
	delete m_ButtonOK;
	delete m_ButtonCancel;
	delete m_ButtonInitialize;
	delete m_ButtonStartOptimizer;

	delete m_PanelCT_top;
	delete m_PanelCT_bottom;
}

BOOL CDialogIMBTOptimizer::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_ButtonOK->SubclassDlgItem(IDOK, this, 0, CStyleSheet::m_colorMainViewButton.normalColor,
								CStyleSheet::m_colorMainViewButton.selectedColor,
								CStyleSheet::m_colorMainViewButton.hoverColor,
								GetSysColor(COLOR_BTNFACE));

	m_ButtonCancel->SubclassDlgItem(IDCANCEL, this, 0, CStyleSheet::m_colorMainViewButton.normalColor,
									CStyleSheet::m_colorMainViewButton.selectedColor,
									CStyleSheet::m_colorMainViewButton.hoverColor,
									GetSysColor(COLOR_BTNFACE));

	m_ButtonInitialize->SubclassDlgItem(IDC_BUTTON_INITIALIZE, this, 0, CStyleSheet::m_colorMainViewButton.normalColor,
										CStyleSheet::m_colorMainViewButton.selectedColor,
										CStyleSheet::m_colorMainViewButton.hoverColor,
										GetSysColor(COLOR_BTNFACE));

	m_ButtonStartOptimizer->SubclassDlgItem(IDC_BUTTON_START_OPTIMIZER, this, 0, CStyleSheet::m_colorMainViewButton.normalColor,
											CStyleSheet::m_colorMainViewButton.selectedColor,
											CStyleSheet::m_colorMainViewButton.hoverColor,
											GetSysColor(COLOR_BTNFACE));

	ShowWindow(SW_MAXIMIZE);

	if (!initializeOptimizer())
		return FALSE;

	return TRUE;
}

bool CDialogIMBTOptimizer::initializeOptimizer()
{
	LoadCTandStruct();

	m_fDoseResolution = 2.5; // mm
	m_fAngleResolution = 10; // deg

	{
		float startAngle = 180.0;
		for (float angle = -180.0; angle <= 170; angle += m_fAngleResolution)
		{
			m_vAnglePositions.push_back(angle);
		}
	}

	// if (LoadMatFiles() == false) // for test
	//     return false;

	// m_maskBody_dose.set_size(m_nDims[1], m_nDims[0], m_nDims[2]);
	// m_maskPTV_dose.set_size(m_nDims[1], m_nDims[0], m_nDims[2]);
	// m_maskBladder_dose.set_size(m_nDims[1], m_nDims[0], m_nDims[2]);;
	// m_maskRectum_dose.set_size(m_nDims[1], m_nDims[0], m_nDims[2]);;
	// m_maskBowel_dose.set_size(m_nDims[1], m_nDims[0], m_nDims[2]);;
	// m_maskSigmoid_dose.set_size(m_nDims[1], m_nDims[0], m_nDims[2]);;

	return true;
}

bool CDialogIMBTOptimizer::LoadMatFiles()
{
	/*
	if( m_cubeSystemA_2nd.load(arma::hdf5_name("systemA_2nd.h5", "systemA_2nd"), arma::hdf5_binary) == false )
		return false;

	if( m_cubeSystemA_ovoid1_left.load(arma::hdf5_name("systemA_ovoid1_left.h5", "systemA_ovoid1_left"), arma::hdf5_binary) == false )
		return false;

	if( m_cubeSystemA_ovoid1_right.load(arma::hdf5_name("systemA_ovoid1_right.h5", "systemA_ovoid1_right"), arma::hdf5_binary) == false)
		return false;
		*/

	CString patient1_UID = L"1.3.6.1.4.1.2452.6.1602736692.1156785426.2741994661.4002625057";
	CString patient2_UID = L"2.16.840.1.114362.1.11956109.24114994307.653420281.405.22";

	if (m_pPlan->m_pPlannedCTData->m_arrSOPInstanceUID[0].Compare(patient1_UID) == 0)
	{
		if (m_testDose_512.load(arma::hdf5_name("body_dose_512_mat_patient1.h5", "body_dose_512"), arma::hdf5_binary) == false)
			return false;
	}
	else if (m_pPlan->m_pPlannedCTData->m_arrSOPInstanceUID[0].Compare(patient2_UID) == 0)
	{
		if (m_testDose_512.load(arma::hdf5_name("body_dose_512_mat_patient2.h5", "body_dose_512"), arma::hdf5_binary) == false)
			return false;
	}
	else
		return false;

	// if(m_testDose_512_patient2.load(arma::hdf5_name("body_dose_512_mat_patient2.h5", "body_dose_512"), arma::hdf5_binary) == false)
	//     return false;

	// if(m_testDose_512_patient2.load(arma::hdf5_name("body_dose_512_mat_patient2.h5", "body_dose_512"), arma::hdf5_binary) == false)
	//     return false;

	return true;
}

bool CDialogIMBTOptimizer::LoadCTandStruct()
{
	if (m_pPlan != 0)
	{
		m_nDims[0] = m_pPlan->m_pPlannedCTData->m_anDim[0];
		m_nDims[1] = m_pPlan->m_pPlannedCTData->m_anDim[1];
		m_nDims[2] = m_pPlan->m_pPlannedCTData->m_anDim[2];

		m_PanelCT_top->ImportCPlan(m_pPlan, PanelRTDisplay::ImportType::ALL, false);
		m_PanelCT_bottom->ImportCPlan(m_pPlan, PanelRTDisplay::ImportType::ALL, false);

		CRect listRect;

		m_listOarTarget.GetWindowRect(&listRect);

		m_listOarTarget.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES);
		m_listOarTarget.InsertColumn(1, TEXT("Struct Name"), LVCFMT_CENTER, listRect.Width() * 0.5);
		m_listOarTarget.InsertColumn(2, TEXT("Volume (cc)"), LVCFMT_CENTER, listRect.Width() * 0.3);

		for (int i = 0; i < m_pPlan->m_pPlannedStructData->m_vecROIItems.size(); i++)
		{
			CROIITEM *item = m_pPlan->m_pPlannedStructData->m_vecROIItems[i];

			int lastRow = m_listOarTarget.GetItemCount();

			m_listOarTarget.InsertItem(lastRow, TEXT(""));
			m_listOarTarget.SetItem(lastRow, 0, LVIF_TEXT, item->m_strName, NULL, NULL, NULL, NULL);
		}

		// m_infoOptimizer.sett
		InfoTextOut(L"[Start Optimizer]", RGB(0, 0, 0));

		CString str = L"";
		CString sPrescription;

		str += "[Start Optimizer]";
		str += "\r\n";

		// sPrescription.Format(L"%f", m_pPlan->m_fPrescriptionDosePerFraction);
		// str += str.Format(L"%f", m_pPlan->m_fPrescriptionDosePerFraction);

		// Check Info
		str.AppendFormat(L"Planned Dose = %f cGy x %f", m_pPlan->m_fPrescriptionDosePerFraction, m_pPlan->m_fPlannedFraction);
		str += "\r\n";

		for (int i = 0; i < m_pPlan->m_vecCatheters.size(); i++)
		{
			// m_pPlan->m_vecCatheters[i]->
			for (int j = 0; j < m_pPlan->m_vecCatheters[i]->m_vecDwellPositions.size(); j++)
			{
				str.AppendFormat(L"%d, %s = %f, %f, %f, isActivated = %d", m_pPlan->m_vecCatheters[i]->m_nIndex,
								 m_pPlan->m_vecCatheters[i]->m_sName,
								 m_pPlan->m_vecCatheters[i]->m_vecDwellPositions[j].m_fptPosition.fx,
								 m_pPlan->m_vecCatheters[i]->m_vecDwellPositions[j].m_fptPosition.fy,
								 m_pPlan->m_vecCatheters[i]->m_vecDwellPositions[j].m_fptPosition.fz,
								 (int)m_pPlan->m_vecCatheters[i]->m_vecDwellPositions[j].m_bIsActive);
				str += "\r\n";
			}
		}
		// str.AppendFormat(L"", m_pPlan->m_)

		m_infoOptimizer.SetWindowTextW(str);

		return true;
	}

	return false;
}

/// <summary>
/// Setting of the minimum dialog window size
/// </summary>
/// <param name="lpMMI"></param>
void CDialogIMBTOptimizer::OnGetMinMaxInfo(MINMAXINFO FAR *lpMMI)
{
	lpMMI->ptMinTrackSize.x = 1280;
	lpMMI->ptMinTrackSize.y = 1024;

	CRect rect;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);

	lpMMI->ptMaxTrackSize.x = rect.Width();
	lpMMI->ptMaxTrackSize.y = rect.Height();

	CDialog::OnGetMinMaxInfo(lpMMI);
}

/// <summary>
/// Dynamic layout
/// </summary>
/// <param name="nType"></param>
/// <param name="cx"></param>
/// <param name="cy"></param>
void CDialogIMBTOptimizer::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	if (m_ButtonOK->GetSafeHwnd() != NULL)
	{
		CRect okButton_rect;
		m_ButtonOK->GetWindowRect(&okButton_rect);
		ScreenToClient(&okButton_rect);

		int _width = okButton_rect.Width();
		int _height = okButton_rect.Height();

		okButton_rect.left = cx - 180 - okButton_rect.Width();
		okButton_rect.top = cy - 20 - okButton_rect.Height();
		okButton_rect.right = okButton_rect.left + _width;
		okButton_rect.bottom = okButton_rect.top + _height;
		m_ButtonOK->MoveWindow(okButton_rect, TRUE);
	}

	if (m_ButtonCancel->GetSafeHwnd() != NULL)
	{
		CRect cancelButton_rect;
		m_ButtonCancel->GetWindowRect(&cancelButton_rect);
		ScreenToClient(&cancelButton_rect);

		int _width = cancelButton_rect.Width();
		int _height = cancelButton_rect.Height();

		cancelButton_rect.left = cx - 20 - cancelButton_rect.Width();
		cancelButton_rect.top = cy - 20 - cancelButton_rect.Height();
		cancelButton_rect.right = cancelButton_rect.left + _width;
		cancelButton_rect.bottom = cancelButton_rect.top + _height;

		m_ButtonCancel->MoveWindow(cancelButton_rect, TRUE);
	}

	if (m_ButtonInitialize->GetSafeHwnd() != NULL)
	{
		CRect InitializeButton_rect;
		m_ButtonInitialize->GetWindowRect(&InitializeButton_rect);
		ScreenToClient(&InitializeButton_rect);

		int _width = InitializeButton_rect.Width();
		int _height = InitializeButton_rect.Height();

		InitializeButton_rect.left = 20;
		InitializeButton_rect.top = cy - 20 - InitializeButton_rect.Height();
		InitializeButton_rect.right = InitializeButton_rect.left + _width;
		InitializeButton_rect.bottom = InitializeButton_rect.top + _height;

		m_ButtonInitialize->MoveWindow(InitializeButton_rect, TRUE);
	}

	if (m_ButtonStartOptimizer->GetSafeHwnd() != NULL)
	{
		CRect StartOptButton_rect;
		m_ButtonStartOptimizer->GetWindowRect(&StartOptButton_rect);
		ScreenToClient(&StartOptButton_rect);

		int _width = StartOptButton_rect.Width();
		int _height = StartOptButton_rect.Height();

		StartOptButton_rect.left = 300;
		StartOptButton_rect.top = cy - 20 - StartOptButton_rect.Height();
		StartOptButton_rect.right = StartOptButton_rect.left + _width;
		StartOptButton_rect.bottom = StartOptButton_rect.top + _height;

		m_ButtonStartOptimizer->MoveWindow(StartOptButton_rect, TRUE);
	}
}

void CDialogIMBTOptimizer::DoDataExchange(CDataExchange *pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_PANEL_CT_TOP, *m_PanelCT_top);		   // Custom Control PanelRTDisplay
	DDX_Control(pDX, IDC_PANEL_CT_BOTTOM, *m_PanelCT_bottom);  // Custom Control PanelRTDisplay
	DDX_Control(pDX, IDC_LIST_OARTARGET, m_listOarTarget);	   // OAR List
	DDX_Control(pDX, IDC_RICHEDIT_INFO, m_infoOptimizer_rich); // Optimizer Information
	DDX_Control(pDX, IDC_EDIT_INFO, m_infoOptimizer);		   // Optimizer Information
}

// Background Override
BOOL CDialogIMBTOptimizer::OnEraseBkgnd(CDC *pDC)
{
	CRect rect;

	GetClientRect(&rect);

	CBrush myBrush(RGB(50, 61, 72)); // Dialog background color
	CBrush *pOld = pDC->SelectObject(&myBrush);
	BOOL bRes = pDC->PatBlt(0, 0, rect.Width(), rect.Height(), PATCOPY);

	pDC->SelectObject(pOld); // Restore old brush

	return bRes; // CDialog::OnEraseBkgnd(pDC);
}

void CDialogIMBTOptimizer::InfoTextOut(LPCTSTR strText, COLORREF textColor)
{
	// Go to the last line
	int first_pos = m_infoOptimizer_rich.LineIndex(m_infoOptimizer_rich.GetLineCount());
	m_infoOptimizer_rich.SetSel(first_pos, first_pos);
	CPoint point;
	point = m_infoOptimizer_rich.PosFromChar(first_pos);
	m_infoOptimizer_rich.SetCaretPos(point);
	m_infoOptimizer_rich.SetFocus();

	// Print text
	CHARFORMAT cf;
	memset(&cf, 0, sizeof(CHARFORMAT));
	cf.cbSize = sizeof(CHARFORMAT);
	cf.dwMask = CFM_COLOR;
	cf.crTextColor = textColor;

	m_infoOptimizer_rich.SetSelectionCharFormat(cf);
	m_infoOptimizer_rich.ReplaceSel(strText);
}

bool CDialogIMBTOptimizer::CreateDoseMatrix()
{
	return true;
}

static auto _createCellArray(std::vector<std::vector<std::vector<float>>> data_list)
{
	matlab::data::ArrayFactory factory;
	auto cell_array = factory.createCellArray({1, data_list.size()});
	auto list_size = data_list.size();

	for (size_t index_list = 0; index_list < list_size; ++index_list)
	{
		auto item_list_size = data_list[index_list].size();
		auto item_array = factory.createCellArray({1, item_list_size});
		for (size_t index_item = 0; index_item < item_list_size; ++index_item)
		{
			auto list_item = data_list[index_list][index_item];
			auto list_item_size = data_list[index_list][index_item].size();
			// item_array[0][index_item] = factory.createArray({ 1, list_item_size }, list_item.begin(), list_item.end());
			item_array[0][index_item] = factory.createArray({list_item_size, 1}, list_item.begin(), list_item.end());
		}
		cell_array[0][index_list] = item_array;
	}
	return cell_array;
}

void CDialogIMBTOptimizer::OnBnClickedButtonInitialize()
{
	m_bInitializeDone = false;
	m_bOptimizedDone = false;

	CRunningDlg *m_pDialog = new CRunningDlg();
	m_pDialog->Create(IDD_DIALOG_RUNNING, this);
	m_pDialog->ShowWindow(SW_SHOW);

	matlab::data::ArrayFactory factory;

	// Patient ID
	matlab::data::CharArray PatientID = factory.createCharArray(std::string{CT2CA(m_pPlan->m_strID)});

	// CTData
	auto ct_data = m_pPlan->m_pPlannedCTData;
	matlab::data::StructArray CTData = factory.createStructArray({1, 1}, {"C", "ImagePositionPatient", "ImageOrientationPatient", "PixelSpacing", "SliceThickness", "xPosition", "yPosition", "zPosition"});

	auto *pCtMatrix = m_PanelCT_top->CTs.m_pCtMatrix;
	auto C = factory.createArray<double>({pCtMatrix->n_rows, pCtMatrix->n_cols, pCtMatrix->n_slices});
	for (auto slice_index = 0; slice_index < pCtMatrix->n_slices; ++slice_index)
	{
		auto slice = pCtMatrix->slice(slice_index);
		auto row_major_array = factory.createArray({pCtMatrix->n_rows, pCtMatrix->n_cols}, slice.begin(), slice.begin() + slice.n_elem, matlab::data::InputLayout::ROW_MAJOR);
		std::copy_n(row_major_array.begin(), pCtMatrix->slice(slice_index).n_elem, C.begin() + (pCtMatrix->n_rows * pCtMatrix->n_cols * slice_index));
	}
	CTData[0]["C"] = C;

	auto *pImagePositionPatient = (*((CVolumeData *)ct_data)).m_afPatOrg;
	auto ImagePositionPatient = factory.createArray<double>({1, 3});
	std::copy(pImagePositionPatient, pImagePositionPatient + 3, ImagePositionPatient.begin());
	CTData[0]["ImagePositionPatient"] = ImagePositionPatient;

	auto *pImageOrientationPatientHorizontal = (*((CVolumeData *)ct_data)).m_afPatOrientHor;
	auto *pImageOrientationPatientVertical = (*((CVolumeData *)ct_data)).m_afPatOrientVer;
	auto ImageOrientationPatient = factory.createArray<double>({1, 6});
	std::copy(pImageOrientationPatientHorizontal, pImageOrientationPatientHorizontal + 3, ImageOrientationPatient.begin());
	std::copy(pImageOrientationPatientVertical, pImageOrientationPatientVertical + 3, ImageOrientationPatient.begin() + 3);
	CTData[0]["ImageOrientationPatient"] = ImageOrientationPatient;

	auto *pPixelSpacing = (*((CVolumeData *)ct_data)).m_afRes;
	auto PixelSpacing = factory.createArray<double>({1, 2});
	std::copy(pPixelSpacing, pPixelSpacing + 2, PixelSpacing.begin());
	CTData[0]["PixelSpacing"] = PixelSpacing;

	auto *pSliceThickness = (*((CVolumeData *)ct_data)).m_afRes + 2;
	auto SliceThickness = factory.createArray<double>({1, 1});
	std::copy(pSliceThickness, pSliceThickness + 1, SliceThickness.begin());
	CTData[0]["SliceThickness"] = SliceThickness;

	CTData[0]["xPosition"] = factory.createArray({1, m_PanelCT_top->CTs.m_posX.size()}, m_PanelCT_top->CTs.m_posX.begin(), m_PanelCT_top->CTs.m_posX.end());
	CTData[0]["yPosition"] = factory.createArray({1, m_PanelCT_top->CTs.m_posY.size()}, m_PanelCT_top->CTs.m_posY.begin(), m_PanelCT_top->CTs.m_posY.end());
	CTData[0]["zPosition"] = factory.createArray({1, m_PanelCT_top->CTs.m_posZ.size()}, m_PanelCT_top->CTs.m_posZ.begin(), m_PanelCT_top->CTs.m_posZ.end());
	/*
	auto xPosition = factory.createArray<double>({ 1, m_PanelCT_top->CTs.m_posX.size() });
	std::copy(m_PanelCT_top->CTs.m_posX.begin(), m_PanelCT_top->CTs.m_posX.end(), xPosition);
	CTData[0]["xPosition"] = xPosition;

	auto yPosition = factory.createArray<double>({ 1, m_PanelCT_top->CTs.m_posY.size() });
	std::copy(m_PanelCT_top->CTs.m_posY.begin(), m_PanelCT_top->CTs.m_posY.end(), yPosition);
	CTData[0]["yPosition"] = yPosition;

	auto zPosition = factory.createArray<double>({ 1, m_PanelCT_top->CTs.m_posZ.size() });
	std::copy(m_PanelCT_top->CTs.m_posZ.begin(), m_PanelCT_top->CTs.m_posZ.end(), zPosition);
	CTData[0]["zPosition"] = zPosition;
	*/

	// StructData
	auto struct_data = m_pPlan->m_pPlannedStructData;
	matlab::data::StructArray StructData = factory.createStructArray({1, 1}, {"structname", "x", "y", "z"});

	std::vector<std::string> structnames;
	std::vector<std::vector<std::vector<float>>> x_list;
	std::vector<std::vector<std::vector<float>>> y_list;
	std::vector<std::vector<std::vector<float>>> z_list;

	for (auto &item : struct_data->m_vecROIItems)
	{
		structnames.push_back(std::string{CT2CA(item->m_strName)});
		std::vector<std::vector<float>> item_x_list;
		std::vector<std::vector<float>> item_y_list;
		std::vector<std::vector<float>> item_z_list;
		for (auto &contour : item->m_vecContours)
		{
			std::vector<float> contour_x_list;
			std::vector<float> contour_y_list;
			std::vector<float> contour_z_list;
			for (auto arrayPt : contour->m_arrayPts)
			{
				contour_x_list.push_back(arrayPt.fx);
				contour_y_list.push_back(arrayPt.fy);
				contour_z_list.push_back(arrayPt.fz);
			}
			item_x_list.push_back(contour_x_list);
			item_y_list.push_back(contour_y_list);
			item_z_list.push_back(contour_z_list);
		}
		x_list.push_back(item_x_list);
		y_list.push_back(item_y_list);
		z_list.push_back(item_z_list);
	}

	auto structname = factory.createArray({1, structnames.size()}, structnames.begin(), structnames.end());
	StructData[0]["structname"] = structname;
	StructData[0]["x"] = _createCellArray(x_list);
	StructData[0]["y"] = _createCellArray(y_list);
	StructData[0]["z"] = _createCellArray(z_list);

	// Catheter

	auto catheter_data = m_pPlan->m_vecCatheters;
	matlab::data::StructArray Catheter = factory.createStructArray({1, 1}, {"ch1", "ch2", "ch3"});

	std::vector<std::array<float, 3>> ch1;
	for (auto position : catheter_data[0]->m_vecDwellPositions)
	{
		ch1.push_back(std::array<float, 3>{position.m_fptPosition.fx, position.m_fptPosition.fy, position.m_fptPosition.fz});
	}
	std::vector<std::array<float, 3>> ch2;
	for (auto position : catheter_data[1]->m_vecDwellPositions)
	{
		ch2.push_back(std::array<float, 3>{position.m_fptPosition.fx, position.m_fptPosition.fy, position.m_fptPosition.fz});
	}
	std::vector<std::array<float, 3>> ch3;
	for (auto position : catheter_data[2]->m_vecDwellPositions)
	{
		ch3.push_back(std::array<float, 3>{position.m_fptPosition.fx, position.m_fptPosition.fy, position.m_fptPosition.fz});
	}

	auto ch1_array = factory.createArray<double>({ch1.size(), 3});
	for (auto i = 0; i < ch1.size(); ++i)
	{
		ch1_array[i][0] = (double)ch1[i][0];
		ch1_array[i][1] = (double)ch1[i][1];
		ch1_array[i][2] = (double)ch1[i][2];
	}
	Catheter[0]["ch1"] = ch1_array;

	auto ch2_array = factory.createArray<double>({ch2.size(), 3});
	for (auto i = 0; i < ch2.size(); ++i)
	{
		ch2_array[i][0] = (double)ch2[i][0];
		ch2_array[i][1] = (double)ch2[i][1];
		ch2_array[i][2] = (double)ch2[i][2];
	}
	Catheter[0]["ch2"] = ch2_array;

	auto ch3_array = factory.createArray<double>({ch3.size(), 3});
	for (auto i = 0; i < ch3.size(); ++i)
	{
		ch3_array[i][0] = (double)ch3[i][0];
		ch3_array[i][1] = (double)ch3[i][1];
		ch3_array[i][2] = (double)ch3[i][2];
	}
	Catheter[0]["ch3"] = ch3_array;

	std::vector<matlab::data::Array> args({PatientID, CTData, StructData, Catheter});

	// Check output
	ofstream writeOutput("matlab_output_dose_kernel.txt");

	try
	{
		typedef std::basic_stringbuf<char16_t> StringBuf;
		std::shared_ptr<StringBuf> output = std::make_shared<StringBuf>();
		std::shared_ptr<StringBuf> error = std::make_shared<StringBuf>();

		if (writeOutput.is_open())
		{
			writeOutput << "############# Create Dose Kernel" << std::endl;

			MatlabEngine::instance().m_pMatlab->setVariable(u"PatientID", PatientID);
			MatlabEngine::instance().m_pMatlab->setVariable(u"CTData", CTData);
			MatlabEngine::instance().m_pMatlab->setVariable(u"StructData", StructData);
			MatlabEngine::instance().m_pMatlab->setVariable(u"Catheter", Catheter);

			const size_t numReturned = 0;
			auto future = MatlabEngine::instance().m_pMatlab->evalAsync(u"[PTV, bladder, rectum, bowel, sigmoid, body, ind_bodyd] = create_dosematrix_pure_matlab_c__(PatientID, CTData, StructData, Catheter);", output, error);
			future.wait();

			writeOutput << matlab::engine::convertUTF16StringToUTF8String(output.get()->str()).c_str();
			writeOutput << std::endl;

			m_bInitializeDone = true;
		}
	}
	catch (const std::exception e)
	{
		writeOutput << e.what();
	}

	writeOutput.close();

	delete m_pDialog;
}

void CDialogIMBTOptimizer::OnBnClickedButtonStartOptimizer()
{
	if (m_bInitializeDone == false)
	{
		MessageBox(_T("No initialized"), _T("Warning", MB_OK));

		return;
	}

	CRunningDlg *m_pDialog = new CRunningDlg();
	m_pDialog->Create(IDD_DIALOG_RUNNING, this);
	m_pDialog->ShowWindow(SW_SHOW);

	ofstream writeOutput("matlab_output_optimization.txt");

	try
	{
		typedef std::basic_stringbuf<char16_t> StringBuf;
		std::shared_ptr<StringBuf> output = std::make_shared<StringBuf>();
		std::shared_ptr<StringBuf> error = std::make_shared<StringBuf>();
		const size_t numReturned = 0;

		// auto PTV = MatlabEngine::instance().m_pMatlab->getVariable(u"PTV");
		// std::vector<matlab::data::Array> args({ PatientID, CTData, StructData, Catheter });

		auto future = MatlabEngine::instance().m_pMatlab->evalAsync(u"final_dose_512=main_Brachy_GS_L0_patient2_ovoid_2022_matlab_c__(PTV, bladder, rectum, bowel, sigmoid, body, ind_bodyd);", output, error);
		future.wait();
		future = MatlabEngine::instance().m_pMatlab->evalAsync(u"implay(final_dose_512);", output, error);
		future.wait();

		writeOutput << matlab::engine::convertUTF16StringToUTF8String(output.get()->str()).c_str();

		m_bOptimizedDone = true;
	}
	catch (const std::exception e)
	{
	}

	writeOutput.close();

	delete m_pDialog;
}

void CDialogIMBTOptimizer::OnBnClickedOk()
{
	typedef std::basic_stringbuf<char16_t> StringBuf;
	std::shared_ptr<StringBuf> output = std::make_shared<StringBuf>();
	std::shared_ptr<StringBuf> error = std::make_shared<StringBuf>();

	auto future = MatlabEngine::instance().m_pMatlab->evalAsync(u"single(final_dose_512);", output, error);
	future.wait();

	matlab::data::TypedArray<double> dd = MatlabEngine::instance().m_pMatlab->getVariable(u"final_dose_512");

	matlab::data::ArrayDimensions doseDimension = dd.getDimensions();

	m_pPlan->m_pCalculatedDose->m_anDim[0] = doseDimension[0]; // rows
	m_pPlan->m_pCalculatedDose->m_anDim[1] = doseDimension[1]; // cols
	m_pPlan->m_pCalculatedDose->m_anDim[2] = doseDimension[2]; // Slices

	m_pPlan->m_pCalculatedDose->m_afPatOrg[0] = m_pPlan->m_pPlannedCTData->m_afPatOrg[0];
	m_pPlan->m_pCalculatedDose->m_afPatOrg[1] = m_pPlan->m_pPlannedCTData->m_afPatOrg[1];
	m_pPlan->m_pCalculatedDose->m_afPatOrg[2] = m_pPlan->m_pPlannedCTData->m_afPatOrg[2];

	m_pPlan->m_pCalculatedDose->m_afPatOrientHor[0] = m_pPlan->m_pPlannedCTData->m_afPatOrientHor[0];
	m_pPlan->m_pCalculatedDose->m_afPatOrientHor[1] = m_pPlan->m_pPlannedCTData->m_afPatOrientHor[1];
	m_pPlan->m_pCalculatedDose->m_afPatOrientHor[2] = m_pPlan->m_pPlannedCTData->m_afPatOrientHor[2];

	m_pPlan->m_pCalculatedDose->m_afPatOrientVer[0] = m_pPlan->m_pPlannedCTData->m_afPatOrientVer[0];
	m_pPlan->m_pCalculatedDose->m_afPatOrientVer[1] = m_pPlan->m_pPlannedCTData->m_afPatOrientVer[1];
	m_pPlan->m_pCalculatedDose->m_afPatOrientVer[2] = m_pPlan->m_pPlannedCTData->m_afPatOrientVer[2];

	m_pPlan->m_pCalculatedDose->m_afRes[0] = m_pPlan->m_pPlannedCTData->m_afRes[0];
	m_pPlan->m_pCalculatedDose->m_afRes[1] = m_pPlan->m_pPlannedCTData->m_afRes[1];
	m_pPlan->m_pCalculatedDose->m_afRes[2] = m_pPlan->m_pPlannedCTData->m_afRes[2];

	m_pPlan->m_pCalculatedDose->m_fDoseGridScaling = 0.0001;

	m_pPlan->m_pCalculatedDose->m_ppwDen = new float *[doseDimension[2]];
	float **test = new float *[doseDimension[2]];

	unsigned int dataSize = doseDimension[0] * doseDimension[1];

	for (int i = 0; i < doseDimension[2]; i++)
	{
		m_pPlan->m_pCalculatedDose->m_ppwDen[i] = new float[dataSize];
		test[i] = new float[dataSize];
	}

	for (int i = 0; i < doseDimension[2]; i++)
	{
		for (int ny = 0; ny < doseDimension[0]; ny++) // rows
		{
			for (int nx = 0; nx < doseDimension[1]; nx++) // cols
			{
				float fValue = (float)dd[ny][nx][i];

				if (fValue < 0.f)
				{
					fValue = 0.f;
				}

				m_pPlan->m_pCalculatedDose->m_ppwDen[doseDimension[2] - i - 1][ny * doseDimension[1] + nx] = fValue;
			}
		}
	}

	/*
	for (unsigned int i = 0; i < doseDimension[2]; i++)
	{
		memcpy(m_pPlan->m_pCalculatedDose->m_ppwDen[i], &*dd.begin() + dataSize * i, dataSize * sizeof(float));
	}
	*/

	/* Test
	typedef std::basic_stringbuf<char16_t> StringBuf;
	std::shared_ptr<StringBuf> output = std::make_shared<StringBuf>();
	std::shared_ptr<StringBuf> error = std::make_shared<StringBuf>();

	auto future = MatlabEngine::instance().m_pMatlab->evalAsync(u"load test_final_dose_512.mat;", output, error);
	future.wait();

	future = MatlabEngine::instance().m_pMatlab->evalAsync(u"single(final_dose_512);", output, error);
	future.wait();

	matlab::data::TypedArray<double> dd = MatlabEngine::instance().m_pMatlab->getVariable(u"final_dose_512");

	matlab::data::ArrayDimensions doseDimension = dd.getDimensions();

	m_pPlan->m_pCalculatedDose->m_anDim[0] = doseDimension[0]; // rows
	m_pPlan->m_pCalculatedDose->m_anDim[1] = doseDimension[1]; // cols
	m_pPlan->m_pCalculatedDose->m_anDim[2] = doseDimension[2]; // Slices

	m_pPlan->m_pCalculatedDose->m_afPatOrg[0] = m_pPlan->m_pPlannedCTData->m_afPatOrg[0];
	m_pPlan->m_pCalculatedDose->m_afPatOrg[1] = m_pPlan->m_pPlannedCTData->m_afPatOrg[1];
	m_pPlan->m_pCalculatedDose->m_afPatOrg[2] = m_pPlan->m_pPlannedCTData->m_afPatOrg[2];

	m_pPlan->m_pCalculatedDose->m_afPatOrientHor[0] = m_pPlan->m_pPlannedCTData->m_afPatOrientHor[0];
	m_pPlan->m_pCalculatedDose->m_afPatOrientHor[1] = m_pPlan->m_pPlannedCTData->m_afPatOrientHor[1];
	m_pPlan->m_pCalculatedDose->m_afPatOrientHor[2] = m_pPlan->m_pPlannedCTData->m_afPatOrientHor[2];

	m_pPlan->m_pCalculatedDose->m_afPatOrientVer[0] = m_pPlan->m_pPlannedCTData->m_afPatOrientVer[0];
	m_pPlan->m_pCalculatedDose->m_afPatOrientVer[1] = m_pPlan->m_pPlannedCTData->m_afPatOrientVer[1];
	m_pPlan->m_pCalculatedDose->m_afPatOrientVer[2] = m_pPlan->m_pPlannedCTData->m_afPatOrientVer[2];

	m_pPlan->m_pCalculatedDose->m_afRes[0] = m_pPlan->m_pPlannedCTData->m_afRes[0];
	m_pPlan->m_pCalculatedDose->m_afRes[1] = m_pPlan->m_pPlannedCTData->m_afRes[1];
	m_pPlan->m_pCalculatedDose->m_afRes[2] = m_pPlan->m_pPlannedCTData->m_afRes[2];

	m_pPlan->m_pCalculatedDose->m_fDoseGridScaling = 0.01;

	m_pPlan->m_pCalculatedDose->m_ppwDen = new float * [doseDimension[2]];
	double** test = new double* [doseDimension[2]];

	unsigned int dataSize = doseDimension[0] * doseDimension[1];

	for (int i = 0; i < doseDimension[2]; i++)
	{
		m_pPlan->m_pCalculatedDose->m_ppwDen[i] = new float[dataSize];
		test[i] = new double[dataSize];
	}


	for (unsigned int i = 0; i < doseDimension[2]; i++)
	{
		memcpy(m_pPlan->m_pCalculatedDose->m_ppwDen[i], &*dd.begin() + dataSize * i, dataSize * sizeof(float));
	}

	//memcpy(m_pPlan->m_pCalculatedDose->m_ppwDen, &*dd.begin(), dd.getNumberOfElements()*sizeof(double));
	*/

	/*
	int size = dd.getNumberOfElements();

	for (int i = 0; i < doseDimension[2]; i++)
	{
		for (int ny = 0; ny < doseDimension[0]; ny++) // rows
		{
			for (int nx = 0; nx < doseDimension[1]; nx++) // cols
			{
				m_pPlan->m_pCalculatedDose->m_ppwDen[i][ny * doseDimension[1] + nx] = (float)dd[ny][nx][i];

			}
		}
	}
	*/

	// m_PanelCT_top->ImportCPlan(m_pPlan, PanelRTDisplay::ImportType::DOSE, false);

	if (m_bOptimizedDone == false)
	{
		MessageBox(_T("No Optimized"), _T("Warning", MB_OK));

		return;
	}

	try
	{
	}
	catch (const std::exception e)
	{
	}

	/*
	if (m_pPlan->m_vecCatheters.size() >= 3)
	{
		AngleDwellTime ovoidR_1; ovoidR_1.m_fAngle = 0; ovoidR_1.m_fDwellTime = 21.7484;
		AngleDwellTime ovoidR_2; ovoidR_2.m_fAngle = 0; ovoidR_2.m_fDwellTime = 25.9999;
		AngleDwellTime ovoidR_3; ovoidR_3.m_fAngle = 0; ovoidR_3.m_fDwellTime = 10.5561;

		m_pPlan->m_vecCatheters[0]->m_vecDwellPositions[0].m_bIsActive = true; // Ovoid R
		m_pPlan->m_vecCatheters[0]->m_vecDwellPositions[0].m_vecAnglDwell.push_back(ovoidR_1); // Ovoid R
		m_pPlan->m_vecCatheters[0]->m_vecDwellPositions[1].m_bIsActive = true; // Ovoid R
		m_pPlan->m_vecCatheters[0]->m_vecDwellPositions[1].m_vecAnglDwell.push_back(ovoidR_2); // Ovoid R
		m_pPlan->m_vecCatheters[0]->m_vecDwellPositions[2].m_bIsActive = true; // Ovoid R
		m_pPlan->m_vecCatheters[0]->m_vecDwellPositions[2].m_vecAnglDwell.push_back(ovoidR_3); // Ovoid R


		AngleDwellTime ovoidL_1; ovoidL_1.m_fAngle = 0; ovoidL_1.m_fDwellTime = 43.4696;
		AngleDwellTime ovoidL_2; ovoidL_2.m_fAngle = 0; ovoidL_2.m_fDwellTime = 77.6574;
		AngleDwellTime ovoidL_3; ovoidL_3.m_fAngle = 0; ovoidL_3.m_fDwellTime = 17.6621;
		AngleDwellTime ovoidL_4; ovoidL_4.m_fAngle = 0; ovoidL_4.m_fDwellTime = 18.9685;
		m_pPlan->m_vecCatheters[1]->m_vecDwellPositions[0].m_bIsActive = true; // Ovoid L
		m_pPlan->m_vecCatheters[1]->m_vecDwellPositions[0].m_vecAnglDwell.push_back(ovoidL_1); // Ovoid L
		m_pPlan->m_vecCatheters[1]->m_vecDwellPositions[1].m_bIsActive = true; // Ovoid L
		m_pPlan->m_vecCatheters[1]->m_vecDwellPositions[1].m_vecAnglDwell.push_back(ovoidL_2); // Ovoid L
		m_pPlan->m_vecCatheters[1]->m_vecDwellPositions[2].m_bIsActive = true; // Ovoid L
		m_pPlan->m_vecCatheters[1]->m_vecDwellPositions[2].m_vecAnglDwell.push_back(ovoidL_3); // Ovoid L
		m_pPlan->m_vecCatheters[1]->m_vecDwellPositions[2].m_bIsActive = true; // Ovoid L
		m_pPlan->m_vecCatheters[1]->m_vecDwellPositions[2].m_vecAnglDwell.push_back(ovoidL_4); // Ovoid R

		m_pPlan->m_vecCatheters[2]->m_bRotation = true;

		m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[0].m_bIsActive = true; // Tandem
		AngleDwellTime T1; T1.m_fAngle = 0.0; T1.m_fDwellTime = 0.0; m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[0].m_vecAnglDwell.push_back(T1);

		m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[1].m_bIsActive = true; // Tandem
		AngleDwellTime T2; T2.m_fAngle = 0.0; T2.m_fDwellTime = 0.0; m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[1].m_vecAnglDwell.push_back(T2);

		m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[2].m_bIsActive = true; // Tandem
		AngleDwellTime T3; T3.m_fAngle = 0.0; T3.m_fDwellTime = 0.0; m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[2].m_vecAnglDwell.push_back(T3);

		m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[3].m_bIsActive = true; // Tandem
		AngleDwellTime T4; T4.m_fAngle = 0.0; T4.m_fDwellTime = 0.0; m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[3].m_vecAnglDwell.push_back(T4);

		m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[4].m_bIsActive = true; // Tandem
		AngleDwellTime T5; T5.m_fAngle = 0.0; T5.m_fDwellTime = 0.0; m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[4].m_vecAnglDwell.push_back(T5);

		m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[5].m_bIsActive = true; // Tandem
		AngleDwellTime T6; T6.m_fAngle = 0.0; T6.m_fDwellTime = 0.0; m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[5].m_vecAnglDwell.push_back(T6);

		m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[6].m_bIsActive = true; // Tandem
		AngleDwellTime T7; T7.m_fAngle = 0.0; T7.m_fDwellTime = 0.0; m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[6].m_vecAnglDwell.push_back(T7);

		m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[7].m_bIsActive = true; // Tandem
		AngleDwellTime T8; T8.m_fAngle = 0.0; T8.m_fDwellTime = 0.0; m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[7].m_vecAnglDwell.push_back(T8);

		m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[8].m_bIsActive = true; // Tandem
		AngleDwellTime T9; T9.m_fAngle = 210; T9.m_fDwellTime = 38.9327; m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[8].m_vecAnglDwell.push_back(T9);

		m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[9].m_bIsActive = true; // Tandem
		AngleDwellTime T10; T10.m_fAngle = 160; T10.m_fDwellTime = 137.7229; m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[9].m_vecAnglDwell.push_back(T10);

		m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[10].m_bIsActive = true; // Tandem
		AngleDwellTime T11; T11.m_fAngle = 0; T11.m_fDwellTime = 0; m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[10].m_vecAnglDwell.push_back(T11);

		m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[11].m_bIsActive = true; // Tandem
		AngleDwellTime T12; T12.m_fAngle = 0; T12.m_fDwellTime = 0; m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[11].m_vecAnglDwell.push_back(T12);

		m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[12].m_bIsActive = true; // Tandem
		AngleDwellTime T13; T13.m_fAngle = 0; T13.m_fDwellTime = 0; m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[12].m_vecAnglDwell.push_back(T13);

		m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[13].m_bIsActive = true; // Tandem
		AngleDwellTime T14; T14.m_fAngle = 0; T14.m_fDwellTime = 0; m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[13].m_vecAnglDwell.push_back(T14);

		m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[14].m_bIsActive = true; // Tandem
		AngleDwellTime T15; T15.m_fAngle = 0; T15.m_fDwellTime = 0; m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[14].m_vecAnglDwell.push_back(T15);

		m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[15].m_bIsActive = true; // Tandem
		AngleDwellTime T16; T16.m_fAngle = 0; T16.m_fDwellTime = 0; m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[15].m_vecAnglDwell.push_back(T16);

		m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[16].m_bIsActive = true; // Tandem
		AngleDwellTime T17; T17.m_fAngle = 0; T17.m_fDwellTime = 0; m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[16].m_vecAnglDwell.push_back(T17);

		m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[17].m_bIsActive = true; // Tandem
		AngleDwellTime T18; T18.m_fAngle = 250; T18.m_fDwellTime = 22.5043; m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[17].m_vecAnglDwell.push_back(T18);

		m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[18].m_bIsActive = true; // Tandem
		AngleDwellTime T19; T19.m_fAngle = 0; T19.m_fDwellTime = 0; m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[18].m_vecAnglDwell.push_back(T19);

		m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[19].m_bIsActive = true; // Tandem
		AngleDwellTime T20; T20.m_fAngle = 0; T20.m_fDwellTime = 0; m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[19].m_vecAnglDwell.push_back(T20);

		m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[20].m_bIsActive = true; // Tandem
		AngleDwellTime T21; T21.m_fAngle = 0; T21.m_fDwellTime = 0; m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[20].m_vecAnglDwell.push_back(T21);

		m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[21].m_bIsActive = true; // Tandem
		AngleDwellTime T22; T22.m_fAngle = 0; T22.m_fDwellTime = 0; m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[21].m_vecAnglDwell.push_back(T22);

		m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[22].m_bIsActive = true; // Tandem
		AngleDwellTime T23; T23.m_fAngle = 0; T23.m_fDwellTime = 0; m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[22].m_vecAnglDwell.push_back(T23);

		m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[23].m_bIsActive = true; // Tandem
		AngleDwellTime T24; T24.m_fAngle = 0; T24.m_fDwellTime = 0; m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[23].m_vecAnglDwell.push_back(T24);

		m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[24].m_bIsActive = true; // Tandem
		AngleDwellTime T25; T25.m_fAngle = 0; T25.m_fDwellTime = 0; m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[24].m_vecAnglDwell.push_back(T25);

		m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[25].m_bIsActive = true; // Tandem
		AngleDwellTime T26; T26.m_fAngle = 0; T26.m_fDwellTime = 0; m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[25].m_vecAnglDwell.push_back(T26);

		m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[26].m_bIsActive = true; // Tandem
		AngleDwellTime T27; T27.m_fAngle = 0; T27.m_fDwellTime = 0; m_pPlan->m_vecCatheters[2]->m_vecDwellPositions[26].m_vecAnglDwell.push_back(T27);

		m_pPlan->m_pCalculatedDose->m_afPatOrg[0] = m_pPlan->m_pPlannedCTData->m_afPatOrg[0];
		m_pPlan->m_pCalculatedDose->m_afPatOrg[1] = m_pPlan->m_pPlannedCTData->m_afPatOrg[1];
		m_pPlan->m_pCalculatedDose->m_afPatOrg[2] = m_pPlan->m_pPlannedCTData->m_afPatOrg[2];
		m_pPlan->m_pCalculatedDose->m_afPatOrientHor[0] = m_pPlan->m_pPlannedCTData->m_afPatOrientHor[0];
		m_pPlan->m_pCalculatedDose->m_afPatOrientHor[1] = m_pPlan->m_pPlannedCTData->m_afPatOrientHor[1];
		m_pPlan->m_pCalculatedDose->m_afPatOrientHor[2] = m_pPlan->m_pPlannedCTData->m_afPatOrientHor[2];
		m_pPlan->m_pCalculatedDose->m_afPatOrientVer[0] = m_pPlan->m_pPlannedCTData->m_afPatOrientVer[0];
		m_pPlan->m_pCalculatedDose->m_afPatOrientVer[1] = m_pPlan->m_pPlannedCTData->m_afPatOrientVer[1];
		m_pPlan->m_pCalculatedDose->m_afPatOrientVer[2] = m_pPlan->m_pPlannedCTData->m_afPatOrientVer[2];
		m_pPlan->m_pCalculatedDose->m_afRes[0] = m_pPlan->m_pPlannedCTData->m_afRes[0];
		m_pPlan->m_pCalculatedDose->m_afRes[1] = m_pPlan->m_pPlannedCTData->m_afRes[1];
		m_pPlan->m_pCalculatedDose->m_afRes[2] = m_pPlan->m_pPlannedCTData->m_afRes[2];
		m_pPlan->m_pCalculatedDose->m_anDim[0] = m_pPlan->m_pPlannedCTData->m_anDim[0];
		m_pPlan->m_pCalculatedDose->m_anDim[1] = m_pPlan->m_pPlannedCTData->m_anDim[1];
		m_pPlan->m_pCalculatedDose->m_anDim[2] = m_pPlan->m_pPlannedCTData->m_anDim[2];

		int SliceSize = m_pPlan->m_pPlannedCTData->m_anDim[2];
		int rows= m_pPlan->m_pPlannedCTData->m_anDim[0];
		int cols = m_pPlan->m_pPlannedCTData->m_anDim[1];

		m_pPlan->m_pCalculatedDose->m_ppwDen = new WORD * [SliceSize];

		unsigned int dataSize = cols * rows;

		for (int i = 0; i < SliceSize; i++)
		{
			//CTs.Pixel[i] = new uint8_t[dataSize];
			m_pPlan->m_pCalculatedDose->m_ppwDen[i] = new WORD[dataSize];
		}



		for (int i = 0; i < SliceSize; i++)
		{
			for (int ny = 0; ny < rows; ny++)
			{
				for (int nx = 0; nx < cols; nx++)
				{
					WORD dose = (WORD)m_testDose_512.at(ny, nx, i);
					m_pPlan->m_pCalculatedDose->m_ppwDen[SliceSize-1-i][ny * cols + nx] = dose;
				}
			}
		}
	}
	*/

	CDialog::OnOK();
}
#endif // USE_MATLAB_ENGINE