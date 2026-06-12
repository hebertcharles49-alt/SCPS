; ============================================================================
;  SCPS — installateur Windows (NSIS)
;  Moteur de grande stratégie C99 : visualiseur SDL2 + outils headless.
;  Génère SCPS-Setup.exe : installe les binaires + DLL + police, crée les
;  raccourcis (menu Démarrer · bureau) et un désinstalleur propre.
; ============================================================================

Unicode true
SetCompressor /SOLID lzma

!define APPNAME      "SCPS"
!define APPVER       "1.2.0"
!define PUBLISHER    "SCPS"
!define DESC         "Moteur de grande stratégie — Sphères Culturelles & Perméabilité Systémique"
!define EXENAME      "scps_viewer.exe"
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

  ; Visualiseur (SDL2) + ses DLL + police bundlée
  File "scps_viewer.exe"
  File "SDL2.dll"
  File "SDL2_ttf.dll"
  File "DejaVuSans.ttf"
  ; Outils headless (la télémétrie est la preuve d'équilibre)
  File "chronicle.exe"
  File "core_demo.exe"
  File "LISEZMOI.txt"
  ; Texte joueur ÉDITABLE (FR par défaut) : édite-le ou traduis-le, F4 recharge
  ; à chaud. Absent → le jeu garde ses libellés compilés. (Généré par --dump-lang.)
  File "scps_lang.txt"
  ; Planche d'icônes (512×512, magenta transparent) — display-only, chargée au
  ; lancement ; absente → le visualiseur retombe sur ses glyphes vectoriels.
  File "scps_sprites.bmp"

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
  CreateShortcut  "$SMPROGRAMS\${APPNAME}\${APPNAME}.lnk"        "$INSTDIR\${EXENAME}"
  CreateShortcut  "$SMPROGRAMS\${APPNAME}\Désinstaller.lnk"     "$INSTDIR\Uninstall.exe"
SectionEnd

Section "Raccourci bureau" SecDesktop
  CreateShortcut "$DESKTOP\${APPNAME}.lnk" "$INSTDIR\${EXENAME}"
SectionEnd

; ---- Descriptions des composants ------------------------------------------
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecCore}      "Le visualiseur SCPS, ses DLL SDL2 et les outils headless (requis)."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecStartMenu} "Ajoute SCPS au menu Démarrer."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecDesktop}   "Ajoute une icône SCPS sur le bureau."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

; ---- Désinstallation -------------------------------------------------------
Section "Uninstall"
  Delete "$INSTDIR\scps_viewer.exe"
  Delete "$INSTDIR\chronicle.exe"
  Delete "$INSTDIR\core_demo.exe"
  Delete "$INSTDIR\SDL2.dll"
  Delete "$INSTDIR\SDL2_ttf.dll"
  Delete "$INSTDIR\DejaVuSans.ttf"
  Delete "$INSTDIR\LISEZMOI.txt"
  Delete "$INSTDIR\scps_lang.txt"
  Delete "$INSTDIR\scps_sprites.bmp"
  Delete "$INSTDIR\Uninstall.exe"
  ; les sauvegardes/captures éventuelles créées à côté de l'exe
  RMDir /r "$INSTDIR\saves"
  RMDir /r "$INSTDIR\screenshots"
  RMDir "$INSTDIR"

  Delete "$SMPROGRAMS\${APPNAME}\${APPNAME}.lnk"
  Delete "$SMPROGRAMS\${APPNAME}\Désinstaller.lnk"
  RMDir  "$SMPROGRAMS\${APPNAME}"
  Delete "$DESKTOP\${APPNAME}.lnk"

  DeleteRegKey HKLM "${REGKEY}"
  DeleteRegKey HKLM "Software\${APPNAME}"
SectionEnd
