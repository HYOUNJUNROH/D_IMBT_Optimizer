// CCreatingDlg.cpp : implementation file
//

#include "pch.h"
#include "CCreatingDlg.h"
#include "afxdialogex.h"
#include "resource.h"


// CCreatingDlg dialog

IMPLEMENT_DYNAMIC(CCreatingDlg, CDialogEx)

CCreatingDlg::CCreatingDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_CREATING, pParent)
{

}

CCreatingDlg::~CCreatingDlg()
{
}

void CCreatingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CCreatingDlg, CDialogEx)
END_MESSAGE_MAP()


// CCreatingDlg message handlers


BOOL CCreatingDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CenterWindow();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}
