#pragma once

#include <afxtempl.h>
#include <string>
#include <vector>

#define SAFE_DELETE(x)				if (x) { delete (x); (x) = NULL; }
#define SAFE_DELETEARRAY(x)			if (x) { delete [] (x); (x) = NULL; }


enum DoseUnit {cGy=0, Gy=1};

class AFX_EXT_CLASS _FPOINT3
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// constructor and destructor
public:
	_FPOINT3(void) : fx(0.f), fy(0.f), fz(0.f) {};
	_FPOINT3(float x, float y, float z) : fx(x), fy(y), fz(z) {};

	// destructor
	virtual ~_FPOINT3() {};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// operators
public:
	_FPOINT3& operator= (const _FPOINT3& fp)
	{
		fx = fp.fx; fy = fp.fy; fz = fp.fz;
		return (*this);
	}

	_FPOINT3 operator+ (const _FPOINT3& fp) const
	{
		return _FPOINT3(fx + fp.fx, fy + fp.fy, fz + fp.fz);
	}

	_FPOINT3 operator- (const _FPOINT3& fp) const
	{
		return _FPOINT3(fx - fp.fx, fy - fp.fy, fz - fp.fz);
	}

	_FPOINT3 operator* (float scalar) const
	{
		return _FPOINT3(fx * scalar, fy * scalar, fz * scalar);
	}

	_FPOINT3 operator/ (float scalar) const
	{
		ASSERT(scalar != 0.0f);
		return _FPOINT3(fx / scalar, fy / scalar, fz / scalar);
	}

	_FPOINT3& operator+= (const _FPOINT3& fp)
	{
		fx += fp.fx; fy += fp.fy; fz += fp.fz;
		return (*this);
	}

	_FPOINT3& operator-= (const _FPOINT3& fp)
	{
		fx -= fp.fx; fy -= fp.fy; fz -= fp.fz;
		return (*this);
	}

	_FPOINT3& operator*= (float scalar)
	{
		fx *= scalar; fy *= scalar; fz *= scalar;
		return (*this);
	}

	_FPOINT3& operator/= (float scalar)
	{
		ASSERT(scalar != 0.0f);
		fx /= scalar; fy /= scalar; fz /= scalar;
		return (*this);
	}


	bool operator == (const _FPOINT3& fp) const
	{
		return (fp.fx == fx && fp.fy == fy && fp.fz == fz);
	}

	bool operator != (const _FPOINT3& fp) const
	{
		return (fp.fx != fx || fp.fy != fy || fp.fz != fz);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// public data members
public:
	float fx, fy, fz;
};

class AFX_EXT_CLASS CVolumeData
{
public:
	CVolumeData();
	~CVolumeData();

	int m_anDim[3];
	float m_afRes[3];
	float m_afPatOrientHor[3];
	float m_afPatOrientVer[3];
	float m_afPatOrg[3];
	float** m_ppwDen;	
};

class AFX_EXT_CLASS CCTData: public CVolumeData
{
public:
	CCTData();
	~CCTData();

	int m_nWndCenter;
	int m_nWndWidth;
	float m_fRescaleIntercept;
	float m_fRescaleSlope;
	float** m_ppfHU;
	_FPOINT3* m_ptsPatOrg;
	CStringArray m_arrSOPInstanceUID;
};

class AFX_EXT_CLASS CDoseData: public CVolumeData
{
public:

	CDoseData();
	~CDoseData();
	float m_fDoseGridScaling;
};

struct CONTOURDATA
{
public:
	CString strReferencedSOPInstanceUID;
	std::vector<_FPOINT3> m_arrayPts;
};

class AFX_EXT_CLASS CROIITEM
{
public:
	CROIITEM();
	~CROIITEM();

	UINT m_nNumber;
	CString m_strName;
	RGBQUAD m_rgbColor;
	CString m_strObservationLabel;
	CString m_strInterpretedType;
	CString m_strGeometricType;;
	std::vector<CONTOURDATA*> m_vecContours;
};

class AFX_EXT_CLASS StructData
{
public:
	StructData();
	~StructData();

	bool Load(LPCTSTR lpszFilePath);

	CROIITEM* LookupAt(UINT nROINumber);

	std::vector<CROIITEM*> m_vecROIItems;
};


struct CalculationArea
{
public:
	float m_fCenterPositions[3];
	float m_dX, m_dY, m_dZ;
};
