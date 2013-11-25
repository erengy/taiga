/*
** Taiga
** Copyright (C) 2010-2013, Eren Okka
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

#include "ui.h"
#include "menu.h"
#include "base/common.h"
#include "base/foreach.h"
#include "base/string.h"
#include "base/types.h"
#include "library/anime_db.h"
#include "taiga/http.h"
#include "taiga/resource.h"
#include "taiga/settings.h"
#include "taiga/taiga.h"
#include "win/win_taskbar.h"

#include "dlg/dlg_anime_info.h"
#include "dlg/dlg_anime_list.h"
#include "dlg/dlg_history.h"
#include "dlg/dlg_input.h"
#include "dlg/dlg_main.h"
#include "dlg/dlg_search.h"
#include "dlg/dlg_season.h"
#include "dlg/dlg_settings.h"
#include "dlg/dlg_stats.h"
#include "dlg/dlg_torrent.h"
#include "dlg/dlg_update.h"

namespace ui {

void ChangeStatusText(const string_t& status) {
  MainDialog.ChangeStatus(status);
}

void ClearStatusText() {
  MainDialog.ChangeStatus(L"");
}

////////////////////////////////////////////////////////////////////////////////

void OnHttpError(const taiga::HttpClient& http_client, const string_t& error) {
  switch (http_client.mode()) {
    case taiga::kHttpSilent:
    case taiga::kHttpServiceGetMetadataById:
    case taiga::kHttpServiceSearchTitle:
    case taiga::kHttpGetLibraryEntryImage:
      return;
    case taiga::kHttpServiceAuthenticateUser:
    case taiga::kHttpServiceGetLibraryEntries:
    case taiga::kHttpServiceAddLibraryEntry:
    case taiga::kHttpServiceDeleteLibraryEntry:
    case taiga::kHttpServiceUpdateLibraryEntry:
      ChangeStatusText(error);
      MainDialog.EnableInput(true);
      break;
    case taiga::kHttpFeedCheck:
    case taiga::kHttpFeedCheckAuto:
    case taiga::kHttpFeedDownload:
    case taiga::kHttpFeedDownloadAll:
      ChangeStatusText(error);
      TorrentDialog.EnableInput();
      break;
    case taiga::kHttpTwitterRequest:
    case taiga::kHttpTwitterAuth:
    case taiga::kHttpTwitterPost:
      ChangeStatusText(error);
      break;
    case taiga::kHttpTaigaUpdateCheck:
    case taiga::kHttpTaigaUpdateDownload:
      MessageBox(UpdateDialog.GetWindowHandle(), 
                 error.c_str(), L"Update", MB_ICONERROR | MB_OK);
      UpdateDialog.PostMessage(WM_CLOSE);
      return;
  }

  TaskbarList.SetProgressState(TBPF_NOPROGRESS);
}

void OnHttpHeadersAvailable(const taiga::HttpClient& http_client) {
  switch (http_client.mode()) {
    case taiga::kHttpSilent:
      return;
    case taiga::kHttpTaigaUpdateCheck:
    case taiga::kHttpTaigaUpdateDownload:
      if (http_client.content_length() > 0) {
        UpdateDialog.progressbar.SetMarquee(false);
        UpdateDialog.progressbar.SetRange(0, http_client.content_length());
      } else {
        UpdateDialog.progressbar.SetMarquee(true);
      }
      if (http_client.mode() == taiga::kHttpTaigaUpdateDownload) {
        UpdateDialog.SetDlgItemText(IDC_STATIC_UPDATE_PROGRESS,
                                    L"Downloading latest update...");
      }
      break;
    default:
      TaskbarList.SetProgressState(http_client.content_length() > 0 ?
                                   TBPF_NORMAL : TBPF_INDETERMINATE);
      break;
  }
}

void OnHttpProgress(const taiga::HttpClient& http_client) {
  wstring status;

  switch (http_client.mode()) {
    case taiga::kHttpSilent:
    case taiga::kHttpServiceGetMetadataById:
    case taiga::kHttpServiceSearchTitle:
    case taiga::kHttpGetLibraryEntryImage:
      return;
    case taiga::kHttpServiceAuthenticateUser:
      status = L"Reading account information...";
      break;
    case taiga::kHttpServiceGetLibraryEntries:
      status = L"Downloading anime list...";
      break;
    case taiga::kHttpServiceAddLibraryEntry:
    case taiga::kHttpServiceDeleteLibraryEntry:
    case taiga::kHttpServiceUpdateLibraryEntry:
      status = L"Updating list...";
      break;
    case taiga::kHttpFeedCheck:
    case taiga::kHttpFeedCheckAuto:
      status = L"Checking new torrents...";
      break;
    case taiga::kHttpFeedDownload:
    case taiga::kHttpFeedDownloadAll:
      status = L"Downloading torrent file...";
      break;
    case taiga::kHttpTwitterRequest:
      status = L"Connecting to Twitter...";
      break;
    case taiga::kHttpTwitterAuth:
      status = L"Authorizing Twitter...";
      break;
    case taiga::kHttpTwitterPost:
      status = L"Updating Twitter status...";
      break;
    case taiga::kHttpTaigaUpdateCheck:
    case taiga::kHttpTaigaUpdateDownload:
      if (http_client.content_length() > 0)
        UpdateDialog.progressbar.SetPosition(http_client.current_length());
      return;
  }

  if (http_client.content_length() > 0) {
    float current_length = static_cast<float>(http_client.current_length());
    float content_length = static_cast<float>(http_client.content_length());
    int percentage = static_cast<int>((current_length / content_length) * 100);
    status += L" (" + ToWstr(percentage) + L"%)";
    TaskbarList.SetProgressValue(current_length, content_length);
  } else {
    status += L" (" + ToSizeString(http_client.current_length()) + L")";
  }

  ChangeStatusText(status);
}

void OnHttpReadComplete(const taiga::HttpClient& http_client) {
  TaskbarList.SetProgressState(TBPF_NOPROGRESS);
}

////////////////////////////////////////////////////////////////////////////////

void OnLibraryChange() {
  ClearStatusText();

  AnimeListDialog.RefreshList();
  AnimeListDialog.RefreshTabs();
  HistoryDialog.RefreshList();
  SearchDialog.RefreshList();

  MainDialog.EnableInput(true);
}

void OnLibraryEntryChange(int id) {
  if (AnimeDialog.GetCurrentId() == id)
    AnimeDialog.Refresh(false, true, false, false);

  if (AnimeListDialog.IsWindow())
    AnimeListDialog.RefreshListItem(id);

  if (NowPlayingDialog.GetCurrentId() == id)
    NowPlayingDialog.Refresh(false, true, false, false);

  if (SeasonDialog.IsWindow())
    SeasonDialog.RefreshList(true);
}

void OnLibraryEntryImageChange(int id) {
  if (AnimeDialog.GetCurrentId() == id)
    AnimeDialog.Refresh(true, false, false, false);

  if (AnimeListDialog.IsWindow())
    AnimeListDialog.RefreshListItem(id);

  if (NowPlayingDialog.GetCurrentId() == id)
    NowPlayingDialog.Refresh(true, false, false, false);

  if (SeasonDialog.IsWindow())
    SeasonDialog.RefreshList(true);
}

void OnLibrarySearchTitle(const string_t& results) {
  std::vector<string_t> split_vector;
  Split(results, L",", split_vector);

  std::vector<int> ids;
  foreach_(it, split_vector)
    ids.push_back(ToInt(*it));

  SearchDialog.ParseResults(ids);
}

void OnLibraryUpdateFailure(int id, const string_t& reason) {
  anime::Item* anime_item = AnimeDatabase.FindItem(id);
  
  wstring text;
  if (anime_item)
    text += L"Title: " + anime_item->GetTitle() + L"\n";
  if (!reason.empty())
    text += L"Reason: " + reason + L"\n";
  text += L"Click to try again.";

  Taiga.current_tip_type = TIPTYPE_UPDATEFAILED;

  Taskbar.Tip(L"", L"", 0);  // clear previous tips
  Taskbar.Tip(text.c_str(), L"Update failed", NIIF_ERROR);

  ChangeStatusText(L"Update failed: " + reason);
}

////////////////////////////////////////////////////////////////////////////////

void OnFeedCheck(bool success) {
  ChangeStatusText(success ?
      L"There are new torrents available!" : L"No new torrents found.");

  TorrentDialog.RefreshList();
  TorrentDialog.EnableInput();
}

void OnFeedDownload(bool success) {
  if (success)
    TorrentDialog.RefreshList();

  ChangeStatusText(L"Successfully downloaded all torrents.");
  TorrentDialog.EnableInput();
}

////////////////////////////////////////////////////////////////////////////////

bool OnTwitterRequest(string_t& auth_pin) {
  ClearStatusText();

  InputDialog dlg;
  dlg.title = L"Twitter Authorization";
  dlg.info = L"Please enter the PIN shown on the page after logging into Twitter:";
  dlg.Show();

  if (dlg.result == IDOK && !dlg.text.empty()) {
    auth_pin = dlg.text;
    return true;
  }

  return false;
}

void OnTwitterAuth(bool success) {
  ChangeStatusText(success ?
      L"Taiga is now authorized to post to this Twitter account: " + Settings.Announce.Twitter.user :
      L"Twitter authorization failed.");

  SettingsDialog.RefreshTwitterLink();
}

void OnTwitterPost(bool success, const string_t& error) {
  ChangeStatusText(success ?
      L"Twitter status updated." :
      L"Twitter status update failed. (" + error + L")");
}

////////////////////////////////////////////////////////////////////////////////

void OnLogin() {
  ChangeStatusText(L"Logged in as " + Settings.Account.MAL.user);
  UpdateAllMenus();

  MainDialog.UpdateTip();
  MainDialog.UpdateTitle();
  MainDialog.EnableInput(true);
}

void OnLogout() {
}

void OnUpdate() {
  UpdateDialog.PostMessage(WM_CLOSE);
}

}  // namespace ui