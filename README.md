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

// Classic theme
var commentColor = 4; // 247
var funcColor = 34;
var keyword1Color = 3; // 167
var keyword2Color = 100;
var stringColor = 1;
var numberColor = 123;
var matchColor = 45;
var normalColor = 7;
var backgroundColor = 234;
var otherColor = 112;

// window
var statusColor = 244; // 109 202
var linenumColor = 244; // 244
var linenumBGColor = 234; // 234

fn atomOneTheme() {
  // ~ 30, 38, 39 - black
  backgroundColor = 237;
  // 224, 108, 118 - red
  keyword1Color = 203;
  // 97, 175, 239 - light blue
  funcColor = 75;
  // 197 120 221 - magenta
  otherColor = 177;
  numberColor = 177;
  // orange
  keyword2Color = 208;
  // green
  matchColor = 107;
  stringColor = 107;
  // grey
  commentColor = 242;
  statusColor = 238;
}

atomOneTheme();

```

## Supported Languages

### Full Support
* C/C++ (including header files)
* MT (see https://www.github.com/ramsaycarslaw/mt)
* Go 
* Rust
* Text files
* Python

## ToDo
- [ ] Auto-Indent
- [*] Init File
- [ ] Search & Replace  
