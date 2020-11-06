// Utility.cpp : implementation file
//

#include "stdafx.h"

#define new DEBUG_NEW

//____________________________________________________________________________
//
COLORREF AverageColor(COLORREF Color1, COLORREF Color2, int TotalSteps, int Step)
{
	BYTE Red1 = GetRValue(Color1);
	BYTE Green1 = GetGValue(Color1);
	BYTE Blue1 = GetBValue(Color1);

	BYTE Red2 = GetRValue(Color2);
	BYTE Green2 = GetGValue(Color2);
	BYTE Blue2 = GetBValue(Color2);

	BYTE Red = 0;
	BYTE Green = 0;
	BYTE Blue = 0;
	
	double temp = (double)Step / (double)TotalSteps;

	if (Red1 || Red2)
		if (Red1 > Red2)
			Red = BYTE(Red1 - (Red1 - Red2 + 1) * temp);
		else
			Red = BYTE((Red2 - Red1 + 1) * temp + Red1);

	if (Green1 || Green2)
		if (Green1 > Green2)
			Green = BYTE(Green1 - (Green1 - Green2 + 1) * temp);
		else
			Green = BYTE((Green2 - Green1 + 1) * temp + Green1);

	if (Blue1 || Blue2)
		if (Blue1 > Blue2)
			Blue = BYTE(Blue1 - (Blue1 - Blue2 + 1) * temp);
		else
			Blue = BYTE((Blue2 - Blue1 + 1) * temp + Blue1);
	return RGB(Red, Green, Blue);
}

//____________________________________________________________________________
//
#pragma push_macro("new")
#ifdef new
#undef new
#endif

Gdiplus::Bitmap* StretchBitmap(Gdiplus::Bitmap* pBitmap, const Gdiplus::Size& targetSize)
{
	Gdiplus::Bitmap* pStretched = new Gdiplus::Bitmap((INT)targetSize.Width, (INT)targetSize.Height, PixelFormat32bppARGB); 
	if (!pStretched)
		return NULL;

	std::auto_ptr<Gdiplus::Graphics> apGraphics(Gdiplus::Graphics::FromImage(pStretched)); 
	if (!apGraphics.get())
	{
		delete pStretched;
		return NULL;
	}

	if (apGraphics->SetInterpolationMode(Gdiplus::InterpolationModeHighQuality) != Gdiplus::Ok)
	{
		delete pStretched;
		return NULL;
	}

	if (apGraphics->DrawImage(pBitmap, 0, 0, targetSize.Width, targetSize.Height) != Gdiplus::Ok)
	{
		delete pStretched;
		return NULL;
	}

	return pStretched;
}

//____________________________________________________________________________
//
Gdiplus::Bitmap* CreateTextShadow(LPCTSTR strText, const Gdiplus::SizeF& boundingRect, 
								  const Gdiplus::Font& font,
								  BYTE nAlpha, Gdiplus::REAL fDistance)
{
	if (fDistance < 0.1f)
		fDistance = 0.1f;
	else
		if (fDistance > 1.0)
			fDistance = 1.0;

	std::auto_ptr<Gdiplus::Bitmap> apBitmap(new Gdiplus::Bitmap(int(boundingRect.Width * fDistance), int(boundingRect.Height * fDistance)));
	ATLASSERT(apBitmap.get());
	if (!apBitmap.get())
		return NULL;

	// Get a graphics object for it

	std::auto_ptr<Gdiplus::Graphics> apGraphics(Gdiplus::Graphics::FromImage(apBitmap.get()));

	// must use an antialiased rendering hint

	apGraphics->SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

	// this matrix zooms the text out

	Gdiplus::Matrix mx(fDistance, 0, 0, fDistance, 0, 0);

	apGraphics->SetTransform(&mx);

	// The shadow is drawn

	Gdiplus::SolidBrush brush(Gdiplus::Color(nAlpha, 0, 0, 0)); // black

	apGraphics->DrawString(strText, -1, &font, Gdiplus::PointF(0, 0), &brush);

	// The small image is blown up to fill the main client rectangle

	return StretchBitmap(apBitmap.get(), Gdiplus::Size(int(boundingRect.Width), int(boundingRect.Height)));
}

//____________________________________________________________________________
//
//#define DRAW_DEBUG_RECT

Gdiplus::Point DrawMagicTextImpl(Gdiplus::Graphics& graphics,
					   const CMTGSymbol& symbols,
					   LPCTSTR str,
					   const Gdiplus::Point& point,
					   const Gdiplus::ImageAttributes* pImageAttributes = NULL)
{
	int nLength = _tcslen(str);
	Gdiplus::Point current(point);
	for (int i = 0; i < nLength; ++i)
	{
		if (!symbols.DrawSymbol(str[i], graphics, current, pImageAttributes))
			LOG_WARNING(_T("Failed to draw symbol %c"), str[i]);

		current.X += symbols.Size().Width;
	}

	return current;
}

void DrawMagicText(Gdiplus::Graphics& graphics,
				   COLORREF textColor,
				   const Gdiplus::Font& textFont,
				   const CMTGSymbol& symbols,
				   LPCTSTR str,
				   Gdiplus::RectF& rect,	// No line wrap when width is 0
				   const Gdiplus::SizeF& margin,
				   BOOL bCalcRect,	
				   const Gdiplus::ImageAttributes* pImageAttributes,
				   BOOL bAntialias,
				   Gdiplus::StringAlignment horizontalAlign,
				   Gdiplus::StringAlignment verticalAlign)
{
	static std::map<const Gdiplus::Font*, std::vector<Gdiplus::REAL>> placeHolderInfo;
	static CString placeHolder(_T(" "));
	static const int nCalcPlaceHolderMaxLen = 20;

	const std::vector<Gdiplus::REAL>* pPlaceHolderWidths = NULL;

	std::map<const Gdiplus::Font*, std::vector<Gdiplus::REAL>>::const_iterator i = placeHolderInfo.find(&textFont);
	if (i != placeHolderInfo.end())
		pPlaceHolderWidths = &(i->second);
	else
	{
		CString strTemp;
		for (int j = 0; j < nCalcPlaceHolderMaxLen; ++j)
			strTemp += placeHolder;

		std::vector<Gdiplus::REAL> widths;

		Gdiplus::StringFormat strFormat(
			Gdiplus::StringFormatFlagsMeasureTrailingSpaces | 
			Gdiplus::StringFormatFlagsNoWrap);

		for (int j = 1; j <= nCalcPlaceHolderMaxLen; ++j)
		{
			Gdiplus::RectF boundingBox;
			graphics.MeasureString(strTemp, j, &textFont, Gdiplus::PointF(rect.X, rect.Y), &strFormat, &boundingBox);

			//ATLTRACE(_T("Font %x; space len=%d; measured len=%.2f; per placeholder len=%.2f\n"), &textFont, i, boundingBox.Width,
			//	boundingBox.Width / i);

			widths.push_back(boundingBox.Width);
		}

		placeHolderInfo[&textFont] = widths;
		i = placeHolderInfo.find(&textFont);
		if (i == placeHolderInfo.end())
		{
			ATLASSERT(false);
			return;
		}
		
		pPlaceHolderWidths = &(i->second);
	}

	ATLASSERT(pPlaceHolderWidths);
	if (!pPlaceHolderWidths)
	{
		return;
	}

	CString strText(str);
	strText.Trim();

	// Extract all MTG symbols

	const Gdiplus::Size& symbolSize(symbols.Size());

	std::vector<std::pair<CString, Gdiplus::CharacterRange>> symbolData;

	int nIndex;
	while ((nIndex = strText.Find(_T('{'))) != -1)
	{
		int nIndex2 = strText.Find(_T('}'), nIndex);
		if (nIndex2 == -1)
		{
			ATLASSERT(false);
			nIndex2 = strText.GetLength() - 1;
		}

		// Count how many space character to stuff into the location

		std::pair<CString, Gdiplus::CharacterRange> data;
		data.first = strText.Mid(nIndex + 1, nIndex2 - nIndex - 1);

		if (data.first.IsEmpty())
		{
			CString strTemp(strText.Left(nIndex));
			if (nIndex2 != strText.GetLength() - 1)
				strTemp += strText.Mid(nIndex2 + 1);
			strText = strTemp;
			continue;
		}

		// Change the text

		int nPlaceHolders = 0;
		size_t nPlaceHolderSize = pPlaceHolderWidths->size();
		Gdiplus::REAL fSymbolWidth = data.first.GetLength() * Gdiplus::REAL(symbolSize.Width);
		for (size_t i = 0; i < nPlaceHolderSize; ++i)
			if (pPlaceHolderWidths->at(i) >= fSymbolWidth)
			{
				nPlaceHolders = i + 1;
				break;
			}

		if (!nPlaceHolders)
			nPlaceHolders = (int)ceil(fSymbolWidth / (pPlaceHolderWidths->at(nPlaceHolderSize - 1) / nPlaceHolderSize));

		// Hack
		++nPlaceHolders;
			
		CString strTemp(strText.Left(nIndex));
		for (int i = nIndex; i < nIndex + nPlaceHolders; ++i)
			strTemp += placeHolder;
		
		if (nIndex2 != strText.GetLength() - 1)
			strTemp += strText.Mid(nIndex2 + 1);

		strText = strTemp;

		data.second = Gdiplus::CharacterRange(nIndex, nPlaceHolders);

		symbolData.push_back(data);
	}

	Gdiplus::RectF rect2(rect.X, rect.Y, rect.Width, rect.Height);

	Gdiplus::StringFormat strFormat(
		Gdiplus::StringFormatFlagsLineLimit | // do not allow partial line -> less vertial box
		Gdiplus::StringFormatFlagsMeasureTrailingSpaces);

	strFormat.SetAlignment(horizontalAlign);
	strFormat.SetLineAlignment(verticalAlign);
	strFormat.SetTrimming(Gdiplus::StringTrimmingWord);

	BOOL bLineWrap = (rect2.Width > 0 && rect2.Height > 0);	
	if (bLineWrap)
	{
		rect2.X += margin.Width;
		rect2.Width -= margin.Width * 2;

		if (rect2.Height > 0)
		{
			rect2.Y += margin.Height;
			rect2.Height -= margin.Height * 2;
		}
		else
			rect2.Height = 1000;
	}
	else
	{
		rect2.Height = MAX(Gdiplus::REAL(symbols.Size().Height), textFont.GetHeight(&graphics));
		strFormat.SetFormatFlags(Gdiplus::StringFormatFlagsLineLimit 
			| Gdiplus::StringFormatFlagsMeasureTrailingSpaces 
			| Gdiplus::StringFormatFlagsNoWrap
			);
	}

	size_t nCount = symbolData.size();

	if (!bCalcRect && (nCount == 1 && strText.GetLength() == symbolData[0].second.Length))
	{
		if (horizontalAlign == Gdiplus::StringAlignmentNear &&
			verticalAlign == Gdiplus::StringAlignmentNear)
		{
			DrawMagicTextImpl(graphics, symbols, symbolData[0].first, Gdiplus::Point(int(rect2.X), int(rect2.Y)), pImageAttributes);
			return;
		}
	}

	USES_CONVERSION;
	LPCWSTR strText2(T2CW(strText));

	if (bCalcRect)
	{
		// Calculate rect only

		if (bLineWrap)
			graphics.MeasureString(strText2, -1, &textFont, rect2, &strFormat, &rect);
		else
		{
			graphics.MeasureString(strText2, -1, &textFont, Gdiplus::PointF(rect2.X, rect2.Y), &strFormat, &rect);

			// Hack
			if (nCount == 1 && (_tcslen(str) - 2 == symbolData[0].first.GetLength()))
			{
				rect.Width = Gdiplus::REAL(symbolSize.Width) * symbolData[0].first.GetLength();
			}
		}

		rect.Inflate(margin.Width, margin.Height);
		return;
	}

#ifdef DRAW_DEBUG_RECT
	{
		Gdiplus::RectF borderRect;
		if (bLineWrap)
			graphics.MeasureString(strText2, -1, &textFont, rect2, &strFormat, &borderRect);
		else
			graphics.MeasureString(strText2, -1, &textFont, Gdiplus::PointF(rect2.X, rect2.Y), &strFormat, &borderRect);

		borderRect.Inflate(margin.Width, margin.Height);
		borderRect.Width -= 2;
		borderRect.Height -= 2;

		Gdiplus::Pen pen(Gdiplus::Color(0, 255, 0));
		graphics.DrawRectangle(&pen, borderRect);
	}
#endif

	/*
	std::auto_ptr<Gdiplus::Bitmap> apBitmap;

	if (nCount)
	{
		Gdiplus::RectF boundingBox;

		if (bLineWrap)
			graphics.MeasureString(strText2, -1, &textFont, rect2, &strFormat, &boundingBox);
		else
			graphics.MeasureString(strText2, -1, &textFont, Gdiplus::PointF(rect2.X, rect2.Y), &strFormat, &boundingBox);

		apBitmap.reset(new Gdiplus::Bitmap((INT)boundingBox.Width + 1, (INT)boundingBox.Height + 1, &graphics));

		std::auto_ptr<Gdiplus::Graphics> apGraphicsTemp(Gdiplus::Graphics::FromImage(apBitmap.get()));
		HDC hBmDC = apGraphicsTemp->GetHDC();

		HDC hSrcDC = graphics.GetHDC();

		BitBlt(hBmDC, 0, 0, (int)boundingBox.Width + 1, (int)boundingBox.Height + 1, hSrcDC, (int)boundingBox.X, (int)boundingBox.Y, SRCCOPY); 

		graphics.ReleaseHDC(hSrcDC);
		apGraphicsTemp->ReleaseHDC(hBmDC);
	}
	*/

	int nAlpha = 255;
	if (pImageAttributes)
	{
		// HACK: make this a parameter
		nAlpha = 200;
	}

	Gdiplus::SolidBrush brush(Gdiplus::Color(nAlpha, GetRValue(textColor), GetGValue(textColor), GetBValue(textColor)));

	if (bAntialias)
		graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
	else
		graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintSingleBitPerPixel);

	if (bLineWrap)
		graphics.DrawString(strText2, -1, &textFont, rect2, &strFormat, &brush);
	else
		graphics.DrawString(strText2, -1, &textFont, Gdiplus::PointF(rect2.X, rect2.Y), &strFormat, &brush);
			
	if (!nCount)
		return;

	Gdiplus::Region* charRangeRegions = new Gdiplus::Region[nCount];
	Gdiplus::CharacterRange* charRanges = new Gdiplus::CharacterRange[nCount];

	for (size_t i = 0; i < nCount; ++i)
		charRanges[i] = symbolData[i].second;
	
	Gdiplus::Status status = strFormat.SetMeasurableCharacterRanges(nCount, charRanges);
	if (status != Gdiplus::Ok)
		LOG_WARNING(_T("Error calling SetMeasurableCharacterRanges(%d): %d"),
			nCount, status);

	status = graphics.MeasureCharacterRanges(strText2, -1, &textFont, rect2, &strFormat, nCount, charRangeRegions);
	if (status != Gdiplus::Ok)
		LOG_WARNING(_T("Error calling MeasureCharacterRanges('%s', (%f, %f, %f, %f), %d): %d"),
			strText2, rect2.X, rect2.Y, rect2.Width, rect2.Height, nCount, status);

#ifdef DRAW_DEBUG_RECT
	Gdiplus::SolidBrush brush2(Gdiplus::Color(255, 0, 0)); // black
#endif

	for (size_t i = 0; i < nCount; i++)
	{
		Gdiplus::RectF location;
		status = charRangeRegions[i].GetBounds(&location, &graphics);
		if (status != Gdiplus::Ok)
		{
			LOG_WARNING(_T("Error calling Region.GetBounds: %d"), status);
			continue;
		}

		if (location.Width < 1 || location.Height < 1)
		{
			LOG_WARNING(_T("Invalid symbol location size: %d, %d, %dx%d"),
				location.X, location.Y,
				location.Width, location.Height);
				 
			continue;
		}

		/*
		graphics.DrawImage(apBitmap.get(), 
			(INT)location.X, (INT)location.Y, 
			(INT)(location.X - rect2.X), (INT)(location.Y - rect2.Y),
			(INT)location.Width + 1, (INT)location.Height + 1, Gdiplus::UnitPixel);
		*/

#ifdef DRAW_DEBUG_RECT
		graphics.FillRectangle(&brush2, location);
#endif

		location.X -= ( symbolSize.Width * symbolData[i].first.GetLength() - location.Width ) / 2;
		location.Y -= ( symbolSize.Height - location.Height ) / 2;

		DrawMagicTextImpl(graphics, symbols, symbolData[i].first, Gdiplus::Point(int(location.X), int(location.Y)), 
			pImageAttributes);
	}

	delete[] charRangeRegions;
	delete[] charRanges;
}

void DrawMagicText(HTHEME hTheme,
				   HDC hdc,
				   int iPartId,
				   int iStateId,
				   const CMTGSymbol& symbols,
				   LPCTSTR str,
				   const CRect& rect)
{
	CString strText(str);
	int nLength = strText.GetLength();

	CPoint point(rect.left, rect.top);
	
	CDC* pDC = CDC::FromHandle(hdc);

	BOOL bPartDefined = IsThemePartDefined(hTheme, iPartId, iStateId);
	if (!bPartDefined)
	{
		COLORREF clrText;
		if (iStateId == LISS_SELECTED || iStateId == LISS_HOTSELECTED)
			clrText = GetSysColor(COLOR_HIGHLIGHTTEXT);
		else
			clrText = GetSysColor(COLOR_WINDOWTEXT);

		pDC->SetTextColor(clrText);
	}

	for (int i = 0; i < nLength; )
	{
		if (strText[i] == _T('{'))
		{
			int nIndex = strText.Find(_T('}'), i + 1);
			if (nIndex == -1)
				nIndex = nLength;

			CString strSymbols(strText.Mid(i + 1, nIndex - i - 1));

			Gdiplus::Graphics graphics(hdc);
			point.x = DrawMagicTextImpl(graphics, symbols, strSymbols, Gdiplus::Point(point.x, point.y)).X;

			if (point.x >= rect.right)
				return;

			i = nIndex + 1;
			continue;
		}

		int nIndex = strText.Find(_T('{'), i + 1);
		if (nIndex == -1)
			nIndex = nLength;

		CString strTemp(strText.Mid(i, nIndex - i));
		
		// Measure and draw theme text

		CRect boundingRect(point.x, point.y, rect.right, rect.bottom);
		CRect extentRect;

		GetThemeTextExtent(hTheme, hdc, iPartId, iStateId, strTemp, -1, DT_SINGLELINE | DT_VCENTER | DT_LEFT, &boundingRect, &extentRect);

		if (extentRect.right > rect.right)
			extentRect.right = rect.right;

		if (extentRect.bottom > rect.bottom)
			extentRect.bottom = rect.bottom;

		if (bPartDefined)
			DrawThemeText(hTheme, hdc, iPartId, iStateId, strTemp, -1, DT_SINGLELINE | DT_VCENTER | DT_LEFT, 0, &extentRect);
		else
			pDC->DrawText(strTemp, &extentRect, DT_SINGLELINE | DT_VCENTER | DT_LEFT);

		point.x = extentRect.right;
		if (point.x >= rect.right)
			return;

		i = nIndex;
	}
}

/* Deprecated, old code to deal with MTG symbol font
void DrawMagicTextImpl(Gdiplus::Graphics& graphics,
					   const Gdiplus::Font& symbolFont,
					   LPCTSTR str,
					   const Gdiplus::PointF& point)
{
	LOGFONT logfont;
	symbolFont.GetLogFontW(&graphics, &logfont);

	CFont symbolFont2;
	symbolFont2.CreateFontIndirectW(&logfont);

	CDC dc;
	dc.Attach(graphics.GetHDC());

	CFont* pOldFont = (CFont*)dc.SelectObject(&symbolFont2);
	dc.SetBkMode(TRANSPARENT);

	COLORREF prevTextColor = dc.GetTextColor();
	dc.SetTextColor(COLOR_WHITE_MANA);

	int nCount = _tcslen(str);
	CString strChar = _T(" ");

	int x = (int)point.X;
	const int y = (int)point.Y;

	for (int i = 0; i < nCount; i++)
	{
		if (str[i] == _T('@'))
		{
			++i;

			switch(str[i])
			{
			case _T('b'): dc.SetTextColor(COLOR_BLACK_MANA); break;
			case _T('g'): dc.SetTextColor(COLOR_GREEN_MANA); break;
			case _T('r'): dc.SetTextColor(COLOR_RED_MANA); break;
			case _T('u'): dc.SetTextColor(COLOR_BLUE_MANA); break;
			case _T('w'): dc.SetTextColor(COLOR_WHITE_MANA); break;
			case _T('a'): dc.SetTextColor(COLOR_GENERIC_MANA); break;
			case _T('k'): dc.SetTextColor(RGB(0, 0, 0)); break;
			}

			continue;
		}

		strChar.SetAt(0, str[i]);

		CSize charSize = dc.GetTextExtent(strChar);

		dc.TextOut(x, y, strChar);
		x += charSize.cx;
	}

	if (pOldFont)
		dc.SelectObject(pOldFont);
	dc.SetTextColor(prevTextColor);
	
	graphics.ReleaseHDC(dc.Detach());
}

void DrawMagicText(Gdiplus::Graphics& graphics,
				   COLORREF textColor,
				   const Gdiplus::Font& textFont,
				   const Gdiplus::Font& symbolFont,
				   LPCTSTR str,
				   Gdiplus::RectF& rect,
				   const Gdiplus::SizeF& margin,
				   BOOL bCalcRect,
				   BOOL bTransparent,
				   BOOL bAntialias,
				   Gdiplus::StringAlignment horizontalAlign,
				   Gdiplus::StringAlignment verticalAlign)
{
	CString strText(str);
	strText.Replace(_T("}{"), _T(""));
	strText.Trim();

	// Extract all MTG symbols

	std::vector<std::pair<CString, Gdiplus::CharacterRange>> symbolData;

	int nIndex;
	while ((nIndex = strText.Find(_T('{'))) != -1)
	{
		int nIndex2 = strText.Find(_T('}'), nIndex);
		if (nIndex2 == -1)
		{
			ATLASSERT(false);
			nIndex2 = strText.GetLength() - 1;
		}

		// Count how many space character to stuff into the location

		CString strTemp(strText.Mid(nIndex + 1, nIndex2 - nIndex - 1));

		std::pair<CString, Gdiplus::CharacterRange> data;
		data.first = strTemp;
		
		// First remove all color codes

		strTemp.Replace(_T("@b"), _T(""));
		strTemp.Replace(_T("@g"), _T(""));
		strTemp.Replace(_T("@r"), _T(""));
		strTemp.Replace(_T("@u"), _T(""));
		strTemp.Replace(_T("@w"), _T(""));
		strTemp.Replace(_T("@a"), _T(""));
		strTemp.Replace(_T("@k"), _T(""));

		// Remove '0' and '[]'

		strTemp.Replace(_T("o"), _T(""));
		strTemp.Replace(_T("c"), _T(""));

		// Change the text

		static const WCHAR placeHolder[] = _T("qt");
		int nPlaceHolders = strTemp.GetLength();	// approximation

		strTemp = strText.Left(nIndex);
		for (int i = nIndex; i < nIndex + nPlaceHolders; ++i)
			strTemp += placeHolder;
		
		if (nIndex2 != strText.GetLength() - 1)
			strTemp += strText.Mid(nIndex2 + 1);

		strText = strTemp;

		data.second = Gdiplus::CharacterRange(nIndex, nPlaceHolders * 2);

		symbolData.push_back(data);
	}

	Gdiplus::RectF rect2(rect.X, rect.Y, rect.Width, rect.Height);

	BOOL bLineWrap = (rect2.Width > 0);	
	if (bLineWrap)
	{
		rect2.X += margin.Width;
		rect2.Width -= margin.Width * 2;

		if (rect2.Height > 0)
		{
			rect2.Y += margin.Height;
			rect2.Height -= margin.Height * 2;
		}
		else
			rect2.Height = 1000;
	}

	size_t nCount = symbolData.size();

	if (!bCalcRect && (nCount == 1 && strText.GetLength() == symbolData[0].second.Length))
	{
		if (horizontalAlign == Gdiplus::StringAlignmentNear &&
			verticalAlign == Gdiplus::StringAlignmentNear)
		{
			DrawMagicTextImpl(graphics, symbolFont, symbolData[0].first, Gdiplus::PointF(rect2.X, rect2.Y));
			return;
		}
	}

	USES_CONVERSION;
	LPCWSTR strText2(T2CW(strText));

	Gdiplus::StringFormat strFormat(Gdiplus::StringFormatFlagsLineLimit
		// | Gdiplus::StringFormatFlagsMeasureTrailingSpaces
	);
	strFormat.SetTrimming(Gdiplus::StringTrimmingWord);
	strFormat.SetAlignment(horizontalAlign);
	strFormat.SetLineAlignment(verticalAlign);

	if (bCalcRect)
	{
		// Calculate rect only

		if (bLineWrap)
			graphics.MeasureString(strText2, -1, &textFont, rect2, &strFormat, &rect);
		else
			graphics.MeasureString(strText2, -1, &textFont, Gdiplus::PointF(rect2.X, rect2.Y), &strFormat, &rect);

		rect.Inflate(margin.Width, margin.Height);
		return;
	}

#ifdef DRAW_DEBUG_RECT
	{
		Gdiplus::RectF borderRect;
		if (bLineWrap)
			graphics.MeasureString(strText2, -1, &textFont, rect2, &strFormat, &borderRect);
		else
			graphics.MeasureString(strText2, -1, &textFont, Gdiplus::PointF(rect2.X, rect2.Y), &strFormat, &borderRect);

		borderRect.Inflate(margin.Width, margin.Height);
		borderRect.Width -= 2;
		borderRect.Height -= 2;

		Gdiplus::Pen pen(Gdiplus::Color(255, 0, 0));
		graphics.DrawRectangle(&pen, borderRect);
	}
#endif

	Gdiplus::RectF boundingBox;
	std::auto_ptr<Gdiplus::Bitmap> apBitmap;

	if (nCount)
	{
		if (bLineWrap)
			graphics.MeasureString(strText2, -1, &textFont, rect2, &strFormat, &boundingBox);
		else
			graphics.MeasureString(strText2, -1, &textFont, Gdiplus::PointF(rect2.X, rect2.Y), &strFormat, &boundingBox);

		apBitmap.reset(new Gdiplus::Bitmap((INT)boundingBox.Width + 1, (INT)boundingBox.Height + 1, &graphics));

		std::auto_ptr<Gdiplus::Graphics> apGraphicsTemp(Gdiplus::Graphics::FromImage(apBitmap.get()));
		HDC hBmDC = apGraphicsTemp->GetHDC();

		HDC hSrcDC = graphics.GetHDC();

		BitBlt(hBmDC, 0, 0, (int)boundingBox.Width + 1, (int)boundingBox.Height + 1, hSrcDC, (int)boundingBox.X, (int)boundingBox.Y, SRCCOPY); 

		graphics.ReleaseHDC(hSrcDC);
		apGraphicsTemp->ReleaseHDC(hBmDC);
	}

	int nAlpha = (bTransparent ? 200 : 255);
	Gdiplus::SolidBrush brush(Gdiplus::Color(nAlpha, GetRValue(textColor), GetGValue(textColor), GetBValue(textColor)));

	if (bAntialias)
		graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

	if (bLineWrap)
		graphics.DrawString(strText2, -1, &textFont, rect2, &strFormat, &brush);
	else
		graphics.DrawString(strText2, -1, &textFont, Gdiplus::PointF(rect2.X, rect2.Y), &strFormat, &brush);
			
	if (!nCount)
		return;

	Gdiplus::Region* charRangeRegions = new Gdiplus::Region[nCount];
	Gdiplus::CharacterRange* charRanges = new Gdiplus::CharacterRange[nCount];

	for (size_t i = 0; i < nCount; ++i)
		charRanges[i] = symbolData[i].second;
	
	strFormat.SetMeasurableCharacterRanges(nCount, charRanges);

	graphics.MeasureCharacterRanges(strText2, -1, &textFont, boundingBox, &strFormat, nCount, charRangeRegions);

	for (size_t i = 0; i < nCount; i++)
	{
		Gdiplus::RectF location;
		charRangeRegions[i].GetBounds(&location, &graphics);

		if (location.Width < 1 || location.Height < 1)
			continue;

		graphics.DrawImage(apBitmap.get(), 
			(INT)location.X, (INT)location.Y, 
			(INT)(location.X - boundingBox.X), (INT)(location.Y - boundingBox.Y),
			(INT)location.Width + 1, (INT)location.Height + 1, Gdiplus::UnitPixel);

		DrawMagicTextImpl(graphics, symbolFont, symbolData[i].first, Gdiplus::PointF(location.X, location.Y));
	}

	delete[] charRangeRegions;
	delete[] charRanges;
}
*/

Gdiplus::Font* ResizeFont(const Gdiplus::Font& font, int nSizeDelta)
{
	if (font.GetSize() + nSizeDelta < 1)
		return NULL;

	Gdiplus::FontFamily family;
	if (font.GetFamily(&family) != Gdiplus::Ok)
		return NULL;

	return new Gdiplus::Font(&family, font.GetSize() + nSizeDelta, font.GetStyle(), font.GetUnit());
}

#pragma pop_macro("new")

//____________________________________________________________________________
//
void TransformPoint(DPoint& Point, const DXForm* pXForm)
{
	double x = Point.x;
	double y = Point.y;
	Point.x = pXForm->eM11 * x + pXForm->eM12 * y + pXForm->eDx;
	Point.y = pXForm->eM21 * x + pXForm->eM22 * y + pXForm->eDy;
}

//____________________________________________________________________________
//
void TransformRectangle(const DPoint RPoints[2], DPoint XPoints[4], const DXForm* pXForm)
{
	XPoints[0] = RPoints[0];
	XPoints[1].x = RPoints[1].x;
	XPoints[1].y = RPoints[0].y;
	XPoints[2] = RPoints[1];
	XPoints[3].x = RPoints[0].x;
	XPoints[3].y = RPoints[1].y;

	TransformPoint(XPoints[0], pXForm);
	TransformPoint(XPoints[1], pXForm);
	TransformPoint(XPoints[2], pXForm);
	TransformPoint(XPoints[3], pXForm);
}

//____________________________________________________________________________
//
void ConvertLineToPolygon(CPoint FromPoint, CPoint ToPoint, int nWidth, CPoint Points[4])
{
	if (FromPoint.x == ToPoint.x)	// Vertical
	{
		CRect Rect(FromPoint, ToPoint);
		Rect.NormalizeRect();
		Rect.InflateRect(nWidth / 2, 0);
		Points[0] = Rect.TopLeft();
		Points[1] = CPoint(Rect.right, Rect.top);
		Points[2] = Rect.BottomRight();
		Points[3] = CPoint(Rect.left, Rect.bottom);
		return;
	}

	if (FromPoint.y == ToPoint.y)	// Horizontal
	{
		CRect Rect(FromPoint, ToPoint);
		Rect.NormalizeRect();
		Rect.InflateRect(0, nWidth / 2);
		Points[0] = Rect.TopLeft();
		Points[1] = CPoint(Rect.right, Rect.top);
		Points[2] = Rect.BottomRight();
		Points[3] = CPoint(Rect.left, Rect.bottom);
		return;
	}

	CSize Size = ToPoint - FromPoint;
	double LineLength = sqrt(pow((double)Size.cx, 2.0) + pow((double)Size.cy, 2.0));
	double Inflate = double(nWidth) / 2.0;

	DPoint RPoints[2];
	RPoints[0].x = 0.0;
	RPoints[0].y = -Inflate;
	RPoints[1].x = LineLength;
	RPoints[1].y = Inflate;

	double Angle;
	if ((Size.cx > 0) && (Size.cy > 0))
		Angle = -atan(double(Size.cy) / double(Size.cx));
	else
		if ((Size.cx < 0) && (Size.cy > 0))
			Angle = atan(double(Size.cy) / double(-Size.cx)) + PI;
		else
			if ((Size.cx < 0) && (Size.cy < 0))
				Angle = PI - atan(double(-Size.cy) / double(-Size.cx));
			else
				if ((Size.cx > 0) && (Size.cy < 0))
					Angle = atan(double(-Size.cy) / double(Size.cx));

//	ATLTRACE(_T("Angle = %f (tan-1(%d/%d)) PI = %f\n"), Angle, -Size.cy, Size.cx, PI);

	DXForm xForm;
	xForm.eM11 = cos(Angle); 
	xForm.eM12 = sin(Angle); 
	xForm.eM21 = -xForm.eM12; 
	xForm.eM22 = xForm.eM11; 
	xForm.eDx  = FromPoint.x; 
	xForm.eDy  = FromPoint.y; 

	DPoint XPoints[4];
	TransformRectangle(RPoints, XPoints, &xForm);
	
	for (int i = 0; i < 4; ++i)
	{
		long x = long(XPoints[i].x);
		long y = long(XPoints[i].y);
		Points[i].x = x;
		Points[i].y = y;
	}
}

//____________________________________________________________________________
//
Gdiplus::RectF ToRectF(const RECT& rect)
{
	return Gdiplus::RectF((Gdiplus::REAL)rect.left, (Gdiplus::REAL)rect.top, 
		(Gdiplus::REAL)(rect.right - rect.left + 1),
		(Gdiplus::REAL)(rect.bottom - rect.top + 1));
}

Gdiplus::Rect ToRect(const RECT& rect)
{
	return Gdiplus::Rect(rect.left, rect.top,
		(rect.right - rect.left + 1),
		(rect.bottom - rect.top + 1));
}

Gdiplus::Rect ToRect(const CRect& rect)
{
	return Gdiplus::Rect(rect.left, rect.top, rect.Width(), rect.Height());
}

RECT ToRect(const Gdiplus::RectF& rectF)
{
	RECT rect;
	rect.top = (LONG)ceilf(rectF.Y);
	rect.left = (LONG)ceilf(rectF.X);
	rect.bottom = (LONG)ceilf(rect.top + rectF.Height - 1);
	rect.right = (LONG)ceilf(rect.left + rectF.Width - 1);

	return rect;
}

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
   UINT  num = 0;          // number of image encoders
   UINT  size = 0;         // size of the image encoder array in bytes

   Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;

   Gdiplus::GetImageEncodersSize(&num, &size);
   if(size == 0)
      return -1;  // Failure

   pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
   if(pImageCodecInfo == NULL)
      return -1;  // Failure

   Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);

   for(UINT j = 0; j < num; ++j)
   {
      if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
      {
         *pClsid = pImageCodecInfo[j].Clsid;
         free(pImageCodecInfo);
         return j;  // Success
      }    
   }

   free(pImageCodecInfo);
   return -1;  // Failure
}

//____________________________________________________________________________
//
int ExtractMin(std::vector<int>& Q, std::vector<int>& d)
{
	int nMin = 0;
	for (UINT i = 1; i < Q.size(); ++i)
		if (d[Q[i]] < d[Q[nMin]])
			nMin = i;

	int u = Q[nMin];
	Q.erase(Q.begin() + nMin);
	return u;
}

//____________________________________________________________________________
//
void Relax(int u, int v, int w, std::vector<int>& d, std::vector<int>& p)
{
	if (d[u] == INT_MAX)
		return;

	if (d[v] > d[u] + w)
	{
		d[v] = d[u] + w;
		p[v] = u;
	}
}

//____________________________________________________________________________
//
void Dijkstra(AdjacencyList& AdjList,
			  int s,
			  std::vector<int>& p,
			  std::vector<int>& d)
{
	p.clear();
	d.clear();

	std::vector<int> Q;
	for (UINT i = 0; i < AdjList.size(); ++i)
	{
		if (i == s)
			d.push_back(0);
		else
			d.push_back(INT_MAX);
		p.push_back(-1);
		Q.push_back(i);
	}

	while (Q.size())
	{
		UINT u = ExtractMin(Q, d);
		AdjacencyEntry& Entry = AdjList.at(u);
		for (AdjacencyEntry::iterator i = Entry.begin(); i != Entry.end(); ++i)
		{
			int v = (*i).first;
			int w = (*i).second;
			Relax(u, v, w, d, p);
		}
	}

//	for (int i = 0; i < p.size(); ++i)
//		_tprintf(_T("%d:[d:%d, p:%d] "), i, d[i], p[i]);
//	_tprintf(_T("\n"));
}

//____________________________________________________________________________
//
double Distance(CPoint Point1, CPoint Point2)
{
	CSize Difference(Point1 - Point2);
	return sqrt(pow((double)Difference.cx, 2.0) + pow((double)Difference.cy, 2.0));
}

//____________________________________________________________________________
//
double CrossProduct(CPoint p1, CPoint p2)
{
	return p1.x * p2.y - p2.x * p1.y;
}

//____________________________________________________________________________
//
BOOL IntersectImpl(CPoint p1, CPoint p2, CPoint p3, CPoint p4)
{
	double CrossProduct1 = CrossProduct(p3 - p1, p2 - p1);
	double CrossProduct2 = CrossProduct(p4 - p1, p2 - p1);

	if (((CrossProduct1 < 0.0) && (CrossProduct2 > 0.0)) ||
		((CrossProduct1 > 0.0) && (CrossProduct2 < 0.0)))
		return TRUE;

	if (fabs(CrossProduct1) < EPSILON) 
	{
		if (fabs(CrossProduct2) < EPSILON)
			return TRUE;
		else
			if ((p4.x - p3.x == 0) && (p4.y - p3.y == 0))
				return TRUE;
	}
	else
		if ((fabs(CrossProduct2) < EPSILON) &&
			(p2.x - p1.x == 0) && (p2.y - p1.y == 0))
			return TRUE;

	return FALSE;
}

//____________________________________________________________________________
//
BOOL Intersect(CPoint p1, CPoint p2, CPoint p3, CPoint p4)
{
	CRect BoundingBox1(p1, p2);
	CRect BoundingBox2(p3, p4);

	BoundingBox1.NormalizeRect();
	BoundingBox2.NormalizeRect();

	if ((BoundingBox1.top >= BoundingBox2.bottom) ||
		(BoundingBox2.top >= BoundingBox1.bottom) ||
		(BoundingBox1.left >= BoundingBox2.right) ||
		(BoundingBox2.left >= BoundingBox1.right))
		return FALSE;

	return IntersectImpl(p1, p2, p3, p4) && IntersectImpl(p3, p4, p1, p2);
}

//____________________________________________________________________________
//
BOOL IsHorizontal(CPoint p1, CPoint p2)
{
	return p1.y == p2.y;
}

//____________________________________________________________________________
//
BOOL IsVertical(CPoint p1, CPoint p2)
{
	return p1.x == p2.x;
}

//____________________________________________________________________________
//
#define HINT_DIR_UP		(0x01)
#define HINT_DIR_DOWN	(0x02)
#define HINT_DIR_LEFT	(0x04)
#define HINT_DIR_RIGHT	(0x08)
#define HINT_DIR_ALL	(HINT_DIR_UP | HINT_DIR_DOWN | HINT_DIR_LEFT | HINT_DIR_RIGHT)

#define ADJ_CHECK(u, v) if ((i == u) && ViewRect.PtInRect(P[v]) && !ClipRect.PtInRect(P[v])) bFound = TRUE; else
#define ADD_ADJ_ENTRY(u, v, dir) if ((p[u] != -1) && (p[v] != -1)) AdjList[p[u]].push_back(std::make_pair(p[v], int(Distance(P[u], P[v]) * ((dir & dwHintDir) ? 1.0 : 1.05))))
#define ADD_ADJ_ENTRY2(u, v, src) if ((src == bSrc) && (p[u] != -1) && (p[v] != -1)) AdjList[p[u]].push_back(std::make_pair(p[v], int(Distance(P[u], P[v]))))

void GenerateGraph(CRect Rect,
				   CSize BufferSize,
				   CRect ClipRect,
				   CSize ViewSize,
				   std::vector<int>& InnerPoints, std::vector<int>& OuterPoints,
				   std::vector<CPoint>& Points, AdjacencyList& AdjList,
				   BOOL bSrc,
				   DWORD dwHintDir = HINT_DIR_ALL)
{
	CRect ViewRect(CPoint(0, 0), ViewSize);

	CPoint P[12];
	P[0] = CPoint(Rect.CenterPoint().x, Rect.top);
	P[1] = CPoint(Rect.right, Rect.CenterPoint().y);
	P[2] = CPoint(Rect.CenterPoint().x, Rect.bottom);
	P[3] = CPoint(Rect.left, Rect.CenterPoint().y);

	Rect.InflateRect(BufferSize);

	P[4] = CPoint(Rect.CenterPoint().x, Rect.top);
	P[5] = CPoint(Rect.right, Rect.top);
	P[6] = CPoint(Rect.right, Rect.CenterPoint().y);
	P[7] = CPoint(Rect.BottomRight());
	P[8] = CPoint(Rect.CenterPoint().x, Rect.bottom);
	P[9] = CPoint(Rect.left, Rect.bottom);
	P[10] = CPoint(Rect.left, Rect.CenterPoint().y);
	P[11] = CPoint(Rect.TopLeft());

//	ATLTRACE(_T("P[11] = %d, %d, ViewRect= %d, %d, %d, %d\n"), P[11].x, P[11].y, ViewRect.top, ViewRect.left, ViewRect.bottom, ViewRect.right);

	int p[12] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
	for (int i = 0; i < 12; ++i)
		if (!ClipRect.PtInRect(P[i]))
		{
			BOOL bFound = FALSE;

			ADJ_CHECK(0, 4)
			ADJ_CHECK(1, 6)
			ADJ_CHECK(2, 8)
			ADJ_CHECK(3, 10)

			ADJ_CHECK(4, 0)
			ADJ_CHECK(6, 1)
			ADJ_CHECK(8, 2)
			ADJ_CHECK(10, 3)
			
			ADJ_CHECK(4, 5) ADJ_CHECK(4, 11)
			ADJ_CHECK(5, 4) ADJ_CHECK(5, 6)
			ADJ_CHECK(6, 5) ADJ_CHECK(6, 7)
			ADJ_CHECK(7, 6) ADJ_CHECK(7, 8)
			ADJ_CHECK(8, 7) ADJ_CHECK(8, 9)
			ADJ_CHECK(9, 8) ADJ_CHECK(9, 10)
			ADJ_CHECK(10, 9) ADJ_CHECK(10, 11)
			ADJ_CHECK(11, 4) ADJ_CHECK(11, 10) continue;
			if (!bFound)
				continue;

			if (i <= 3)
				InnerPoints.push_back(Points.size());
			else
				OuterPoints.push_back(Points.size());
			p[i] = Points.size();
			Points.push_back(P[i]);
		}

	AdjList.resize(Points.size());			

	/*		   4
	  11+------+------+5
		|             |
		|      0      |
		|   +--+--+   |
		|   |     |   |
	  10+  3+     +1  +6
		|   |     |   |
		|   +--+--+   |
		|      2      |
		|             |
	   9+------+------+7
			   8
	*/

	ADD_ADJ_ENTRY2(0, 4, TRUE);
	ADD_ADJ_ENTRY2(1, 6, TRUE);
	ADD_ADJ_ENTRY2(2, 8, TRUE);
	ADD_ADJ_ENTRY2(3, 10, TRUE);

	ADD_ADJ_ENTRY2(4, 0, FALSE);
	ADD_ADJ_ENTRY2(6, 1, FALSE);
	ADD_ADJ_ENTRY2(8, 2, FALSE);
	ADD_ADJ_ENTRY2(10, 3, FALSE);

	ADD_ADJ_ENTRY(4, 5, HINT_DIR_RIGHT);
	ADD_ADJ_ENTRY(4, 11, HINT_DIR_LEFT);
	ADD_ADJ_ENTRY(5, 4, HINT_DIR_LEFT);
	ADD_ADJ_ENTRY(5, 6, HINT_DIR_DOWN);
	ADD_ADJ_ENTRY(6, 5, HINT_DIR_UP);
	ADD_ADJ_ENTRY(6, 7, HINT_DIR_DOWN);
	ADD_ADJ_ENTRY(7, 6, HINT_DIR_UP);
	ADD_ADJ_ENTRY(7, 8, HINT_DIR_LEFT);
	ADD_ADJ_ENTRY(8, 7, HINT_DIR_RIGHT);
	ADD_ADJ_ENTRY(8, 9, HINT_DIR_LEFT);
	ADD_ADJ_ENTRY(9, 8, HINT_DIR_RIGHT);
	ADD_ADJ_ENTRY(9, 10, HINT_DIR_UP);
	ADD_ADJ_ENTRY(10, 9, HINT_DIR_DOWN);
	ADD_ADJ_ENTRY(10, 11, HINT_DIR_UP);
	ADD_ADJ_ENTRY(11, 4, HINT_DIR_RIGHT);
	ADD_ADJ_ENTRY(11, 10, HINT_DIR_DOWN);
}

//____________________________________________________________________________
//
BOOL ManhattanIntersectionPoint(CPoint p1, CPoint p2, CPoint p3, CPoint p4, CPoint& Intersect)
{
	BOOL bHorizontal1 = IsHorizontal(p1, p2);
	if (!bHorizontal1 && !IsVertical(p1, p2))
		return FALSE;

	BOOL bHorizontal2 = IsHorizontal(p3, p4);
	if (!bHorizontal2 && !IsVertical(p3, p4))
		return FALSE;

	if (bHorizontal1 == bHorizontal2)
		return FALSE;

	if (bHorizontal1)
	{
		Intersect.x = p3.x;
		Intersect.y = p1.y;
	}
	else
	{
		Intersect.x = p1.x;
		Intersect.y = p3.y;
	}

	return TRUE;
}

//____________________________________________________________________________
//
int FindShortestPath(CRect Rect1, CSize BufferSize1,
					 CRect Rect2, CSize BufferSize2,
					 CSize ViewSize,
					 std::vector<CPoint>& Waypoints,
					 BOOL bPerpendicularLinesOnly)
{
	std::vector<CPoint> Points;
	AdjacencyList AdjList;
	std::vector<int> SrcPoints;
	std::vector<int> OuterPoints1;
	std::vector<int> DesPoints;
	std::vector<int> OuterPoints2;

	DWORD dwHint = 0;
	CPoint MidPoint1 = Rect1.CenterPoint();
	CPoint MidPoint2 = Rect2.CenterPoint();

	if (MidPoint1.x < MidPoint2.x)
		dwHint |= HINT_DIR_RIGHT;
	else
		dwHint |= HINT_DIR_LEFT;

	if (MidPoint1.y < MidPoint2.y)
		dwHint |= HINT_DIR_DOWN;
	else
		dwHint |= HINT_DIR_UP;

	CRect ClippingRect1(Rect2);
	CRect ClippingRect2(Rect1);

	GenerateGraph(Rect1, BufferSize1, ClippingRect1, ViewSize, SrcPoints, OuterPoints1, Points, AdjList, TRUE, dwHint);
	GenerateGraph(Rect2, BufferSize2, ClippingRect2, ViewSize, DesPoints, OuterPoints2, Points, AdjList, FALSE, dwHint);

	std::vector<std::pair<CPoint, CPoint> > Lines;
	Lines.push_back(std::make_pair(Rect1.TopLeft(), CPoint(Rect1.right, Rect1.top)));
	Lines.push_back(std::make_pair(CPoint(Rect1.right, Rect1.top), Rect1.BottomRight()));
	Lines.push_back(std::make_pair(CPoint(Rect1.left, Rect1.bottom), Rect1.BottomRight()));
	Lines.push_back(std::make_pair(Rect1.TopLeft(), CPoint(Rect1.left, Rect1.bottom)));
	Lines.push_back(std::make_pair(Rect2.TopLeft(), CPoint(Rect2.right, Rect2.top)));
	Lines.push_back(std::make_pair(CPoint(Rect2.right, Rect2.top), Rect2.BottomRight()));
	Lines.push_back(std::make_pair(CPoint(Rect2.left, Rect2.bottom), Rect2.BottomRight()));
	Lines.push_back(std::make_pair(Rect2.TopLeft(), CPoint(Rect2.left, Rect2.bottom)));
		
#ifdef _MY_DEBUG
	int nOuterLinkCount = 0;
#endif

	for (UINT i = 0; i < OuterPoints1.size(); ++i)
		for (UINT j = 0; j < OuterPoints2.size(); ++j)
		{
			BOOL bFound = FALSE;
			for (UINT k = 0; k < Lines.size(); ++k)
				if (Intersect(Points[OuterPoints1[i]], Points[OuterPoints2[j]], Lines[k].first, Lines[k].second))
				{
					bFound = TRUE;
					break;
				}

			if (!bFound)
				AdjList[OuterPoints1[i]].push_back(std::make_pair(OuterPoints2[j], int(Distance(Points[OuterPoints1[i]], Points[OuterPoints2[j]]))));
		}

#ifdef _MY_DEBUG
	/*
	ATLTRACE(_T("%d vectices\n"), Points.size());
	int nEdgeCount = 0;
	for (int i = 0; i < AdjList.size(); ++i)
	{
		for (int j = 0; j < AdjList[i].size(); ++j)
		{
			ATLTRACE(_T("pDC->MoveTo(%d, %d); pDC->LineTo(%d, %d)\n"),
				Points[i].x, Points[i].y,
				Points[AdjList[i][j].first].x, Points[AdjList[i][j].first].y);
		}

		nEdgeCount += AdjList[i].size();
	}
	ATLTRACE(_T("%d directed edges\n"), nEdgeCount);
	*/
#endif

	int nMinCost = INT_MAX;
	Waypoints.clear();

	for (UINT i = 0; i < SrcPoints.size(); ++i)
	{
		std::vector<int> p;
		std::vector<int> d;

		Dijkstra(AdjList, SrcPoints[i], p, d);

		for (UINT j = 0; j < DesPoints.size(); ++j)
			if (d[DesPoints[j]] < nMinCost)
			{
				Waypoints.clear();

				int nLast = DesPoints[j];
				Waypoints.push_back(Points[nLast]);
				while (p[nLast] != -1)
				{
					Waypoints.push_back(Points[p[nLast]]);
					nLast = p[nLast];
				}
				nMinCost = d[DesPoints[j]];
			}
	}

	// Fix unnecessary loops, i.e. opposite directions in 3 consecutive points
	for (UINT i = 2; i < Waypoints.size(); ++i)
	{
		DWORD dwDir1 = 0;
		if (Waypoints[i - 2].y > Waypoints[i - 1].y)
			dwDir1 |= HINT_DIR_UP;
		else
			if (Waypoints[i - 2].y < Waypoints[i - 1].y)
				dwDir1 |= HINT_DIR_DOWN;

		if (Waypoints[i - 2].x > Waypoints[i - 1].x)
			dwDir1 |= HINT_DIR_LEFT;
		else
			if (Waypoints[i - 2].x < Waypoints[i - 1].x)
				dwDir1 |= HINT_DIR_RIGHT;

		DWORD dwDir2 = 0;
		if (Waypoints[i - 1].y > Waypoints[i].y)
			dwDir2 |= HINT_DIR_UP;
		else
			if (Waypoints[i - 1].y < Waypoints[i].y)
				dwDir2 |= HINT_DIR_DOWN;

		if (Waypoints[i - 1].x > Waypoints[i].x)
			dwDir2 |= HINT_DIR_LEFT;
		else
			if (Waypoints[i - 1].x < Waypoints[i].x)
				dwDir2 |= HINT_DIR_RIGHT;		

		if ((dwDir1 & HINT_DIR_LEFT) && (dwDir2 & HINT_DIR_RIGHT))
			Waypoints[i - 1].x = MIN(Waypoints[i].x, Waypoints[i - 2].x);
		else
			if ((dwDir2 & HINT_DIR_LEFT) && (dwDir1 & HINT_DIR_RIGHT))
				Waypoints[i - 1].x = MAX(Waypoints[i].x, Waypoints[i - 2].x);

		if ((dwDir1 & HINT_DIR_UP) && (dwDir2 & HINT_DIR_DOWN))
			Waypoints[i - 1].y = MIN(Waypoints[i].y, Waypoints[i - 2].y);
		else
			if ((dwDir2 & HINT_DIR_UP) && (dwDir1 & HINT_DIR_DOWN))
				Waypoints[i - 1].y = MAX(Waypoints[i].y, Waypoints[i - 2].y);
	}

	if (bPerpendicularLinesOnly && Waypoints.size())
	{
		std::vector<CPoint> NewWaypoints;
		NewWaypoints.push_back(Waypoints[0]);

		for (UINT i = 1; i < Waypoints.size(); ++i)
			if (IsHorizontal(Waypoints[i - 1], Waypoints[i]) ||
				IsVertical(Waypoints[i - 1], Waypoints[i]))
				NewWaypoints.push_back(Waypoints[i]);
			else
			{
				CRect Rect(Waypoints[i - 1], Waypoints[i]);
				Rect.NormalizeRect();
				CPoint TopLeft(Rect.TopLeft());
				CPoint TopRight(Rect.right, Rect.top);
				CPoint BottomRight(Rect.BottomRight());
				CPoint BottomLeft(Rect.left, Rect.bottom);
				CPoint Center(Rect.CenterPoint());

				/*

				(bA1 == bA2) and (Direction(P(i-2)->P(i-1)) == Direction(P(i)->P(i+1)))

                i-2    i-1
                + - - ->+----#
                   A1        |
                             |
                             |    i      i+1
                             #----+ - - ->+
                                     A2

                        or

				(bA1 == bA2) and (Direction(P(i-2)->P(i-1)) != Direction(P(i)->P(i+1)))

                i-2    i-1
                + - - ->+----#
                   A1        |
                             |
                             |
                +<- - - - - -+
                i+1    A2    i

                        or

				(bA1 != bA2)

                i-2    i-1
                + - - ->+----#
                   A1        |
                             |
                             |      
                             +i
							 :
							 : A2
							 V
							 + i+1
                */

				BOOL bA1 = IsHorizontal(Waypoints[i - 2], Waypoints[i - 1]);	// TRUE - horizontal
				BOOL bA2 = IsHorizontal(Waypoints[i], Waypoints[i + 1]);

				if (bA1 != bA2)
				{
					CPoint NewPoint;
					ManhattanIntersectionPoint(	Waypoints[i - 2], Waypoints[i - 1],
												Waypoints[i], Waypoints[i + 1],
												NewPoint);
					NewWaypoints.push_back(NewPoint);
				}
				else
				{
					CPoint TempPoint1;
					CPoint TempPoint2;

					if (bA1)
					{
						BOOL bD1 = Waypoints[i - 1].x - Waypoints[i - 2].x > 0 ? TRUE : FALSE;	// TRUE -> left to right
						BOOL bD2 = Waypoints[i + 1].x - Waypoints[i].x > 0 ? TRUE : FALSE;

						if (bD1 == bD2)
							TempPoint1.x = Center.x;
						else
							TempPoint1.x =
								(bD1 ?
									MAX(Waypoints[i - 1].x, Waypoints[i].x) :
									MIN(Waypoints[i - 1].x, Waypoints[i].x));

						TempPoint2.x = TempPoint1.x;
						TempPoint1.y = Rect.top;
						TempPoint2.y = Rect.bottom;
					}
					else
					{
						BOOL bD1 = Waypoints[i - 1].y - Waypoints[i - 2].y > 0 ? TRUE : FALSE;	// TRUE -> top to bottom
						BOOL bD2 = Waypoints[i + 1].y - Waypoints[i].y > 0 ? TRUE : FALSE;

						if (bD1 == bD2)
							TempPoint1.y = Center.y;
						else
							TempPoint1.y =
								(bD1 ?
									MAX(Waypoints[i - 1].y, Waypoints[i].y) :
									MIN(Waypoints[i - 1].y, Waypoints[i].y));

						TempPoint1.x = Rect.left;
						TempPoint2.x = Rect.right;
						TempPoint2.y = TempPoint1.y;
					}

					CPoint NewPoint1;
					CPoint NewPoint2;

					ManhattanIntersectionPoint(	Waypoints[i - 2], Waypoints[i - 1],
												TempPoint1, TempPoint2,
												NewPoint1);

					ManhattanIntersectionPoint(	Waypoints[i], Waypoints[i + 1],
												TempPoint1, TempPoint2,
												NewPoint2);

					if (NewPoint1 != Waypoints[i - 1])
						NewWaypoints.push_back(NewPoint1);

					if (NewPoint2 != Waypoints[i])
						NewWaypoints.push_back(NewPoint2);
				}

				NewWaypoints.push_back(Waypoints[i]);
			}
		
		Waypoints.clear();
		Waypoints.insert(Waypoints.begin(), NewWaypoints.begin(), NewWaypoints.end());
	}

	return Waypoints.size();
}
