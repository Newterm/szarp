" Vim syntax file
" Language:	Szablony konfiguracji
" Maintainer:	Pawe³ Pa³ucha <pawel@praterm.com.pl>
" Last change:	2001 Sept 15
"
" $Id: szablon.vim 478 2002-10-01 19:00:46Z pawel $
"
"   :au! Syntax szablon so szablon.vim
"   :set syntax=szablon
"

syn clear

if ("version" >= 610)
	syn sync fromstart
else
	syn sync minlines=1000
endif

syn match comma "," contained
syn match g0 "^.\+$" contains=comma,g1,g2,g3,g4,g5 

syn match g5 "\([^,]*,\)\|\([^,]*$\)" nextgroup=g1 contains=comma contained
syn match g4 "\([^,]*,\)\|\([^,]*$\)" nextgroup=g5 contains=comma contained
syn match g3 "\([^,]*,\)\|\([^,]*$\)" nextgroup=g4 contains=comma contained
syn match g2 "\([^,]*,\)\|\([^,]*$\)" nextgroup=g3 contains=comma contained
syn match g1 "\([^,]*,\)\|\([^,]*$\)" nextgroup=g2 contains=comma contained


syn match def_ch_kw "def_channel" contained
syn match def_ch "^def_channel,[A-Z_][A-Z_0-9]*$" contains=def_ch_kw
syn match part_kw "part" contained
syn match part "^part .*$" contains=part_kw
syn match com_spec "\$Id: " contained
syn match comments "^#.*$" contains=com_spec

" Kolorki jakie s±, ka¿dy widzi - mo¿na zmieniaæ
if !exists("did_szablon_syntax_inits")
  let did_szablon_syntax_inits = 1
  hi comments	ctermfg=Brown	guifg=Brown	gui=bold
  hi com_spec	ctermfg=Red	guifg=Red	gui=bold
  hi part	ctermfg=DarkGreen	guifg=DarkGreen
  hi part_kw	ctermfg=Yellow	guifg=Yellow
  hi def_ch	ctermfg=Green	guifg=Green
  hi def_ch_kw	ctermfg=Yellow guifg=Yellow
  hi comma	ctermfg=Yellow guifg=Yellow
  hi g1 ctermfg=DarkBlue guifg=DarkBlue
  hi g3 ctermfg=Cyan guifg=Cyan
  hi g2 ctermfg=Blue guifg=Blue
  hi g4 ctermfg=Magenta guifg=Magenta
  hi g5 ctermfg=Red guifg=Red

endif

let b:current_syntax = "szablon"

" vim: ts=8
