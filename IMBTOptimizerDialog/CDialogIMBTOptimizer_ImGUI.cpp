// CDialogIMBTOptimizer_ImGUI.cpp : implementation file
//

#include "IMBTOptimizerDialog_ImGui.h"

//#include "pch.h"
#include "afxdialogex.h"

#include "CDialogIMBTOptimizer_ImGUI.h"


#include "resource.h"


////////////////////////////////////////////////////////////////////
// CDialogIMBTOptimizer_ImGUI dialog
////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CDialogIMBTOptimizer_ImGUI, CDialogEx)

CDialogIMBTOptimizer_ImGUI::CDialogIMBTOptimizer_ImGUI(CPlan *_pPlan, bool bPlanModified, bool* _pCalculatedDoseConfirm, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_OPTIMIZER_IMGUI, pParent)
{
	m_pCPlan = _pPlan;
	m_bPlanModified = bPlanModified;  // 전달받은 값을 멤버에 저장
	m_CalculatedDoseConfirm = _pCalculatedDoseConfirm;
	m_pGUI = NULL;
}

CDialogIMBTOptimizer_ImGUI::~CDialogIMBTOptimizer_ImGUI()
{
	if (m_pGUI != NULL)
		delete m_pGUI;
}

void CDialogIMBTOptimizer_ImGUI::OnCancel()
{
	if (m_pGUI != NULL)
	{
		while (!m_pGUI->closeGUI()) {};
	}

	CDialogEx::OnCancel();
}


void CDialogIMBTOptimizer_ImGUI::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}



BEGIN_MESSAGE_MAP(CDialogIMBTOptimizer_ImGUI, CDialogEx)
	ON_WM_ERASEBKGND()
	ON_WM_GETMINMAXINFO()
END_MESSAGE_MAP()


// CDialogIMBTOptimizer_ImGUI message handlers



BOOL CDialogIMBTOptimizer_ImGUI::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	std::thread thread{ [=]() {
		m_pGUI = new IMBTOptimizerDialog_ImGui(m_pCPlan, this, m_bPlanModified, m_CalculatedDoseConfirm);
		m_pGUI->initGUI(GetSafeHwnd());
	} };
	thread.detach();

	return true;
}


BOOL CDialogIMBTOptimizer_ImGUI::OnEraseBkgnd(CDC* pDC)
{
	CRect rect;
	GetClientRect(&rect);
	CBrush myBrush(RGB(50, 61, 72));    // dialog background color
	CBrush* pOld = pDC->SelectObject(&myBrush);
	BOOL bRes = pDC->PatBlt(0, 0, rect.Width(), rect.Height(), PATCOPY);
	pDC->SelectObject(pOld);    // restore old brush
	return bRes;                       // CDialog::OnEraseBkgnd(pDC);

}

void CDialogIMBTOptimizer_ImGUI::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI)
{
	CDialogEx::OnGetMinMaxInfo(lpMMI);
	lpMMI->ptMinTrackSize.x = 1536;
	lpMMI->ptMinTrackSize.y = 864;
}
