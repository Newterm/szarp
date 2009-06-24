" Vim syntax file
" Language:	*.cor files
" Maintainer:	Pawe³ Pa³ucha <pawel@praterm.com.pl>
" Last change:	2001 Sept 15
"
" $Id: cor.vim 149 2001-09-29 13:00:00Z pawel $
"
" Pliki z oknami definiowalnymi programu Szarp Draw
"   :au! Syntax cor so cor.vim
"   :set syntax=cor
"

syn clear

syn match rest "^.*$"
syn match numbers "^[0-9 ]*$"
syn match braces "[{}]" contained
syn match comments "^#.*" contains=braces

" Kolorki jakie s±, ka¿dy widzi - mo¿na zmieniaæ
if !exists("did_cor_syntax_inits")
  let did_cor_syntax_inits = 1
  hi link numbers	PreProc
  hi link comments	Comment
  hi link rest		Character
  hi link braces	Special

endif

let b:current_syntax = "cor"

" vim: ts=8
