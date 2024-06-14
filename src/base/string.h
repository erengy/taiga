/**
 * Taiga
 * Copyright (C) 2010-2024, Eren Okka
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <array>
#include <string>
#include <vector>
#include <windows.h>

void Erase(std::wstring& str1, const std::wstring& str2, bool case_insensitive = false);
void EraseChars(std::wstring& str, const wchar_t chars[]);
void EraseLeft(std::wstring& str1, const std::wstring& str2, bool case_insensitive = false);
void EraseRight(std::wstring& str1, const std::wstring& str2, bool case_insensitive = false);
void RemoveEmptyStrings(std::vector<std::wstring>& input);

int CompareStrings(const std::wstring& str1, const std::wstring& str2, bool case_insensitive = true, size_t max_count = MAX_PATH);
inline bool IsCharsEqual(const wchar_t c1, const wchar_t c2);
bool IsEqual(const std::wstring& str1, const std::wstring& str2);

int InStr(const std::wstring& str1, const std::wstring& str2, int pos = 0, bool case_insensitive = false);
std::wstring InStr(const std::wstring& str1, const std::wstring& str2_left, const std::wstring& str2_right);
int InStrChars(const std::wstring& str1, const std::wstring& str2, int pos);

bool IsAlphanumericChar(const wchar_t c);
bool IsAlphanumericString(const std::wstring& str);
bool IsHexadecimalChar(const wchar_t c);
bool IsHexadecimalString(const std::wstring& str);
bool IsNumericChar(const wchar_t c);
bool IsNumericString(const std::wstring& str);

bool StartsWith(const std::wstring& str, const std::wstring& search);
bool EndsWith(const std::wstring& str, const std::wstring& search);

size_t LongestCommonSubsequenceLength(const std::wstring& str1, const std::wstring& str2);
double JaroWinklerDistance(const std::wstring& str1, const std::wstring& str2);
double LevenshteinDistance(const std::wstring& str1, const std::wstring& str2);

using trigram_t = std::array<wchar_t, 3>;
using trigram_container_t = std::vector<trigram_t>;
void GetTrigrams(const std::wstring& str, trigram_container_t& output);
double CompareTrigrams(const trigram_container_t& t1, const trigram_container_t& t2);

void ReplaceChar(std::wstring& str, const wchar_t c, const wchar_t replace_with);
bool ReplaceString(std::wstring& str, size_t offset, const std::wstring& find_this, const std::wstring& replace_with, bool whole_word_only, bool replace_all_instances);
bool ReplaceString(std::wstring& str, const std::wstring& find_this, const std::wstring& replace_with);

std::wstring Join(const std::vector<std::wstring>& join_vector, const std::wstring& separator);
void Split(const std::wstring& str, const std::wstring& separator, std::vector<std::wstring>& split_vector);
size_t Tokenize(const std::wstring& str, const std::wstring& delimiters, std::vector<std::wstring>& tokens);

std::wstring StrToWstr(const std::string& str, UINT code_page = CP_UTF8);
std::string WstrToStr(const std::wstring& str, UINT code_page = CP_UTF8);

void ToLower(std::wstring& str, bool use_locale = false);
std::wstring ToLower_Copy(std::wstring str, bool use_locale = false);
void ToUpper(std::wstring& str, bool use_locale = false);
std::wstring ToUpper_Copy(std::wstring str, bool use_locale = false);

bool ToBool(const std::wstring& str);
double ToDouble(const std::string& str);
double ToDouble(const std::wstring& str);
int ToInt(const std::string& str);
int ToInt(const std::wstring& str);
UINT64 ToUint64(const std::string& str);
UINT64 ToUint64(const std::wstring& str);
time_t ToTime(const std::string& str);
time_t ToTime(const std::wstring& str);
std::string ToStr(const INT& value);
std::wstring ToWstr(const INT& value);
std::wstring ToWstr(const UINT& value);
std::wstring ToWstr(const ULONG& value);
std::wstring ToWstr(const INT64& value);
std::wstring ToWstr(const UINT64& value);
std::string ToStr(const double& value, int count = 16);
std::wstring ToWstr(const double& value, int count = 16);

std::wstring LimitText(const std::wstring& str, unsigned int limit, const std::wstring& tail = L"...");
void Trim(std::wstring& str, const wchar_t trim_chars[] = L" ", bool trim_left = true, bool trim_right = true);
void TrimLeft(std::wstring& str, const wchar_t trim_chars[] = L" ");
void TrimRight(std::wstring& str, const wchar_t trim_chars[] = L" ");

void AddTrailingSlash(std::wstring& str);
std::wstring AddTrailingSlash(const std::wstring& str);
void RemoveTrailingSlash(std::wstring& str);
std::wstring GetFileExtension(const std::wstring& str);
std::wstring GetFileName(const std::wstring& str);
std::wstring GetFileWithoutExtension(std::wstring str);
std::wstring GetPathOnly(const std::wstring& str);
bool ValidateFileExtension(const std::wstring& extension, unsigned int max_length = 0);

void AppendString(std::wstring& str0, const std::wstring& str1, const std::wstring& str2 = L", ");
const std::wstring& EmptyString();
std::wstring PadChar(std::wstring str, const wchar_t ch, const size_t len);
std::wstring PushString(const std::wstring& str1, const std::wstring& str2);
