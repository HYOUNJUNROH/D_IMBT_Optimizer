
// IMBT_OptimizerDlg.h : header file
//

#pragma once

#include "../IMBTOptimizerDialog/PanelRTDisplay.h"


// CIMBTOptimizerDlg dialog
class CIMBTOptimizerDlg : public CDialogEx
{
// Construction
public:
	CIMBTOptimizerDlg(CWnd* pParent = nullptr);	// standard constructor
	~CIMBTOptimizerDlg() {};

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_IMBT_OPTIMIZER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

	PanelRTDisplay m_imagePanel;
	CPlan *m_plan;
	bool m_CtModified = false;  
	bool m_StructModified = false;

public:
	afx_msg void OnBnClickedButtonOptimizer();
	afx_msg void OnBnClickedButtonLoadCt();
	afx_msg void OnBnClickedButtonLoadStruct();
};
