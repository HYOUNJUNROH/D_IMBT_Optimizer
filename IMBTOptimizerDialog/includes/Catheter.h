#pragma once
#include <string>
#include <vector>

#include "CommonAPI.h"

struct AngleDwellTime
{
public:
	float m_fAngle;
	float m_fDwellTime;
};

class AFX_EXT_CLASS DwellPosition
{
public:
	DwellPosition()
	{
		m_nPositionID = 0;
		m_bIsActive = false;
	}

	DwellPosition(int positionID, int angle, _FPOINT3 fptPosition, std::vector<AngleDwellTime> vecAngleDwell, bool bIsActive)
	{
		m_nPositionID = positionID;
		m_fptPosition = fptPosition;
		m_vecAnglDwell = vecAngleDwell;
		m_nAngle = angle;	
		m_bIsActive = bIsActive;
	};
	~DwellPosition() {};

public:
	unsigned int m_nPositionID;
	_FPOINT3 m_fptPosition;
	std::vector<AngleDwellTime> m_vecAnglDwell;
	int m_nAngle;
	bool m_bIsActive;
};



class AFX_EXT_CLASS Catheter
{
public:
	unsigned int m_nIndex;
	CString m_sName;
	unsigned int m_nLength;
	float m_fOffset;
	float m_fStepSize;
	bool m_bRotation;

	std::vector<DwellPosition> m_vecDwellPositions;
};


