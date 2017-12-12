; Modern UI
!include "MUI2.nsh"

; ------------------------------------------------------------------------------
; General

; Version number
!define VERSION_PATH "..\src\taiga\version.h"
!searchparse /file "${VERSION_PATH}" `#define TAIGA_VERSION_MAJOR ` VERSION_MAJOR
!searchparse /file "${VERSION_PATH}" `#define TAIGA_VERSION_MINOR ` VERSION_MINOR
!searchparse /file "${VERSION_PATH}" `#define TAIGA_VERSION_PATCH ` VERSION_PATCH

; Definitions
!define PRODUCT_EXE "Taiga.exe"
!define PRODUCT_NAME "Taiga"
!define PRODUCT_PUBLISHER "erengy"
!define PRODUCT_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}"
!define PRODUCT_WEBSITE "http://taiga.moe"

; Version information
VIAddVersionKey "CompanyName" "${PRODUCT_PUBLISHER}"
VIAddVersionKey "FileDescription" "${PRODUCT_NAME} Setup"
VIAddVersionKey "FileVersion" "${PRODUCT_VERSION}.0"
VIAddVersionKey "LegalCopyright" ""
VIAddVersionKey "ProductName" "${PRODUCT_NAME}"
VIAddVersionKey "ProductVersion" "${PRODUCT_VERSION}.0"
VIProductVersion "${PRODUCT_VERSION}.0"

; Uninstaller
!define UNINST_EXE "Uninstall.exe"
!define UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"

; Main settings
BrandingText "${PRODUCT_NAME} v${PRODUCT_VERSION}"
Name "${PRODUCT_NAME}"
OutFile "..\bin\${PRODUCT_NAME}Setup.exe"
SetCompressor /SOLID lzma

; Default installation folder
!define DEFAULT_INSTALL_DIR "$APPDATA\${PRODUCT_NAME}"
InstallDir "${DEFAULT_INSTALL_DIR}"
; Get installation folder from registry, if available
InstallDirRegKey HKCU "Software\${PRODUCT_NAME}" ""

; Request application privileges for Windows Vista and above
RequestExecutionLevel user

; ------------------------------------------------------------------------------
; Interface settings

; Icons
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\orange-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\orange-uninstall.ico"

; Page header
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "bitmap\header.bmp"
!define MUI_HEADERIMAGE_UNBITMAP "bitmap\header.bmp"

; Welcome/Finish page
!define MUI_WELCOMEFINISHPAGE_BITMAP "bitmap\wizard.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "bitmap\wizard.bmp"

; Abort warning
!define MUI_ABORTWARNING
!define MUI_UNABORTWARNING

; ------------------------------------------------------------------------------
; Page settings

; Components page
!define MUI_COMPONENTSPAGE_NODESC

; Directory page
!define MUI_DIRECTORYPAGE_TEXT_TOP "\
    WARNING: Installing under Program Files may cause issues if you have User Account Control enabled on your system.$\r$\n$\r$\n\
    The default installation folder is:$\r$\n${DEFAULT_INSTALL_DIR}"

; Finish page
!define MUI_FINISHPAGE_RUN "$INSTDIR\${PRODUCT_EXE}"
!define MUI_FINISHPAGE_LINK "Visit home page"
!define MUI_FINISHPAGE_LINK_LOCATION "${PRODUCT_WEBSITE}"

; ------------------------------------------------------------------------------
; Pages

; Installation pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\LICENSE"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

; Uninstallation pages
!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

; ------------------------------------------------------------------------------
; Language

; Default language
!insertmacro MUI_LANGUAGE "English"

; ------------------------------------------------------------------------------
; Install sections

Section "!${PRODUCT_NAME}" SECTION_DEFAULT
  ; Set section as read-only
  SectionIn RO

  ; Set properties
  SetOutPath "$INSTDIR"
  SetOverwrite on

  ; Wait for application to close
  Call CheckInstance

  ; Add files
  SetOutPath "$INSTDIR"
  File "..\bin\Release\Taiga.exe"
  SetOutPath "$INSTDIR\data\"
  File /r "..\data\"
  SetOutPath "$INSTDIR\data\db\"
  File "..\deps\data\anime-relations\anime-relations.txt"
  SetOutPath "$INSTDIR\data\db\season\"
  File /r "..\deps\data\anime-seasons\data\"

  ; Uninstaller
  WriteUninstaller "$INSTDIR\${UNINST_EXE}"
  WriteRegStr HKCU "${UNINST_KEY}" "DisplayName" "${PRODUCT_NAME}"
  WriteRegStr HKCU "${UNINST_KEY}" "UninstallString" "$INSTDIR\${UNINST_EXE}"
  WriteRegStr HKCU "${UNINST_KEY}" "DisplayIcon" "$INSTDIR\${PRODUCT_EXE}"
  WriteRegStr HKCU "${UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr HKCU "${UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEBSITE}"
  WriteRegStr HKCU "${UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
SectionEnd

Section "Start menu shortcuts" SECTION_START_MENU_SHORTCUTS
  CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"
  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk" "$INSTDIR\${PRODUCT_EXE}"
  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall.lnk" "$INSTDIR\${UNINST_EXE}"
SectionEnd

Section "Desktop shortcut" SECTION_DESKTOP_SHORTCUT
  CreateShortCut "$DESKTOP\${PRODUCT_NAME}.lnk" "$INSTDIR\${PRODUCT_EXE}"
SectionEnd

; ------------------------------------------------------------------------------
; Uninstall section

Section Uninstall
  ; Delete registry entries
  DeleteRegKey HKCU "${UNINST_KEY}"
  DeleteRegKey /ifempty HKCU "Software\${PRODUCT_NAME}"

  ; Delete start menu shortcuts
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk"
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall.lnk"
  RMDir "$SMPROGRAMS\${PRODUCT_NAME}"

  ; Delete desktop shortcut
  Delete "$DESKTOP\${PRODUCT_NAME}.lnk"

  ; Delete files
  Delete "$INSTDIR\${PRODUCT_EXE}"
  Delete "$INSTDIR\${UNINST_EXE}"
  RMDir /r "$INSTDIR\data\"
SectionEnd

; ------------------------------------------------------------------------------
; Callback functions

Function .onInit
  Call InitializeSections
FunctionEnd

Function .onInstSuccess
  Call SaveInstDir
  Call SaveSections
  Call RunProductExe
FunctionEnd

; ------------------------------------------------------------------------------
; Custom functions

Function CheckInstance
  Var /GLOBAL messageSent
  StrCpy $messageSent "false"
  Var /GLOBAL secondsWaited
  IntOp $secondsWaited 0 + 0

  checkInstanceLoop:
    System::Call "kernel32::OpenMutex(i 0x100000, b 0, t 'Taiga-33d5a63c-de90-432f-9a8b-f6f733dab258') i .R0"
    IntCmp $R0 0 skipInstanceCheck
    System::Call "kernel32::CloseHandle(i $R0)"

    IfSilent skipSendMessage ; Assuming the application is going to close itself
    StrCmp $messageSent "true" skipSendMessage
      FindWindow $R0 "TaigaMainW"
      SendMessage $R0 ${WM_DESTROY} 0 0
      StrCpy $messageSent "true"
      Goto skipMessageBox
    skipSendMessage:

    IntOp $secondsWaited $secondsWaited + 1
    IntCmp $secondsWaited 3 0 skipMessageBox 0 ; Display the message box after 3 seconds
      MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION "${PRODUCT_NAME} is still running. Close it first, then click OK to continue setup." IDOK skipMessageBox
      Quit
    skipMessageBox:

    Sleep 1000
    Goto checkInstanceLoop

  skipInstanceCheck:
FunctionEnd

Function SaveInstDir
  IfSilent 0 +2
  Return
  WriteRegStr HKCU "Software\${PRODUCT_NAME}" "" $INSTDIR
FunctionEnd

Function RunProductExe
  IfSilent +2
  Return
  Exec "$INSTDIR\${PRODUCT_EXE}"
FunctionEnd

Function InitializeSections
  !macro DeselectSection sectionId
    Push $0
    SectionGetFlags ${sectionId} $0
    IntOp $0 $0 ^ ${SF_SELECTED}
    SectionSetFlags ${sectionId} $0
    Pop $0
  !macroend

  !macro LoadSection sectionId
    Push $0
    SectionGetText ${sectionId} $0
    ClearErrors
    ReadRegDWORD $0 HKCU "Software\${PRODUCT_NAME}\Setup\Sections" "$0"
    IfErrors +2
    SectionSetFlags ${sectionId} $0
    Pop $0
  !macroend

  IfSilent 0 loadSections
  !insertmacro DeselectSection ${SECTION_START_MENU_SHORTCUTS}
  !insertmacro DeselectSection ${SECTION_DESKTOP_SHORTCUT}
  Return

  loadSections:
  !insertmacro LoadSection ${SECTION_START_MENU_SHORTCUTS}
  !insertmacro LoadSection ${SECTION_DESKTOP_SHORTCUT}
FunctionEnd

Function SaveSections
  IfSilent 0 +2
  Return

  !macro SaveSection sectionId
    Push $0
    Push $1
    SectionGetText ${sectionId} $0
    SectionGetFlags ${sectionId} $1
    WriteRegDWORD HKCU "Software\${PRODUCT_NAME}\Setup\Sections" "$0" $1
    Pop $1
    Pop $0
  !macroend

  !insertmacro SaveSection ${SECTION_START_MENU_SHORTCUTS}
  !insertmacro SaveSection ${SECTION_DESKTOP_SHORTCUT}
FunctionEnd
