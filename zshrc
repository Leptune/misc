export ZSH=$HOME/.oh-my-zsh
ZSH_THEME="bira"
plugins=(git)
source $ZSH/oh-my-zsh.sh

# User configuration
source ~/.leprc
#export PS1='%{$fg_bold[green]%}%p%n${ret_status}%m %{$fg[cyan]%}%c$(git_prompt_info)%{$fg_bold[green]%} %#%{$reset_color%} '
export PS1='╭─%{$terminfo[bold]$fg[green]%}%n@%m%{$reset_color%} %{$terminfo[bold]$fg[blue]%} %~%{$reset_color%}  $(git_prompt_info)%{$reset_color%}[%*]
╰─%B%#%b '

export PATH="$PATH:$HOME/.rvm/bin" # Add RVM to PATH for scripting

alias rake="noglob rake"
alias vm='ssh vagrant@192.168.10.10'
