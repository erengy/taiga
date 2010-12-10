/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010, Eren Okka
** 
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "win_control.h"

// =============================================================================

BOOL CImageList::Create(int cx, int cy) {
  if (m_hImageList) Destroy();
  m_hImageList = ::ImageList_Create(cx, cy, ILC_COLOR32 | ILC_MASK, 0, 0);
  return m_hImageList != NULL;
}

VOID CImageList::Destroy() {
  ::ImageList_Destroy(m_hImageList);
  m_hImageList = NULL;
}

// =============================================================================

int CImageList::AddBitmap(HBITMAP hBitmap, COLORREF crMask) {
  if (crMask != CLR_NONE) {
    return ::ImageList_AddMasked(m_hImageList, hBitmap, crMask);
  } else {
    return ::ImageList_Add(m_hImageList, hBitmap, NULL);
  }
}

BOOL CImageList::BeginDrag(int iTrack, int dxHotspot, int dyHotspot) {
  return ::ImageList_BeginDrag(m_hImageList, iTrack, dxHotspot, dyHotspot);
}

BOOL CImageList::DragEnter(HWND hwndLock, int x, int y) {
  return ::ImageList_DragEnter(hwndLock, x, y);
}

BOOL CImageList::DragLeave(HWND hwndLock) {
  return ::ImageList_DragLeave(hwndLock);
}

BOOL CImageList::DragMove(int x, int y) {
  return ::ImageList_DragMove(x, y);
}

BOOL CImageList::Draw(int nIndex, HDC hdcDest, int x, int y) {
  return ::ImageList_Draw(m_hImageList, nIndex, hdcDest, x, y, ILD_NORMAL);
}

VOID CImageList::EndDrag() {
  ImageList_EndDrag();
}

HIMAGELIST CImageList::GetHandle() {
  return m_hImageList;
}

HICON CImageList::GetIcon(int nIndex) {
  return ::ImageList_GetIcon(m_hImageList, nIndex, ILD_NORMAL);
}

BOOL CImageList::Remove(int index) {
  return ::ImageList_Remove(m_hImageList, index);
}

VOID CImageList::SetHandle(HIMAGELIST hImageList) {
  Destroy();
  m_hImageList = hImageList;
}