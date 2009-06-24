" $Id: szarp.vim 4588 2007-11-02 12:20:46Z pawel $
"Loader dla plików sk³adni SZARPa do Vim'a.

let g:xml_syntax_folding = 1
:au BufRead,BufNewFile ekrn* set filetype=cor
:au BufRead,BufNewFile PTT.* set filetype=ptt
:au BufRead,BufNewFile szarp.* set filetype=libpar
:au BufRead,BufNewFile *.rap set filetype=rap
:au BufRead,BufNewFile sdfsjdf set filetype=szablon
:au BufRead,BufNewFile parcook.cfg set filetype=parcook
:au BufRead,BufNewFile *.dsc set filetype=dsc
:au BufRead,BufNewFile *.log set filetype=elog
:au BufRead,BufNewFile definable.cfg set filetype=definable
:au BufRead,BufNewFile *.svg set filetype=html
:au BufRead,BufNewFile *.tsk set filetype=task
:au Syntax cor so /opt/szarp/resources/cor.vim 
:au Syntax ptt so /opt/szarp/resources/ptt.vim 
:au Syntax rap so /opt/szarp/resources/rap.vim 
:au Syntax szablon so /opt/szarp/resources/szablon.vim 
:au Syntax libpar so /opt/szarp/resources/libpar.vim 
:au Syntax parcook so /opt/szarp/resources/parcook.vim
:au Syntax dsc so /opt/szarp/resources/dsc.vim
:au Syntax elog so /opt/szarp/resources/elog.vim
:au Syntax definable so /opt/szarp/resources/definable.vim
highlight Comment gui=bold

" vim:ft=vim:
