// RowIcons.cpp : vector line icons for the quick paste list (ui-redesign)

#include "stdafx.h"
#include "RowIcons.h"
#include <gdiplus.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void CRowIcons::Draw(HDC hdc, CDPI &dpi, ClipRowIcon icon, const CRect &rc, COLORREF color)
{
	Gdiplus::Graphics graphics(hdc);
	graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

	Gdiplus::Color stroke(255, GetRValue(color), GetGValue(color), GetBValue(color));
	Gdiplus::Pen pen(stroke, 1.4f);
	pen.SetStartCap(Gdiplus::LineCapRound);
	pen.SetEndCap(Gdiplus::LineCapRound);
	pen.SetLineJoin(Gdiplus::LineJoinRound);

	// scale the 16 unit design grid into the destination rect
	Gdiplus::Matrix previous;
	graphics.GetTransform(&previous);
	Gdiplus::REAL scale = min(rc.Width(), rc.Height()) / 16.0f;
	graphics.TranslateTransform((Gdiplus::REAL)rc.left, rc.top + (rc.Height() - 16.0f * scale) / 2.0f);
	graphics.ScaleTransform(scale, scale);

	DrawIcon(graphics, icon, pen);

	graphics.SetTransform(&previous);
}

void CRowIcons::DrawIcon(Gdiplus::Graphics &graphics, ClipRowIcon icon, Gdiplus::Pen &pen)
{
	Gdiplus::GraphicsPath path;
	switch (icon)
	{
	case ClipRowIcon::Text:
		path.StartFigure(); path.AddLine(3.0f, 4.5f, 13.0f, 4.5f);
		path.StartFigure(); path.AddLine(3.0f, 8.0f, 13.0f, 8.0f);
		path.StartFigure(); path.AddLine(3.0f, 11.5f, 8.5f, 11.5f);
		break;
	case ClipRowIcon::Code:
		path.StartFigure(); path.AddLine(5.5f, 4.0f, 2.5f, 8.0f); path.AddLine(2.5f, 8.0f, 5.5f, 12.0f);
		path.StartFigure(); path.AddLine(10.5f, 4.0f, 13.5f, 8.0f); path.AddLine(13.5f, 8.0f, 10.5f, 12.0f);
		break;
	case ClipRowIcon::Link:
	{
		// two chain links drawn as rounded rects rotated 45 degrees
		Gdiplus::Matrix previous;
		graphics.GetTransform(&previous);
		Gdiplus::Matrix rotation(0.70710678f, 0.70710678f, -0.70710678f, 0.70710678f, 8.0f, 0.0f);
		graphics.MultiplyTransform(&rotation);

		Gdiplus::RectF linkA(0.5f, 6.0f, 8.0f, 4.5f);
		Gdiplus::RectF linkB(7.5f, 5.5f, 8.0f, 4.5f);

		path.AddArc(linkA.X, linkA.Y, linkA.Height, linkA.Height, 90, 180);
		path.AddArc(linkA.X + linkA.Width - linkA.Height, linkA.Y, linkA.Height, linkA.Height, 270, 180);
		path.CloseFigure();
		graphics.DrawPath(&pen, &path);
		path.Reset();
		path.AddArc(linkB.X, linkB.Y, linkB.Height, linkB.Height, 90, 180);
		path.AddArc(linkB.X + linkB.Width - linkB.Height, linkB.Y, linkB.Height, linkB.Height, 270, 180);
		path.CloseFigure();
		graphics.DrawPath(&pen, &path);

		graphics.SetTransform(&previous);
		return;
	}
	case ClipRowIcon::Image:
		path.AddLine(3.0f, 4.0f, 4.5f, 4.0f); path.StartFigure();
		path.AddLine(6.0f, 4.0f, 13.0f, 4.0f); path.AddLine(13.0f, 4.0f, 13.0f, 12.0f);
		path.AddLine(13.0f, 12.0f, 3.0f, 12.0f); path.AddLine(3.0f, 12.0f, 3.0f, 4.0f);
		path.StartFigure(); path.AddLine(4.5f, 10.0f, 6.5f, 7.5f); path.AddLine(6.5f, 7.5f, 8.5f, 9.5f);
		path.AddLine(8.5f, 9.5f, 10.0f, 8.0f); path.AddLine(10.0f, 8.0f, 12.0f, 10.0f);
		break;
	case ClipRowIcon::File:
		path.StartFigure(); path.AddLine(4.5f, 2.5f, 8.5f, 2.5f); path.AddLine(8.5f, 2.5f, 11.5f, 5.5f);
		path.AddLine(11.5f, 5.5f, 11.5f, 13.5f); path.AddLine(11.5f, 13.5f, 4.5f, 13.5f);
		path.AddLine(4.5f, 13.5f, 4.5f, 2.5f);
		path.StartFigure(); path.AddLine(8.5f, 2.5f, 8.5f, 5.5f); path.AddLine(8.5f, 5.5f, 11.5f, 5.5f);
		break;
	case ClipRowIcon::RichText:
		path.StartFigure(); path.AddLine(4.0f, 4.0f, 12.0f, 4.0f);
		path.StartFigure(); path.AddLine(8.0f, 4.0f, 8.0f, 12.0f);
		path.StartFigure(); path.AddLine(6.5f, 12.0f, 9.5f, 12.0f);
		break;
	case ClipRowIcon::Email:
		path.StartFigure(); path.AddLine(3.0f, 4.5f, 13.0f, 4.5f); path.AddLine(13.0f, 4.5f, 13.0f, 11.5f);
		path.AddLine(13.0f, 11.5f, 3.0f, 11.5f); path.AddLine(3.0f, 11.5f, 3.0f, 4.5f);
		path.StartFigure(); path.AddLine(3.5f, 5.2f, 8.0f, 8.5f); path.AddLine(8.0f, 8.5f, 12.5f, 5.2f);
		break;
	case ClipRowIcon::Folder:
		path.StartFigure(); path.AddLine(2.5f, 12.5f, 2.5f, 4.0f); path.AddLine(2.5f, 4.0f, 6.5f, 4.0f);
		path.AddLine(6.5f, 4.0f, 8.0f, 5.8f); path.AddLine(8.0f, 5.8f, 13.5f, 5.8f);
		path.AddLine(13.5f, 5.8f, 13.5f, 12.5f); path.AddLine(13.5f, 12.5f, 2.5f, 12.5f);
		break;
	default:
		break;
	}

	graphics.DrawPath(&pen, &path);
}

ClipRowIcon CRowIcons::Classify(const CString &csDesc)
{
	CString csDescTrimmed = csDesc;
	csDescTrimmed.Trim();
	if (csDescTrimmed.GetLength() == 0)
	{
		return ClipRowIcon::Text;
	}

	CString lower = csDescTrimmed;
	lower.MakeLower();

	// image clips are described by their format name ("CF_DIB", "PNG", "image/png")
	if (lower.Find(_T("cf_dib")) == 0 ||
		lower.Find(_T("png")) == 0 ||
		lower.Find(_T("image/")) == 0)
	{
		return ClipRowIcon::Image;
	}

	if (lower.Find(_T("cf_rtf")) == 0)
	{
		return ClipRowIcon::RichText;
	}

	if (lower.Find(_T("copied file")) == 0)
	{
		return ClipRowIcon::File;
	}

	if (lower.Find(_T("http://")) == 0 ||
		lower.Find(_T("https://")) == 0 ||
		lower.Find(_T("ftp://")) == 0 ||
		lower.Find(_T("www.")) == 0)
	{
		return ClipRowIcon::Link;
	}

	// drive letter or UNC path
	if ((csDescTrimmed.GetLength() > 2 &&
		iswalpha(csDescTrimmed[0]) &&
		csDescTrimmed[1] == _T(':') &&
		(csDescTrimmed[2] == _T('\\') || csDescTrimmed[2] == _T('/'))) ||
		lower.Find(_T("\\\\")) == 0)
	{
		return ClipRowIcon::File;
	}

	// a single @ with a dot after it and no spaces is treated as an email address
	int atSign = csDescTrimmed.Find(_T('@'));
	if (atSign > 0 &&
		csDescTrimmed.Find(_T('@'), atSign + 1) < 0 &&
		csDescTrimmed.Find(_T(' ')) < 0 &&
		csDescTrimmed.Find(_T('.'), atSign) > atSign)
	{
		return ClipRowIcon::Email;
	}

	// crude code detection, only to pick an icon
	if (csDescTrimmed.Find(_T('{')) >= 0 ||
		csDescTrimmed.Find(_T("=>")) >= 0 ||
		csDescTrimmed.Find(_T("</")) >= 0 ||
		csDescTrimmed.ReverseFind(_T(';')) == csDescTrimmed.GetLength() - 1)
	{
		return ClipRowIcon::Code;
	}

	return ClipRowIcon::Text;
}
