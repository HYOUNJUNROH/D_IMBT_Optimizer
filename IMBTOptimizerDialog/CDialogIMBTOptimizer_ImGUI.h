#pragma once

class CPlan;
class IMBTOptimizerDialog_ImGui;

// ����(Ȥ�� �Լ� ���� ����)�� ����
static int sSelectedAngleIndex = 0;
static int sSelectedDwellIndex = 0;
// CDialogIMBTOptimizer_ImGUI dialog
class __declspec(dllexport)  CDialogIMBTOptimizer_ImGUI : public CDialogEx
{
	DECLARE_DYNAMIC(CDialogIMBTOptimizer_ImGUI)

public:
	CDialogIMBTOptimizer_ImGUI(CPlan * _pPlan, bool bPlanModified, bool* _pCalculatedDoseConfirm, CWnd* pParent = nullptr);   // standard constructor
	virtual ~CDialogIMBTOptimizer_ImGUI();
	void closeDialog()
	{
		OnCancel();
	}

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_OPTIMIZER_IMGUI };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnCancel();
	bool m_bPlanModified = true;  // ���� �߰�. �÷��� �����Ǿ����� ����
	bool* m_CalculatedDoseConfirm; // ���� �߰�. Confirm�� �����Ǿ����� ����

	BOOL PreTranslateMessage(MSG* pMsg)
	{
		switch (pMsg->message)
		{
		case WM_KEYDOWN:

			switch (pMsg->wParam)
			{
			case VK_ESCAPE:
			case VK_CANCEL: return true;
			}
			break;
		}

		return CDialog::PreTranslateMessage(pMsg);
	}

	DECLARE_MESSAGE_MAP()


private:
	CPlan* m_pCPlan;
	IMBTOptimizerDialog_ImGui* m_pGUI;
public:
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
};
