; Taiga installer
;
; Build with:
;   iscc setup\Taiga.iss
;
; Optional defines:
;   /DArch=x64|arm64      Architecture name used in the output file name
;                         (default: x64)
;   /DArchAllowed=<value> ArchitecturesAllowed value; pass "arm64" when
;                         building the ARM64 installer (default: x64compatible)
;   /DStageDir=<path>     Directory containing Taiga.exe and the Qt runtime,
;                         relative to this script (default: ..\bin\Release)

#define AppName "Taiga"
#define AppExeName "Taiga.exe"
#define AppPublisher "erengy"
#define AppURL "https://taiga.moe"
#define AppGuid "DBFE75B5-58DC-4C51-8953-367AF9A1FF35"
#define RelationsFileName "anime-relations.txt"

#ifndef Arch
  #define Arch "x64"
#endif

#ifndef ArchAllowed
  #define ArchAllowed "x64compatible"
#endif

#ifndef StageDir
  #define StageDir "..\bin\Release"
#endif

; The executable carries the version from src/taiga/config.h, so there is no
; second copy of the version number to keep in sync.
#define StageDirFull AddBackslash(SourcePath) + StageDir
#define ExeVersion GetVersionNumbersString(AddBackslash(StageDirFull) + AppExeName)
; Drop the fourth version component (build number) while it is zero.
#if Copy(ExeVersion, Len(ExeVersion) - 1, 2) == ".0"
  #define AppVersion Copy(ExeVersion, 1, Len(ExeVersion) - 2)
#else
  #define AppVersion ExeVersion
#endif

; ------------------------------------------------------------------------------
; Setup

[Setup]
AppId={{{#AppGuid}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppURL}
AppSupportURL={#AppURL}
AppUpdatesURL={#AppURL}
VersionInfoVersion={#AppVersion}
VersionInfoCompany={#AppPublisher}
VersionInfoDescription={#AppName} Setup
VersionInfoProductName={#AppName}
VersionInfoProductVersion={#AppVersion}
DefaultDirName={userappdata}\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
LicenseFile=..\LICENSE
OutputDir=..\bin
OutputBaseFilename={#AppName}-{#AppVersion}-{#Arch}-Setup
SetupIconFile=..\src\resources\icons\taiga.ico
UninstallDisplayIcon={app}\{#AppExeName}
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
WizardImageFile=bitmap\wizard.bmp
WizardSmallImageFile=bitmap\header.bmp
; Install per user, like the NSIS-based installer did.
PrivilegesRequired=lowest
ArchitecturesAllowed={#ArchAllowed}
; The application is closed and restarted by the [Code] section below, which
; also implements the /NOCLOSE and /RUN options.
CloseApplications=no
RestartApplications=no

[Tasks]
Name: "startmenuicon"; Description: "Create a &Start Menu shortcut"; GroupDescription: "Additional shortcuts:"
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional shortcuts:"

[Files]
Source: "{#StageDirFull}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
; Bundled, but not installed by Setup: data\ holds the user's own files, and
; Setup removes the contents of any directory it created. InstallRelationsData
; copies this file next to them instead.
Source: "..\deps\anime-relations\{#RelationsFileName}"; Flags: dontcopy skipifsourcedoesntexist

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppExeName}"; Tasks: startmenuicon
Name: "{group}\Uninstall {#AppName}"; Filename: "{uninstallexe}"; Tasks: startmenuicon
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppExeName}"; Description: "Launch {#AppName}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
; Taiga keeps its settings, anime list and watch history in {app}\data. They
; belong to the user, so they are kept unless the user asks for them to be
; removed; see CurUninstallStepChanged below.
Type: dirifempty; Name: "{app}"

; ------------------------------------------------------------------------------
; Code

[Code]
const
  WM_CLOSE = $0010;
  AppWindowTitle = 'Taiga';
  AppCloseWaitMs = 5000;

var
  AppWasRunning: Boolean;
  NoCloseApp: Boolean;
  ForceRunApp: Boolean;
  PreviousInstall: Boolean;
  DeleteUserData: Boolean;

function HasCommandLineOption(const Option: String): Boolean;
var
  I: Integer;
begin
  Result := False;
  for I := 1 to ParamCount do
  begin
    if CompareText(ParamStr(I), Option) = 0 then
    begin
      Result := True;
      Exit;
    end;
  end;
end;

function AppWindow: HWND;
begin
  Result := FindWindowByWindowName(AppWindowTitle);
  if Result = 0 then
  begin
    Result := FindWindowByWindowName(AppWindowTitle + ' [debug]');
  end;
end;

function IsUpgrade(const Dir: String): Boolean;
begin
  Result :=
    RegKeyExists(HKCU, 'Software\Microsoft\Windows\CurrentVersion\Uninstall\{#AppGuid}_is1') or
    RegKeyExists(HKCU, 'Software\Microsoft\Windows\CurrentVersion\Uninstall\{#AppName}') or
    FileExists(AddBackslash(Dir) + '{#AppExeName}');
end;

function InitializeSetup: Boolean;
begin
  NoCloseApp := HasCommandLineOption('/NOCLOSE');
  ForceRunApp := HasCommandLineOption('/RUN');
  AppWasRunning := AppWindow() <> 0;

  Log(Format('Application running: %d', [Ord(AppWasRunning)]));

  Result := True;
end;

function WaitForAppToClose: Boolean;
var
  Waited: Integer;
begin
  Waited := 0;
  while (AppWindow() <> 0) and (Waited < AppCloseWaitMs) do
  begin
    Sleep(200);
    Waited := Waited + 200;
  end;

  Result := AppWindow() = 0;
end;

// Ask the application to close, then wait for it to exit. Returns False if it
// is still running and the installation should not continue.
function CloseRunningApp: Boolean;
begin
  Result := True;

  if NoCloseApp or (AppWindow() = 0) then
  begin
    Exit;
  end;

  Log('Taiga is running; asking it to close.');
  PostMessage(AppWindow(), WM_CLOSE, 0, 0);

  if WaitForAppToClose() then
  begin
    Log('Taiga closed.');
    // Give the process a moment to release its files before they are replaced.
    Sleep(1000);
    Exit;
  end;

  if WizardSilent then
  begin
    Result := False;
    Exit;
  end;

  while True do
  begin
    if MsgBox('Taiga is still running. Close it first, then click OK to continue setup.',
              mbError, MB_OKCANCEL) = IDCANCEL then
    begin
      Result := False;
      Exit;
    end;

    if WaitForAppToClose() then
    begin
      Exit;
    end;
  end;
end;

// Remove what the NSIS-based installer left behind when upgrading, so that a
// single, working entry remains in Programs and Features.
procedure RemovePreviousInstaller;
begin
  if FileExists(ExpandConstant('{app}\Uninstall.exe')) then
  begin
    DeleteFile(ExpandConstant('{app}\Uninstall.exe'));
  end;

  RegDeleteKeyIncludingSubkeys(HKCU, 'Software\Microsoft\Windows\CurrentVersion\Uninstall\{#AppName}');
  RegDeleteKeyIncludingSubkeys(HKCU, 'Software\{#AppName}');
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  // Capture this before any file is installed, so that the finish page can
  // tell a fresh installation from an upgrade. {app} is only available once
  // Setup has been initialized, which rules out InitializeSetup.
  PreviousInstall := IsUpgrade(ExpandConstant('{app}'));
  Log(Format('Previous install: %d', [Ord(PreviousInstall)]));

  if not CloseRunningApp() then
  begin
    Result := 'Taiga is still running. Close it, then run setup again.';
    Exit;
  end;

  Result := '';
end;

// data\ belongs to the user, so it must not be created by Setup: the
// uninstaller removes everything inside a directory that Setup created. Copy
// the bundled relations data in afterwards, and never overwrite a file that is
// already there.
procedure InstallRelationsData;
var
  Target: String;
begin
  Target := ExpandConstant('{app}\data\{#RelationsFileName}');
  if FileExists(Target) then
  begin
    Exit;
  end;

  ForceDirectories(ExpandConstant('{app}\data'));

  try
    ExtractTemporaryFile('{#RelationsFileName}');
  except
    Log('Anime relations data is not bundled.');
    Exit;
  end;

  if not CopyFile(ExpandConstant('{tmp}\{#RelationsFileName}'), Target, False) then
  begin
    Log('Could not copy ' + Target);
  end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  ErrorCode: Integer;
begin
  if CurStep = ssInstall then
  begin
    RemovePreviousInstaller();
  end;

  if CurStep = ssPostInstall then
  begin
    InstallRelationsData();
  end;

  if (CurStep = ssPostInstall) and WizardSilent then
  begin
    // Old versions of Taiga start the installer silently and pass /RUN when
    // they expect to be restarted. Restart only when that was asked for, or
    // when the application was running before the upgrade.
    if ForceRunApp or AppWasRunning then
    begin
      ShellExec('', ExpandConstant('{app}\{#AppExeName}'), '', ExpandConstant('{app}'),
                SW_SHOWNORMAL, ewNoWait, ErrorCode);
    end;
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  Hwnd: HWND;
begin
  if CurUninstallStep = usUninstall then
  begin
    Hwnd := AppWindow();
    if Hwnd <> 0 then
    begin
      Log('Taiga is running; asking it to close.');
      PostMessage(Hwnd, WM_CLOSE, 0, 0);
      if WaitForAppToClose() then
      begin
        Log('Taiga closed.');
        Sleep(1000);
      end;
    end;

    // The settings, anime list and watch history belong to the user, so they
    // survive an uninstall by default. Silent uninstalls keep them; only an
    // explicit answer in the interactive uninstaller removes them.
    if not UninstallSilent then
    begin
      DeleteUserData := MsgBox('Do you also want to delete your Taiga settings, anime list and watch history?'
                               + #13#10 + #13#10
                               + 'They are stored in:' + #13#10
                               + ExpandConstant('{app}\data') + #13#10 + #13#10
                               + 'If you keep them, they will be picked up again when you reinstall Taiga.',
                               mbConfirmation, MB_YESNO or MB_DEFBUTTON2) = IDYES;
    end;

    if DeleteUserData then
    begin
      Log('User data will be deleted.');
    end
    else
    begin
      Log('User data will be kept.');
    end;
  end;

  if (CurUninstallStep = usPostUninstall) and DeleteUserData then
  begin
    DelTree(ExpandConstant('{app}\data'), True, True, True);
    RemoveDir(ExpandConstant('{app}'));
  end;
end;

function NextButtonClick(CurPageID: Integer): Boolean;
begin
  Result := True;

  if CurPageID = wpSelectDir then
  begin
    if Pos(LowerCase(ExpandConstant('{commonpf}')), LowerCase(WizardForm.DirEdit.Text)) = 1 then
    begin
      if MsgBox('WARNING: Installing under Program Files may cause issues if you have User Account Control enabled on your system.'
                + #13#10 + #13#10
                + 'Taiga keeps its settings and database in the installation folder, so the folder must be writable.'
                + #13#10 + #13#10
                + 'The default installation folder is:' + #13#10
                + ExpandConstant('{userappdata}\{#AppName}'),
                mbConfirmation, MB_OKCANCEL) = IDCANCEL then
      begin
        Result := False;
      end;
    end;
  end;
end;

procedure CurPageChanged(CurPageID: Integer);
begin
  if (CurPageID = wpFinished) and (WizardForm.RunList <> nil) and
     (WizardForm.RunList.Items.Count > 0) then
  begin
    // Do not offer to start Taiga after a fresh install: only restart it when
    // it was running before the upgrade, or when this is an upgrade.
    WizardForm.RunList.Checked[0] := ForceRunApp or AppWasRunning or PreviousInstall;
  end;
end;
