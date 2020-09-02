/*  Resources.Designer.cs $

 	   This file is part of the HandBrake source code.
 	   Homepage: <http://HandBrake.fr/>.
 	   It may be used under the terms of the GNU General Public License. */

!define PRODUCT_NAME "HandBrake Nightly"
!define PRODUCT_VERSION "Nightly"
!define PRODUCT_VERSION_NUMBER "Nightly"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\${PRODUCT_NAME}"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

;Required .NET framework for 4.7.1
!define MIN_FRAMEWORK_VER 461308  

SetCompressor lzma
ManifestDPIAware true

; Required for Github Actions (or local builds were inetc is not part of the installed NSIS)
; Extract inetc.zip to the HandBrake root directory into a folder called plugins.
!addplugindir /x86-ansi "..\..\..\..\..\..\plugins\Plugins\x86-ansi"

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

Name "${PRODUCT_NAME}"
OutFile "HandBrake-${PRODUCT_VERSION_NUMBER}-x86_64-Win_GUI.exe"

!include WordFunc.nsh
!insertmacro VersionCompare
!include LogicLib.nsh
!include x64.nsh

InstallDir "$PROGRAMFILES64\HandBrake Nightly"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show

Var InstallDotNET

Function .onInit

  IfSilent 0 +2
    SetShellVarContext all  

  ; Begin Only allow one version
  System::Call 'kernel32::CreateMutexA(i 0, i 0, t "myMutex") i .r1 ?e'
  Pop $R0

  StrCmp $R0 0 +3
  MessageBox MB_OK|MB_ICONEXCLAMATION "The installer is already running." /SD IDOK
  Abort

  ; Detect if the intsaller is running on Windows XP/Vista and abort if it is.
  ${IfNot} ${AtLeastWin7}
    MessageBox MB_OK "Windows 7 with Service Pack 1 or later is required in order to run HandBrake."
    Quit
  ${EndIf}

  ${IfNot} ${RunningX64}
    MessageBox MB_OK "HandBrake requires a 64bit version of Windows 7 SP1 or later to install. Your system has a 32bit version of Windows."
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

  ; Begin Check .NET version
  StrCpy $InstallDotNET "No"
  Call CheckFramework
     StrCmp $0 "1" +3
        StrCpy $InstallDotNET "Yes"
      MessageBox MB_OK|MB_ICONINFORMATION "${PRODUCT_NAME} requires that the Microsoft .NET Framework 4.8 is installed. The latest .NET Framework will be downloaded and installed automatically during installation of ${PRODUCT_NAME}." /SD IDOK
     Pop $0

  ; Get .NET if required
  ${If} $InstallDotNET == "Yes"
     SetDetailsView hide
     inetc::get /caption "Downloading Microsoft .NET Framework 4.8" /canceltext "Cancel" "https://go.microsoft.com/fwlink/?linkid=2088631" "$INSTDIR\dotnetfx.exe" /end
     Pop $1

     ${If} $1 != "OK"
           Delete "$INSTDIR\dotnetfx.exe"
           Abort "Installation cancelled, ${PRODUCT_NAME} requires the Microsoft .NET 4.8 Framework"
     ${EndIf}

     ExecWait "$INSTDIR\dotnetfx.exe"
     Delete "$INSTDIR\dotnetfx.exe"

     SetDetailsView show
  ${EndIf}
  
  ; Install Files
  File "*.exe"
  File "*.dll"
  File "*.template"
  File "*.config"
  File "HandBrake*.pdb"

 ; Copy the languages
  SetOutPath "$INSTDIR\de"
  SetOverwrite ifnewer
  File "de\*.*"

  SetOutPath "$INSTDIR\zh"
  SetOverwrite ifnewer
  File "zh\*.*"

  SetOutPath "$INSTDIR\es"
  SetOverwrite ifnewer
  File "es\*.*"

  SetOutPath "$INSTDIR\fr"
  SetOverwrite ifnewer
  File "fr\*.*"

  SetOutPath "$INSTDIR\ko"
  SetOverwrite ifnewer
  File "ko\*.*"

  SetOutPath "$INSTDIR\ru"
  SetOverwrite ifnewer
  File "ru\*.*"

  SetOutPath "$INSTDIR\tr"
  SetOverwrite ifnewer
  File "tr\*.*"

  SetOutPath "$INSTDIR\ja"
  SetOverwrite ifnewer
  File "ja\*.*"

  SetOutPath "$INSTDIR\pt-BR"
  SetOverwrite ifnewer
  File "pt-BR\*.*"

  SetOutPath "$INSTDIR\co"
  SetOverwrite ifnewer
  File "co\*.*"

  ; Copy the standard doc set into the doc folder
  SetOutPath "$INSTDIR\doc"
  SetOverwrite ifnewer
  File "doc\*.*"
  
  ; Start Menu Shortcut for All users.   
  SetShellVarContext all
  CreateDirectory "$SMPROGRAMS\HandBrake Nightly"
  CreateShortCut "$SMPROGRAMS\HandBrake Nightly\HandBrake Nightly.lnk" "$INSTDIR\HandBrake.exe"
SectionEnd

Section -AdditionalIcons
  CreateShortCut "$SMPROGRAMS\HandBrake Nightly\Uninstall.lnk" "$INSTDIR\uninst.exe"
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
  Delete "$INSTDIR\de\*.*"
  RMDir  "$INSTDIR\de"
  Delete "$INSTDIR\zh\*.*"
  RMDir  "$INSTDIR\zh"
  Delete "$INSTDIR\es\*.*"
  RMDir  "$INSTDIR\es"
  Delete "$INSTDIR\fr\*.*"
  RMDir  "$INSTDIR\fr"
  Delete "$INSTDIR\ko\*.*"
  RMDir  "$INSTDIR\ko"
  Delete "$INSTDIR\ru\*.*"
  RMDir  "$INSTDIR\ru"
  Delete "$INSTDIR\tr\*.*"
  RMDir  "$INSTDIR\tr"
  Delete "$INSTDIR\ja\*.*"
  RMDir  "$INSTDIR\ja"
  Delete "$INSTDIR\pt-BR\*.*"
  RMDir  "$INSTDIR\pt-BR"
  Delete "$INSTDIR\co\*.*"
  RMDir  "$INSTDIR\co"

  RMDir  "$INSTDIR"
   
  Delete "$SMPROGRAMS\HandBrake Nightly\Uninstall.lnk"
  Delete "$DESKTOP\HandBrake Nightly.lnk"
  Delete "$SMPROGRAMS\HandBrake Nightly\HandBrake Nightly.lnk"
  RMDir  "$SMPROGRAMS\HandBrake Nightly"

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
    CreateShortCut "$DESKTOP\HandBrake Nightly.lnk" "$INSTDIR\HandBrake.exe"
FunctionEnd

;Check for .NET framework
Function CheckFrameWork
  ; Magic numbers from http://msdn.microsoft.com/en-us/library/ee942965.aspx
    ClearErrors
    ReadRegDWORD $0 HKLM "SOFTWARE\Microsoft\NET Framework Setup\NDP\v4\Full" "Release"

    IfErrors NotDetected

    ${If} $0 >= MIN_FRAMEWORK_VER
        StrCpy $0 "1"
    ${Else}
		NotDetected:
		  StrCpy $0 "2"
    ${EndIf}

FunctionEnd
