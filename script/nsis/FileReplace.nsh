
; $Id: FileReplace.nsh 3022 2006-03-17 17:16:58Z reksio $
; NSIS function that replaces occurences of a string in file 
; with another string
; Copied from :
; http://nsis.sourceforge.net/Advanced_Replace_within_text_II

!ifndef _AdvReplaceInFile_nsh
!define _AdvReplaceInFile_nsh
 
;>>>>>> Function Junction BEGIN
;Original Written by Afrow UK
; Rewrite to Replace on line within text by rainmanx
; This version works on R4 and R3 of Nullsoft Installer
; It replaces whatever is in the line throughout the entire text matching it.
Function AdvReplaceInFile
	Exch $0 ;file to replace in
	Exch
	Exch $1 ;number to replace after
	Exch
	Exch 2
	Exch $2 ;replace and onwards
	Exch 2
	Exch 3
	Exch $3 ;replace with
	Exch 3
	Exch 4
	Exch $4 ;to replace
	Exch 4
	Push $5 ;minus count
	Push $6 ;universal
	Push $7 ;end string
	Push $8 ;left string
	Push $9 ;right string
	Push $R0 ;file1
	Push $R1 ;file2
	Push $R2 ;read
	Push $R3 ;universal
	Push $R4 ;count (onwards)
	Push $R5 ;count (after)
	Push $R6 ;temp file name
	;-------------------------------
	GetTempFileName $R6
	FileOpen $R1 $0 r ;file to search in
	FileOpen $R0 $R6 w ;temp file
	StrLen $R3 $4
	StrCpy $R4 -1
	StrCpy $R5 -1
	loop_read:
		ClearErrors
		FileRead $R1 $R2 ;read line
		IfErrors exit
		StrCpy $5 0
		StrCpy $7 $R2
	loop_filter:
		IntOp $5 $5 - 1
		StrCpy $6 $7 $R3 $5 ;search
		StrCmp $6 "" file_write2
		StrCmp $6 $4 0 loop_filter
		StrCpy $8 $7 $5 ;left part
		IntOp $6 $5 + $R3
		StrCpy $9 $7 "" $6 ;right part
		StrLen $6 $7
		StrCpy $7 $8$3$9 ;re-join
		StrCmp -$6 $5 0 loop_filter
		IntOp $R4 $R4 + 1
		StrCmp $2 all file_write1
		StrCmp $R4 $2 0 file_write2
		IntOp $R4 $R4 - 1
		IntOp $R5 $R5 + 1
		StrCmp $1 all file_write1
		StrCmp $R5 $1 0 file_write1
		IntOp $R5 $R5 - 1
	Goto file_write2
	file_write1:
		FileWrite $R0 $7 ;write modified line
	Goto loop_read
	file_write2:
		FileWrite $R0 $7 ;write modified line
	Goto loop_read
	exit:
	FileClose $R0
	FileClose $R1
	SetDetailsPrint none
	Delete $0
	Rename $R6 $0
	Delete $R6
	SetDetailsPrint both
	;-------------------------------
	Pop $R6
	Pop $R5
	Pop $R4
	Pop $R3
	Pop $R2
	Pop $R1
	Pop $R0
	Pop $9
	Pop $8
	Pop $7
	Pop $6
	Pop $5
	Pop $4
	Pop $3
	Pop $2
	Pop $1
	Pop $0
FunctionEnd

!endif ; _AdvReplaceInFile_nsh

; vim: set filetype=nsis :
