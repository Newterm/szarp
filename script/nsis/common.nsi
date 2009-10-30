; common.nsi
; Szablon skryptu do tworzenia instalki SZARP'a pod Windows.
;
; $Id: common.nsi 6199 2008-11-20 08:28:37Z reksio $
; 
; Pawe³ Pa³ucha <pawel@pratem.com.pl>
;
;--------------------------------

; Functions for PATH variable modifications
!include "path.nsh"

; use ModernUI

!include "MUI.nsh"

; The name of the installer
Name "SZARP"

; The file to write
OutFile "SzarpSetup.exe"

; The default installation directory
InstallDir "C:\Program Files\SZARP"


; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\SZARP" "Install_Dir"

; Previous install dir
Var PrevInstDir
Var PROCESS

;--------------------------------
; GUI settings

; display Praterm logo
; !define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "header.bmp"
!define MUI_HEADERIMAGE_UNBITMAP "header.bmp"
!define MUI_WELCOMEFINISHPAGE_BITMAP "welcome.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "welcome.bmp"
!define MUI_WELCOMEPAGE_TITLE \
"Witamy w kreatorze instalacji\r\n\
oprogramowania SZARP"
!define MUI_WELCOMEPAGE_TEXT \
"Ten kreator pomo¿e Ci zainstalowaæ oprogramowanie\r\n\
SZARP 3.1.\r\n\
\r\n\
Zalecane jest zamkniêcie wszystkich uruchomionych\r\n\
programów przed rozpoczêciem instalacji.\r\n\
\r\n\
Kliknij Dalej aby kontynuowaæ."

; warning before installation abort
!define MUI_ABORTWARNING

; no components description (currently, no components)
!define MUI_COMPONENTSPAGE_NODESC

; installer/uninstaller icons 
!define MUI_ICON "install.ico"
!define MUI_UNICON "uninstall.ico"

!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_TEXT "Uruchom aplikacje systemu SZARP"
!define MUI_FINISHPAGE_RUN_FUNCTION "LaunchLink"

;---------------------------------
; Functions

; variable for use in functions
Var FUNC_TEMP

; Used as pre- function for pages - abort page display if this is upgrade of
; existing program installation.
Function CheckPrevInstall
	StrCmp $PrevInstDir "" nobla
		Abort
	nobla:
FunctionEnd

; Delete Start Menu
Function un.DeleteStartMenu
	ReadRegStr $1 HKCU "Software\SZARP" "Start_Menu_Folder"
	Delete "$SMSTARTUP\Centrum Sterowania.lnk"
	Delete "$SMSTARTUP\Synchronizator danych.lnk"
	Delete "$SMSTARTUP\Szau.lnk"
	Delete "$SMSTARTUP\Automatyczna aktualizacja SZARP.lnk"
 	RMDir /r "$SMPROGRAMS\$1"	
FunctionEnd

Function DeleteStartMenu
	ReadRegStr $1 HKCU "Software\SZARP" "Start_Menu_Folder"
	Delete "$SMSTARTUP\Centrum Sterowania.lnk"
	Delete "$SMSTARTUP\Synchronizator danych.lnk"
	Delete "$SMSTARTUP\Szau.lnk"
	Delete "$SMSTARTUP\Automatyczna aktualizacja SZARP.lnk"
 	RMDir /r "$SMPROGRAMS\$1"	
FunctionEnd

; Delete animation icons
Function DeleteAnimIcons
	Delete "$PrevInstDir\*.png"
FunctionEnd

Function LaunchLink
  ExecShell "" "$SMSTARTUP\Centrum Sterowania.lnk"
  ExecShell "" "$SMSTARTUP\Synchronizator danych.lnk"
  ExecShell "" "$SMSTARTUP\Szau.lnk"
FunctionEnd

; Displays page with upgrade info.
Function UpgradePage
	StrCmp $PrevInstDir "" nobla
		!insertmacro MUI_HEADER_TEXT "Sprawdzanie poprzedniej wersji" \
			"Znaleziono istniej±c± instalacjê systemu SZARP w katalogu $PrevInstDir"
		!insertmacro MUI_INSTALLOPTIONS_DISPLAY "upgrade.ini"
  		!include KillProcUpgrade.nsh
		Call DeleteStartMenu
		Call DeleteAnimIcons
	nobla:
FunctionEnd

Function Kill
  
  StrCpy $0 $PROCESS
  KillProc::KillProcesses

  ;TODO: if not ($RO==0 || $R0==603)
  ;     MsgBox ...
  IntCmp $R0 0 killing_ok
  IntCmp $R0 603 killing_ok
  messagebox mb_ok|mb_iconexclamation "cannot kill process $process"
  killing_ok:
FunctionEnd

Function un.Kill
  
  StrCpy $0 $PROCESS
  KillProc::KillProcesses

  ;TODO: if not ($RO==0 || $R0==603)
  ;     MsgBox ...
  IntCmp $R0 0 killing_ok
  IntCmp $R0 603 killing_ok
  messagebox mb_ok|mb_iconexclamation "zabicie procesu $process by³o niemo¿liwe"
  killing_ok:
FunctionEnd

!include FileReplace.nsh
; Substitue __PREFIX and __INStALL_DIR__ in szarp.cfg and szarp_in.cfg
Function SetConfigFiles

	; replace __INSTALL_DIR__ and __PREFIX__
	Push "__INSTALL_DIR__"                     #text to be replaced
	Push "$INSTDIR"		   	           #replace with
	Push all                                   #replace all occurrences
	Push all                                   #replace all occurrences
	Push "$INSTDIR\resources\szarp.cfg"        #file to replace in
	Call AdvReplaceInFile  

	Push "__PREFIX__"                          #text to be replaced
	Push ""				     	   #replace with
	Push all                                   #replace all occurrences
	Push all                                   #replace all occurrences
	Push "$INSTDIR\resources\szarp.cfg"        #file to replace in
	Call AdvReplaceInFile  

	Push "__PREFIX__"                          #text to be replaced
	Push ""				     	   #replace with
	Push all                                   #replace all occurrences
	Push all                                   #replace all occurrences
	Push "$INSTDIR\resources\szarp_in.cfg"        #file to replace in
	Call AdvReplaceInFile  

	Push "__INSTALL_DIR__"                     #text to be replaced
	Push "$INSTDIR"				   #replace with
	Push all                                   #replace all occurrences
	Push all                                   #replace all occurrences
	Push "$INSTDIR\resources\szarp_in.cfg"        #file to replace in
	Call AdvReplaceInFile  


FunctionEnd 
;---------------------------------
; Pages to display


; Welcome page
!insertmacro MUI_PAGE_WELCOME
; License page
!insertmacro MUI_PAGE_LICENSE "license.pl"

; Upgrade info
Page custom UpgradePage

; Choosing directory
!define MUI_PAGE_CUSTOMFUNCTION_PRE CheckPrevInstall
!insertmacro MUI_PAGE_DIRECTORY

; Settings for StartMenu
Var STARTMENU_FOLDER
Var MUI_TEMP

;Start Menu Folder Page Configuration
!define MUI_STARTMENUPAGE_DEFAULTFOLDER "SZARP"
!define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKCU"
!define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\SZARP"
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start_Menu_Folder"

!define MUI_PAGE_CUSTOMFUNCTION_PRE CheckPrevInstall
!insertmacro MUI_PAGE_STARTMENU "SZARP" $STARTMENU_FOLDER

; Install files
!insertmacro MUI_PAGE_INSTFILES
; Finish page
!insertmacro MUI_PAGE_FINISH

; pages for uninstall
!define MUI_WELCOMEPAGE_TEXT \
"Kreator poprowadzi Ciê przez proces deinstalacji oprogramowania \
SZARP.\r\n\r\n\
Kliknij Anuluj, je¶li chcesz przerwaæ proces \
deinstalacji.\r\n\r\n\
Przed rozpoczêciem deinstalacji upewnij siê, czy nie s± uruchomione \
jakiekolwiek programy z pakietu SZARP. \
Kliknij Dalej aby kontynuowaæ."
!insertmacro MUI_UNPAGE_WELCOME


!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "Polish"
	    
;--------------------------------
;Reserve Files

;These files should be inserted before other files in the data block
;Keep these lines before any File command
;Only for solid compression (by default, solid compression is enabled
;      for BZIP2 and LZMA)

ReserveFile "upgrade.ini"
!insertmacro MUI_RESERVEFILE_INSTALLOPTIONS

	    
;--------------------------------

; The stuff to install
Section "SZARP (required)" MainSection

  SectionIn RO

  ReadRegDWORD $0 HKLM SOFTWARE\SZARP "Install_Count"
  IntCmp $0 0 no_progs
  	IfFileExists $INSTDIR\draw3.exe progs_installed
		WriteRegDWORD HKLM SOFTWARE\SZARP "Install_Count" 0
		Goto no_progs
  progs_installed:	
  	MessageBox MB_OK "Najpierw odinstaluj wszystkie programy systemu SZARP"
	Quit

  no_progs:

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Include file list
  !include FileList.nsh
  SetOutPath $INSTDIR
  
  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\SZARP "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Szarp" "DisplayName" "SZARP"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Szarp" "UninstallString" '"$INSTDIR\uninstallSzarp.exe"'
  WriteRegDWORD HKLM  "Software\Microsoft\Windows\CurrentVersion\Uninstall\Szarp" "NoModify" 1
  WriteRegDWORD HKLM  "Software\Microsoft\Windows\CurrentVersion\Uninstall\Szarp" "NoRepair" 1
  WriteUninstaller "uninstallSzarp.exe"

  Call SetConfigFiles

  ; Create StartMenu
  !insertmacro MUI_STARTMENU_WRITE_BEGIN "Szarp"
    CreateDirectory "$SMPROGRAMS\$STARTMENU_FOLDER"
    CreateShortCut \
    	"$SMPROGRAMS\$STARTMENU_FOLDER\Synchronizator danych.lnk" \
	"$INSTDIR\bin\SSC.EXE" \
	"" \
	"$INSTDIR\resources\icons\ssc.ico" \
	0 \
	"SW_SHOWNORMAL" \
	"" \
	"Aktualizacja danych"
    CreateShortCut \
    	"$SMPROGRAMS\$STARTMENU_FOLDER\Centrum Sterowania.lnk" \
	"$INSTDIR\bin\SCC.EXE" \
	"" \
	"$INSTDIR\resources\icons\scc.ico" \
	0 \
	"SW_SHOWNORMAL" \
	"" \
	"Uruchamianie aplikacji SZARP"
    CreateShortCut \
    	"$SMPROGRAMS\$STARTMENU_FOLDER\Automatyczna aktualizacja SZARP.lnk" \
	"$INSTDIR\bin\SZAU.EXE" \
	"" \
	"$INSTDIR\resources\icons\szau.ico" \
	0 \
	"SW_SHOWNORMAL" \
	"" \
	"Automatyczna aktualizacja SZARP"
    CreateShortCut \
    	"$SMPROGRAMS\$STARTMENU_FOLDER\Konfigurator regulatora Z-Elektronik.lnk" \
	"$INSTDIR\bin\SZAST.EXE" \
	"" \
	"$INSTDIR\resources\icons\szast.ico" \
	0 \
	"SW_SHOWNORMAL" \
	"" \
	"Ustawienia parametrów regulatorów"
    CreateShortCut \
    	"$SMPROGRAMS\$STARTMENU_FOLDER\Kontroler.lnk" \
	"$INSTDIR\bin\KONTROLER3.EXE" \
	"" \
	"$INSTDIR\resources\icons\kontroler3.ico" \
	0 \
	"SW_SHOWNORMAL" \
	"" \
	"Kontrola parametrów"
    CreateShortCut \
    	"$SMSTARTUP\Synchronizator danych.lnk" \
	"$INSTDIR\bin\SSC.EXE" \
	"" \
	"$INSTDIR\resources\icons\ssc.ico" \
	0 \
	"SW_SHOWNORMAL" \
	"" \
	"Aktualizacja danych"
    CreateShortCut \
    	"$SMSTARTUP\Centrum Sterowania.lnk" \
	"$INSTDIR\bin\SCC.EXE" \
	"" \
	"$INSTDIR\resources\icons\scc.ico" \
	0 \
	"SW_SHOWNORMAL" \
	"" \
	"Uruchamianie aplikacji SZARP"
    CreateShortCut \
    	"$SMSTARTUP\Automatyczna aktualizacja SZARP.lnk" \
	"$INSTDIR\bin\SZAU.EXE" \
	"" \
	"$INSTDIR\resources\icons\szau.ico" \
	0 \
	"SW_SHOWNORMAL" \
	"" \
	"Automatyczna aktualizacja SZARP"
  !insertmacro MUI_STARTMENU_WRITE_END
  
SectionEnd


;--------------------------------

; Uninstaller

Section "Uninstall"
;for old installer
  ReadRegDWORD $0 HKLM SOFTWARE\SZARP "Install_Count"
  IntCmp $0 0 no_progs
  	MessageBox MB_OK "Najpierw odinstaluj wszystkie programy systemu SZARP"
	Quit

  no_progs:
  
  !include KillProcUninstall.nsh
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Szarp"
  DeleteRegKey HKLM SOFTWARE\SZARP

  ; Remove files
  !include DeleteList.nsh
  ; Remove uninstaller
  Delete $INSTDIR\uninstallSzarp.exe
  ; Clean StartMenu
  Call un.DeleteStartMenu
 
  DeleteRegKey /ifempty HKCU "Software\SZARP"
  RMDir $INSTDIR

;  Delete "$SMPROGRAMS\$STARTMENU_FOLDER\SZARP.lnk
;Delete empty start menu parent diretories
;  StrCpy $MUI_TEMP "$SMPROGRAMS\$MUI_TEMP"
;  startMenuDeleteLoop:
;    ClearErrors
;    GetFullPathName $MUI_TEMP "$MUI_TEMP\.."
;    IfErrors startMenuDeleteLoopDone
;    StrCmp $MUI_TEMP $SMPROGRAMS startMenuDeleteLoopDone startMenuDeleteLoop
;  startMenuDeleteLoopDone:
; Remove directories used


SectionEnd

;--------------------

Function .onInit

  	SetShellVarContext all
	ReadRegStr $PrevInstDir HKLM "Software\SZARP" "Install_Dir"
	!insertmacro MUI_INSTALLOPTIONS_EXTRACT "upgrade.ini"

FunctionEnd

Function un.onInit

  	SetShellVarContext all

FunctionEnd
