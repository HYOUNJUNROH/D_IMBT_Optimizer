#pragma once

#include "afxdialogex.h"

// CCreatingDlg dialog

class CCreatingDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CCreatingDlg)

public:
	CCreatingDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CCreatingDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_CREATING };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
};
