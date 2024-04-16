/*  Resources.Designer.cs $

 	   This file is part of the HandBrake source code.
 	   Homepage: <http://HandBrake.fr/>.
 	   It may be used under the terms of the GNU General Public License. */

!define PRODUCT_NAME "HandBrake"
!define PRODUCT_VERSION "1.8.0"
!define PRODUCT_VERSION_NUMBER "1.8.0"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\HandBrake.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

SetCompressor lzma
ManifestDPIAware true

; MUI 1.67 compatible ------
!include "MUI.nsh"
!include WinVer.nsh

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "HandBrakepineapple.ico"
!define MUI_UNICON "HandBrakepineapple.ico"
; GPL is not an EULA, no need to agree to it.
!define MUI_LICENSEPAGE_BUTTON $(^NextBtn)
!define MUI_LICENSEPAGE_TEXT_BOTTOM "You are now aware of your rights. Click Next to continue."

!define MUI_WELCOMEFINISHPAGE_BITMAP "InstallerBackground.bmp"
!define MUI_TEXT_WELCOME_INFO_TEXT "Setup will guide you through the installation of HandBrake.$\r$\n$\r$\nIt is recommended you close any running instances of HandBrake before running setup.$\r$\n$\r$\nWARNING: Before updating, please make sure that there are no pending encodes in the Queue. Please make sure you have backed up any presets and made a note of your settings.$\r$\n$\r$\nHandBrake requires Microsoft .NET *Desktop* Runtime 6. If this is not installed, you will be prompted to install it when you first run the app."
!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_TEXT "Create desktop shortcut (all users)"
!define MUI_FINISHPAGE_RUN_FUNCTION "desktopShortcut"

; Welcome page
!insertmacro MUI_PAGE_WELCOME
; License page
!insertmacro MUI_PAGE_LICENSE "doc\COPYING"
; Directory page
!insertmacro MUI_PAGE_DIRECTORY
; Instfiles page
!insertmacro MUI_PAGE_INSTFILES
; Finish page
;!define MUI_FINISHPAGE_RUN "$INSTDIR\HandBrake.exe"
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "English"

; MUI end ------

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "HandBrake-${PRODUCT_VERSION_NUMBER}-x86_64-Win_GUI.exe"

!include WordFunc.nsh
!insertmacro VersionCompare
!include LogicLib.nsh
!include x64.nsh

InstallDir "$PROGRAMFILES64\HandBrake"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show

Function .onInit

  ; For Silent Installs, Assume All Users
  IfSilent 0 +2
    SetShellVarContext all  

  ; Begin Only allow one version
  System::Call 'kernel32::CreateMutexA(i 0, i 0, t "myMutex") i .r1 ?e'
  Pop $R0

  StrCmp $R0 0 +3
  MessageBox MB_OK|MB_ICONEXCLAMATION "The installer is already running." /SD IDOK
  Abort

  ; Detect if the installer is running on an unsupported version of windows.
  ${IfNot} ${AtLeastBuild} 10240
    MessageBox MB_OK "Windows 10 or later is required in order to run HandBrake. Old versions of HandBrake can be downloaded from handbrake.fr "
    Quit
  ${EndIf}

  ${IfNot} ${RunningX64}
    MessageBox MB_OK "HandBrake requires a 64bit version of Windows 10 or later to install. Your system has a 32bit version of Windows."
    Quit
  ${EndIf}

  ;Remove previous version
  ReadRegStr $R0 HKLM \
  "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}\" \
  "UninstallString"
  StrCmp $R0 "" done

  MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION \
  "${PRODUCT_NAME} is already installed. $\n$\nClick `OK` to remove the \
  previous version or `Cancel` to continue." /SD IDOK \
  IDOK uninst
  goto done

 ;Run the uninstaller
  uninst:
   CopyFiles /SILENT /FILESONLY "$INSTDIR\uninst.exe" "$TEMP\uninstallhb.exe"
   IfSilent +3
   ExecWait '"$TEMP\uninstallhb.exe" _?=$INSTDIR'
   goto done
   ExecWait '"$TEMP\uninstallhb.exe" _?=$INSTDIR /S'
  done:
FunctionEnd

Section "HandBrake" SectionApp
  SetOutPath "$INSTDIR"
  SetOverwrite ifnewer
  SectionIn RO ; Read only, always installed

  ; Install Files
  File "*.exe"
  File "*.dll"
  File "*.template"

  ; Copy the standard doc set into the doc folder
  SetOutPath "$INSTDIR\doc"
  SetOverwrite ifnewer
  File "doc\*.*"
  
  ; Start Menu Shortcut for All users.   
  SetShellVarContext all
  CreateDirectory "$SMPROGRAMS\HandBrake"
  CreateShortCut "$SMPROGRAMS\HandBrake\HandBrake.lnk" "$INSTDIR\HandBrake.exe"
SectionEnd

Section -AdditionalIcons
  CreateShortCut "$SMPROGRAMS\HandBrake\Uninstall.lnk" "$INSTDIR\uninst.exe"
SectionEnd

Section -Post
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\HandBrake.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\HandBrake.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
SectionEnd

Function un.onUninstSuccess
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) was successfully removed from your computer." /SD IDOK
FunctionEnd

Function un.onInit

  ; For Silent Installs, Assume All Users
  IfSilent 0 +2
    SetShellVarContext all  

  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely remove $(^Name) and all of its components?" /SD IDYES IDYES +2
  Abort
FunctionEnd

Section Uninstall
  Delete "$INSTDIR\uninst.exe"
  
  Delete "$INSTDIR\*.*"
  Delete "$INSTDIR\doc\*.*"
  RMDir  "$INSTDIR\doc"

  RMDir  "$INSTDIR"

  Delete "$SMPROGRAMS\HandBrake\Uninstall.lnk"
  Delete "$DESKTOP\HandBrake.lnk"
  Delete "$SMPROGRAMS\HandBrake\HandBrake.lnk"
  RMDir  "$SMPROGRAMS\HandBrake"
  RMDir  "$INSTDIR"

  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  
  SetShellVarContext all
  Delete "$SMPROGRAMS\HandBrake Nightly\Uninstall.lnk"
  Delete "$SMPROGRAMS\HandBrake Nightly\HandBrake Nightly.lnk"
  RMDir  "$SMPROGRAMS\HandBrake Nightly"
  Delete "$DESKTOP\HandBrake Nightly.lnk"
  
  SetAutoClose true
SectionEnd

Function "desktopShortcut"
    SetShellVarContext all
    CreateShortCut "$DESKTOP\HandBrake.lnk" "$INSTDIR\HandBrake.exe"
FunctionEnd
