" Vim syntax file
" Language:     parcook.cfg file
" Maintainer:   Pawel Palucha <pawel@praterm.com.pl>
" Last_change:  6 Nov 2015
"
" Syntax highlighting for parcook.cfg file.
"

if exists("b:current_syntax")
    finish
endif

syn clear

syn sync lines=500

" defined parameters
syn match err "."
syn match ok " "

syn match comm "#[^0-9].*$"
syn match const "\(#[-]\=[0-9]\+\)\|\(null\)"
syn match op "[*+-/\$\^N]"
syn match eq "#[0-9]\+[ ]\+="
syn match ipcind "[0-9]\+"

" demon parameters
syn match par_tty "[ ]\+/dev/tty[A-Z][0-9][0-9]*[ ]*.*$" contained
syn match par_d "[ ]\+/opt/szarp/bin/.\+dmn" nextgroup=par_tty contained
syn match par_s "[0-9]\+" nextgroup=par_dm contained
syn match par_nm "^[0-9]\+ " nextgroup=par_s contained
syn match par_start "^[0-9]\+ [0-9]\+ [0-9]\+$" contained
syn match par_end "^[0-9]\+$" contained
syn region pars start="^[0-9]\+ [0-9]\+ [0-9]\+$" end="^[0-9]\+$" keepend contains=par_nm,par_s,par_d,par_tty,par_start,par_end

if !exists("did_parcook_syntax_inits")
    let did_parcook_syntax_inits = 1
    hi link pars          Error
    hi link err           Error
    hi link par_nm        Keyword
    hi link par_s         PreProc
    hi link par_start     Special
    hi link par_end       Special
    hi link comm          Comment
    hi link par_d         Character
    hi link par_tty       Type
    hi link op            Operator
    hi link eq            Special
    hi ipcind ctermfg=Green     guifg=Green
    hi const  ctermfg=DarkGreen guifg=DarkGreen
endif

let b:current_syntax = "parcook"

