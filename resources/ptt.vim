" Vim syntax file
" Language:	PTT.act file
" Maintainer:	Pawe³ Pa³ucha <pawel@praterm.com.pl>
" Last change:	2001 Sept 15
"
" $Id: ptt.vim 151 2001-10-01 21:25:42Z pawel $
"
" Pod¶wietlanie sk³adni dla pliku PTT.act
"   :au! Syntax ptt so ptt.vim
"   :set syntax=ptt
"

" Usuwamy ewentualne ¶mieci
syn clear

syn match unknownCharacter "^.*$"

syn match comm "#[^)]*)"
syn match param_def ".*$" contains=comm
syn match unit "\[[^\]]*\]" contained
syn match semi_colon ";" nextgroup=param_def
syn match full_name "[^;]*" nextgroup=semi_colon contains=colons,unit
syn match colons ":" contained
syn match short_name "[^ ]*" nextgroup=full_name  skipwhite
syn match sec_num "[0-9]" nextgroup=short_name skipwhite
syn match first_num "^[0-9]*" nextgroup=sec_num skipwhite
syn match first_line "^[0-9]*[ ]*[0-9 ]*$"

" Kolorki jakie s±, ka¿dy widzi - mo¿na zmieniaæ
if !exists("did_ptt_syntax_inits")
  let did_ptt_syntax_inits = 1
  hi link first_line	Special
  hi link first_num	Keyword
  hi link sec_num	PreProc
  hi link comm		Comment
  hi link full_name	Character
  hi link semi_colon	Character
  hi link colons	Special
  hi link unit		Keyword
  hi link param_def	Type
  hi short_name ctermfg=DarkGreen guifg=DarkGreen

endif

let b:current_syntax = "ptt"

" vim: ts=8
