# Avidemux WsQ installer (installs next to the official Avidemux, never over it)
#
# makensis -DSRCDIR=<folder with avidemux.exe> -DOUTFILE=<setup.exe> -DVERSION=<text> avidemux_wsq.nsi

Unicode True
SetCompressor /SOLID lzma

!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "x64.nsh"

!ifndef SRCDIR
  !define SRCDIR "../../avidemux64"
!endif
!ifndef OUTFILE
  !define OUTFILE "Avidemux_WsQ_setup.exe"
!endif
!ifndef VERSION
  !define VERSION "dev"
!endif

!define APPNAME   "Avidemux WsQ"
!define UNINSTKEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\AvidemuxWsQ"
!define MARKER    "avidemux_wsq.installed"

Name "${APPNAME}"
OutFile "${OUTFILE}"
RequestExecutionLevel admin
InstallDir "$PROGRAMFILES64\Avidemux WsQ"
InstallDirRegKey HKLM "${UNINSTKEY}" "InstallLocation"
BrandingText "${APPNAME} ${VERSION}"

!define MUI_ICON   "qtifw/config/avidemux.ico"
!define MUI_UNICON "qtifw/config/avidemux.ico"
!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_RUN "$INSTDIR\avidemux.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Uruchom ${APPNAME}"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "Polish"

Section "Install"
    SetRegView 64
    SetShellVarContext all

    SetOutPath "$INSTDIR"
    File /r "${SRCDIR}\*.*"
    FileOpen $0 "$INSTDIR\${MARKER}" w
    FileWrite $0 "${APPNAME} ${VERSION}"
    FileClose $0

    WriteUninstaller "$INSTDIR\uninstall.exe"

    CreateDirectory "$SMPROGRAMS\${APPNAME}"
    CreateShortcut "$SMPROGRAMS\${APPNAME}\${APPNAME}.lnk" "$INSTDIR\avidemux.exe"
    CreateShortcut "$SMPROGRAMS\${APPNAME}\Odinstaluj ${APPNAME}.lnk" "$INSTDIR\uninstall.exe"
    CreateShortcut "$DESKTOP\${APPNAME}.lnk" "$INSTDIR\avidemux.exe"

    WriteRegStr HKLM "${UNINSTKEY}" "DisplayName" "${APPNAME}"
    WriteRegStr HKLM "${UNINSTKEY}" "DisplayVersion" "${VERSION}"
    WriteRegStr HKLM "${UNINSTKEY}" "DisplayIcon" "$INSTDIR\avidemux.exe"
    WriteRegStr HKLM "${UNINSTKEY}" "InstallLocation" "$INSTDIR"
    WriteRegStr HKLM "${UNINSTKEY}" "UninstallString" "$\"$INSTDIR\uninstall.exe$\""
    WriteRegStr HKLM "${UNINSTKEY}" "Publisher" "wooshaq (based on Avidemux)"
    WriteRegDWORD HKLM "${UNINSTKEY}" "NoModify" 1
    WriteRegDWORD HKLM "${UNINSTKEY}" "NoRepair" 1
SectionEnd

Section "Uninstall"
    SetRegView 64
    SetShellVarContext all

    Delete "$SMPROGRAMS\${APPNAME}\${APPNAME}.lnk"
    Delete "$SMPROGRAMS\${APPNAME}\Odinstaluj ${APPNAME}.lnk"
    RMDir "$SMPROGRAMS\${APPNAME}"
    Delete "$DESKTOP\${APPNAME}.lnk"
    DeleteRegKey HKLM "${UNINSTKEY}"

    # Only wipe the folder if it is really our installation (protects against a badly chosen folder)
    ${If} ${FileExists} "$INSTDIR\${MARKER}"
        RMDir /r "$INSTDIR"
    ${Else}
        Delete "$INSTDIR\uninstall.exe"
        RMDir "$INSTDIR"
    ${EndIf}
SectionEnd

Function .onInit
    ${IfNot} ${RunningX64}
        MessageBox MB_ICONSTOP "${APPNAME} wymaga 64-bitowego Windows."
        Abort
    ${EndIf}
    SetRegView 64
FunctionEnd
