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

#include "std.h"

#define NT_SUCCESS(x) ((x) >= 0)
#define STATUS_INFO_LENGTH_MISMATCH 0xc0000004
#define OBJECT_TYPE_FILE 0x1c

#define ObjectBasicInformation    0
#define ObjectNameInformation     1
#define ObjectTypeInformation     2
#define SystemHandleInformation   16
#define SystemHandleInformationEx 64

typedef NTSTATUS (NTAPI *_NtQuerySystemInformation)(
  ULONG SystemInformationClass,
  PVOID SystemInformation,
  ULONG SystemInformationLength,
  PULONG ReturnLength
);
typedef NTSTATUS (NTAPI *_NtDuplicateObject)(
  HANDLE SourceProcessHandle,
  HANDLE SourceHandle,
  HANDLE TargetProcessHandle,
  PHANDLE TargetHandle,
  ACCESS_MASK DesiredAccess,
  ULONG Attributes,
  ULONG Options
);
typedef NTSTATUS (NTAPI *_NtQueryObject)(
  HANDLE ObjectHandle,
  ULONG ObjectInformationClass,
  PVOID ObjectInformation,
  ULONG ObjectInformationLength,
  PULONG ReturnLength
);

typedef struct _UNICODE_STRING {
  USHORT Length;
  USHORT MaximumLength;
  PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _SYSTEM_HANDLE {
  ULONG ProcessId;
  BYTE ObjectTypeNumber;
  BYTE Flags;
  USHORT Handle;
  PVOID Object;
  ACCESS_MASK GrantedAccess;
} SYSTEM_HANDLE, *PSYSTEM_HANDLE;
typedef struct _SYSTEM_HANDLE_INFORMATION {
  ULONG HandleCount;
  SYSTEM_HANDLE Handles[1];
} SYSTEM_HANDLE_INFORMATION, *PSYSTEM_HANDLE_INFORMATION;

typedef struct _SYSTEM_HANDLE_EX {
  PVOID Object;
  HANDLE ProcessId;
  HANDLE Handle;
  ULONG GrantedAccess;
  USHORT CreatorBackTraceIndex;
  USHORT ObjectTypeIndex;
  ULONG HandleAttributes;
  ULONG Reserved;
} SYSTEM_HANDLE_EX, *PSYSTEM_HANDLE_EX;
typedef struct _SYSTEM_HANDLE_INFORMATION_EX {
  ULONG_PTR HandleCount;
  ULONG_PTR Reserved;
  SYSTEM_HANDLE_EX Handles[1];
} SYSTEM_HANDLE_INFORMATION_EX, *PSYSTEM_HANDLE_INFORMATION_EX;

typedef enum _POOL_TYPE {
  NonPagedPool,
  PagedPool,
  NonPagedPoolMustSucceed,
  DontUseThisType,
  NonPagedPoolCacheAligned,
  PagedPoolCacheAligned,
  NonPagedPoolCacheAlignedMustS
} POOL_TYPE, *PPOOL_TYPE;

typedef struct _OBJECT_TYPE_INFORMATION {
  UNICODE_STRING Name;
  ULONG TotalNumberOfObjects;
  ULONG TotalNumberOfHandles;
  ULONG TotalPagedPoolUsage;
  ULONG TotalNonPagedPoolUsage;
  ULONG TotalNamePoolUsage;
  ULONG TotalHandleTableUsage;
  ULONG HighWaterNumberOfObjects;
  ULONG HighWaterNumberOfHandles;
  ULONG HighWaterPagedPoolUsage;
  ULONG HighWaterNonPagedPoolUsage;
  ULONG HighWaterNamePoolUsage;
  ULONG HighWaterHandleTableUsage;
  ULONG InvalidAttributes;
  GENERIC_MAPPING GenericMapping;
  ULONG ValidAccess;
  BOOLEAN SecurityRequired;
  BOOLEAN MaintainHandleCount;
  USHORT MaintainTypeList;
  POOL_TYPE PoolType;
  ULONG PagedPoolUsage;
  ULONG NonPagedPoolUsage;
} OBJECT_TYPE_INFORMATION, *POBJECT_TYPE_INFORMATION;

// =============================================================================

PVOID GetLibraryProcAddress(PSTR LibraryName, PSTR ProcName) {
  return GetProcAddress(GetModuleHandleA(LibraryName), ProcName);
}

BOOL GetProcessFiles(HWND hwnd_process, vector<wstring>& files_vector) {
  _NtQuerySystemInformation NtQuerySystemInformation = reinterpret_cast<_NtQuerySystemInformation>
    (GetLibraryProcAddress("ntdll.dll", "NtQuerySystemInformation"));
  _NtDuplicateObject NtDuplicateObject = reinterpret_cast<_NtDuplicateObject>
    (GetLibraryProcAddress("ntdll.dll", "NtDuplicateObject"));
  _NtQueryObject NtQueryObject = reinterpret_cast<_NtQueryObject>
    (GetLibraryProcAddress("ntdll.dll", "NtQueryObject"));

  ULONG pid;
  HANDLE processHandle;
  GetWindowThreadProcessId(hwnd_process, &pid);
  if (!(processHandle = OpenProcess(PROCESS_DUP_HANDLE, FALSE, pid))) {
    return FALSE;
  }

  NTSTATUS status;
  ULONG handleInfoSize = 0x10000;
  PSYSTEM_HANDLE_INFORMATION_EX handleInfo;
  handleInfo = reinterpret_cast<PSYSTEM_HANDLE_INFORMATION_EX>(malloc(handleInfoSize));
  while ((status = NtQuerySystemInformation(SystemHandleInformationEx, handleInfo, handleInfoSize, NULL)) == STATUS_INFO_LENGTH_MISMATCH) {
    handleInfo = reinterpret_cast<PSYSTEM_HANDLE_INFORMATION_EX>(realloc(handleInfo, handleInfoSize *= 2));
    if (handleInfoSize > 16 * 1024 * 1024) return FALSE;
  }
  if (!NT_SUCCESS(status)) return FALSE;

  for (ULONG_PTR i = 0; i < handleInfo->HandleCount; i++) {
    SYSTEM_HANDLE_EX handle = handleInfo->Handles[i];
    HANDLE dupHandle = NULL;
    POBJECT_TYPE_INFORMATION objectTypeInfo;
    PVOID objectNameInfo;
    UNICODE_STRING objectName;
    ULONG returnLength;

    // Check if this handle belongs to the PID the user specified.
    if (reinterpret_cast<ULONG>(handle.ProcessId) != pid) {
      continue;
    }
    // Duplicate the handle so we can query it.
    if (!NT_SUCCESS(NtDuplicateObject(processHandle, handle.Handle, GetCurrentProcess(), &dupHandle, 0, 0, 0))) {
      continue;
    }
    // Query the object type.
    objectTypeInfo = reinterpret_cast<POBJECT_TYPE_INFORMATION>(malloc(0x1000));
    if (!NT_SUCCESS(NtQueryObject(dupHandle, ObjectTypeInformation, objectTypeInfo, 0x1000, NULL))) {
      CloseHandle(dupHandle);
      continue;
    }
    // Query the object name (unless it has an access of 0x0012019f, on which NtQueryObject could hang).
    if (handle.GrantedAccess == 0x0012019f) {
      free(objectTypeInfo);
      CloseHandle(dupHandle);
      continue;
    }
    objectNameInfo = malloc(0x1000);
    if (!NT_SUCCESS(NtQueryObject(dupHandle, ObjectNameInformation, objectNameInfo, 0x1000, &returnLength))) {
      // Reallocate the buffer and try again.
      objectNameInfo = realloc(objectNameInfo, returnLength);
      if (!NT_SUCCESS(NtQueryObject(dupHandle, ObjectNameInformation, objectNameInfo, returnLength, NULL))) {
        free(objectTypeInfo);
        free(objectNameInfo);
        CloseHandle(dupHandle);
        continue;
      }
    }

    // Cast our buffer into an UNICODE_STRING.
    objectName = *reinterpret_cast<PUNICODE_STRING>(objectNameInfo);
    // Print the information (if it is a file).
    if (objectName.Length && handle.ObjectTypeIndex == OBJECT_TYPE_FILE) {
      wstring buff = objectName.Buffer;
      files_vector.push_back(buff);
    }

    free(objectTypeInfo);
    free(objectNameInfo);
    CloseHandle(dupHandle);
  }

  free(handleInfo);
  CloseHandle(processHandle);
  return TRUE;
}

// =============================================================================

void ActivateWindow(HWND hwnd) {
  if (IsIconic(hwnd)) SendMessage(hwnd, WM_SYSCOMMAND, SC_RESTORE, NULL);
  if (!IsWindowVisible(hwnd)) ShowWindow(hwnd, SW_SHOWNORMAL);
  SetForegroundWindow(hwnd);
  BringWindowToTop(hwnd);
}

bool CheckInstance(LPCWSTR lpClassName) {
  INT_PTR hwnd_64 = reinterpret_cast<INT_PTR>(FindWindow(lpClassName, NULL)); // TEMP
  HWND hwnd = reinterpret_cast<HWND>(hwnd_64);
  if (reinterpret_cast<INT_PTR>(hwnd) != hwnd_64) {
    MessageBox(NULL, L"hwnd != hwnd_64", L"CheckInstance()", 0); // TEMP
  }
  if (IsWindow(hwnd)) {
    ActivateWindow(hwnd);
    FlashWindow(hwnd, TRUE);
  }
  return hwnd != NULL;
}

wstring GetWindowClass(HWND hwnd) {
  WCHAR buff[MAX_PATH];
  GetClassName(hwnd, buff, MAX_PATH);
  return buff;
}

wstring GetWindowTitle(HWND hwnd) {
  WCHAR buff[MAX_PATH];
  GetWindowText(hwnd, buff, MAX_PATH);
  return buff;
}

wstring GetWindowPath(HWND hwnd) {
  DWORD dwProcessId;
  DWORD dwSize = MAX_PATH;
  WCHAR buff[MAX_PATH];

  GetWindowThreadProcessId(hwnd, &dwProcessId);
  HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcessId);

  if (hProcess != NULL) {
    GetModuleFileNameEx(hProcess, NULL, buff, dwSize);
    CloseHandle(hProcess);
  }

  return buff;
}

bool IsFullscreen(HWND hwnd) {
  LONG style = GetWindowLong(hwnd, GWL_EXSTYLE);
  
  if (style & WS_EX_TOPMOST) {
    RECT rect; GetClientRect(hwnd, &rect);
    if (rect.right >= GetSystemMetrics(SM_CXSCREEN) &&
      rect.bottom >= GetSystemMetrics(SM_CYSCREEN)) {
        return true;
    }
  }
  
  return false;
}

bool TranslateDeviceName(wstring& path) {
  size_t pos = path.find('\\', 8);
  if (pos == wstring::npos) return false;
  wstring device_name = path.substr(0, pos);
  path = path.substr(pos);
  
  const int size = 1024;
  WCHAR szTemp[size] = {'\0'};
  if (!GetLogicalDriveStrings(size - 1, szTemp)) {
    return false;
  }
  
  WCHAR* p = szTemp;
  bool bFound = false;
  WCHAR szName[MAX_PATH];
  WCHAR szDrive[3] = L" :";
  
  do {
    *szDrive = *p;
    if (QueryDosDevice(szDrive, szName, MAX_PATH)) {
      if (device_name == szName) {
        device_name = szDrive;
        bFound = true;
      }
    }
    while (*p++);
  } while (!bFound && *p);

  path = device_name + path;
  return bFound;
}