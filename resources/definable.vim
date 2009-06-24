" Vim syntax file
" Language:	definable.cfg file
" Maintainer:	Pawe³ Pa³ucha <pawel@praterm.com.pl>
" Last change:	2001 Nov 6
"
" $Id: definable.vim 790 2003-06-23 15:06:47Z pawel $
"
" SZARP 2.1
" Pod¶wietlanie sk³adni dla pliku definable.cfg.
"

" Ten plik lepiej czyta siê od do³u ;-)

" Sprawdzana jest poprawno¶æ wyra¿enia w ONP (odwrotnej notacji polskiej). 
" Pomys³ polega na tym, ¿eby grupowaæ kolejne pary liczba - operator
" dwuargumentowy, przy dopuszczalnych zagnie¿d¿eniach. Na przyklad w 
" wyra¿eniu '5 4 3 * + 7 -' 5 zostawiamy na boku,  po czym grupujemy 4 z '+',
" 3 z '*' (zagnie¿d¿enie) a 7 z '-'. Je¿eli uda nam siê w ten sposób wszystko
" zgrupowaæ, to wyra¿enie jest poprawne (na stosie zostaje jedna liczba).
" Szczególnego traktowania wymagaj± nastêpuj±ce operatory:
" '&' - zamiana warto¶ci na stosie - ignorowany
" '!' - dublowanie warto¶ci na stosie - jest traktowany jak liczba
"
" Rzeczy nie obs³u¿one lub niesprawdzane :
" - nie sprawdzamy czy jest wystarczaj±ca ilo¶æ argumentów dla operatora '&'
" - nie obs³ugujê operatorów warunku '?f' oraz wywo³anie funkcji '$' - w tym
" drugim przypadku nie znamy ilo¶ci argumentów funkcji, w pierwszym przypadku
" wynika to z pewnych ograniczeñ Vim'a. Je¿eli w formule wyst±pi jeden z 
" tych operatorów (pod¶wietlany na czerwono), to formu³a jest kolorowana,
" ale NIE JEST SPRAWDZANA jej poprawno¶æ.

" Usuwamy ewentualne ¶mieci
syn clear

" Ten operator mo¿e wyst±piæ samodzielnie - nie zmienia wielko¶ci stosu.
syn match rpn_ignore_op "[ ]\+&" contained

" Czê¶æ wewnêtrzna formu³y - ³±czymy liczbê z pierwszym wystêpuj±cym po niej
" operatorem, pozwalaj±c na zagnie¿d¿anie. Operator '!' jest traktowany jako
" liczba.
syn region rpn_inner contained matchgroup=rpn_ind start="[ ]\+[0-9]\+" matchgroup=rpn_const start="[ ]\+#[-]\=[0-9]\+" matchgroup=operator start="[ ]\+!" end="[ ]\+[\+\-\/\*<>~^:N]" contains=rpn_inner,rpn_ignore_op

" A tu zaczynamy rozpoznawaæ formu³e w ONP - na pocz±tku musi byæ sta³a lub
" indeks w IPC, na koñcu koniec wiersza, a w ¶rodku czê¶æ wewnêtrzna formu³y
syn region rpn matchgroup=rpn_ind start="=[0-9]\+"lc=1 matchgroup=rpn_const start="=#[-]\=[0-9]\+"lc=1 end="[ ]*$" contains=rpn_inner 

" To siê w kilku miejscach przydaje - liczba na ¿ó³to.
syn match rpn_count "[0-9]\+" contained

" Tutaj obs³ugujemy formu³y zawieraj±ce warunki lub wywo³ania funkcji - 
" nie potrafimy sprawdziæ ich poprawno¶ci, wiêc kolorujemy tylko odpowiednie
" elementy: operatory, spacje, indeksy w IPC, sta³e i operatory specjalne
" ($ i ?f).
syn match rpn_ops "[\+\-\/\*!&<>~^:N]" contained
syn match rpn_ind "[0-9]\+" contained
syn match rpn_space "[ ]\+" contained
syn match rpn_const "#[-]\=[0-9]\+" contained
syn match rpn_spec_op "[ ]\+\(\$\)\|\(?f\)" contained
syn match rpn_special "\([^\$]\+\$.*$\)\|\([^?]\+?f.*$\)" contains=rpn_spec_op,rpn_const,rpn_ind,rpn_space,rpn_ops contained

" Kolejne (od ty³u) fragmenty linii z informacjami ptt.
syn match ptt_comm "#.*$"
syn match ptt_desc "[^#]\+" nextgroup=ptt_comm
syn match ptt_unit contained "\[[^\]]\+\]"
syn match ptt_colon contained ":"
syn match ptt_full "[^;]\+;" nextgroup=ptt_desc contains=ptt_unit,ptt_colon
syn match ptt_short "[^ ]\+[^ ]\+" nextgroup=ptt_full
syn match ptt_prec "[0-9]\+[ ]\+" nextgroup=ptt_short

" Je¶li jest jaka¶ inna linia, to jest to b³±d!
syn match error ".\+"

" Zaczynamy dopasowywanie informacji o ptt
syn match def_ptt "^def_par_ptt([0-9]\+)=" nextgroup=ptt_prec contains=rpn_count

" Zaczynamy dopasowywanie formu³y
syn match rpn_start "^def_par_formula([0-9]\+)=" nextgroup=rpn,rpn_special contains=rpn_count

" Linie z komentarzami
syn match comment "^#.*$" 
" Pierwsze dwie linie
syn match def "^def_\(\(offset\)\|\(count\)\)=[0-9]\+$" contains=rpn_count

" Kolorki
if !exists("did_definable_syntax_inits")
  let did_definable_syntax_inits = 1
  hi link rpn		Error
  hi link rpn_equal	Keyword
  hi link rpn_outer	PreProc
  hi link rpn_inner	Error
  hi link rpn_start	Special
  hi link rpn_count	Keyword
  hi link rpn_ignore_op	Operator
  hi link rpn_special	Error
  hi link rpn_spec_op	Special
  hi link rpn_ops	Operator
  hi link ptt_comm	Comment
  hi link ptt_desc 	rpn_ind
  hi link ptt_full	Constant
  hi link ptt_colon	Special
  hi link ptt_unit	Keyword
  hi link ptt_short	rpn_const
  hi link ptt_prec	PreProc
  hi link def_ptt	Special
  hi link def		Special
  hi rpn_const	ctermfg=DarkGreen guifg=DarkGreen
  hi rpn_ind	ctermfg=Green guifg=Green
endif

let b:current_syntax = "definable"

" vim: ts=8
