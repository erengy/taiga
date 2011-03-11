/*
** Copyright (c) 2011, Eren Okka
** 
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
** 
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
** 
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
** THE SOFTWARE.
*/

#include <windows.h>

// Usage example:
// UpdateHelper.exe App.exe App.exe.new [Process ID]

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd) {
  // argv[0]: Application path (not used)
  // argv[1]: Old file
  // argv[2]: New file
  // argv[3]: Process ID (optional)
  int argc = 0;
  LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
  if (argc < 3 || argv == NULL) return 1;

  // Wait for the application to close
  DWORD dwProcessId = argc > 3 ? _wtoi(argv[3]) : 0;
  if (dwProcessId) {
    // Wait until the process is no longer available
    HANDLE hProcess = NULL;
    do {
      hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dwProcessId);
      if (hProcess) {
        CloseHandle(hProcess);
        Sleep(100);
      }
    } while (hProcess != NULL);
  } else {
    // Wait a second
    Sleep(1000);
  }
  
  // Take a backup of the old file (App.exe -> App.exe.bak)
  WCHAR backup[MAX_PATH] = {'\0'};
  wcscpy_s(backup, MAX_PATH, argv[1]);
  wcscat_s(backup, MAX_PATH, L".bak");
  while (!MoveFileW(argv[1], backup)) Sleep(100);
  
  // Rename the new file (App.exe.new -> App.exe)
  if (MoveFileW(argv[2], argv[1])) {
    // Execute the new file
    ShellExecuteW(NULL, L"open", argv[1], NULL, NULL, SW_SHOW);
    // Delete the backup
    DeleteFileW(backup);
  } else {
    MessageBoxW(NULL, L"An error occured while updating.", NULL, MB_ICONERROR | MB_OK);
    // Revert to backup
    MoveFileW(backup, argv[1]);
  }

  return 0;
}