" Load Vim syntax files for SZARP

let &runtimepath.=','.'/opt/szarp/resources/vimfiles'

au BufRead,BufNewFile szarp.cfg set filetype=libpar
au BufRead,BufNewFile *.log set filetype=elog

