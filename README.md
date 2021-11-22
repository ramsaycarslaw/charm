# Charm 
Charm is a light-weight terminal based editor for editing MT (and other) files. It is a command line editor based on the kilo editor by Salvatore Sanfilippo aka antirez.

![]( charm.png)

## Features
* Light-weight design - it's fast
* Easy to use
* Syntax highlighting (see supported for more info)
* Incremental Search

## Usage

If you are familiar with `vi` you are familiar with charm.
You could probably use vimtutor to learn it. One difference is begginign and end of line which are H and L respectivley.
Please note that $ and 0 still work.

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
