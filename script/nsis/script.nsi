; script.nsi
; Szablon skryptu do tworzenia instalki programów SZARP'a pod Windows.
; Wszelkie wyst±pienia napisu __NAME__ s± zastêpowane nazw± programu,
; a napisu __EXEC__ nazw± pliku z programem.
;
; $Id: script.nsi 5425 2008-04-05 14:33:49Z schylek $
; 
; Pawe³ Pa³ucha <pawel@pratem.com.pl>
;
;--------------------------------

; using our dll plugins
!addplugindir '.'

; use ModernUI
!include "MUI.nsh"

; to get advanced registry search
!include "Registry.nsh"

; The name of the installer
Name "SZARP __NAME__"

; The file to write
OutFile "__NAME__Setup.exe"

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\SZARP__NAME__" "Install_Dir"

;--------------------------------
; Variables

; Settings for StartMenu
Var STARTMENU_FOLDER
Var MUI_TEMP
Var USER

; Temporary variable
Var PAGE_SUBH
Var PROCESS
Var USERNR
Var Ybeg
Var Yend

;--------------------------------
; GUI settings

; display Praterm logo
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "header.bmp"
!define MUI_HEADERIMAGE_UNBITMAP "header.bmp"
!define MUI_WELCOMEFINISHPAGE_BITMAP "welcome.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "welcome.bmp"

; warning before installation abort
!define MUI_ABORTWARNING

; no components description (currently, no components)
!define MUI_COMPONENTSPAGE_NODESC

; installer/uninstaller icons 
!define MUI_ICON "install.ico"
!define MUI_UNICON "uninstall.ico"

;!define SzarpPrograms

;---------------------------------
; Functions

Function SetUsers
	; TODO: selecting users 
	;InstallOptions::initDialog /NOUNLOAD "user.ini"
	;Pop $0
	;InstallOptions::show
	;Pop $0
	;
	;ReadINIStr $USERNR "user.ini" "Settings" "NumFields" 
	; 
	;ri_loop:
	;	IntOp $USERNR $USERNR - 1 ;USERNR--
	;	ReadINIStr $0 "user.ini" "Field $USERNR" "State" 
	;	ReadINIStr $1 "user.ini" "Field $USERNR" "Text" 
	;	IntCmp $0 0 ri_loop ri_loop
	;
	;	StrCpy $USER "$USER $0"
	;	
	;	IntCmp $USERNR 0 ri_close ri_loop ri_loop
	;ri_close:
  	
	; give access premisions to $USER
	StrCpy $USER "(BU)" ; to all users
	!include FileAccessList.nsh
	


FunctionEnd

; Check for SZARP libraries instalation, abort if libraries are not installed.
Function CheckSzarp
	
	ReadRegStr $INSTDIR HKLM "Software\SZARP" "Install_Dir"
	ReadRegStr $STARTMENU_FOLDER HKCU "Software\SZARP" "Start Menu Folder"

	StrCpy $PAGE_SUBH "Nie znaleziono bibliotek SZARP"
	StrCmp $INSTDIR "" NoSzarp
	StrCpy $PAGE_SUBH "Uszkodzona instalacja bibliotek SZARP"
	StrCmp $STARTMENU_FOLDER "" NoStartMenu
		
	Goto Ok
	NoSzarp:
		!insertmacro MUI_HEADER_TEXT \
			"Sprawdzanie wymaganych komponentów" \
			$PAGE_SUBH
		!insertmacro MUI_INSTALLOPTIONS_DISPLAY "require.ini"
		Quit 
	Ok:
		WriteRegStr HKCU "Software\SZARP__NAME__" \
			"Start Menu Folder" $STARTMENU_FOLDER

	NoStartMenu:
	
FunctionEnd

Function un.Kill
  
  KillProcDLL::KillProc $PROCESS

  ;TODO: if not ($RO==0 || $R0==603)
  ;     MsgBox ...
  IntCmp $R0 0 killing_ok
  IntCmp $R0 603 killing_ok
  messagebox mb_ok|mb_iconexclamation "zabicie procesu $process by³o niemo¿liwe"
  killing_ok:
FunctionEnd




;---------------------------------
; Pages to display

; Welcome page
!insertmacro MUI_PAGE_WELCOME
; License page
!insertmacro MUI_PAGE_LICENSE "license.pl"

; Cheking for Szarp libraries
Page custom CheckSzarp

; Choosing users
Page custom SetUsers

; Install files
!insertmacro MUI_PAGE_INSTFILES
; Finish page
!insertmacro MUI_PAGE_FINISH

; pages for uninstall
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

ReserveFile "require.ini"
!insertmacro MUI_RESERVEFILE_INSTALLOPTIONS


;--------------------------------

; The stuff to install
Section "__NAME__ (required)" MainSection

	; TODO: write list of all users to ini file
	;IntOp $USERNR 0 + 0 ;USERNR=0 FIXME
	;IntOp $Ybeg 0 + 17 ;Ybeg=17 FIXME
	;IntOp $Yend 0 + 25 ;Yend=25 FIXME
	
	;wi_loop:
	;	${registry::Find} "$0" $1 $2 $3 $4
  	;	messagebox mb_ok "$2" ; kolejna nazwa u¿ytkownika
	;	; USERNR+/+
	;	IntOp $USERNR $USERNR + 1
	;	IntOp $Ybeg $Ybeg + 13 ;Ybeg+=13
	;	IntOp $Yend $Yend + 23 ;Yend+=13
	;	WriteINIStr "user.ini" "Field $USERNR" "Type" "checkbox"
	;	WriteINIStr "user.ini" "Field $USERNR" "Text" "$2"
	;	WriteINIStr "user.ini" "Field $USERNR" "State" "0"
	;	WriteINIStr "user.ini" "Field $USERNR" "Left" "10"
	;	WriteINIStr "user.ini" "Field $USERNR" "Right" "-10"
	;	WriteINIStr "user.ini" "Field $USERNR" "Top" "$Ybeg"
	;	WriteINIStr "user.ini" "Field $USERNR" "Bottom" "$Yend"
	;
	;	StrCmp $2 "" wi_close wi_loop
	;
	;wi_close:
	;WriteINIStr "user.ini" "Settings" "NumFields" "$USERNR"
	;${registry::Close} "$0"
	;${registry::Unload}

  SectionIn RO
  
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Include file list
  !include FileList.nsh
  SetOutPath $INSTDIR
  
  ; increment installation count
  ReadRegDWORD $0 HKLM SOFTWARE\SZARP "Install_Count"
  IntOp $0 $0 + 1
  WriteRegDWORD HKLM SOFTWARE\SZARP "Install_Count" $0


  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\SZARP___NAME__ "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\__NAME__" "DisplayName" "SZARP __NAME__"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\__NAME__" "UninstallString" '"$INSTDIR\uninstall__NAME__.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\__NAME__" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\__NAME__" "NoRepair" 1
  WriteUninstaller "uninstall__NAME__.exe"

  ; Create StartMenu
  StrCmp $STARTMENU_FOLDER "" NoCreate
  
    CreateDirectory "$SMPROGRAMS\$STARTMENU_FOLDER"
    CreateShortCut \
    	"$SMPROGRAMS\$STARTMENU_FOLDER\Odinstaluj __NAME__.lnk" \
    	"$INSTDIR\uninstall__NAME__.exe" \
	"" \
	"$INSTDIR\uninstall__NAME__.exe" \
	0 \
	"SW_SHOWNORMAL" \
	"" \
	"Odinstaluj __NAME__"
!ifdef SzarpPrograms
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
    	"$SMPROGRAMS\$STARTMENU_FOLDER\Szau.lnk" \
	"$INSTDIR\bin\SZAU.EXE" \
	"" \
	"$INSTDIR\resources\icons\szau.ico" \
	0 \
	"SW_SHOWNORMAL" \
	"" \
	"Automatyczna aktualizacja SZARP"
    CreateShortCut \
    	"$SMPROGRAMS\$STARTMENU_FOLDER\Centrum Sterowania.lnk" \
	"$INSTDIR\bin\SCC.EXE" \
	"" \
	"$INSTDIR\resources\icons\scc.ico" \
	0 \
	"SW_SHOWNORMAL" \
	"" \
	"Wybór miasta"
    CreateShortCut \
    	"$SMSTARTUP\Szau.lnk" \
	"$INSTDIR\bin\SZAU.EXE" \
	"" \
	"$INSTDIR\resources\icons\szau.ico" \
	0 \
	"SW_SHOWNORMAL" \
	"" \
	"Automatyczna aktualizacja SZARP"
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
	"Wybór miasta"
    CopyFiles "$INSTDIR\resources\icons\desktop.ini" \ 
    	"$SMPROGRAMS\$STARTMENU_FOLDER\desktop.ini"

!else
    CreateShortCut \
    	"$SMPROGRAMS\$STARTMENU_FOLDER\__NAME__.lnk" \
	"$INSTDIR\bin\__EXEC__" \
	"" \
	"$INSTDIR\bin\__EXEC__" \
	0 \
	"SW_SHOWNORMAL" \
	"" \
	"Uruchom __NAME__"
!endif
    NoCreate:
  
SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
  
  ; decrement installation count
  ReadRegDWORD $0 HKLM SOFTWARE\SZARP "Install_Count"
  IntOp $0 $0 - 1
  WriteRegDWORD HKLM SOFTWARE\SZARP "Install_Count" $0
  IntCmp $0 0 del ndel ndel
  del: ; if Install_Count == 0
  DeleteRegKey HKLM "SOFTWARE\SZARP\Install_Count"
  ndel: ; else

  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\__NAME__"
  DeleteRegKey HKLM "SOFTWARE\SZARP___NAME__"

  ; Remove files and uninstaller
  
 
 ; but first: kill running process
  !include KillProc.nsh

  !include DeleteList.nsh
  Delete $INSTDIR\uninstall__NAME__.exe

  ; Clean StartMenu
  ReadRegStr $MUI_TEMP HKCU "Software\SZARP__NAME__" "Start Menu Folder"

  StrCmp $MUI_TEMP "" startMenuDeleteLoopDone
  
!ifdef SzarpPrograms
  Delete "$SMPROGRAMS\$STARTMENU_FOLDER\DRAW3.lnk" 
  Delete "$SMPROGRAMS\$STARTMENU_FOLDER\SSC.lnk" 
  Delete "$SMPROGRAMS\$STARTMENU_FOLDER\SCC.lnk" 
  Delete "$SMPROGRAMS\$STARTMENU_FOLDER\SZAU.lnk" 
  Delete "$SMSTARTUP\SSC.lnk" 
  Delete "$SMSTARTUP\SCC.lnk" 
  Delete "$SMPROGRAMS\$STARTMENU_FOLDER\SZAU.lnk" 
!endif


  Delete "$SMPROGRAMS\$MUI_TEMP\Odinstaluj __NAME__.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\__NAME__.lnk"
  RMDir "$SMPROGRAMS\$MUI_TEMP"

  ;Delete empty start menu parent diretories
  StrCpy $MUI_TEMP "$SMPROGRAMS\$MUI_TEMP"

  startMenuDeleteLoop:
    ClearErrors
    GetFullPathName $MUI_TEMP "$MUI_TEMP\.."
    
    IfErrors startMenuDeleteLoopDone
    StrCmp $MUI_TEMP $SMPROGRAMS startMenuDeleteLoopDone startMenuDeleteLoop
  startMenuDeleteLoopDone:
    DeleteRegKey /ifempty HKCU "Software\SZARP__NAME__"
				    
  ; Remove directories used
  RMDir "$INSTDIR"

SectionEnd

;--------------------

Function .onInit

  	SetShellVarContext all
	!insertmacro MUI_INSTALLOPTIONS_EXTRACT "require.ini"
	
	${registry::Open} "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\DocFolderPaths" "/V=0" $0

FunctionEnd
