#pragma once

#include <vector>

#include "CommonAPI.h"
#include "Catheter.h"
#include "Afterloader.h"

/// <summary>
/// Plan Class
/// </summary>
class AFX_EXT_CLASS CPlan
{
public:
	CPlan();
	~CPlan();

	bool LoadCTData(LPCTSTR lpszFolderPath);
	bool LoadStructData(LPCTSTR lpszFilePath);

public:
	CString					m_strID;
	CString					m_strName;
	std::vector<Catheter*>	m_vecCatheters;
	Afterloader				m_Afterloader;
	CCTData*				m_pPlannedCTData;
	StructData*				m_pPlannedStructData;
	float					m_CalculationGridSize[3];
	CalculationArea			m_CalculationArea;
	CDoseData*				m_pCalculatedDose;
	DoseUnit				m_DoseUnit;
	float					m_fPrescriptionDosePerFraction;
	float					m_fPlannedFraction;
	bool					m_bIsApproved;
	time_t					m_PlanCreatedDate;
	time_t					m_ApprovedDate;
	CString					m_strLastSavedBy;
};
