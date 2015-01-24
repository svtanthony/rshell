# rshell

## Overview
This program is a shell terminal that can handle commands or program calls and can use the following connectors {; || &&}.
More features are planned and will be implemented soon. Next update will include the working directory in next to the username.
## How to use
This program is a linux program and will not work with windows as there are systems calls in linux that are not in windows.
1. The program will prompt the user for commands and keep doing so until the `exit` command is typed.
2. The user may also enter command flags such as `ls -a` .
3. Multiple commands can be chained with the appropriate connectors `;`, `&&`, `||`. Note that the semicolon `;` will run the next command regarsdless of the previos commands exit status. The and connector `&&` will only run if the next command if the previous command succeeded. The or operator `||` will only run the next command if the previous command failed.
4. When done type `exit`.
5. Have fun.
6. Any input is greatly appreciated!

## Required
`Linux`
`g++`

## Installation
1. Clone this repository type the following command on the terminal `git clone http://github.com/svtanthony/rshell.git`.
2. Go to the downloaded directory `cd rshell/.`
3. If the prerequisites are met, run `make`.
4. Run the program by typing `bin/rshell`

# Bugs
Single `&` or `|` will be interpreted as a bad command.
Quotes are not parsed, meaning `echo "this is && a test"` will be print `"this is` then try run `a test` as a command.
