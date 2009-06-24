" Vim syntax file
" Language:	Raporter Pattern File
" Maintainer:	Pawe³ Pa³ucha <pawel@praterm.com.pl>
" Last change:	2001 Sept 15
"
" $Id: rap.vim 149 2001-09-29 13:00:00Z pawel $
" SZARP 2.1
" 
" Syntax file for raporter pattern files.
" To load it manually, use:
"   :au! Syntax rap so rap.vim
"   :set syntax=rap
"

syn clear

syn match unknownCharacter "^.*$"

syn match unit "\[[^\]]*\]" contained
syn match full_name ".*$" contains=unit
syn match short_name "[^ ]*" nextgroup=full_name skipwhite
syn match sec_num "[0-9]" nextgroup=short_name skipwhite
syn match first_num "^[0-9]*" nextgroup=sec_num skipwhite
syn match num_of_params "^[0-9]*" contained
syn match second_line "^[0-9]*[ ]*[a-zA-Z].*$" contains=num_of_params
syn match first_line "^#Raporter Pattern File#$"

" Kolorki jakie s±, ka¿dy widzi - mo¿na zmieniaæ
if !exists("did_rap_syntax_inits")
  let did_rap_syntax_inits = 1
  hi link first_line	Comment
  hi link second_line	Type
  hi link num_of_params Special
  hi link first_num	Keyword
  hi link sec_num	PreProc
  hi link short_name	Comment
  hi link full_name	Character
  hi link unit		Keyword
  hi link unknownCharacter	Error
endif

let b:current_syntax = "rap"

" vim: ts=8
