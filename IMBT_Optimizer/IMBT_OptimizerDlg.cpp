
// IMBT_OptimizerDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "IMBT_Optimizer.h"
#include "IMBT_OptimizerDlg.h"
#include "afxdialogex.h"

#ifdef USE_MATLAB_ENGINE
#include "../IMBTOptimizerDialog/CDialogIMBTOptimizer.h"
#endif

#include "../IMBTOptimizerDialog/IMBTOptimizerDialog_ImGui.h"
#include "../IMBTOptimizerDialog/CDialogIMBTOptimizer_ImGUI.h"

#include "CCreatingDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CAboutDlg dialog used for App About

static bool calculatedDoseConfirm;


class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum
	{
		IDD = IDD_ABOUTBOX
	};
#endif

protected:
	virtual void DoDataExchange(CDataExchange *pDX); // DDX/DDV support

	// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange *pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// CIMBTOptimizerDlg dialog

CIMBTOptimizerDlg::CIMBTOptimizerDlg(CWnd *pParent /*=nullptr*/)
	: CDialogEx(IDD_IMBT_OPTIMIZER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	calculatedDoseConfirm = false;
}

void CIMBTOptimizerDlg::DoDataExchange(CDataExchange *pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_IMAGE_PANEL, m_imagePanel); // Custom Control를 PanelRTDisplay와 연결
}

BEGIN_MESSAGE_MAP(CIMBTOptimizerDlg, CDialogEx)
ON_WM_SYSCOMMAND()
ON_WM_PAINT()
ON_WM_QUERYDRAGICON()
ON_BN_CLICKED(IDC_BUTTON_OPTIMIZER, &CIMBTOptimizerDlg::OnBnClickedButtonOptimizer)
ON_BN_CLICKED(IDC_BUTTON_LOAD_CT, &CIMBTOptimizerDlg::OnBnClickedButtonLoadCt)
ON_BN_CLICKED(IDC_BUTTON_LOAD_STRUCT, &CIMBTOptimizerDlg::OnBnClickedButtonLoadStruct)
END_MESSAGE_MAP()

// CIMBTOptimizerDlg message handlers

BOOL CIMBTOptimizerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu *pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);	 // Set big icon
	SetIcon(m_hIcon, FALSE); // Set small icon

	// TODO: Add extra initialization here

	return TRUE; // return TRUE  unless you set the focus to a control
}

void CIMBTOptimizerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CIMBTOptimizerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CIMBTOptimizerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CIMBTOptimizerDlg::OnBnClickedButtonOptimizer()
{
	/*
	HINSTANCE prevRes = AfxGetResourceHandle();
	HINSTANCE hRes = NULL;

	hRes = LoadLibrary(L"IMBTOptimizerDialog.dll");


	if (hRes)
	{
		CDialogIMBTOptimizer dlg(0, &m_plan);

		dlg.DoModal();
	}

	FreeLibrary(hRes);
	*/

	m_plan->m_strName = "Patinet Name";
	m_plan->m_strID = "Test_Patient_2";
	m_plan->m_fPrescriptionDosePerFraction = 500.0;
	m_plan->m_fPlannedFraction = 6.0;

	m_plan->m_pCalculatedDose = new CDoseData();

	Catheter *catheter_1 = new Catheter();
	Catheter *catheter_2 = new Catheter();
	Catheter *catheter_3 = new Catheter();

	// Channel 1: Ovoid R
	_FPOINT3 channel_1[8] = {
		{-54.47, -48.83, -64.07},
		{-54.6, -62.05, -64.51},
		{-54.58, -65.24, -66.29},
		{-54.12, -67.84, -69.23},
		{-54.21, -70.22, -72.96},
		{-54.32, -72.84, -77.66},
		{-54.53, -75.07, -84.11},
		{-56.23, -87.50, -133.29}};

	// Channel 2: Ovoid L
	_FPOINT3 channel_2[8] = {
		{-38.80, -50.23, -63.03},
		{-38.49, -60.78, -63.91},
		{-38.45, -63.14, -65.03},
		{-38.43, -65.46, -66.66},
		{-38.42, -68.06, -68.74},
		{-38.46, -70.23, -71.80},
		{-38.54, -72.66, -76.50},
		{-40.10, -87.99, -136.21}};

	// Channel 3: Tandem
	_FPOINT3 channel_3[27] = {
		{-47.02, -60.64, -3.95},
		{-47.03, -60.56, -6.45},
		{-47.03, -60.47, -8.95},
		{-47.04, -60.39, -11.45},
		{-47.05, -60.30, -13.94},
		{-47.05, -60.22, -16.44},
		{-47.06, -60.13, -18.94},
		{-47.07, -60.05, -21.44},
		{-47.07, -59.96, -23.94},
		{-47.08, -59.88, -26.44},
		{-47.09, -59.72, -28.93},
		{-47.10, -59.55, -31.43},
		{-47.11, -59.38, -33.92},
		{-47.11, -59.27, -36.42},
		{-47.12, -59.22, -38.92},
		{-47.12, -59.17, -41.42},
		{-47.13, -59.13, -43.92},
		{-47.13, -59.21, -46.41},
		{-47.14, -59.32, -48.91},
		{-47.14, -59.50, -51.40},
		{-47.14, -59.74, -53.89},
		{-47.15, -59.98, -56.38},
		{-47.15, -60.22, -58.87},
		{-47.15, -60.60, -61.34},
		{-47.15, -61.07, -63.79},
		{-47.15, -61.54, -66.25},
		{-47.15, -62.01, -68.70}};

	// Channel 1: Ovoid R
	for (int i = 0; i < 8; i++)
	{
		DwellPosition a;
		a.m_bIsActive = false;
		a.m_nPositionID = i;
		a.m_fptPosition = channel_1[i];
		a.m_nAngle = 0;
		catheter_1->m_vecDwellPositions.push_back(a);
	}

	// Channel 2: Ovoid L
	for (int i = 0; i < 8; i++)
	{
		DwellPosition a;
		a.m_bIsActive = false;
		a.m_nPositionID = i;
		a.m_fptPosition = channel_2[i];
		a.m_nAngle = 0;
		catheter_2->m_vecDwellPositions.push_back(a);
	}

	// Channel 3: Tandem
	for (int i = 0; i < 27; i++)
	{
		DwellPosition a;
		a.m_bIsActive = false;
		a.m_nPositionID = i;
		a.m_fptPosition = channel_3[i];
		a.m_nAngle = 0;
		catheter_3->m_vecDwellPositions.push_back(a);
	}

	//m_plan.m_vecCatheters = new std::vector<Catheter*>;

	m_plan->m_vecCatheters.clear();

	m_plan->m_vecCatheters.push_back(catheter_1);
	m_plan->m_vecCatheters.push_back(catheter_2);
	m_plan->m_vecCatheters.push_back(catheter_3);

	/*
	CDialogIMBTOptimizer dlg(0, &m_plan);

	dlg.DoModal();
	*/

	// IMBTOptimizerDialog_ImGui dlg(&m_plan);
	// dlg.showDialog();
	// dlg.showDialog(GetSafeHwnd());
	// dlg.initGUI(GetSafeHwnd());

	/*
	CDialogIMBTOptimizer_ImGUI dlg(&m_plan);
	dlg.DoModal();
	*/
	bool m_Modified = m_CtModified || m_StructModified;
	CDialogIMBTOptimizer_ImGUI dlg(m_plan, m_Modified, &calculatedDoseConfirm);
	dlg.DoModal();

	m_CtModified = false;
	m_StructModified = false;
	/*
	IMBTDialog_imgui test;

	test.Show();
	*/
}

void CIMBTOptimizerDlg::OnBnClickedButtonLoadCt()
{
	m_plan = new CPlan();
	LPWSTR cPath[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, (LPWSTR)cPath);

	CFolderPickerDialog Picker((LPWSTR)cPath, OFN_FILEMUSTEXIST, NULL, 0);

	if (Picker.DoModal() == IDOK)
	{
		auto ct_path = Picker.GetPathName();
		reinterpret_cast<CIMBTOptimizerApp *>(AfxGetApp())->ct_path = ct_path;
		if (m_plan->LoadCTData(ct_path) == FALSE)
		{
			MessageBox(_T("Failed"), MB_OK);
			return;
		}

		m_imagePanel.ImportCPlan(m_plan, PanelRTDisplay::ImportType::CT, true);
	}

	m_CtModified = true;
	/*
	if (Picker.DoModal() == IDOK)
	{
		m_imagePanel.GetSlices(Picker.GetPathName());
	}
	*/
}

void CIMBTOptimizerDlg::OnBnClickedButtonLoadStruct()
{
	static TCHAR BASED_CODE szFilter[] = _T("DICOM File(*.dcm) | *.dcm|All Files(*.*)|*.*||");

	CFileDialog dlg(TRUE, _T("*.dcm"), _T("dicom file"), OFN_HIDEREADONLY, szFilter);

	if (IDOK == dlg.DoModal())
	{
		auto dcm_path = dlg.GetPathName();
		reinterpret_cast<CIMBTOptimizerApp *>(AfxGetApp())->dcm_path = dcm_path;
		if (m_plan->LoadStructData(dcm_path) == FALSE)
		{
			MessageBox(_T("Failed"), MB_OK);
			return;
		}

		m_imagePanel.ImportCPlan(m_plan, PanelRTDisplay::ImportType::RTSTRUCT, true);
	}

	m_StructModified = true;
	/*
	if (IDOK == dlg.DoModal())
	{
		CString pathName = dlg.GetPathName();

		m_imagePanel.GetStruct(pathName);
	}

	*/

	/*
	int t = m_imagePanel.RTs.Data->m_vecROIItems.size();

	for (int i = 0; i < t; i++)
	{
		UpdatedRows.push_back(0);

		m_view.InsertItem(i, m_Panel01.RTs.Data->m_vecROIItems[i]->m_strName);
		m_view.SetItem(i, 1, LVIF_TEXT, _T(""), 0, 0, 0, 0);
		m_view.SetItem(i, 2, LVIF_TEXT, _T(""), 0, 0, 0, 0);
		m_view.SetItem(i, 3, LVIF_TEXT, _T(""), 0, 0, 0, 0);
		m_view.SetItem(i, 4, LVIF_TEXT, _T(""), 0, 0, 0, 0);
		m_view.SetItem(i, 5, LVIF_TEXT, _T(""), 0, 0, 0, 0);
	}
	*/
}
