/*
** Taiga
** Copyright (C) 2010-2014, Eren Okka
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

#include "win_ctrl.h"

namespace win {

ImageList::ImageList()
    : image_list_(nullptr) {
}

ImageList::~ImageList() {
  Destroy();
}

void ImageList::operator=(const HIMAGELIST image_list) {
  SetHandle(image_list);
}

BOOL ImageList::Create(int cx, int cy) {
  Destroy();

  image_list_ = ::ImageList_Create(cx, cy, ILC_COLOR32 | ILC_MASK, 0, 0);

  return image_list_ != nullptr;
}

VOID ImageList::Destroy() {
  if (image_list_) {
    ::ImageList_Destroy(image_list_);
    image_list_ = nullptr;
  }
}

////////////////////////////////////////////////////////////////////////////////

int ImageList::AddBitmap(HBITMAP bitmap, COLORREF mask) {
  if (mask != CLR_NONE) {
    return ::ImageList_AddMasked(image_list_, bitmap, mask);
  } else {
    return ::ImageList_Add(image_list_, bitmap, nullptr);
  }
}

BOOL ImageList::BeginDrag(int track, int hotspot_x, int hotspot_y) {
  return ::ImageList_BeginDrag(image_list_, track, hotspot_x, hotspot_y);
}

BOOL ImageList::DragEnter(HWND hwnd_lock, int x, int y) {
  return ::ImageList_DragEnter(hwnd_lock, x, y);
}

BOOL ImageList::DragLeave(HWND hwnd_lock) {
  return ::ImageList_DragLeave(hwnd_lock);
}

BOOL ImageList::DragMove(int x, int y) {
  return ::ImageList_DragMove(x, y);
}

BOOL ImageList::Draw(int index, HDC hdc, int x, int y) {
  return ::ImageList_Draw(image_list_, index, hdc, x, y, ILD_NORMAL);
}

void ImageList::Duplicate(HIMAGELIST image_list) {
  Destroy();

  image_list_ = ::ImageList_Duplicate(image_list);
}

VOID ImageList::EndDrag() {
  ::ImageList_EndDrag();
}

HIMAGELIST ImageList::GetHandle() {
  return image_list_;
}

HICON ImageList::GetIcon(int index) {
  return ::ImageList_GetIcon(image_list_, index, ILD_NORMAL);
}

BOOL ImageList::GetIconSize(int& cx, int& cy) {
  return ::ImageList_GetIconSize(image_list_, &cx, &cy);
}

BOOL ImageList::Remove(int index) {
  return ::ImageList_Remove(image_list_, index);
}

VOID ImageList::SetHandle(HIMAGELIST image_list) {
  Destroy();

  image_list_ = image_list;
}

}  // namespace win