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

#ifndef COMMON_H
#define COMMON_H

#include "std.h"

// Timer IDs
enum TimerID {
  TIMER_MAIN = 1337,
  TIMER_TAIGA = 74164
};

// ListView sort types
enum ListSortType {
  LISTSORTTYPE_DEFAULT,
  LISTSORTTYPE_NUMBER,
  LISTSORTTYPE_SEASON,
  LISTSORTTYPE_FILESIZE
};

typedef unsigned __int64 QWORD, *LPQWORD;

// =============================================================================

// action.cpp
void ExecuteAction(wstring action, WPARAM wParam = 0, LPARAM lParam = 0);

// base64.cpp
wstring base64_encode(wchar_t const* bytes_to_encode, unsigned int in_len);
wstring base64_decode(wstring const& encoded_string);

// common.cpp
class CEpisode;
int StatusToIcon(int status);
wstring FormatError(DWORD dwError, LPCWSTR lpSource = NULL);
wstring GetDate(LPCWSTR lpFormat = L"yyyy'-'MM'-'dd");
wstring GetTime(LPCWSTR lpFormat = L"HH':'mm':'ss");
wstring GetLastEpisode(const wstring& episode);
wstring ToTimeString(int seconds);
bool Execute(const wstring& path, const wstring& parameters = L"");
BOOL ExecuteEx(const wstring& path, const wstring& parameters = L"");
void ExecuteLink(const wstring& link);
wstring ExpandEnvironmentStrings(const wstring& path);
wstring GetUserPassEncoded(const wstring& user, const wstring& pass);
void Registry_DeleteValue(HKEY hKey, LPCWSTR lpSubKey, LPCWSTR lpValueName);
wstring Registry_ReadValue(HKEY hKey, LPCWSTR lpSubKey, LPCWSTR lpValueName);
void Registry_SetValue(HKEY hKey, LPCWSTR lpSubKey, LPCWSTR lpValueName, LPCWSTR lpData, DWORD cbData);
wstring BrowseForFile(HWND hwndOwner, LPCWSTR lpstrTitle, LPCWSTR lpstrFilter = NULL);
BOOL BrowseForFolder(HWND hwndOwner, LPCWSTR lpszTitle, UINT ulFlags, wstring& output);
BOOL FileExists(const wstring& file);
BOOL FolderExists(const wstring& folder);
BOOL PathExists(const wstring& path);
void ValidateFileName(wstring& path);
wstring GetDefaultAppPath(const wstring& extension, const wstring& default_value);
int PopulateFiles(vector<wstring>& file_list, wstring path, wstring extension = L"*.*");
int PopulateFolders(vector<wstring>& folder_list, wstring path);
wstring ToSizeString(QWORD qwSize);

// debug.cpp
void DebugTest();

// encryption.cpp
wstring SimpleEncrypt(wstring str);
wstring SimpleDecrypt(wstring str);

// list_sort.cpp
int CALLBACK ListViewCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

// menu.cpp
void UpdateAllMenus(int anime_index);
void UpdateAccountMenu();
void UpdateAnimeMenu(int anime_index);
void UpdateAnnounceMenu(int anime_index);
void UpdateFilterMenu();
void UpdateFoldersMenu();
void UpdateSearchMenu();
void UpdateSearchListMenu(bool enabled = false);
void UpdateTrayMenu();

// script.cpp
wstring EvaluateFunction(const wstring& func_name, const wstring& func_body);
bool IsScriptFunction(const wstring& str);
bool IsScriptVariable(const wstring& str);
wstring ReplaceVariables(wstring str, const CEpisode& episode, bool url_encode = false);

// search.cpp
wstring SearchFileFolder(int anime_index, wstring root, int episode_number, bool search_folder);

//tc2.cpp
typedef std::map<wstring,wstring> HTTPParameters;
wstring OAuthWebRequestSubmit(const wstring& url, 
							  const wstring& httpMethod, 
							  const HTTPParameters* postParameters,
							  const wstring& consumerKey, 
							  const wstring& consumerSecret, 
							  const wstring& oauthToken = L"", 
							  const wstring& oauthTokenSecret = L"", 
							  const wstring& pin = L"");
wstring OAuthWebRequestSignedSubmit(const HTTPParameters& oauthParameters, 
									const wstring& url,
									const wstring& httpMethod, 
									const HTTPParameters* postParameters);
HTTPParameters BuildSignedOAuthParameters(const HTTPParameters& requestParameters, 
										  const wstring& url, 
										  const wstring& httpMethod, 
										  const HTTPParameters* postParameters, 
										  const wstring& consumerKey, 
										  const wstring& consumerSecret,
										  const wstring& requestToken = L"", 
										  const wstring& requestTokenSecret = L"", 
										  const wstring& pin = L"" );
wstring OAuthNormalizeUrl(const wstring& url) ;
wstring OAuthNormalizeRequestParameters(const HTTPParameters& requestParameters);
wstring OAuthConcatenateRequestElements(const wstring& httpMethod, wstring url, const wstring& parameters);
wstring OAuthCreateSignature(const wstring& signatureBase, const wstring& consumerSecret, const wstring& requestTokenSecret);
wstring UrlEncode(const wstring& url);
string urlencode(const string &c);
string char2hex(char dec);
wstring Base64String(const string& hash);
string HMACSHA1(const string& keyBytes, const string& data);
wstring OAuthCreateTimestamp();
wstring OAuthCreateNonce();
wstring OAuthBuildHeader(const HTTPParameters &parameters);
wstring BuildQueryString(const HTTPParameters &parameters);
HTTPParameters ParseQueryString(const wstring& url);
wstring UrlGetQuery(const wstring& url);

#endif // COMMON_H