#pragma once
#include <string>
#include <vector>
#include <ctime>

enum SourceMovementType { SteppingBackward = 0, SteppingForward = 1 };
enum CommissionStatus { Status_Commissioned = 11, Status_Deprecated = 12, Status_Uncommissioned = 13 };

/// <summary>
/// IsotopeType: Class to desribe the type of Isotope from the manufacture
/// </summary>
struct IsotopeType
{
public:
	CString m_strID;
	CString m_strName;
	CString m_strManufacturedBy;
	CString m_strModel;
	int		m_iHalfLife; // Ir192 = 73.827 days
	CString m_strTG43_anisotropyPath;
	CString	m_strTG43_radialPath;
};

/// <summary>
/// CommissionedIsotope: Class to present the commissioned Isotope information and activity
/// </summary>
struct CommissionedIsotope
{
public:
	CString				m_strSerialNumber;
	CString				m_strName;
	IsotopeType			m_IsotopeType;
	float				m_fActivity;
	time_t				m_CalibrationDate;
	CString				m_strCalibratedBy;
	CommissionStatus	m_commissionStatus;
	CString				m_strCommissionedBy;
	time_t				m_DeprecatedDate;
	CString				m_strDeprecatedBy;
	bool				m_bIsApproved;
};



/// <summary>
/// Afterloader: Remote Afterloader Class
/// </summary>
class AFX_EXT_CLASS Afterloader
{
private:
	CString					m_strModel;
	CString					m_strName;
	CommissionedIsotope		m_CommissionedIsotope;
	unsigned int			m_iNumberOfChannels;
	unsigned int			m_iTransferLength;
	SourceMovementType		m_SourceMovementType;
	std::vector<float>		m_StepSizeSpec;
	unsigned int			m_NumOfDwellSteps;
	unsigned int			m_SourceCycles;
	CommissionStatus		m_commissionStatus;
	time_t					m_CommisionedDate;
	CString					m_strCommissionedBy;
	time_t					m_DeprecatedDate;
	CString					m_strDeprecatedBy;

public:
	Afterloader() {};
	~Afterloader() {};
};



