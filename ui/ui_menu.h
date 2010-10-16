// ui_menu.h

#ifndef UI_MENU_H
#define UI_MENU_H

#include "ui_main.h"

// =============================================================================

enum MenuItemType {
  MENU_ITEM_NORMAL = 0,
  MENU_ITEM_SEPERATOR,
  MENU_ITEM_SUBMENU
};

class CMenuList {
public:
  CMenuList() { m_hImageList = NULL; }
  ~CMenuList() {}

  void    Create(LPCWSTR lpName, LPCWSTR lpType);
  HMENU   CreateNewMenu(LPCWSTR lpName, vector<HMENU>& hMenu);
  int     GetIndex(LPCWSTR lpName);
  void    SetImageList(HIMAGELIST hImageList);
  wstring Show(HWND hwnd, int x, int y, LPCWSTR lpName);

  class CMenu {
  public:
    void CreateItem(
      wstring action = L"", 
      wstring name   = L"", 
      wstring sub    = L"",
      bool checked   = false, 
      bool def       = false, 
      bool enabled   = true, 
      bool newcolumn = false, 
      bool radio     = false);

    class CMenuItem {
    public:
      bool    Checked, Default, Enabled, NewColumn, Radio;
      wstring Action, Name, SubMenu;
      int     Type;

      class CMenuIcon {
      public:
        void    Destroy();
        void    Load(HICON hIcon);
        HBITMAP Handle;
        int     Index;
      } Icon;
    };
    vector<CMenuItem> Item;
    wstring Name, Type;
  };
  vector<CMenu> Menu;

private:
  HIMAGELIST m_hImageList;
};

#endif // UI_MENU_H