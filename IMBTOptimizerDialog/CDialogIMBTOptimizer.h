#pragma once

#include "PanelRTDisplay.h"

#include "MatlabDataArray.hpp"
#include "MatlabEngine.hpp"



class CPlan;
class CUniButton;

// CDialogIMBTOptimizer dialog
class __declspec(dllexport) CDialogIMBTOptimizer : public CDialog
{
	DECLARE_DYNAMIC(CDialogIMBTOptimizer)

public:
	CDialogIMBTOptimizer(CWnd* pParent /*= nullptr*/, CPlan* _plan);   // standard constructor
	virtual ~CDialogIMBTOptimizer();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_Optimizer };
#endif

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	afx_msg void OnSize(UINT nType, int cx, int cy);

	DECLARE_MESSAGE_MAP()


protected: // UI-related members
	CUniButton *m_ButtonOK;
	CUniButton *m_ButtonCancel;
	CUniButton *m_ButtonInitialize;
	CUniButton* m_ButtonStartOptimizer;
	CListCtrl m_listOarTarget;
	CRichEditCtrl m_infoOptimizer_rich;
	CEdit m_infoOptimizer;

protected:
	CPlan* m_pPlan;
	PanelRTDisplay *m_PanelCT_top, *m_PanelCT_bottom;

protected: // Dose Calculation Options
	int m_nDims[3]; //[0]:x, [1]:y, [2]:z
	float m_fDoseResolution;
	float m_fAngleResolution;
	std::vector<float> m_vAnglePositions;
	// Dwell Position Reference: m_pPlan->m_vecCatheters[i]->m_vecDwellPositions[j].m_fptPosition.fx fy fz

	bool m_bInitializeDone;
	bool m_bOptimizedDone;


protected: // MC data - Matlab Hdf5 format
	arma::Cube<float> m_cubeSystemA_2nd;
	arma::Cube<float> m_cubeSystemA_ovoid1_left;
	arma::Cube<float> m_cubeSystemA_ovoid1_right;
	arma::Cube<float> m_testDose_512;
	//arma::Cube<float> m_testDose_512_patient1;
	//arma::Cube<float> m_testDose_512_patient2;

protected: // OAR Structure
	/*
	arma::Cube<float> m_maskBody_dose;
	arma::Cube<float> m_maskPTV_dose;
	arma::Cube<float> m_maskBladder_dose;
	arma::Cube<float> m_maskRectum_dose;
	arma::Cube<float> m_maskBowel_dose;
	arma::Cube<float> m_maskSigmoid_dose;
	*/


public: // UI-realated functions
	void InfoTextOut(LPCTSTR strText, COLORREF textColor);

protected: // Optimizer-related functions
	bool initializeOptimizer();
	bool LoadMatFiles();
	bool LoadCTandStruct();
	bool CreateDoseMatrix();

public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedButtonInitialize();
	afx_msg void OnBnClickedButtonStartOptimizer();
};
