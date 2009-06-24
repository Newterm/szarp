" Vim syntax file
" Language:	EPROM log file
" Maintainer:	Pawe³ Pa³ucha <pawel@praterm.com.pl>
" Last change:	2001 Oct 16
"
" $Id: elog.vim 1614 2004-07-20 05:14:24Z lucek $
" SZARP 2.1
" 
" Syntax file for EPROM log files.
"

syn clear

syn match unknownCharacter "[^ ]"

syn match author_id "v[0-9]" contained
syn match version "v[0-9]\{4}" contained contains=author_id
syn match date "[12][0-9]\{3}\.[01][0-9]\.[0-9]\{2} [012][0-9]:[0-9]\{2}:[0-9]\{2} v[0-9]\{4}" contains=version nextgroup=date
syn match desc "#[^#]*#" nextgroup=date
syn match program_kw "Program: " contained
syn match program "Program: [^ ]\+" nextgroup=descr,date contains=program_kw skipwhite
syn match eprom_version "[sk]*[0-9]\{4}" contained
syn match eprom "^Eprom: [sk]*[0-9]\{4}" contains=eprom_version nextgroup=program skipwhite

" Kolorki jakie s±, ka¿dy widzi - mo¿na zmieniaæ
if !exists("did_elog_syntax_inits")
  let did_elog_syntax_inits = 1
  hi link eprom		Keyword
  hi link eprom_version	Special
  hi link program	Constant
  hi link program_kw	Keyword
  hi link desc		PreProc
  hi link date		Comment
  hi link author_id	Type
  hi version ctermfg=DarkGreen guifg=DarkGreen 
endif

let b:current_syntax = "elog"

" vim: ts=8
