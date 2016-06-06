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

#include <windows.h>
#include <psapi.h>

#include "error.h"
#include "log.h"
#include "process.h"
#include "string.h"

#define NT_SUCCESS(x) ((x) >= 0)
#define STATUS_INFO_LENGTH_MISMATCH 0xc0000004

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

////////////////////////////////////////////////////////////////////////////////

static PSYSTEM_HANDLE_INFORMATION_EX GetSystemHandleInformation() {
  auto NtQuerySystemInformation = reinterpret_cast<_NtQuerySystemInformation>(
      GetLibraryProcAddress("ntdll.dll", "NtQuerySystemInformation"));

  PSYSTEM_HANDLE_INFORMATION_EX handleInfo = nullptr;

  ULONG handleInfoSize = 0x10000;
  NTSTATUS status = STATUS_INFO_LENGTH_MISMATCH;

  do {
    handleInfo = reinterpret_cast<PSYSTEM_HANDLE_INFORMATION_EX>(handleInfo ?
        realloc(handleInfo, handleInfoSize) : malloc(handleInfoSize));
    ZeroMemory(handleInfo, handleInfoSize);

    ULONG requiredSize = 0;
    status = NtQuerySystemInformation(
        SystemHandleInformationEx, handleInfo, handleInfoSize, &requiredSize);

    if (status == STATUS_INFO_LENGTH_MISMATCH) {
      if (requiredSize > handleInfoSize) {
        handleInfoSize = requiredSize;
      } else {
        handleInfoSize *= 2;
      }
    }
  } while (status == STATUS_INFO_LENGTH_MISMATCH &&
           handleInfoSize <= 16 * 0x100000);

  if (!NT_SUCCESS(status)) {
    free(handleInfo);
    return nullptr;
  }

  return handleInfo;
}

BOOL GetProcessFiles(ULONG process_id,
                     std::vector<std::wstring>& files_vector) {
  auto NtDuplicateObject = reinterpret_cast<_NtDuplicateObject>(
      GetLibraryProcAddress("ntdll.dll", "NtDuplicateObject"));
  auto NtQueryObject = reinterpret_cast<_NtQueryObject>(
      GetLibraryProcAddress("ntdll.dll", "NtQueryObject"));

  HANDLE processHandle = OpenProcess(PROCESS_DUP_HANDLE, FALSE, process_id);
  if (!processHandle)
    return FALSE;

  auto handleInfo = GetSystemHandleInformation();
  if (!handleInfo) {
    CloseHandle(processHandle);
    return FALSE;
  }

  // Type index for files varies between OS versions
  static unsigned short objectTypeFile = 0;
  /*
  switch (win::GetVersion()) {
    case win::kVersionVista:
      objectTypeFile = 25;
      break;
    case win::kVersion8:
    case win::kVersion8_1:
    case win::kVersion10:
      objectTypeFile = 31;
      break;
    default:
      objectTypeFile = 28;
      break;
  }
  */

  for (ULONG_PTR i = 0; i < handleInfo->HandleCount; i++) {
    SYSTEM_HANDLE_EX handle = handleInfo->Handles[i];
    NTSTATUS status;

    // Check if this handle belongs to the PID the user specified
    if (reinterpret_cast<ULONG>(handle.ProcessId) != process_id)
      continue;
    // Skip if the handle does not belong to a file
    if (objectTypeFile > 0 && handle.ObjectTypeIndex != objectTypeFile)
      continue;
    // Skip access codes that can cause NtDuplicateObject() or NtQueryObject()
    // to hang
    if (handle.GrantedAccess == 0x00100000 ||
        handle.GrantedAccess == 0x0012008d ||
        handle.GrantedAccess == 0x00120189 ||
        handle.GrantedAccess == 0x0012019f ||
        handle.GrantedAccess == 0x001a019f)
      continue;

    // Duplicate the handle so we can query it
    HANDLE dupHandle = nullptr;
    status = NtDuplicateObject(processHandle, handle.Handle,
                               GetCurrentProcess(), &dupHandle, 0, 0, 0);
    if (!NT_SUCCESS(status))
      continue;
    // Query the object type
    POBJECT_TYPE_INFORMATION objectTypeInfo =
        reinterpret_cast<POBJECT_TYPE_INFORMATION>(malloc(0x1000));
    status = NtQueryObject(dupHandle, ObjectTypeInformation, objectTypeInfo,
                           0x1000, nullptr);
    if (!NT_SUCCESS(status)) {
      free(objectTypeInfo);
      CloseHandle(dupHandle);
      continue;
    }
    // Determine the type index for files
    if (objectTypeFile == 0) {
      std::wstring type_name(objectTypeInfo->Name.Buffer,
                             objectTypeInfo->Name.Length / 2);
      if (IsEqual(type_name, L"File")) {
        objectTypeFile = handle.ObjectTypeIndex;
        LOG(LevelDebug, L"objectTypeFile is set to " +
                        ToWstr(objectTypeFile) + L".");
      } else {
        free(objectTypeInfo);
        CloseHandle(dupHandle);
        continue;
      }
    }

    base::ErrorMode error_mode(SEM_FAILCRITICALERRORS);

    ULONG returnLength;
    PVOID objectNameInfo = malloc(0x1000);
    status = NtQueryObject(dupHandle, ObjectNameInformation,
                           objectNameInfo, 0x1000, &returnLength);
    if (!NT_SUCCESS(status)) {
      if (returnLength <= 0x10000) {
        // Reallocate the buffer and try again
        objectNameInfo = realloc(objectNameInfo, returnLength);
        status = NtQueryObject(dupHandle, ObjectNameInformation,
                               objectNameInfo, returnLength, nullptr);
      }
      if (!NT_SUCCESS(status)) {
        free(objectTypeInfo);
        free(objectNameInfo);
        CloseHandle(dupHandle);
        continue;
      }
    }

    DWORD errorCode = GetLastError();
    if (errorCode != ERROR_SUCCESS)
      SetLastError(ERROR_SUCCESS);

    // Cast our buffer into a UNICODE_STRING
    UNICODE_STRING objectName = *reinterpret_cast<PUNICODE_STRING>(objectNameInfo);
    // Add file path to our list
    if (objectName.Length) {
      std::wstring object_name(objectName.Buffer, objectName.Length / 2);
      files_vector.push_back(object_name);
    }

    free(objectTypeInfo);
    free(objectNameInfo);
    CloseHandle(dupHandle);
  }

  free(handleInfo);
  CloseHandle(processHandle);
  return TRUE;
}

////////////////////////////////////////////////////////////////////////////////

bool CheckInstance(LPCWSTR mutex_name, LPCWSTR class_name) {
  if (CreateMutex(NULL, FALSE, mutex_name) == NULL ||
      GetLastError() == ERROR_ALREADY_EXISTS ||
      GetLastError() == ERROR_ACCESS_DENIED) {
    HWND hwnd = FindWindow(class_name, NULL);

    if (IsWindow(hwnd)) {
      ActivateWindow(hwnd);
      FlashWindow(hwnd, TRUE);
    }

    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////

void ActivateWindow(HWND hwnd) {
  if (IsIconic(hwnd))
    SendMessage(hwnd, WM_SYSCOMMAND, SC_RESTORE, NULL);

  if (!IsWindowVisible(hwnd))
    ShowWindow(hwnd, SW_SHOWNORMAL);

  SetForegroundWindow(hwnd);
  BringWindowToTop(hwnd);
}

std::wstring GetWindowClass(HWND hwnd) {
  WCHAR buff[MAX_PATH];
  GetClassName(hwnd, buff, MAX_PATH);
  return buff;
}

std::wstring GetWindowTitle(HWND hwnd) {
  WCHAR buff[MAX_PATH];
  GetWindowText(hwnd, buff, MAX_PATH);
  return buff;
}

std::wstring GetWindowPath(HWND hwnd) {
  DWORD dwProcessId;
  DWORD dwSize = MAX_PATH;
  WCHAR buff[MAX_PATH] = {'\0'};

  GetWindowThreadProcessId(hwnd, &dwProcessId);
  HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                                FALSE, dwProcessId);

  typedef DWORD (WINAPI *_QueryFullProcessImageName)(
      HANDLE hProcess, DWORD dwFlags, LPTSTR lpExeName, PDWORD lpdwSize);

  if (hProcess != NULL) {
    bool success = false;
    if (win::GetVersion() >= win::kVersionVista) {
      HMODULE hKernel32 = LoadLibrary(L"kernel32.dll");
      if (hKernel32 != NULL) {
        auto proc = reinterpret_cast<_QueryFullProcessImageName>(
            GetProcAddress(hKernel32, "QueryFullProcessImageNameW"));
        if (proc != NULL) {
          success = (proc)(hProcess, 0, buff, &dwSize) != 0;
        }
        FreeLibrary(hKernel32);
      }
    }
    if (!success) {
      GetModuleFileNameEx(hProcess, NULL, buff, dwSize);
    }
    CloseHandle(hProcess);
  }

  return buff;
}

bool IsFullscreen(HWND hwnd) {
  LONG style = GetWindowLong(hwnd, GWL_EXSTYLE);

  if (style & WS_EX_TOPMOST) {
    RECT rect;
    GetClientRect(hwnd, &rect);

    if (rect.right >= GetSystemMetrics(SM_CXSCREEN) &&
        rect.bottom >= GetSystemMetrics(SM_CYSCREEN))
      return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////

PVOID GetLibraryProcAddress(PSTR dll_module, PSTR proc_name) {
  return GetProcAddress(GetModuleHandleA(dll_module), proc_name);
}