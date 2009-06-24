" Vim syntax file
" Language:	Dyspozytor Config File (*.dsc)
" Maintainer:	Pawe³ Pa³ucha <pawel@praterm.com.pl>
" Last change:	2001 Oct 6
"
" $Id: dsc.vim 279 2002-02-22 00:02:44Z pawel $
" SZARP 2.1
" 
" Syntax file for dyspozytor config files.
"

syn clear

syn match unknownCharacter "^.*$"

syn match empty_line "^[ \t]*$"

syn match value +[ \t]"[^"]\+"[ \t]\+[-]\=[0-9]\++ contained contains=number
syn match brace "[{}]" contained
syn match number "[ \t][-]\=[0-9]\+" contained
syn match list " {[^}]*}" contained contains=number,value,comma,brace
syn match range "[ \t][-]\=[0-9]\+[ \t]*[-][ \t]*[-]\=[0-9]\+" contained contains=number
syn match ptt_line "^[ \t]*[0-9]\+" contained
syn match par_line "^[ \t]*[0-9]\+.*$" contains=ptt_line,range,ipc,base,comma,list,remote
syn match base "\([ \t]base\)\|\([ \t]nobase\)"
syn match ipc "\([ \t]ipc\)\|\([ \t]noipc\)" contained
syn match remote_kw "remote" contained
syn match ipc_ind "[0-9]\+" contained
syn match remote "[ \t]remote[ \t]\+[^ \t]\+[ \t]\+[0-9]\+" contained contains=remote_kw,ipc_ind
"syn match remote "remote" contained contains=remote_kw,ptt_line
syn match passwd_kw "[ \t]passwd" contained
syn match comma "," contained
syn match passwd "[ \t]passwd[^,]*" contained contains=passwd_kw
syn match group "^[ \t]*[^ :\t]\+:" contained
syn match group_line "^[ \t]*[^ :\t]\+:.*$" contains=group,passwd,ipc,base,comma
syn match title_kw "title" contained
syn match title_line +[ \t]*title[ ]\+\(\("[^"]*"\)\|\([^"][^ \t]*\)\)+ contains=title_kw
syn match super_kw "super" contained
syn match super_line "^[ \t]*super[ \t]\+[^ \t]*$" contains=super_kw
syn match comment "^[ \t]*#.*$"

" Kolorki jakie s±, ka¿dy widzi - mo¿na zmieniaæ
if !exists("did_dsc_syntax_inits")
  let did_dsc_syntax_inits = 1
  hi link remote_kw	Keyword
  hi link remote	Special
  hi link title_kw	Keyword
  hi link super_kw	Keyword
  hi link title_line	Special
  hi link super_line	Special
  hi link group		Type
  hi link passwd	Special
  hi link comma		Keyword
  hi link passwd_kw 	Keyword
  hi link ipc	 	Keyword
  hi link base	 	Keyword
  hi link ptt_line	PreProc
  hi link range		Operator
  hi link brace		Operator
  hi link value		Comment

  hi link first_line	Comment
  hi link second_line	Type
  hi link num_of_params Special
  hi link first_num	Keyword
  hi link sec_num	PreProc
  hi link ipc_ind	PreProc
  hi link short_name	Comment
  hi link full_name	Character
  hi link unit		Keyword
  hi link unknownCharacter	Error
endif

let b:current_syntax = "dsc"

" vim: ts=8
