#pragma once
#include <wtypes.h>
#include <afxwin.h>

struct ColorButton 
{
	COLORREF normalColor;
	COLORREF selectedColor;
	COLORREF hoverColor;

	void setColor(COLORREF normalColor, COLORREF selectedColor, COLORREF hoverColor)
	{
		this->normalColor = normalColor;
		this->selectedColor = selectedColor;
		this->hoverColor = hoverColor;
	}
};


class CStyleSheet
{
public:
	// Button Color
	static ColorButton m_colorSignInButton;
	static ColorButton m_colorMainViewButton;
	static ColorButton m_colorMenuViewButton;
	static ColorButton m_colorTaskViewButton;

	// Button Font
	static CFont m_cfontMainViewButton;
	static CFont m_cfontMenuViewButton;
	static CFont m_cfontTaskViewButton;

	// Text Font
	static CFont m_cfontTaskViewTitleLabel;
	static CFont m_cfontTaskViewLabel;
	static CFont m_cfontSurgeryViewLable;

	// Color
	static COLORREF m_rgbTaskViewTitle;
	static COLORREF m_rgbMainViewSubject;
	static COLORREF m_rgbMainViewBottom;
	static COLORREF m_rgbMainViewUserName;

protected:	
	static void createCFont(CFont *pCFont, int nHeight, int nWeight, BYTE bItalic, LPCTSTR lpszFacename);
	static bool initialize();

	static bool m_bInitialized;
};


