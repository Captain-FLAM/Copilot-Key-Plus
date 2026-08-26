; Installateur Copilot Key+ (NSIS - https://nsis.sourceforge.io/)
;
; Aucun plugin tiers : MUI2, LogicLib, nsDialogs, Sections et x64.nsh font
; partie de la distribution standard de NSIS.
;
; Design "exe unique" : ce même exécutable sert d'installateur ET de
; réparateur/désinstalleur. En fin d'installation, il se recopie
; silencieusement dans le dossier de destination (à côté de l'exécutable de
; l'application) : ce fichier copié, et non plus le fichier téléchargé
; d'origine, est ensuite enregistré comme UninstallString (entrée Windows >
; Paramètres > Applications) et comme cible du raccourci "Désinstaller".
; L'utilisateur peut donc déplacer ou supprimer son fichier téléchargé sans
; casser la réparation/désinstallation. Quand on relance ce même exe,
; .onInit détecte l'installation existante (clé de registre) et affiche une
; page "Réparer / Désinstaller" au lieu du parcours normal.
; Trois volets : copie des fichiers, raccourcis et entrée de registre
; Applications (Section "-Programme" ci-dessous) ; configuration via
; "-config", entièrement gérée en interne par RunConfig() (voir WinMain /
; RunConfig, Sources/CopilotKey.c) ; désinstallation (Function
; RemoveInstallation ci-dessous). La fermeture de l'instance résidente avant
; réécriture de l'exe se fait via son propre flag "-quit" (Exec + Sleep),
; pas par un kill externe par nom de process (pas besoin de KillProcDLL ni
; de nsProcess pour ça).

; Champs repris automatiquement de CopilotKey.rc à chaque compilation, pour
; ne rien dupliquer/désynchroniser à la main (même valeurs que dans les
; propriétés de Copilot_Key+.exe lui-même) :

; - MyAppVersion (ex: "1.0") : chaîne "FileVersion" affichée (registre
;   Applications, propriétés du fichier).
; - MyAppVersionFull (ex: "1.0.0.0") : uniquement pour VIProductVersion /
;   VIFileVersion, qui exigent ce format X.X.X.X (contrainte Windows).
; - MyAppPublisher / MyAppCopyright : "CompanyName" / "LegalCopyright".
!searchparse /file "CopilotKey.rc" `VALUE "FileVersion", "` MyAppVersion `"`
!searchparse /file "CopilotKey.rc" `FILEVERSION ` MyAppVerMajor `,` MyAppVerMinor `,` MyAppVerBuild `,` MyAppVerRev
!searchparse /file "CopilotKey.rc" `VALUE "CompanyName", "` MyAppPublisher `"`
!searchparse /file "CopilotKey.rc" `VALUE "LegalCopyright", "` MyAppCopyright `"`

!define MyAppName "Copilot Key+"
!define MyAppVersionFull "${MyAppVerMajor}.${MyAppVerMinor}.${MyAppVerBuild}.${MyAppVerRev}"
!define MyAppExeName "Copilot_Key+.exe"
!define MyAppInstallerName "Copilot_Key+_Install.exe"
!define UninstKey "Software\Microsoft\Windows\CurrentVersion\Uninstall\CopilotKeyPlus"
!define SettingsKey "Software\CopilotKey+"
!define RunKey "Software\Microsoft\Windows\CurrentVersion\Run"
!define RunValueName "CopilotKey+"

!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "x64.nsh"
!include "nsDialogs.nsh"
!include "WinMessages.nsh"
!include "Sections.nsh"

Name "${MyAppName}"
OutFile "..\dist\${MyAppInstallerName}"
InstallDir "$PROFILE"
RequestExecutionLevel user
SetCompressor /SOLID lzma
Unicode true
ManifestDPIAware true
ShowInstDetails show

VIProductVersion "${MyAppVersionFull}"
VIFileVersion "${MyAppVersionFull}"

!define MUI_ICON "CopilotKey.ico"
!define MUI_UNICON "CopilotKey.ico"
!define MUI_ABORTWARNING

Page custom MaintenancePageCreate MaintenancePageLeave
!define MUI_LICENSEPAGE_RADIOBUTTONS
!insertmacro MUI_PAGE_LICENSE "..\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_COMPONENTS
Page custom SummaryPageCreate SummaryPageLeave
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_RESERVEFILE_LANGDLL
!insertmacro MUI_LANGUAGE "French"
!insertmacro MUI_LANGUAGE "English"

VIAddVersionKey /LANG=${LANG_FRENCH} "ProductName" "${MyAppName}"
VIAddVersionKey /LANG=${LANG_FRENCH} "ProductVersion" "${MyAppVersion}"
VIAddVersionKey /LANG=${LANG_FRENCH} "FileVersion" "${MyAppVersion}"
VIAddVersionKey /LANG=${LANG_FRENCH} "FileDescription" "Installateur ${MyAppName}"
VIAddVersionKey /LANG=${LANG_FRENCH} "CompanyName" "${MyAppPublisher}"
VIAddVersionKey /LANG=${LANG_FRENCH} "LegalCopyright" "${MyAppCopyright}"

VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductName" "${MyAppName}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductVersion" "${MyAppVersion}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileVersion" "${MyAppVersion}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileDescription" "${MyAppName} Installer"
VIAddVersionKey /LANG=${LANG_ENGLISH} "CompanyName" "${MyAppPublisher}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalCopyright" "${MyAppCopyright}"

LangString ErrorX64 ${LANG_FRENCH} "${MyAppName} nécessite Windows 64 bits."
LangString ErrorX64 ${LANG_ENGLISH} "${MyAppName} requires 64-bit Windows."

LangString TITLE_SecStartup ${LANG_FRENCH} "Lancer au démarrage"
LangString TITLE_SecStartup ${LANG_ENGLISH} "Launch the program when Windows starts"

LangString DESC_SecStartup ${LANG_FRENCH} "Lance ${MyAppName} automatiquement à l'ouverture de session Windows."
LangString DESC_SecStartup ${LANG_ENGLISH} "Launches ${MyAppName} automatically when you sign in to Windows."

LangString LnkLaunch ${LANG_FRENCH} "1 - Lancer ${MyAppName}"
LangString LnkLaunch ${LANG_ENGLISH} "1 - Launch ${MyAppName}"
LangString LnkConfig ${LANG_FRENCH} "2 - Configurer ${MyAppName}"
LangString LnkConfig ${LANG_ENGLISH} "2 - Configure ${MyAppName}"
LangString LnkQuit ${LANG_FRENCH} "3 - Arrêter ${MyAppName}"
LangString LnkQuit ${LANG_ENGLISH} "3 - Quit ${MyAppName}"
LangString LnkUninstall ${LANG_FRENCH} "4 - Désinstaller ${MyAppName}"
LangString LnkUninstall ${LANG_ENGLISH} "4 - Uninstall ${MyAppName}"

LangString SummaryPageTitle ${LANG_FRENCH} "Prêt à installer"
LangString SummaryPageTitle ${LANG_ENGLISH} "Ready to Install"
LangString SummaryPageSubtitle ${LANG_FRENCH} "Vérifiez vos choix avant de continuer."
LangString SummaryPageSubtitle ${LANG_ENGLISH} "Review your choices before continuing."
LangString SummaryLabelDestDir ${LANG_FRENCH} "Dossier de destination :"
LangString SummaryLabelDestDir ${LANG_ENGLISH} "Destination folder:"
LangString SummaryLabelStartup ${LANG_FRENCH} "Lancement au démarrage de Windows :"
LangString SummaryLabelStartup ${LANG_ENGLISH} "Launch at Windows startup:"
LangString SummaryYes ${LANG_FRENCH} "Oui"
LangString SummaryYes ${LANG_ENGLISH} "Yes"
LangString SummaryNo ${LANG_FRENCH} "Non"
LangString SummaryNo ${LANG_ENGLISH} "No"
LangString SummaryInstallButton ${LANG_FRENCH} "Installer"
LangString SummaryInstallButton ${LANG_ENGLISH} "Install"

LangString MaintenanceTitle ${LANG_FRENCH} "${MyAppName} est déjà installé"
LangString MaintenanceTitle ${LANG_ENGLISH} "${MyAppName} is already installed"
LangString MaintenanceSubtitle ${LANG_FRENCH} "Que voulez-vous faire ?"
LangString MaintenanceSubtitle ${LANG_ENGLISH} "What do you want to do?"
LangString MaintenanceText ${LANG_FRENCH} "Une installation de ${MyAppName} a été détectée sur ce compte. Réparez-la (réinstallation complète) ou désinstallez-la."
LangString MaintenanceText ${LANG_ENGLISH} "An installation of ${MyAppName} was found on this account. Repair it (full reinstall) or uninstall it."
LangString MaintenanceRepairBtn ${LANG_FRENCH} "Réparer"
LangString MaintenanceRepairBtn ${LANG_ENGLISH} "Repair"
LangString MaintenanceUninstallBtn ${LANG_FRENCH} "Désinstaller"
LangString MaintenanceUninstallBtn ${LANG_ENGLISH} "Uninstall"
LangString MaintenanceConfirmUninstall ${LANG_FRENCH} "Confirmez-vous la désinstallation de ${MyAppName} ?"
LangString MaintenanceConfirmUninstall ${LANG_ENGLISH} "Do you confirm you want to uninstall ${MyAppName}?"
LangString MaintenanceDone ${LANG_FRENCH} "${MyAppName} a été désinstallé."
LangString MaintenanceDone ${LANG_ENGLISH} "${MyAppName} has been uninstalled."

Function .onInit
  !insertmacro MUI_LANGDLL_DISPLAY
  ${IfNot} ${RunningX64}
    MessageBox MB_OK|MB_ICONSTOP "$(ErrorX64)"
    Abort
  ${EndIf}
FunctionEnd

; Déclarée avant "-Programme" ci-dessous car ce dernier référence ${SecStartup}
; (SectionGetFlags) : les constantes d'index de section NSIS ne sont résolues
; que si la section a déjà été déclarée plus haut dans le script.
Section "$(TITLE_SecStartup)" SecStartup
SectionEnd

Section "-Programme" SecMain
  ; Ferme une éventuelle instance résidente avant de (ré)écrire l'exécutable,
  ; sinon la copie échoue (fichier verrouillé par le processus en cours).
  IfFileExists "$INSTDIR\${MyAppExeName}" 0 +3
    Exec '"$INSTDIR\${MyAppExeName}" -quit'
    Sleep 500

  SetOutPath "$INSTDIR"
  File "..\Copilot_Key+.exe"

  CreateDirectory "$SMPROGRAMS\${MyAppName}"
  CreateShortCut "$SMPROGRAMS\${MyAppName}\$(LnkLaunch).lnk" "$INSTDIR\${MyAppExeName}" "" "$INSTDIR\${MyAppExeName}"
  CreateShortCut "$SMPROGRAMS\${MyAppName}\$(LnkConfig).lnk" "$INSTDIR\${MyAppExeName}" "-config" "$INSTDIR\${MyAppExeName}"
  CreateShortCut "$SMPROGRAMS\${MyAppName}\$(LnkQuit).lnk" "$INSTDIR\${MyAppExeName}" "-quit" "$INSTDIR\${MyAppExeName}"
  ; CreateShortCut "$SMPROGRAMS\${MyAppName}\$(LnkUninstall).lnk" "$INSTDIR\${MyAppInstallerName}" "" "$INSTDIR\${MyAppExeName}"

  ; Entrée Windows > Paramètres > Applications, pour pouvoir désinstaller
  ; sans passer par ce dossier à la main. Pointe vers la copie de
  ; l'installateur dans $INSTDIR (voir CopyFiles ci-dessus), relancée plus
  ; tard en mode maintenance (voir MaintenancePageCreate).
  WriteRegStr HKCU "${UninstKey}" "DisplayName" "${MyAppName}"
  WriteRegStr HKCU "${UninstKey}" "UninstallString" '"$INSTDIR\${MyAppInstallerName}"'
  WriteRegStr HKCU "${UninstKey}" "DisplayIcon" "$INSTDIR\${MyAppExeName}"
  WriteRegStr HKCU "${UninstKey}" "Publisher" "${MyAppPublisher}"
  WriteRegStr HKCU "${UninstKey}" "DisplayVersion" "${MyAppVersion}"
  WriteRegDWORD HKCU "${UninstKey}" "NoModify" 1
  WriteRegDWORD HKCU "${UninstKey}" "NoRepair" 1

  ; Reflète l'état actuel de la case "$(TITLE_SecStartup)" dans les deux sens
  ; (et pas seulement quand elle est cochée) : en Réparation, l'utilisateur
  ; peut très bien décocher une case qui était cochée à l'installation
  ; précédente, il faut alors retirer la clé de registre, pas juste ignorer
  ; le changement.
  SectionGetFlags ${SecStartup} $0
  IntOp $0 $0 & ${SF_SELECTED}
  ${If} $0 == ${SF_SELECTED}
    WriteRegStr HKCU "${RunKey}" "${RunValueName}" '"$INSTDIR\${MyAppExeName}"'
  ${Else}
    DeleteRegValue HKCU "${RunKey}" "${RunValueName}"
  ${EndIf}

  ; Copie l'installateur lui-même à côté de l'exécutable installé, en toute
  ; fin de processus, pour que la réparation/désinstallation (raccourci +
  ; entrée Applications ci-dessus) ne dépende plus du fichier téléchargé
  ; d'origine. Silencieux, pas besoin d'en informer l'utilisateur. Pas de
  ; copie si on tourne déjà depuis cet emplacement (cas Réparation lancée
  ; depuis la copie déjà installée).
  ${If} "$EXEPATH" != "$INSTDIR\${MyAppInstallerName}"
    CopyFiles /SILENT "$EXEPATH" "$INSTDIR\${MyAppInstallerName}"
  ${EndIf}

  ; Lance directement la capture de la touche Copilot et la configuration,
  ; sans page intermédiaire à bouton : RunConfig() (Sources/CopilotKey.c)
  ; gère seul l'arrêt d'une éventuelle instance résidente, la capture et la
  ; relance.
  Exec '"$INSTDIR\${MyAppExeName}" -config'
SectionEnd

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecStartup} $(DESC_SecStartup)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

; Supprime les fichiers, raccourcis et entrées de registre installés par
; Section "-Programme" ci-dessus. Appelée depuis la page de maintenance
; (voir MaintenanceUninstallClick).
Function RemoveInstallation
  IfFileExists "$INSTDIR\${MyAppExeName}" 0 +3
    Exec '"$INSTDIR\${MyAppExeName}" -quit'
    Sleep 500

  Delete "$INSTDIR\${MyAppExeName}"

  Delete "$SMPROGRAMS\${MyAppName}\*.lnk"
  RMDir "$SMPROGRAMS\${MyAppName}"
  DeleteRegValue HKCU "${RunKey}" "${RunValueName}"

  DeleteRegKey HKCU "${UninstKey}"
  DeleteRegKey HKCU "${SettingsKey}"

  ; /REBOOTOK : la copie de l'installateur (voir Section "-Programme") est en
  ; train de s'exécuter quand on arrive ici depuis le raccourci/l'entrée
  ; Applications, donc verrouillée ; Windows la supprimera au prochain
  ; redémarrage si la suppression immédiate échoue.
  Delete /REBOOTOK "$INSTDIR\${MyAppInstallerName}"
FunctionEnd

Var RepairButton
Var UninstallButton

; Page invisible (Abort immédiat) si aucune installation existante n'est
; détectée ; sinon propose Réparer (poursuit l'assistant normalement) ou
; Désinstaller (appelle RemoveInstallation puis quitte).
Function MaintenancePageCreate
  ReadRegStr $0 HKCU "${UninstKey}" "DisplayName"
  ${If} $0 == ""
    Abort
  ${EndIf}

  !insertmacro MUI_HEADER_TEXT "$(MaintenanceTitle)" "$(MaintenanceSubtitle)"
  nsDialogs::Create 1018
  Pop $0

  ${NSD_CreateLabel} 0 0 100% 30u "$(MaintenanceText)"
  Pop $0

  ${NSD_CreateButton} 15% 60u 30% 15u "$(MaintenanceRepairBtn)"
  Pop $RepairButton
  ${NSD_OnClick} $RepairButton MaintenanceRepairClick

  ${NSD_CreateButton} 55% 60u 30% 15u "$(MaintenanceUninstallBtn)"
  Pop $UninstallButton
  ${NSD_OnClick} $UninstallButton MaintenanceUninstallClick

  nsDialogs::Show
FunctionEnd

Function MaintenanceRepairClick
  ; Simule un clic sur "Suivant" pour poursuivre l'assistant normalement.
  SendMessage $HWNDPARENT ${WM_COMMAND} 1 0
FunctionEnd

Function MaintenanceUninstallClick
  MessageBox MB_YESNO|MB_ICONQUESTION "$(MaintenanceConfirmUninstall)" IDYES do_uninstall
  Return

  do_uninstall:
  Call RemoveInstallation
  MessageBox MB_OK|MB_ICONINFORMATION "$(MaintenanceDone)"
  Quit
FunctionEnd

Function MaintenancePageLeave
FunctionEnd

Function SummaryPageCreate
  !insertmacro MUI_HEADER_TEXT "$(SummaryPageTitle)" "$(SummaryPageSubtitle)"

  ; Renomme le bouton "Suivant" en "Installer" : c'est la dernière page
  ; avant la copie des fichiers.
  GetDlgItem $0 $HWNDPARENT 1
  SendMessage $0 ${WM_SETTEXT} 0 "STR:$(SummaryInstallButton)"

  nsDialogs::Create 1018
  Pop $0

  SectionGetFlags ${SecStartup} $1
  IntOp $1 $1 & ${SF_SELECTED}
  ${If} $1 == ${SF_SELECTED}
    StrCpy $2 "$(SummaryYes)"
  ${Else}
    StrCpy $2 "$(SummaryNo)"
  ${EndIf}

  ${NSD_CreateLabel} 0 0 100% 40u "$(SummaryLabelDestDir)$\r$\n    $INSTDIR$\r$\n$\r$\n$(SummaryLabelStartup)$\r$\n    $2"
  Pop $0

  nsDialogs::Show
FunctionEnd

Function SummaryPageLeave
FunctionEnd
