; ============================================================================
;  SCPS — installateur Windows (NSIS)
;  Jeu de grande stratégie déterministe : front Godot + moteur C99 (libscps).
;  Génère SCPS-Setup.exe : installe scps.exe + la DLL du moteur + le LISEZMOI,
;  crée les raccourcis (menu Démarrer · bureau) et un désinstalleur propre.
; ============================================================================

Unicode true
SetCompressor /SOLID lzma

!define APPNAME      "SCPS"
!define APPVER       "2.0.0"
!define PUBLISHER    "SCPS"
!define DESC         "Grande stratégie déterministe — Sphères Culturelles & Perméabilité Systémique"
!define EXENAME      "scps.exe"
!define DLLNAME      "libscps.windows.template_release.x86_64.dll"
!define REGKEY       "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"

Name "${APPNAME} ${APPVER}"
!ifndef OUTFILE
  !define OUTFILE "SCPS-Setup.exe"
!endif
OutFile "${OUTFILE}"
InstallDir "$PROGRAMFILES64\${APPNAME}"
InstallDirRegKey HKLM "Software\${APPNAME}" "InstallDir"
RequestExecutionLevel admin   ; écrit dans Program Files + HKLM

!include "MUI2.nsh"
!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_RUN "$INSTDIR\${EXENAME}"
!define MUI_FINISHPAGE_RUN_TEXT "Lancer ${APPNAME}"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "French"
!insertmacro MUI_LANGUAGE "English"

; ---- Installation ----------------------------------------------------------
Section "SCPS (requis)" SecCore
  SectionIn RO
  SetOutPath "$INSTDIR"

  ; Le jeu (Godot, PCK embarqué) + le moteur déterministe (DOIT rester à côté)
  File "scps.exe"
  File "${DLLNAME}"
  File "LISEZMOI.txt"

  ; Clés de désinstallation (Ajout/Suppression de programmes)
  WriteRegStr HKLM "Software\${APPNAME}" "InstallDir" "$INSTDIR"
  WriteRegStr HKLM "${REGKEY}" "DisplayName"     "${APPNAME} — ${DESC}"
  WriteRegStr HKLM "${REGKEY}" "DisplayVersion"  "${APPVER}"
  WriteRegStr HKLM "${REGKEY}" "Publisher"       "${PUBLISHER}"
  WriteRegStr HKLM "${REGKEY}" "DisplayIcon"     "$INSTDIR\${EXENAME}"
  WriteRegStr HKLM "${REGKEY}" "UninstallString" "$INSTDIR\Uninstall.exe"
  WriteRegDWORD HKLM "${REGKEY}" "NoModify" 1
  WriteRegDWORD HKLM "${REGKEY}" "NoRepair" 1

  WriteUninstaller "$INSTDIR\Uninstall.exe"
SectionEnd

Section "Raccourci menu Démarrer" SecStartMenu
  CreateDirectory "$SMPROGRAMS\${APPNAME}"
  CreateShortcut  "$SMPROGRAMS\${APPNAME}\${APPNAME}.lnk"    "$INSTDIR\${EXENAME}"
  CreateShortcut  "$SMPROGRAMS\${APPNAME}\Désinstaller.lnk" "$INSTDIR\Uninstall.exe"
SectionEnd

Section "Raccourci bureau" SecDesktop
  CreateShortcut "$DESKTOP\${APPNAME}.lnk" "$INSTDIR\${EXENAME}"
SectionEnd

; ---- Descriptions des composants ------------------------------------------
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecCore}      "Le jeu SCPS et son moteur déterministe (requis)."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecStartMenu} "Ajoute SCPS au menu Démarrer."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecDesktop}   "Ajoute une icône SCPS sur le bureau."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

; ---- Désinstallation -------------------------------------------------------
; NB : les sauvegardes & rapports de bug vivent dans les données utilisateur
; (%APPDATA%\Godot\app_userdata\SCPS) — l'uninstall n'y touche PAS.
Section "Uninstall"
  Delete "$INSTDIR\scps.exe"
  Delete "$INSTDIR\${DLLNAME}"
  Delete "$INSTDIR\LISEZMOI.txt"
  Delete "$INSTDIR\Uninstall.exe"
  RMDir "$INSTDIR"

  Delete "$SMPROGRAMS\${APPNAME}\${APPNAME}.lnk"
  Delete "$SMPROGRAMS\${APPNAME}\Désinstaller.lnk"
  RMDir  "$SMPROGRAMS\${APPNAME}"
  Delete "$DESKTOP\${APPNAME}.lnk"

  DeleteRegKey HKLM "${REGKEY}"
  DeleteRegKey HKLM "Software\${APPNAME}"
SectionEnd
