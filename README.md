# Charm 
Charm is a light-weight terminal based editor for editing MT (and other) files. It is a command line editor based on the kilo editor by Salvatore Sanfilippo aka antirez.

![]( charm.png)

## Features
* Light-weight design - it's fast
* Easy to use
* Syntax highlighting (see supported for more info)
* Incremental Search

## Usage
Charm takes the best (my favorite) features from vi and Emacs to create a balanced editor experience. There are no modes - just type. The shortcuts to not to be chords however this is needed sometimes. The full list of commands are as follows

- `C-x C-s` Save a File
- `C-x C-c` Quit a File
- `C-s` Search within file
- `C-o` Newline no break (vi `o`)
- `C-c` Change Inner (vi `C`)

## Customisation
The init file is located at `$HOME/.charm.mt`.
Below is an example init file. Init files are written in MT as charm has the full VM built in. If there is an error in your init file all of the colors in the editor will turn green. To debug this just run the init file as you would any other MT file.

```
// Init file for charm

// theme
var commentColor = 4; // 247
var funcColor = 34;
var keyword1Color = 3; // 167
var keyword2Color = 100;
var stringColor = 1;
var numberColor = 123;
var matchColor = 45;
var normalColor = 7;

// window
var statusColor = 244; // 109 202
var linenumColor = 3;
var linenumBGColor = 234;
```

## Supported Languages

### Full Support
* C/C++ (including header files)
* MT (see https://www.github.com/ramsaycarslaw/mt)
* Go 
* Text files
* Python

## ToDo
- [ ] Auto-Indent
- [*] Init File
- [ ] Search & Replace  
