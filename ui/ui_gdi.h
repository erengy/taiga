// ui_gdi.h

#ifndef UI_GDI_H
#define UI_GDI_H

#include "ui_main.h"

// =============================================================================

class CDC {
public:
  CDC(HDC hDC = NULL);
  virtual ~CDC();

  void operator = (const HDC hDC) {
    AttachDC(hDC);
  }

  void AttachDC(HDC hDC);
  HDC  DetachDC();
  HDC  Get() const;

  // Brush
  void   AttachBrush(HBRUSH hBrush);
  void   CreateSolidBrush(COLORREF color);
  HBRUSH DetachBrush();

  // Font
  void  AttachFont(HFONT hFont);
  HFONT DetachFont();
  void  EditFont(LPCWSTR lpFaceName = NULL, INT iSize = -1, BOOL bBold = -1, BOOL bItalic = -1, BOOL bUnderline = -1);

  //
  BOOL FillRect(const RECT& rc, HBRUSH hbr) const;
  void FillRect(const RECT& rc, COLORREF color) const;
  
  // Bitmap
  void    AttachBitmap(HBITMAP hBitmap);
  BOOL    BitBlt(int x, int y, int nWidth, int nHeight, HDC hSrcDC, int xSrc, int ySrc, DWORD dwRop) const;
  HBITMAP DetachBitmap();
  int     SetStretchBltMode(int mode);
  BOOL    StretchBlt(int x, int y, int nWidth, int nHeight, HDC hSrcDC, int xSrc, int ySrc, int nSrcWidth, int nSrcHeight, DWORD dwRop) const;

  // Text
  int      DrawText(LPCWSTR lpszString, int nCount, const RECT& rc, UINT nFormat) const;
  COLORREF SetBkColor(COLORREF crColor) const;
  int      SetBkMode(int iBkMode) const;
  COLORREF SetTextColor(COLORREF crColor) const;

private:
  HDC     m_hDC;
  HBITMAP m_hBitmapOld;
  HBRUSH  m_hBrushOld;
  HFONT   m_hFontOld;
};

// =============================================================================

class CRect : public RECT {
public:
  CRect();
  CRect(int l, int t, int r, int b);
  CRect(const RECT& rc);
  CRect(LPCRECT lprc);

  BOOL operator == (const RECT& rc) {
    return ::EqualRect(this, &rc);
  }
  BOOL operator != (const RECT& rc) {
    return !::EqualRect(this, &rc);
  }
  void operator = (const RECT& srcRect) {
    ::CopyRect(this, &srcRect);
  }

  int Height() {
    return bottom - top;
  }
  int Width() {
    return right - left;
  }

  void Copy(const RECT& rc);
  BOOL Equal(const RECT& rc);
  BOOL Inflate(int dx, int dy);
  BOOL Intersect(const RECT& rc1, const RECT& rc2);
  BOOL IsEmpty();
  BOOL Offset(int dx, int dy);
  BOOL PtIn(POINT pt);
  BOOL Set(int left, int top, int right, int bottom);
  BOOL SetEmpty();
  BOOL Subtract(const RECT& rc1, const RECT& rc2);
  BOOL Union(const RECT& rc1, const RECT& rc2);
};

#endif // UI_GDI_H