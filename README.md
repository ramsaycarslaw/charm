# Charm 
Charm is a light-weight terminal based editor for editing MT (and other) files. It is a command line editor based on the kilo editor by Salvatore Sanfilippo aka antirez.

## Features
* Light-weight design - it's fast
* Easy to use
* Syntax highlighting (see supported for more info)
* Incremental Search

## Usage
Charm takes the best (my favorite) features from vi and Emacs to create a balanced editor experience. There are no modes - just type. The shortcuts to not to be chords however this is needed sometimes. The full list of commands are as follows
* C-x - Save a file
* C-o - Open a file
** Please note there are still some bugs surrounding this, it is better to use `charm /filename/`.
* C-s - (Incremental) Search within the file 
* C-q - Quit - leave the charm editor
* C-c - Change Inner, similar to `vi`'s `c` prefix

## Supported Languages

### Full Support
* C/C++ (including header files)
* MT (see https://www.github.com/ramsaycarslaw/mt)
* Go 
* Text files

### Partial Support
* Python

## ToDo
* Init file
-- Status bar top
-- all the colors
-- maybe keybinds?        
* Highlight text  
* Incremental Search and replace
* Better auto-indent
* Auto-complete  
