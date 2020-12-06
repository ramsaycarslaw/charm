// Copyright 2020 Ramsay Carslaw
    
/*** includes ***/

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <time.h>
#include <stdarg.h>
#include <fcntl.h>

/*** defines ***/

#define CTRL_KEY(k) ((k) & 0x1f)

#define RCC_VERSION "0.0.3"

#define RCC_TAB_STOP 2

#define RCC_QUIT_TIMES 2


enum editorKey {
  BACKSPACE = 127,
  ARROW_LEFT = 1000,
  ARROW_RIGHT,
  ARROW_UP,
  ARROW_DOWN,
  DEL_KEY,
  HOME_KEY,
  END_KEY,
  PAGE_UP,
  PAGE_DOWN
};    

enum editorHighlight {
  HL_NORMAL = 0,
  HL_FUNC,
  HL_COMMENT,
  HL_MLCOMMENT,
  HL_KEYWORD1,
  HL_KEYWORD2,
  HL_STRING,
  HL_NUMBER,
  HL_MATCH
};

#define HL_HIGHLIGHT_NUMBERS (1<<0)
#define HL_HIGHLIGHT_STRINGS (1<<1)
#define HL_HIGHLIGHT_FUNC (1<<10)

/*** data ***/

struct editorSyntax {
  char *filetype;
  char **filematch;
  char **keywords;
  char *singleline_comment_start;
  char *multiline_comment_start;
  char *multiline_comment_end;
  int flags;
};


typedef struct erow {
  int idx;
  int size;
  int rsize;
  char *chars;
  char *render;
  unsigned char *hl;
  int hl_open_comment;
} erow;

struct editorConfig {
  int cx, cy;
  int rx;
  int rowoff;
  int coloff;
  int screenrows;
  int screencols;
  int numrows;
  int autopair;
  int auto_complete;
  int suggestion;
  int mx;
  int my;
  int mroff;
  int mcoff;
  int prefix;
  int linenum_indent;            
  int raw_screenrows;
  int raw_screencols;
  int indent;
  erow *row;
  int dirty;
  char *filename;
  char *suggestions;
  char statusmessage[80];
  time_t statusmsg_time;
  struct editorSyntax *syntax;
  struct termios orig_termios;
};

struct editorConfig E;

/*** filetypes ***/

char *C_HL_extensions[] = { ".c", ".h", ".cpp", NULL};
char *C_HL_keywords[] = {
  "switch", "if", "while", "for", "break", "continue", "return", "else",
  "struct", "union", "typedef", "static", "enum", "class", "case",
  "int|", "long|", "double|", "float|", "char|", "unsigned|", "signed|",
  "void|","#define|", "#include|", NULL
};

char *GO_HL_extensions[] = {".go", NULL};
char *GO_HL_keywords[] = {
  "switch", "if", "defualt", "for", "break", "continue", "return", "else",
  "struct", "package", "typedef", "static", "enum", "class", "case", "func", "var",
  "int|", "string|", "uint|", "float|", "char|", "int16|", "int32|",
  "int64|","import|", NULL
};

char *MT_HL_extensions[] = {".MT", ".mt", ".mtl", NULL};
char *MT_HL_keywords[] = {
  "if", "else", "fn", "print", "return", "len|", "printf|", "println|", "read|",
  "write|", "clock|", "string|", "number|", "color|", "for", "while", "exit|", "clear|", "show|", "Cd|", "Ls|", "input|", "append|", "delete|", "var", NULL
};

char *PY_HL_extensions[] = { ".py", NULL };
char *PY_HL_keywords[] = {
    // Python keywords and built-in functions
    "and", "as", "assert", "break", "class", "continue", "def", "del", "elif", "else", "except", "exec", "finally", "for", "from", "global",
    "if", "import", "in", "is", "lambda", "not", "or", "pass", "print", "raise", "return", "try", "while", "with", "yield", "async", "await",
    "nonlocal", "range", "xrange", "reduce", "map", "filter", "all", "any", "sum", "dir", "abs", "breakpoint", "compile", "delattr", "divmod",
    "format", "eval", "getattr", "hasattr", "hash", "help", "id", "input", "isinstance", "issubclass", "len", "locals", "max", "min", "next",
    "open", "pow", "repr", "reversed", "round", "setattr", "slice", "sorted", "super", "vars", "zip", "__import__", "reload", "raw_input",
    "execfile", "file", "cmp", "basestring",
    // Python types
    "buffer|", "bytearray|", "bytes|", "complex|", "float|", "frozenset|", "int|", "list|", "long|", "None|", "set|", "str|", "chr|", "tuple|",
    "bool|", "False|", "True|", "type|", "unicode|", "dict|", "ascii|", "bin|", "callable|", "classmethod|", "enumerate|", "hex|", "oct|", "ord|",
    "iter|", "memoryview|", "object|", "property|", "staticmethod|", "unichr|", NULL
};

char *TEX_HL_extensions[] = {".tex", NULL};
char *TEX_HL_keywords[] = { "\\usepackage", "\\documentclass", "\\author", "\\title", "\\centering", "\\maketitle", "\\begin", "\\end",  
  "$$|", "\\newcommand", "equation|", "figure|", "theorem|", "tabular|", NULL};  
    

struct editorSyntax HLDB[] = {
  {
    "C",
    C_HL_extensions,
    C_HL_keywords,
    "//", "/*", "*/",
    HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS 
  },
  {
    "Go",
    GO_HL_extensions,
    GO_HL_keywords,
    "//", "/*", "*/",
    HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS 
  },
  {
    "MT",
    MT_HL_extensions,
    MT_HL_keywords,
    "//", "/*", "*/",
    HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
  },
  {
    "Python",
    PY_HL_extensions,
    PY_HL_keywords,
    "#", "/*", "*/",
    HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS 
  },
  {
    "LaTeX", 
    TEX_HL_extensions,
    TEX_HL_keywords,
    "%%", "", "",
    HL_HIGHLIGHT_STRINGS      
  }    	
};

#define HLDB_ENTRIES (sizeof(HLDB) / sizeof(HLDB[0]))

/*** prototypes ***/

void editorDeleteChar();

void editorInsertChar(int c);

void editorSetStatusMessage(const char *fmt, ...);

void editorRefreshScreen();

void editorClearScreen();

char *editorPrompt(char *prompt, void (*callback)(char *, int));

void editorMoveCursor(int key);

/*** terminal ***/

void die(const char *s) {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);
  
  perror(s);
  exit(1);
}


void disableRawMode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
    die("tcsetattr");
}

void enableRawMode() {
  if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
  atexit(disableRawMode);
  struct termios raw = E.orig_termios;
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}


int editorReadKey() {
  int nread;
  char c;
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN) die("read");
  }
  if (c == '\x1b') {
    char seq[3];
    if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
    if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';
    if (seq[0] == '[') {
      if (seq[1] >= '0' && seq[1] <= '9') {
        if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
        if (seq[2] == '~') {
          switch (seq[1]) {
            case '1': return HOME_KEY;
    	  case '3': return DEL_KEY;
            case '4': return END_KEY;
            case '5': return PAGE_UP;
            case '6': return PAGE_DOWN;
            case '7': return HOME_KEY;
            case '8': return END_KEY;
          }
        }
      } else {
        switch (seq[1]) {
          case 'A': return ARROW_UP;
          case 'B': return ARROW_DOWN;
          case 'C': return ARROW_RIGHT;
          case 'D': return ARROW_LEFT;
          case 'H': return HOME_KEY;
          case 'F': return END_KEY;
        }
      }
    } else if (seq[0] == 'O') {
      switch (seq[1]) {
        case 'H': return HOME_KEY;
        case 'F': return END_KEY;
      }
    }
    return '\x1b';
  } else {
    return c;
  }
}

int getCursorPosition(int *rows, int *cols) {
  char buf[32];
  unsigned int i = 0;
  if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;
  while (i < sizeof(buf) - 1) {
    if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
    if (buf[i] == 'R') break;
    i++;
  }
  buf[i] = '\0';
  if (buf[0] != '\x1b' || buf[1] != '[') return -1;
  if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;
  return 0;
}


/* Returns the size of the current termibal window */  
int getWindowSize(int *rows, int *cols) {
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
    return getCursorPosition(rows, cols);
  } else {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}

/*** Completion Tables ***/

char *completionSourceC() {
  char *C_keywords[] = {
  "switch", "if", "while", "for", "break", "continue", "return", "else",
  "struct", "union", "typedef", "static", "enum", "class", "case",
  "int", "long", "double", "float", "char", "unsigned", "signed",
  "void","#define", "#include", NULL
  };
  return *C_keywords;
}

/*** highlighting rules ***/

int is_separator(int c) {
  return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
}



void editorUpdateSyntax(erow *row) {
  row->hl = realloc(row->hl, row->rsize);
  memset(row->hl, HL_NORMAL, row->rsize);
  if (E.syntax == NULL) return;
  char **keywords = E.syntax->keywords;
  char *scs = E.syntax->singleline_comment_start;
  char *mcs = E.syntax->multiline_comment_start;
  char *mce = E.syntax->multiline_comment_end;
  int scs_len = scs ? strlen(scs) : 0;
  int mcs_len = mcs ? strlen(mcs) : 0;
  int mce_len = mce ? strlen(mce) : 0;
  int prev_sep = 1;
  int in_string = 0;

  int in_func;
  
  int in_comment = (row->idx > 0 && E.row[row->idx - 1].hl_open_comment);
  int i = 0;
  //int target = 0;
  while (i < row->rsize) {
    char c = row->render[i];
    unsigned char prev_hl = (i > 0) ? row->hl[i - 1] : HL_NORMAL;
    if (scs_len && !in_string && !in_comment) {
      if (!strncmp(&row->render[i], scs, scs_len)) {
        memset(&row->hl[i], HL_COMMENT, row->rsize - i);
        break;
      }
    }

    // functions
    if (E.syntax->flags & HL_HIGHLIGHT_FUNC) {
      if (in_func) {
	row->hl[i] = HL_FUNC;
	if (c == in_func) in_func = 0;
	i++;
	continue;
      } else {
	if (c == '(') {
	  int count = i-1;
	  while(!is_separator(row->render[count])) {
	    row->hl[count] = HL_FUNC;
	    in_func = 0;
	    count--;
	    continue;
	      
	  }
	}
      }
    }
    
    if (mcs_len && mce_len && !in_string) {
      if (in_comment) {
        row->hl[i] = HL_MLCOMMENT;
        if (!strncmp(&row->render[i], mce, mce_len)) {
          memset(&row->hl[i], HL_MLCOMMENT, mce_len);
          i += mce_len;
          in_comment = 0;
          prev_sep = 1;
          continue;
        } else {
          i++;
          continue;
        }
      } else if (!strncmp(&row->render[i], mcs, mcs_len)) {
        memset(&row->hl[i], HL_MLCOMMENT, mcs_len);
        i += mcs_len;
        in_comment = 1;
        continue;
      }
    }
    if (E.syntax->flags & HL_HIGHLIGHT_STRINGS) {
      if (in_string) {
        row->hl[i] = HL_STRING;
        if (c == '\\' && i + 1 < row->rsize) {
          row->hl[i + 1] = HL_STRING;
          i += 2;
          continue;
        }
        if (c == in_string) in_string = 0;
        i++;
        prev_sep = 1;
        continue;
      } else {
        if (c == '"' || c == '\'') {
          in_string = c;
          row->hl[i] = HL_STRING;
          i++;
          continue;
        }
      }
    }
    if (E.syntax->flags & HL_HIGHLIGHT_NUMBERS) {
      if ((isdigit(c) && (prev_sep || prev_hl == HL_NUMBER)) ||
          (c == '.' && prev_hl == HL_NUMBER)) {
        row->hl[i] = HL_NUMBER;
        i++;
        prev_sep = 0;
        continue;
      }
    }
    if (prev_sep) {
      int j;
      for (j = 0; keywords[j]; j++) {
        int klen = strlen(keywords[j]);
        int kw2 = keywords[j][klen - 1] == '|';
        if (kw2) klen--;
        if (!strncmp(&row->render[i], keywords[j], klen) &&
            is_separator(row->render[i + klen])) {
          memset(&row->hl[i], kw2 ? HL_KEYWORD2 : HL_KEYWORD1, klen);
          i += klen;
          break;
        }
      }
      if (keywords[j] != NULL) {
        prev_sep = 0;
        continue;
      }
    }

    if (prev_sep) {
      
    }
    
    prev_sep = is_separator(c);
    i++;
  }
  int changed = (row->hl_open_comment != in_comment);
  row->hl_open_comment = in_comment;
  if (changed && row->idx + 1 < E.numrows)
    editorUpdateSyntax(&E.row[row->idx + 1]);
}

// adjust theme here using ANSI 3/4 bit colors
int editorSyntaxToColor(int hl) {
  switch (hl) {
  case HL_MLCOMMENT:
  case HL_COMMENT: return 248;
  case HL_FUNC: return 34;
  case HL_KEYWORD1: return 167;
  case HL_KEYWORD2: return 108;
  case HL_STRING: return 136;
  case HL_NUMBER: return 24;
  case HL_MATCH: return 81;
  default: return 7;
  }
}

void editorSelectSyntaxHighlight() {
  E.syntax = NULL;
  if (E.filename == NULL) return;

  char *ext = strrchr(E.filename, '.');

  for (unsigned int j = 0; j < HLDB_ENTRIES; j++) {
    struct editorSyntax *s = &HLDB[j];
    unsigned int i = 0;

    while (s->filematch[i]) {
      int is_ext = (s->filematch[i][0] == '.');
      if ((is_ext && ext && !strcmp(ext, s->filematch[i])) ||
	  (!is_ext && strstr(E.filename, s->filematch[i]))) {
	E.syntax = s;

	int filerow;
	for (filerow = 0; filerow < E.numrows; filerow++) {
	  editorUpdateSyntax(&E.row[filerow]);
	}
	return;
      }
      i++;
    }
  }
}


/*** row operations ***/


int editorRowCxToRx(erow *row, int cx) {
  int rx = 0;
  int j;
  for (j = 0; j < cx; j++) {
    if (row->chars[j] == '\t')
      rx += (RCC_TAB_STOP - 1) - (rx % RCC_TAB_STOP);
    rx++;
  }
  return rx;
}


int editorRowRxToCx(erow *row, int rx) {
  int cur_rx = 0;
  int cx;
  for (cx = 0; cx < row->size; cx++) {
    if (row->chars[cx] == '\t')
      cur_rx += (RCC_TAB_STOP - 1) - (cur_rx % RCC_TAB_STOP);
    cur_rx++;
    if (cur_rx > rx) return cx;
  }
  return cx;
}
  

void editorUpdateRow(erow *row) {
  int tabs = 0;
  int j;

  for (j = 0; j < row->size; j++)
    if (row->chars[j] == '\t') tabs++;
  
  free(row->render);
  row->render = malloc(row->size + tabs*(RCC_TAB_STOP - 1) + 1);

  int idx = 0;
  for (j = 0; j < row->size; j++) {
    if (row->chars[j] == '\t') {
      row->render[idx++] = ' ';
      while (idx % RCC_TAB_STOP != 0) row->render[idx++] = ' ';
    } else {
      row->render[idx++] = row->chars[j];
    }
  }
  row->render[idx] = '\0';
  row->rsize = idx;

  editorUpdateSyntax(row);
}


void editorInsertRow(int at, char *s, size_t len) {
  if (at < 0 || at > E.numrows) return;
  E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));
  memmove(&E.row[at + 1], &E.row[at], sizeof(erow) * (E.numrows - at));
  for (int j = at + 1; j <= E.numrows; j++) E.row[j].idx++;
  E.row[at].idx = at;
  E.row[at].size = len;
  E.row[at].chars = malloc(len + 1);
  memcpy(E.row[at].chars, s, len);
  E.row[at].chars[len] = '\0';
  E.row[at].rsize = 0;
  E.row[at].render = NULL;
  E.row[at].hl = NULL;
  E.row[at].hl_open_comment = 0;
  editorUpdateRow(&E.row[at]);
  E.numrows++;
  E.dirty++;
}

void editorFreeRow(erow *row) {
  free(row->render);
  free(row->chars);
  free(row->hl);
}

void editorDelRow(int at) {
  if (at < 0 || at >= E.numrows) return;
  editorFreeRow(&E.row[at]);
  memmove(&E.row[at], &E.row[at + 1], sizeof(erow) * (E.numrows - at - 1));
  for (int j = at; j < E.numrows - 1; j++) E.row[j].idx--;
  E.numrows--;
  E.dirty++;
}

void editorRowInsertChar(erow *row, int  at, int c) {
  if (at < 0 || at > row->size) at = row->size;
  row->chars = realloc(row->chars, row->size + 2);
  memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
  row->size++;
  row->chars[at] = c;
  editorUpdateRow(row);
  E.dirty++;
}

void editorRowAppendString(erow *row, char *s, size_t len) {
  row->chars = realloc(row->chars, row->size + len+ 1);
  memcpy(&row->chars[row->size], s, len);
  row->size += len;
  row->chars[row->size] = '\0';
  editorUpdateRow(row);
  E.dirty++;
}

void editorRowDeleteChar(erow *row, int at) {
  if (at < 0 || at >= row->size) return;
  memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
  row->size--;
  editorUpdateRow(row);
  E.dirty++;
}

/*** editor operations ***/

void editorKillLine() {
  if (E.cy == 0) 
  {
    return;  
  }
  editorDelRow(E.cy);
  E.cy--;  
  return;    
}
    
void editorAutoPair(char c) {
  if (E.autopair == 1) {
    erow *row = &E.row[E.cy];
    if (c == '{') {
      editorRowInsertChar(row, E.cx, '{');
      E.cx += 3;
      editorRowInsertChar(row, E.cx, '}');
     return;
    } else if (c == '(') {
      editorRowInsertChar(row, E.cx, '(');
      E.cx += 3;
      editorRowInsertChar(row, E.cx, ')');
      return;
    } else if (c == '"') {
      editorRowInsertChar(row, E.cx, '"');
      E.cx += 3;
      editorRowInsertChar(row, E.cx, '"');
      return;
    } else if (c == '\'') {
      editorRowInsertChar(row, E.cx, '\'');
      E.cx += 3;
      editorRowInsertChar(row, E.cx, '\'');
      return;
    } else if ( c == '[') {
      editorRowInsertChar(row, E.cx, '[');
      E.cx += 3;
      editorRowInsertChar(row, E.cx, ']');
      return;
    } else if ( c == '<') {
      editorRowInsertChar(row, E.cx, '<');
      E.cx += 3;
      editorRowInsertChar(row, E.cx, '>');
      return;
    }
  } else {
    editorInsertChar(c);
   }
}

char *wordFromCursor(erow *row) {
  int x = E.cx;
  int prev_sep;
  int i = x;
  char *result = "\0";

  // backtack to find last space
  while (!is_separator(row->chars[i])) {
    i--;
    continue;
  }
  prev_sep = x - i;
  i = prev_sep + 1;
  
  while (!is_separator(row->chars[i])) {
    strncat(result, &row->chars[i], 1);
  }
  return result;
}

int startsWith(const char *pre, const char *str)
{
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? 0 : memcmp(pre, str, lenpre) == 0;
}

void editorAutoComplete() {
  char *ext = strrchr(E.filename, '.');
  erow *row = &E.row[E.cy];
  char *str = row->chars;
  char *source;
  int len = row->size;

  char words[32][32];
  
  
  int i, j, ctr;
  
  if (strcmp(ext, ".c") != 0) {
    source = completionSourceC();
  } else if (strcmp(ext, ".go") != 0) {
    source = *GO_HL_keywords;
  } else if (strcmp(ext, ".py")) {
    source = *PY_HL_keywords;
  }

  j=0; ctr=0;
  for (i = 0; i <=len; i++) {
    if (str[i] == ' ' || str[i] == '\0') {
      words[ctr][j] = '\0';
      ctr++;
      j = 0;
    } else {
      words[ctr][j] = str[i];
      j++;
    }
  }

  /* We know the number of words on any given line is given by the variable ctr
     The following will iterate over all words in the row */

  len = 128; // arb length for completions list
  int track = 0;
  for (i = 0; i < ctr; i++) {
    for (j = 0; j < len; j++) {
      if (strcmp(words[i], &source[j]) == 0) {
	// words match so no need to suggest anything
	continue;
      }
      if ((startsWith(words[i], &source[j])) && strlen(words[i]) > 1) {
	// now a way to show completions
	E.suggestions = &source[j];
	track++;
	continue;
      } 
    }
    editorSetStatusMessage(E.suggestions);
  }  
  return;
}

void substring(char s[], char sub[], int p, int l) {
   int c = 0;
   
   while (c < l) {
      sub[c] = s[p+c-1];
      c++;
   }
   sub[c] = '\0';
}

void editorInsertSuggestions() {
  if (E.auto_complete && E.suggestions != NULL) {
    erow *row = &E.row[E.cy];
    int i, j, ctr;
    char words[32][32];
    char *str = row->chars;
    char *buf = "\0";
    int len = row->size;

    j=0; ctr=0;
    for (i = 0; i <=len; i++) {
      if (str[i] == ' ' || str[i] == '\0') {
        words[ctr][j] = '\0';
        ctr++;
        j = 0;
      } else {
        words[ctr][j] = str[i];
        j++;
      }
    }

    buf = E.suggestions;
    int temp = 0;

    for (i = 0; i < 32; i++) {
      if (startsWith(words[i], buf)) {
	temp = strlen(words[i]);
	substring(buf, words[i], 0, strlen(words[i]));
	break;
	}
    }

    for (i = 0; i < temp; i++) {
      editorDeleteChar();
    }
    
    editorRowAppendString(row, buf, strlen(buf));
    E.cx += strlen(buf);
    E.suggestions = NULL;
  }
}

int editorCountWhitespace(erow *row) {
  int i = 0;
  int indent = 0;
  char c;
  while ((c = row->chars[i])) {
    if (c ==' ') {
      indent++;
    } else if (c == '\t') {
      indent += RCC_TAB_STOP;
    } else {
      break;
    }
    i++;
  }
  return indent;
}

void editorAutoIndent() {
  int amount = E.indent * RCC_TAB_STOP;
  for (int i = 0; i < amount; i++) 
  {
    editorInsertChar(' ');
  }
}

static void editorCenter(void) {
    if (E.cy - E.screenrows / 2 != 0 && E.rowoff + E.cy - E.screenrows / 2 > 0 && E.rowoff + E.cy + E.screenrows / 2 < E.numrows) {
        for (int i = 0; i < E.screenrows / 2; i++) {
			    E.cy++;
        }
        for (int i = 0; i < E.screenrows / 2; i++) {
			    E.cy--;
        }
    }
}

void editorInsertChar(int c) {
  if (E.cy == E.numrows) {
    editorInsertRow(E.numrows, "", 0);
  }
  editorRowInsertChar(&E.row[E.cy], E.cx, c);
  E.cx++;
}

void editorInsertNewline() {
  if (E.cx == 0) {
    editorInsertRow(E.cy, "", 0);
  } else {
    erow *row = &E.row[E.cy];
    editorInsertRow(E.cy + 1, &row->chars[E.cx], row->size - E.cx);
    row = &E.row[E.cy];
    row->size = E.cx;
    row->chars[row->size] = '\0';
    editorUpdateRow(row);
  }
  E.cy++;
  E.cx = 0;
}

void editorDeleteChar() {
  if (E.cy == E.numrows) return;
  if (E.cx == 0 && E.cy == 0) return;

  erow *row = &E.row[E.cy];
  if (E.cx > 0) {
    editorRowDeleteChar(row, E.cx - 1);
    E.cx--;
  } else {
    E.cx = E.row[E.cy -1].size;
    editorRowAppendString(&E.row[E.cy -1], row->chars, row->size);
    editorDelRow(E.cy);
    E.cy--;
  }
}

void editorChangeInner() {  
   erow *row = &E.row[E.cy];

   if (E.cx == row->size) {
     return;
   }

   for (int i = 0; i < row->size; i++)
   {
     if (row->chars[i] == '(')
     {
       E.cx = i+1;
       while (row->chars[E.cx] != ')')
       {
	 editorRowDeleteChar(row, E.cx);
       }
     }

     if (row->chars[i] == '[')
     {
       E.cx = i+1;
       while (row->chars[E.cx] != ']')
       {
	 editorRowDeleteChar(row, E.cx);
       }
     }
     if (row->chars[i] == '{') {
       E.cx = i + 1;
       while (row->chars[E.cx] != '}') {
         editorRowDeleteChar(row, E.cx);
       }
     }
   }
   return;
}

/*** file i/o ***/

char *editorRowsToString(int *buflen) {
  int totlen = 0;
  int j;
  for (j = 0; j < E.numrows; j++)
    totlen += E.row[j].size + 1;
  *buflen = totlen;
  char *buf = malloc(totlen);
  char *p = buf;
  for (j = 0; j < E.numrows; j++) {
    memcpy(p, E.row[j].chars, E.row[j].size);
    p += E.row[j].size;
    *p = '\n';
    p++;
  }
  return buf;
}

void editorOpen(char *filename) {
  free(E.filename);
  E.filename = strdup(filename);

  editorSelectSyntaxHighlight();
  
  FILE *fp = fopen(filename, "r");
  if (!fp) die("fopen");
  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;
  while ((linelen = getline(&line, &linecap, fp)) != -1) {
    while (linelen > 0 && (line[linelen - 1] == '\n' ||
                           line[linelen - 1] == '\r'))
      linelen--;
    editorInsertRow(E.numrows, line, linelen);
  }
  free(line);
  fclose(fp);
  E.dirty = 0;
}


void editorOpenNew() {
  char *file = editorPrompt("path/to/file: %s", NULL);
  if (E.filename == NULL) {
    editorOpen(file);
  } else {
    editorOpen(file);
  }
}

void editorSave() {
  if (E.filename == NULL) {
    E.filename = editorPrompt("Save as %s", NULL);
    if (E.filename == NULL) {
      editorSetStatusMessage("Cancelled Save");
      return;
    }
    editorSelectSyntaxHighlight();
  }
  int len;
  char *buf = editorRowsToString(&len);
  int fd = open(E.filename, O_RDWR | O_CREAT, 0644);
  if (fd != -1) {
    if (ftruncate(fd, len) != -1) {
      if (write(fd, buf, len) == len) {
        close(fd);
        free(buf);
	E.dirty = 0;
        editorSetStatusMessage("%d bytes written to disk", len);
        return;
      }
    }
    close(fd);
  }
  free(buf);
  editorSetStatusMessage("Can't save! I/O error: %s", strerror(errno));
}

/*** find ***/

void editorFindCallback(char *query, int key) {
  static int last_match = -1;
  static int direction = 1;

  static int saved_hl_line;
  static char *saved_hl = NULL;

  if (saved_hl) {
    memcpy(E.row[saved_hl_line].hl, saved_hl, E.row[saved_hl_line].rsize);
    free(saved_hl);
    saved_hl = NULL;
  }
  
  if (key == '\r' || key == '\x1b') {
    last_match = -1;
    direction = 1;
    return;
  } else if (key == ARROW_RIGHT || key == ARROW_DOWN) {
    direction = 1;
  } else if (key == ARROW_LEFT || key == ARROW_UP) {
    direction = -1;
  } else {
    last_match = -1;
    direction = 1;
  }
  if (last_match == -1) direction = 1;
  int current = last_match;
  int i;
  for (i = 0; i < E.numrows; i++) {
    current += direction;
    if (current == -1) current = E.numrows - 1;
    else if (current == E.numrows) current = 0;
    erow *row = &E.row[current];
    char *match = strcasestr(row->render, query);
    if (match) {
      last_match = current;
      E.cy = current;
      E.cx = editorRowRxToCx(row, match - row->render);
      E.rowoff = E.numrows;

      saved_hl_line = current;
      saved_hl = malloc(row->rsize);
      memcpy(saved_hl, row->hl, row->rsize);

      memset(&row->hl[match - row->render], HL_MATCH, strlen(query));
      break;
    }
  }
}

void editorFind() {
  int saved_cx = E.cx;
  int saved_cy = E.cy;
  int saved_coloff = E.coloff;
  int saved_rowoff = E.rowoff;
  char *query = editorPrompt("Pattern: %s (Use ESC|Arrows|Enter)",
                             editorFindCallback);
  if (query) {
    free(query);
  } else {
    E.cx = saved_cx;
    E.cy = saved_cy;
    E.coloff = saved_coloff;
    E.rowoff = saved_rowoff;
  }
}

/*** append buffer ***/

struct abuf {
  char *b;
  int len;
};

#define ABUF_INIT {NULL, 0}

void abAppend(struct abuf *ab, const char *s, int len) {
  char *new = realloc(ab->b, ab->len + len);
  if (new == NULL) return;
  memcpy(&new[ab->len], s, len);
  ab->b = new;
  ab->len += len;
}

void abFree(struct abuf *ab) {
  free(ab->b);
}

/*** output ***/

void editorScroll() {
  E.rx = 0;
  if (E.cy < E.numrows) {
    E.rx = editorRowCxToRx(&E.row[E.cy], E.cx);
  }
  
  if (E.cy < E.rowoff) {
    E.rowoff = E.cy;
  }
  if (E.cy >= E.rowoff + E.screenrows) {
    E.rowoff = E.cy - E.screenrows + 1;
  }
  if (E.rx < E.coloff) {
    E.coloff = E.rx;
  }
  if (E.rx >= E.coloff + E.screencols) {
    E.coloff = E.rx - E.screencols + 1;
  }
}

void editorDrawRows(struct abuf *ab) {
  int y;
  for (y = 0; y < E.screenrows; y++) {

    int filerow = y + E.rowoff;

    //draw line number
   char format[16];
   char linenum[E.linenum_indent + 1];
   memset(linenum, ' ', E.linenum_indent);
   snprintf(format, 4, "%%%dd", E.linenum_indent - 1);
   if (filerow < E.numrows) {
      snprintf(linenum, E.linenum_indent + 1, format, filerow + 1);
    }
    abAppend(ab, "\x1b[38;5;244m", 11);
    abAppend(ab, "\x1b[48;5;234m", 11);    
    abAppend(ab, linenum, E.linenum_indent);
    abAppend(ab, "\x1b[49m", 5);
    abAppend(ab, "\x1b[39m", 5);  
    abAppend(ab, " ", 1);  
    if (filerow >= E.numrows) {
      if (E.numrows == 0 && y == E.screenrows / 3) {
        char welcome[80];
        int welcomelen = snprintf(welcome, sizeof(welcome),
          "Charm -- version %s", RCC_VERSION);
        if (welcomelen > E.screencols) welcomelen = E.screencols;
        int padding = (E.screencols - welcomelen) / 2;
        if (padding) {
          abAppend(ab, "~", 1);
          padding--;
        }
        while (padding--) abAppend(ab, " ", 1);
        abAppend(ab, welcome, welcomelen);
      } else {
        abAppend(ab, "~", 1);
      }
    } else {
      int len = E.row[filerow].rsize - E.coloff;
      if (len < 0) len = 0;
      if (len > E.screencols) len = E.screencols;
      char *c = &E.row[filerow].render[E.coloff];
      unsigned char *hl = &E.row[filerow].hl[E.coloff];
      int current_color = -1;
      int j;
      for (j = 0; j < len; j++) {
          if (iscntrl(c[j])) {
          char sym = (c[j] <= 26) ? '@' + c[j] : '?';
          abAppend(ab, "\x1b[7m", 4);
          abAppend(ab, &sym, 1);
          abAppend(ab, "\x1b[m", 3);
          if (current_color != -1) {
            char buf[16];
            int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", current_color);
            abAppend(ab, buf, clen);
          }
        } else if (hl[j] == HL_NORMAL) {
          if (current_color != -1) {
            abAppend(ab, "\x1b[39m", 5);
            current_color = -1;
          }
          abAppend(ab, &c[j], 1);
        } else {
          int color = editorSyntaxToColor(hl[j]);
          if (color != current_color) {
            current_color = color;
            char buf[16];
            int clen = snprintf(buf, sizeof(buf), "\x1b[38;5;%dm", color);
            abAppend(ab, buf, clen);
          }
          abAppend(ab, &c[j], 1);
        }
      }
      abAppend(ab, "\x1b[39m", 5);
    }
    abAppend(ab, "\x1b[K", 3);
    abAppend(ab, "\r\n", 2);
  }
}

void editorDrawStatusBar(struct abuf *ab) {
  abAppend(ab, "\x1b[38;5;109;7m", 14);
  char status[80], rstatus[80];
  int len = snprintf(status, sizeof(status), "%.20s - %d lines %s",
    E.filename ? E.filename : "[No Name]", E.numrows,
    E.dirty ? "(modified)" : "");
  int rlen = snprintf(rstatus, sizeof(rstatus), "%s | %d/%d",
		      E.syntax ? E.syntax->filetype : "text", E.cy + 1, E.numrows);
  if (len > E.raw_screencols) len = E.raw_screencols;
  abAppend(ab, status, len);
  while (len < E.raw_screencols) {
    if (E.raw_screencols - len == rlen) {
      abAppend(ab, rstatus, rlen);
      break;
    } else {
      abAppend(ab, " ", 1);
      len++;
    }
  }
  abAppend(ab, "\x1b[m", 3);
  abAppend(ab, "\r\n", 2);
}

void editorDrawMessageBar(struct abuf *ab) {
  abAppend(ab, "\x1b[K", 3);
  int msglen = strlen(E.statusmessage);
  if (msglen > E.raw_screencols) msglen = E.raw_screencols;
  if (msglen && time(NULL) - E.statusmsg_time < 5)
    abAppend(ab, E.statusmessage, msglen);
}

void editorUpdateLinenumIndent() {
    int digit;
    int numrows = E.numrows;

    if (numrows == 0) {
        digit = 0;
        E.linenum_indent = 2;
        return;
    }

    digit = 1;
    while (numrows >= 10) {
        numrows = numrows / 10;
        digit++;
    }
    E.linenum_indent = digit + 2;
}


void editorClearScreen() {
  E.row = NULL;
  E.screenrows = 0;
  E.screencols = 0;
  return;
}


void editorRefreshScreen() {
  editorUpdateLinenumIndent();
  E.screencols = E.raw_screencols - E.linenum_indent;
  editorScroll();
  struct abuf ab = ABUF_INIT;
  abAppend(&ab, "\x1b[?25l", 6);
  abAppend(&ab, "\x1b[H", 3);
  editorDrawRows(&ab);
  editorDrawStatusBar(&ab);
  editorDrawMessageBar(&ab);
  
  char buf[32];
  //snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1,// (E.rx - E.coloff) + 1 + E.linenum_indent);
  //snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1, E.cx + 1);
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy - E.rowoff + 1, E.rx - E.coloff + 1 + E.linenum_indent);
  abAppend(&ab, buf, strlen(buf));
  abAppend(&ab, "\x1b[?25h", 6);
  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);
}

void editorSetStatusMessage(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(E.statusmessage  , sizeof(E.statusmessage  ), fmt, ap);
  va_end(ap);
  E.statusmsg_time = time(NULL);
}

/*** input ***/

char *editorPrompt(char *prompt, void (*callback)(char *, int)) {
  size_t bufsize = 128;
  char *buf = malloc(bufsize);
  size_t buflen = 0;
  buf[0] = '\0';
  while (1) {
    editorSetStatusMessage(prompt, buf);
    editorRefreshScreen();
    int c = editorReadKey();
    if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
      if (buflen != 0) buf[--buflen] = '\0';
    } else if (c == '\x1b') {
      editorSetStatusMessage("");
      if (callback) callback(buf, c);
      free(buf);
      return NULL;
    } else if (c == '\r') {
      if (buflen != 0) {
        editorSetStatusMessage("");
	if (callback) callback(buf, c);
        return buf;
      }
    } else if (!iscntrl(c) && c < 128) {
      if (buflen == bufsize - 1) {
        bufsize *= 2;
        buf = realloc(buf, bufsize);
      }
      buf[buflen++] = c;
      buf[buflen] = '\0';
    }
    if (callback) callback(buf, c);
  }
}

void editorMoveCursor(int key) {
  erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
  switch (key) {
    case ARROW_LEFT:
      if (E.cx != 0) {
        E.cx--;
      } else if (E.cy > 0) {
        E.cy--;
        E.cx = E.row[E.cy].size;
      }
      break;
    case ARROW_RIGHT:
      if (row && E.cx < row->size) {
        E.cx++;
      } else if (row && E.cx == row->size) {
        E.cy++;
        E.cx = 0;
      }
      break;
    case ARROW_UP:
      if (E.cy != 0) {
        E.cy--;
      }
      break;
    case ARROW_DOWN:
      if (E.cy < E.numrows) {
        E.cy++;
      }
      break;
  }
  row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
  int rowlen = row ? row->size : 0;
  if (E.cx > rowlen) {
    E.cx = rowlen;
  }
}


void editorProcessKeypress() {
  static int quit_times = RCC_QUIT_TIMES;
  char *l = NULL;
  int c = editorReadKey();
  switch (c) {
    case '\r':
      editorInsertNewline();
      editorAutoIndent();
      break;
    case CTRL_KEY('q'):
      if (E.dirty && quit_times > 0) {
        editorSetStatusMessage("Warning: File has unsaved changes. "
          "Press Ctrl-Q %d more times to quit.", quit_times);
        quit_times--;
        return;
      }
      write(STDOUT_FILENO, "\x1b[2J", 4);
      write(STDOUT_FILENO, "\x1b[H", 3);
      exit(0);
      break;
    case CTRL_KEY('l'):
      editorCenter();
      break;
    case CTRL_KEY('k'):
      editorKillLine();
      break;
    case CTRL_KEY('o'):
      if (E.cy < E.numrows)
       E.cx = E.row[E.cy].size;
      editorInsertNewline();
      break;  
    case CTRL_KEY('x'):
      E.prefix = (E.prefix == 1) ? 0 : 1;
      break;
    case CTRL_KEY('a'):
    case HOME_KEY:
      E.cx = 0;
      break;
    case CTRL_KEY('e'):
    case END_KEY:
      if (E.cy < E.numrows)
        E.cx = E.row[E.cy].size;
      break;
      
    case CTRL_KEY('f'):
      if (E.row[E.cy].chars[E.cx] == ' ') 
      {
        E.cx++;
      } 
      while (E.row[E.cy].chars[E.cx] != ' ')  
      {
        E.cx++;  
      }
      editorScroll();
      break;
      
    case CTRL_KEY('b'):
       if (E.row[E.cy].chars[E.cx] == ' ') 
       {
         E.cx--;
       } 
       while (E.row[E.cy].chars[E.cx] != ' ')  
       {
         E.cx--;  
       }
       editorScroll();
       break;
       
    case CTRL_KEY('n'):
      E.cy += 10;
      editorScroll();
      break;     
      
    case CTRL_KEY('p'):
      E.cy -= 10;
      if (E.cy < 0) 
      {
        E.cy = 0;
      }
      editorScroll();
      break;

    case CTRL_KEY('j'):
      l = editorPrompt("Jump to line: %s", NULL);
      int line = atoi(l);
      int change = line - E.cy-1;
      E.cy += change;
      if (E.cy < 0)
        E.cy = 0;
      break;

    case CTRL_KEY('s'):
      if (E.prefix) {
        E.prefix = 0;
        editorSave();    
        break;
      }
      editorFind();
      break;
    case CTRL_KEY('c'):
      if (E.prefix) {
        E.prefix = 0;
        if (E.dirty && quit_times > 0) {
        editorSetStatusMessage("Warning: File has unsaved changes. "
          "Press Ctrl-Q %d more times to quit.", quit_times);
        quit_times--;
        return;
      }
      write(STDOUT_FILENO, "\x1b[2J", 4);
      write(STDOUT_FILENO, "\x1b[H", 3);
      exit(0);
      
      }
      editorChangeInner();
      break;
    case CTRL_KEY(' '):
      E.mx = E.cx;
      E.my = E.cy;
      E.mroff = E.rowoff;
      E.mcoff = E.coloff;
	    editorSetStatusMessage("Mark set");
	    break;
    case CTRL_KEY('u'):
      E.cx = E.mx;
      E.cy = E.my;
      E.rowoff = E.mroff;
      E.coloff = E.mcoff;                
      editorSetStatusMessage("Mark popped");
      break;
    case '{':
      E.indent++;
      editorInsertChar('{');
      break;
    
    case '}':
      E.indent--;
      editorInsertChar('}');
      break;

    case ':':
      E.indent--;
      editorInsertChar(':');
      break;

    // soft tabs
    case '\t':
      for (int i = 0; i < RCC_TAB_STOP; i++)
        editorInsertChar(' ');
      break;

    case BACKSPACE:
    case CTRL_KEY('h'):
    case DEL_KEY:
    case CTRL_KEY('d'):
      if (c == DEL_KEY || c == CTRL_KEY('d')) editorMoveCursor(ARROW_RIGHT);
      editorDeleteChar();
      break;
    
    case PAGE_UP:
    case PAGE_DOWN:
      {
        if (c == PAGE_UP) {
          E.cy = E.rowoff;
        } else if (c == PAGE_DOWN) {
          E.cy = E.rowoff + E.screenrows - 1;
          if (E.cy > E.numrows) E.cy = E.numrows;
        }
        int times = E.screenrows;
        while (times--)
          editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
      }
      break;
    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
      editorMoveCursor(c);
      break;
    case CTRL_KEY('g'):
    case '\x1b':
      break;
    default:
      editorInsertChar(c);
      break;
  }
  quit_times = RCC_QUIT_TIMES;
}

/*** init ***/

void initEditor() {
  E.cx = 0;
  E.cy = 0;
  E.rx = 0;
  E.rowoff = 0;
  E.coloff = 0;
  E.numrows = 0;
  E.row = NULL;
  E.dirty = 0;
  E.autopair = 0;
  E.auto_complete = 0;
  E.suggestion = 0;
  E.filename = NULL;
  E.suggestions = NULL;
  E.statusmessage[0] = '\0';
  E.statusmsg_time = 0;
  E.linenum_indent = 6;
  E.indent = 0;
  E.syntax = NULL;
  E.prefix = 0;
  E.mx =0;
  E.my = 0;
  E.mroff = 0;
  E.mcoff = 0;        
  
  if (getWindowSize(&E.raw_screenrows, &E.raw_screencols) == -1) die("getWindowSize");
  E.screenrows = E.raw_screenrows - 2;
  E.screencols = E.raw_screencols;
}



int main(int argc, char *argv[]) {
  enableRawMode();
  initEditor();
  if (argc >= 2) {
    editorOpen(argv[1]);
  }

 editorSetStatusMessage("C-x C-c = quit | C-x C-s = save | C-f = find | C-o = newline");
  
  while (1) {
    editorRefreshScreen();
    editorProcessKeypress();
    /*if (E.auto_complete) {
      editorAutoComplete();
    }*/
  }
  return 0;
}
