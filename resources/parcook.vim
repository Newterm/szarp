" Vim syntax file
" Language:	parcook file
" Maintainer:	Pawe³ Pa³ucha <pawel@praterm.com.pl>
" Last change:	2001 Oct 1
"
" $Id: parcook.vim 1840 2004-10-08 20:48:56Z lucek $
"
" SZARP 2.1
" Pod¶wietlanie sk³adni dla pliku parcook.cfg.
"

" Usuwamy ewentualne ¶mieci
syn clear

syn sync lines=500

" Sekcja definicji parametrów definiowalnych
syn match err "."
syn match ok " "

syn match comm 	"#[^0-9].*$"
syn match const "\(#[-]\=[0-9]\+\)\|\(null\)"
syn match op "[*+-/\$\^N]"
syn match eq	"#[0-9]\+[ ]\+="
syn match ipcind "[0-9]\+"

" Linie z parametrami od demonów
syn match par_tty "[ ]\+/dev/tty[A-Z][0-9][0-9]*[ ]*.*$" contained
syn match par_d "[ ]\+/opt/szarp/bin/.\+dmn" nextgroup=par_tty contained
syn match par_s "[0-9]\+" nextgroup=par_dm contained
syn match par_nm "^[0-9]\+ " nextgroup=par_s contained
syn match par_start "^[0-9]\+ [0-9]\+ [0-9]\+$" contained
syn match par_end "^[0-9]\+$" contained
syn region pars start="^[0-9]\+ [0-9]\+ [0-9]\+$" end="^[0-9]\+$" keepend contains=par_nm,par_s,par_d,par_tty,par_start,par_end


" Kolorki jakie s±, ka¿dy widzi - mo¿na zmieniaæ
if !exists("did_parcook_syntax_inits")
  let did_parcook_syntax_inits = 1
  hi link pars		Error
  hi link err		Error
  hi link par_nm	Keyword
  hi link par_s		PreProc
  hi link par_start	Special
  hi link par_end	Special
  hi link comm		Comment
  hi link par_d		Character
  hi link par_tty	Type
  hi ipcind	ctermfg=Green		guifg=Green
  hi const	ctermfg=DarkGreen	guifg=DarkGreen
  hi link op		Operator
  hi link eq		Special

endif

let b:current_syntax = "parcook"

" vim: ts=8
