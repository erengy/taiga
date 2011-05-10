/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2011, Eren Okka
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

#ifndef STRING_H
#define STRING_H

#include "std.h"

#define EMPTY_STR L"%empty%"

// =============================================================================

wstring CharLeft(const wstring& str, int length);
wstring CharRight(const wstring& str, int length);
bool CheckFileExtension(wstring extension, const vector<wstring>& extension_list);
void CheckSlash(wstring& str);
wstring CheckSlash(const wstring& str);
int CompareStrings(const wstring& str1, const wstring& str2, bool case_insensitive = true, size_t max_count = MAX_PATH);
void DecodeHTML(wstring& str);
wstring EncodeURL(const wstring& str, bool encode_unreserved = false);
bool EndsWith(const wstring& str, const wstring& search);
void Erase(wstring& str1, const wstring& str2, bool case_insensitive = false);
void EraseChars(wstring& str, const wchar_t chars[]);
void EraseLeft(wstring& str1, const wstring str2, bool case_insensitive);
void ErasePunctuation(wstring& str, bool keep_trailing = false);
void EraseRight(wstring& str1, const wstring str2, bool case_insensitive);
wstring GetFileExtension(const wstring str);
wstring GetFileName(const wstring str);
wchar_t GetMostCommonCharacter(const wstring& str);
wstring GetPathOnly(const wstring str);
int InStr(const wstring& str1, const wstring str2, int pos = 0, bool case_insensitive = false);
wstring InStr(const wstring& str1, const wstring& str2_left, const wstring& str2_right);
int InStrRev(const wstring& str1, const wstring str2, int pos);
int InStrChars(const wstring& str1, const wstring str2, int pos);
int InStrCharsRev(const wstring& str1, const wstring str2, int pos);
bool IsAlphanumeric(const wchar_t c);
bool IsAlphanumeric(const wstring& str);
inline bool IsCharsEqual(const wchar_t c1, const wchar_t c2);
bool IsEqual(const wstring& str1, const wstring& str2);
bool IsHex(const wchar_t c);
bool IsHex(const wstring& str);
bool IsNumeric(const wchar_t c);
bool IsNumeric(const wstring& str);
bool IsWhitespace(const wchar_t c);
void LimitText(wstring& str, unsigned int limit, const wstring& tail = L"...");
wstring PushString(const wstring& str1, const wstring& str2);
void ReadStringTable(UINT uID, wstring& str);
void Replace(wstring& str1, wstring str2, wstring replace_with, bool replace_all = false, bool case_insensitive = false);
void ReplaceChar(wstring& str, const wchar_t c, const wchar_t replace_with);
void ReplaceChars(wstring& str, const wchar_t chars[], const wstring replace_with);
void Split(const wstring& str, const wstring& separator, std::vector<wstring>& split_vector);
bool StartsWith(const wstring& str, const wstring& search);
void StripHTML(wstring& str);
wstring SubStr(const wstring& str, const wstring& sub_begin, const wstring& sub_end);
size_t Tokenize(const wstring& str, const wstring& delimiters, vector<wstring>& tokens);
const char* ToANSI(const wstring& str, UINT code_page = CP_UTF8);
wstring ToUTF8(const string& str, UINT code_page = CP_UTF8);
void ToLower(wstring& str);
wstring ToLower_Copy(wstring str);
void ToUpper(wstring& str);
wstring ToUpper_Copy(wstring str);
int ToINT(const wstring& str);
wstring ToWSTR(const INT& value);
wstring ToWSTR(const ULONG& value);
wstring ToWSTR(const INT64& value);
wstring ToWSTR(const UINT64& value);
wstring ToWSTR(const double& value, int count = 16);
void Trim(wstring& str, const wchar_t trim_chars[] = L" ", bool trim_left = true, bool trim_right = true);
void TrimLeft(wstring& str, const wchar_t trim_chars[] = L" ");
void TrimRight(wstring& str, const wchar_t trim_chars[] = L" ");
bool ValidateFileExtension(const wstring& extension, unsigned int max_length = 0);

#endif // STRING_H