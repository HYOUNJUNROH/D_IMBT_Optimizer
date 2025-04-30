// CRunningDlg.cpp : implementation file
//

#include "pch.h"
#include "CRunningDlg.h"
#include "afxdialogex.h"
#include "resource.h"


// CRunningDlg dialog

IMPLEMENT_DYNAMIC(CRunningDlg, CDialogEx)

CRunningDlg::CRunningDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_RUNNING, pParent)
{

}

CRunningDlg::~CRunningDlg()
{
}

void CRunningDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CRunningDlg, CDialogEx)
END_MESSAGE_MAP()


// CRunningDlg message handlers


BOOL CRunningDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CenterWindow();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}
