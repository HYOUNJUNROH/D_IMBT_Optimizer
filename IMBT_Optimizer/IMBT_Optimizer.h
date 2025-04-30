
// IMBT_Optimizer.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols

// CIMBTOptimizerApp:
// See IMBT_Optimizer.cpp for the implementation of this class
//

class CIMBTOptimizerApp : public CWinApp
{
public:
	CIMBTOptimizerApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation
public:
    CString ct_path;
    CString dcm_path;
	DECLARE_MESSAGE_MAP()
};

extern CIMBTOptimizerApp theApp;
