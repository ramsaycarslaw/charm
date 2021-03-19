// Copyright 2020 Ramsay Carslaw

/*** includes ***/
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "../include/buffer.h"
#include "../include/editor.h"
#include "../include/init.h"
#include "../include/term.h"

/*** defines ***/

#define CTRL_KEY(k) ((k)&0x1f)

#define RCC_QUIT_TIMES 2

#define HL_HIGHLIGHT_NUMBERS (1 << 0)
#define HL_HIGHLIGHT_STRINGS (1 << 1)
#define HL_HIGHLIGHT_FUNC (1 << 2)
#define HL_HIGHLIGHT_MACROS (1 << 3)
#define HL_TEX_ENV (1 << 4)

/*** data ***/

/*** filetypes ***/

char *C_HL_extensions[] = {".c", ".h", ".cpp", NULL};
char *C_HL_keywords[] = {
    "switch",    "if",    "while",     "for",     "break",   "continue",
    "return",    "else",  "struct",    "union",   "typedef", "static",
    "enum",      "class", "case",      "int|",    "long|",   "double|",
    "float|",    "char|", "unsigned|", "signed|", "void|",   "#define|",
    "#include|", NULL};

char *GO_HL_extensions[] = {".go", NULL};
char *GO_HL_keywords[] = {
    "switch", "if",     "defualt", "for",     "break",   "continue", "return",
    "else",   "struct", "package", "typedef", "static",  "enum",     "class",
    "case",   "func",   "var",     "int|",    "string|", "uint|",    "float|",
    "char|",  "int16|", "int32|",  "int64|",  "import|", NULL};

char *MT_HL_extensions[] = {".MT", ".mt", ".mtl", NULL};
char *MT_HL_keywords[] = {
    "if",     "else",   "fn",       "print",   "return",   "class",
    "this",   "break",  "continue", "switch",  "case",     "default",
    "super",  "init|",  "len|",     "printf|", "println|", "read|",
    "write|", "clock|", "use",      "string|", "number|",  "color|",
    "for",    "while",  "exit|",    "clear|",  "show|",    "Cd|",
    "Ls|",    "input|", "append|",  "delete|", "var", "let", NULL};

char *RUST_HL_extensions[] = {".rs", ".RS", NULL};
char* RUST_HL_keywords[] = {
  // all rust reserved keywords
  "as", "break", "const", "continue", "crate", 
  "else", "enum", "extern", "false", "fn", "for", "if", "impl", "in", 
  "let", "loop", "match", "mod", "move", "mut", "pub", "ref", 
  "return", "self", "Self", "static", "struct", "super", 
  "trait", "true", "type", "unsafe", "use", "where", "while", "async", 
  "await", "dyn", "abstract|", "become|", "box|", "do|", "final|", 
  "macro|", "override|", "priv|", "typeof|", "unsized|", "virtual|", 
  "yield|", "try|", NULL};

char *PY_HL_extensions[] = {".py", NULL};
char *PY_HL_keywords[] = {
    // Python keywords and built-in functions
    "and", "as", "assert", "break", "class", "continue", "def", "del", "elif",
    "else", "except", "exec", "finally", "for", "from", "global", "if",
    "import", "in", "is", "lambda", "not", "or", "pass", "print", "raise",
    "return", "try", "while", "with", "yield", "async", "await", "nonlocal",
    "range", "xrange", "reduce", "map", "filter", "all", "any", "sum", "dir",
    "abs", "breakpoint", "compile", "delattr", "divmod", "format", "eval",
    "getattr", "hasattr", "hash", "help", "id", "input", "isinstance",
    "issubclass", "len", "locals", "max", "min", "next", "open", "pow", "repr",
    "reversed", "round", "setattr", "slice", "sorted", "super", "vars", "zip",
    "__import__", "reload", "raw_input", "execfile", "file", "cmp",
    "basestring",
    // Python types
    "buffer|", "bytearray|", "bytes|", "complex|", "float|", "frozenset|",
    "int|", "list|", "long|", "None|", "set|", "str|", "chr|", "tuple|",
    "bool|", "False|", "True|", "type|", "unicode|", "dict|", "ascii|", "bin|",
    "callable|", "classmethod|", "enumerate|", "hex|", "oct|", "ord|", "iter|",
    "memoryview|", "object|", "property|", "staticmethod|", "unichr|", NULL};

char *TEX_HL_extensions[] = {".tex", NULL};
char *TEX_HL_keywords[] = {
    "\\usepackage", "\\documentclass", "\\author", "\\title",  "\\centering",
    "\\maketitle",  "\\begin",         "\\end",    "$$|",      "\\newcommand",
    "equation|", "equation}|",    "figure|", "figure}|",  "theorem|", "theorem}|", 
    "tabular|",  "tabular}|", 
    "document}|", "\\[", "\\]", "&", NULL};

char *JAVA_HL_extensions[] = {".java", NULL};
char *JAVA_HL_keywords[] = {"_", "abstract", "assert ", "boolean|", "Boolean|", "break",
  "byte", "case", "catch", "char|", "class", "const", "continue", "default", "do", 
  "double|", "else", "enum ", "extends", "final", "finally", "float|", "for", "goto", 
  "if", "implements", "import", "instanceof", "int|", "interface", "long|", "native", 
  "new", "non-sealed", "package", "private", "protected", "public", "return", "short|",
  "static", "strictfp", "super", "switch", "String|", "synchronized", "this|", "throw",
  "throws", "transient", "try", "void|", "volatile", "while", "true|", "false|", "isinstanceof", NULL };

struct editorSyntax HLDB[] = {
    {"C", C_HL_extensions, C_HL_keywords, "//", "/*", "*/",
     HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_FUNC},
    {"Go", GO_HL_extensions, GO_HL_keywords, "//", "/*", "*/",
     HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_FUNC},
    {"MT", MT_HL_extensions, MT_HL_keywords, "//", "/*", "*/",
     HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_FUNC},
    {"Rust", RUST_HL_extensions, RUST_HL_keywords, "//", "/*", "*/",
      HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_FUNC | HL_HIGHLIGHT_MACROS },
    {"Python", PY_HL_extensions, PY_HL_keywords, "#", "/*", "*/",
     HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_FUNC},
    {"Java", JAVA_HL_extensions, JAVA_HL_keywords, "//", "/*", "*/",
      HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_FUNC},
    {"LaTeX", TEX_HL_extensions, TEX_HL_keywords, "%", "", "",
     HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS | HL_TEX_ENV}};

#define HLDB_ENTRIES (sizeof(HLDB) / sizeof(HLDB[0]))

/*** prototypes ***/

void editorDeleteChar();

void editorInsertChar(int c);

void editorSetStatusMessage(const char *fmt, ...);

void editorRefreshScreen();

void editorClearScreen();

char *editorPrompt(char *prompt, void (*callback)(char *, int));

void editorMoveCursor(int key);

/*** highlighting rules ***/

int is_separator(int c) {
  return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
}

void editorUpdateVisual() 
{
  // if we have exited visual mode
  if (E.visualy == -1 || E.visualx == -1) {
    editorSetStatusMessage("  NORMAL");
    return;
  }
  if (E.visualy > E.cy)
  {
    for (int i = E.cy; i < E.visualy; i++)
    {

      for (int j = 0; j < strlen(E.row[i].chars); j++)
      {
        E.row[i].hl[j] = HL_VISUAL;
      }
    }
  }
  else 
  {
    for (int i = E.visualy; i < E.cy; i++) {
      for (int j = 0; j < strlen(E.row[i].chars); j++) {
        E.row[i].hl[j] = HL_VISUAL;
      }
    } 
  }
}

void editorUpdateSyntax(erow *row) {
  row->hl = realloc(row->hl, row->rsize);
  memset(row->hl, HL_NORMAL, row->rsize);
  if (E.syntax == NULL)
    return;
  char **keywords = E.syntax->keywords;
  char *scs = E.syntax->singleline_comment_start;
  char *mcs = E.syntax->multiline_comment_start;
  char *mce = E.syntax->multiline_comment_end;
  int scs_len = scs ? strlen(scs) : 0;
  int mcs_len = mcs ? strlen(mcs) : 0;
  int mce_len = mce ? strlen(mce) : 0;
  int prev_sep = 1;
  int in_string = 0;

  int in_comment = (row->idx > 0 && E.row[row->idx - 1].hl_open_comment);
  int i = 0;
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
      if (c == '(' && row->render[i-1] != '!') {
        int j = i-1;
        while ((row->render[j] != ' ' || row->render[j] != '.') && j > 0) {
          if (row->render[j] == ' ' || row->render[j] == '.') {
            break;
          }
          row->hl[j] = HL_FUNC;
          j--;
        } 
      }
    }

    if (E.syntax->flags & HL_TEX_ENV) {
      if (c == '\\' && row->size > 1) {
        while (c != '{' && c != '\n' && c != ' ' ) {
          c = row->render[i];
          row->hl[i] = HL_KEYWORD1;
          i++;
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
        if (c == in_string)
          in_string = 0;
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

    if (E.syntax->flags & HL_HIGHLIGHT_MACROS) {
      if (c == '!') {
        row->hl[i] = HL_OTHER;
        int k = i;
        while (row->render[k] != ' ') {
          row->hl[k] = HL_OTHER;
          k--;
        } 
      }
    }

    if (prev_sep) {
      int j;
      for (j = 0; keywords[j]; j++) {
        int klen = strlen(keywords[j]);
        int kw2 = keywords[j][klen - 1] == '|';
        if (kw2)
          klen--;
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

/* Detect file type from extension */
void editorSelectSyntaxHighlight() {
  E.syntax = NULL;
  if (E.filename == NULL)
    return;

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

void editorUpdateRow(erow *row) {
  int tabs = 0;
  int j;

  for (j = 0; j < row->size; j++)
    if (row->chars[j] == '\t')
      tabs++;

  free(row->render);
  row->render = malloc(row->size + tabs * (RCC_TAB_STOP - 1) + 1);

  int idx = 0;
  for (j = 0; j < row->size; j++) {
    if (row->chars[j] == '\t') {
      row->render[idx++] = ' ';
      while (idx % RCC_TAB_STOP != 0)
        row->render[idx++] = ' ';
    } else {
      row->render[idx++] = row->chars[j];
    }
  }
  row->render[idx] = '\0';
  row->rsize = idx;

  editorUpdateSyntax(row);
}

void editorInsertRow(int at, char *s, size_t len) {
  if (at < 0 || at > E.numrows)
    return;
  E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));
  memmove(&E.row[at + 1], &E.row[at], sizeof(erow) * (E.numrows - at));
  for (int j = at + 1; j <= E.numrows; j++)
    E.row[j].idx++;
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
  if (at < 0 || at >= E.numrows)
    return;
  editorFreeRow(&E.row[at]);
  memmove(&E.row[at], &E.row[at + 1], sizeof(erow) * (E.numrows - at - 1));
  for (int j = at; j < E.numrows - 1; j++)
    E.row[j].idx--;
  E.numrows--;
  E.dirty++;
}

void editorRowInsertChar(erow *row, int at, int c) {
  if (at < 0 || at > row->size)
    at = row->size;
  row->chars = realloc(row->chars, row->size + 2);
  memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
  row->size++;
  row->chars[at] = c;
  editorUpdateRow(row);
  E.dirty++;
}

void editorRowAppendString(erow *row, char *s, size_t len) {
  row->chars = realloc(row->chars, row->size + len + 1);
  memcpy(&row->chars[row->size], s, len);
  row->size += len;
  row->chars[row->size] = '\0';
  editorUpdateRow(row);
  E.dirty++;
}

void editorRowDeleteChar(erow *row, int at) {
  if (at < 0 || at >= row->size)
    return;
  memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
  row->size--;
  editorUpdateRow(row);
  E.dirty++;
}

/*** editor operations ***/

void editorKillLine() {
  if (E.cy == 0) {
    return;
  }
  strcpy(E.paste, E.row[E.cy].chars);
  editorDelRow(E.cy);
  E.cy--;
  return;
}

int editorCountWhitespace(erow *row) {
  int i = 0;
  int indent = 0;
  char c;
  while ((c = row->chars[i])) {
    if (c == ' ') {
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
  if (E.cy != 0) {

    // case where last line was }
    if (E.row[E.cy-1].chars[strlen(E.row[E.cy-1].chars)-1] == '}') 
    {
      int above = editorCountWhitespace(&E.row[E.cy-1]);

      if (above >= RCC_TAB_STOP) {
        if (E.row[E.cy-1].chars[0] == '\t') 
        {
          editorRowDeleteChar(&E.row[E.cy-1], 0);
        } else {
          for (int j = 0; j < RCC_TAB_STOP; j++) 
          {  
            editorRowDeleteChar(&E.row[E.cy-1], 0);
          }
        }
      }   
    }

  // normal indent
  int level = editorCountWhitespace(&E.row[E.cy-1]);

    for (int i = 0; i < level; i++)
      editorInsertChar(' ');

  }
}

static void editorCenter(void) {
  if (E.cy - E.screenrows / 2 != 0 && E.rowoff + E.cy - E.screenrows / 2 > 0 &&
      E.rowoff + E.cy + E.screenrows / 2 < E.numrows) {
    for (int i = 0; i < E.screenrows / 2; i++) {
      E.cy++;
    }
    for (int i = 0; i < E.screenrows / 2; i++) {
      E.cy--;
    }
  }
}

void editorInsertChar(int c) {
  E.current_word[E.current_word_len] = c;
  E.current_word_len++;

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
  if (E.cy == E.numrows)
    return;
  if (E.cx == 0 && E.cy == 0)
    return;

  erow *row = &E.row[E.cy];
  if (E.cx > 0) {
    editorRowDeleteChar(row, E.cx - 1);
    E.cx--;
  } else {
    E.cx = E.row[E.cy - 1].size;
    editorRowAppendString(&E.row[E.cy - 1], row->chars, row->size);
    editorDelRow(E.cy);
    E.cy--;
  }
}

/* Like vims c keyword */
void editorChangeInner() {
  erow *row = &E.row[E.cy];

  if (E.cx == row->size) {
    return;
  }

  for (int i = 0; i < row->size; i++) {
    if (row->chars[i] == '(') {
      E.cx = i + 1;
      while (row->chars[E.cx] != ')') {
        editorRowDeleteChar(row, E.cx);
      }
    }

    if (row->chars[i] == '[') {
      E.cx = i + 1;
      while (row->chars[E.cx] != ']') {
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

/* Open a file */
void editorOpen(char *filename) {
  free(E.filename);
  E.filename = strdup(filename);

  editorSelectSyntaxHighlight();

  FILE *fp = fopen(filename, "r");
  if (!fp) {
    fp = fopen (filename, "w");
  }
  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;
  while ((linelen = getline(&line, &linecap, fp)) != -1) {
    while (linelen > 0 &&
           (line[linelen - 1] == '\n' || line[linelen - 1] == '\r'))
      linelen--;
    editorInsertRow(E.numrows, line, linelen);
  }
  free(line);
  fclose(fp);
  E.dirty = 0;
}

/* Saves an open file */
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
  if (last_match == -1)
    direction = 1;
  int current = last_match;
  int i;
  for (i = 0; i < E.numrows; i++) {
    current += direction;
    if (current == -1)
      current = E.numrows - 1;
    else if (current == E.numrows)
      current = 0;
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
  char *query = editorPrompt("/%s", editorFindCallback);
  if (query) {
    free(query);
  } else {
    E.cx = saved_cx;
    E.cy = saved_cy;
    E.coloff = saved_coloff;
    E.rowoff = saved_rowoff;
  }
}

/*** output ***/

void resetCurrentWord() {
  for (int i = 0; i < E.current_word_len; i++)
    E.current_word[i] = '\0';
  E.current_word_len = 0;
}

void editorAutoComplete() {
  char words[10][20] = { "let", "fn", "break", "return", "append", "println!", "use", "for", "while", "else" }; 

  for (int i = 0; i < 10; i++) {
    int match = 1;
    for (int j = 0; j < E.current_word_len; j++) {
      match = match && (E.current_word[j] == words[i][j]);
    }
    if (match) {
      for (int k = 0; k < E.current_word_len; k++)
        editorDeleteChar();
      editorRowAppendString(&E.row[E.cy], words[i], strlen(words[i]));
      // E.cx += strlen(words[i]);
      break;
    }
  }
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
      if (buflen != 0)
        buf[--buflen] = '\0';
    } else if (c == '\x1b') {
      editorSetStatusMessage("");
      if (callback)
        callback(buf, c);
      free(buf);
      return NULL;
    } else if (c == '\r') {
      if (buflen != 0) {
        editorSetStatusMessage("");
        if (callback)
          callback(buf, c);
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
    if (callback)
      callback(buf, c);
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
  int c = editorReadKey();
  editorUpdateVisual();
  if (E.normal && E.vim) {
    // NORMAL
    
    if (E.replace_char) {
      E.row[E.cy].chars[E.cx] = c;
      editorUpdateRow(&E.row[E.cy]);
      E.replace_char = 0;
      return;
    }

    if (E.find_mode == 1) {
      for (int i = E.cx+1; i < E.row[E.cy].size; i++) {
        if (E.row[E.cy].chars[i] == c) {
          E.cx = i;
          E.find_mode = 0;
          return;
        }
      }
      E.find_mode = 0;
      return;
    } else if (E.find_mode == 2) {
      for (int i = E.cx+1; i >= 0; i--) {
        if (E.row[E.cy].chars[i] == c) {
          E.cx = i;
          E.find_mode = 0;
          return;
        }
      }
      E.find_mode = 0;
      return;
    }

    switch (c) {
    /* Comamnd Mode */
    case ':':
      editorSetStatusMessage("  COMMAND");
      char *response = editorPrompt(":%s", NULL);
      if (strcmp(response, "w") == 0) {
        editorSave();
      } else if (strcmp(response, "wq") == 0 || strcmp(response, "x") == 0) {
        editorSave();
        write(STDOUT_FILENO, "\x1b[2J", 4);
        write(STDOUT_FILENO, "\x1b[H", 3);
        exit(74);
      } else if (strcmp(response, "q") == 0) {
        if (E.dirty) {
          editorSetStatusMessage("Unsaved changes use 'q!' to override.");
          break;
        }
        write(STDOUT_FILENO, "\x1b[2J", 4);
        write(STDOUT_FILENO, "\x1b[H", 3);
        exit(74);

      } else if (strcmp(response, "q!") == 0) {
        
        write(STDOUT_FILENO, "\x1b[2J", 4);
        write(STDOUT_FILENO, "\x1b[H", 3);
        exit(74);
      } else if (response[0] == ':' && response[1] == 'e') {
        
        if (strlen(response) < 3) {
          editorSetStatusMessage("Specify a file to open!"); 
          break;
        }
        response += 3;
        printf("got %s\n", response);
        editorOpen(response);
      } else if (strcmp(response, "source") == 0) {
        parseInitFile();
        break;
      } else {
        editorRunFunction(response);
      }
      break;

    /* Modifiers */
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      E.normal_mod *= 10;
      char buf[2];
      buf[0] = c;
      buf[1] = '\0';
      E.normal_mod += atoi(buf);
      break;

    /* Insert mode */
    case 'i':
      editorSetStatusMessage("--INSERT--");
      E.normal = 0;
      break;

    case 'I':
      E.cx = 0;
      E.normal = 0;
      editorSetStatusMessage("--INSERT--");
      break;

    case 'a':
      editorSetStatusMessage("--INSERT--");
      E.cx++;
      E.normal = 0;
      break;

    case 'A':
      if (E.cy < E.numrows)
        E.cx = E.row[E.cy].size;
      E.normal = 0;
      editorSetStatusMessage("--INSERT--");
      break;

    case 'c':
      editorChangeInner();
      editorSetStatusMessage("--INSERT--");
      E.normal = 0;
      break;

    case 'o':
      editorSetStatusMessage("--INSERT--");
      if (E.cy < E.numrows)
        E.cx = E.row[E.cy].size;
      E.normal = 0;
      editorInsertNewline();
      editorAutoIndent();
      break;

    case 'O':
      editorSetStatusMessage("--INSERT--");
      E.cy--;
      if (E.cy < E.numrows)
        E.cx = E.row[E.cy].size;
      E.normal = 0;
      editorInsertNewline();
      editorAutoIndent();
      break;

    case 'r': {
      E.replace_char = 1;
      editorSetStatusMessage("Replace (Char)");
      break;
    }

    /* Find commands */
    case 'f': {
      E.find_mode = 1;          
      break;
    }

    case 'F': {
      E.find_mode = 2;          
    }

      /* Words etc */
    case 'w':
      if (E.normal_mod == 0)
        E.normal_mod = 1;

      while (E.normal_mod != 0) {

        E.normal_mod--;
        if (strlen(E.row[E.cy].chars) == 0) {
          if (E.cy < E.numrows)
            E.cy++;
          break;
        }

        if (E.row[E.cy].chars[E.cx] == ' ') {
          E.cx++;
        }
        while (E.row[E.cy].chars[E.cx] != ' ' && E.cy <= E.numrows) {
          if (E.cx == strlen(E.row[E.cy].chars)) {
            E.cx = strlen(E.row[E.cy].chars) - 1;
            break;
          }

          if (E.cy >= E.numrows)
            break;

          if (E.cx + 1 > strlen(E.row[E.cy].chars) - 1) {
            if (E.cy < E.numrows)
              E.cy++;
            E.cx = 0;
            break;
          }

          E.cx++;
        }
        editorScroll();
      }
      break;

    case 'b':
      if (E.row[E.cy].chars[E.cx] == ' ') {
        E.cx--;
      }
      while (E.row[E.cy].chars[E.cx] != ' ') {
        E.cx--;
      }
      editorScroll();
      break;

    /* delete modes */
    case 'd':
      if (E.deletemode) {
        E.deletemode = 0;
        editorKillLine();
        E.cx = 0;
        break;
      }
      E.deletemode = 1;
      break;

    case 'x':
      E.normal_mod = (E.normal_mod == 0) ? 1 : E.normal_mod;
      while (E.normal_mod != 0) {
      E.cx++;
      editorDeleteChar();
      E.normal_mod--;
      }
      break;

    case 'p':
      editorInsertRow(E.cy, E.paste, strlen(E.paste)); 
      break;

    /* Visual modes */
    case 'V': {
      editorSetStatusMessage("VISUAL LINE");
      E.visual_line_start = E.cy;
      break;
    }

    case 27: {
      if (E.visualy != -1 && E.visualx != -1)
      {
        E.cx = E.visualx;
        E.cy = E.visualy;
      }
      E.visualy = -1;
      E.visualx = -1;
      for (int i = 0; i < E.numrows; i++) {
        editorUpdateRow(&E.row[i]);
      }
      break;
    }
      

    case 'v': {
      editorSetStatusMessage("-- VISUAL --");          
      E.visualx = E.cx;
      E.visualy = E.cy;
      editorUpdateVisual();
      break;
    }

    /* top and bottom */
    case 'J':
    case 'G':
        E.cy = E.numrows-1;

    case 'g':
    case 'K':
        E.cy = 0;

    /* cursor movement */
    case '\r':
    case ARROW_DOWN:
    case 'j':
      /* Handle '10j' etc */
      if (E.normal_mod != 0) {
        if (E.cy + E.normal_mod > E.numrows) {
          E.cy = E.numrows;
          E.normal_mod = 0;
          break;
        }
        E.cy += E.normal_mod;
        E.normal_mod = 0;
        break;
      }
      if (E.visualx != -1 || E.visualy != -1) {
        for (int i = 0; i < E.numrows; i++)
          editorUpdateRow(&E.row[i]);        
        editorUpdateVisual();
      }

      editorMoveCursor(ARROW_DOWN);
      break;
    case ARROW_UP:
    case 'k':
      if (E.normal_mod != 0) {
        if (E.cy - E.normal_mod < 0) {
          E.cy = 0;
          E.normal_mod = 0;
          break;
        }
        E.cy -= E.normal_mod;
        E.normal_mod = 0;
        break;
      }
      if (E.visualx != -1 || E.visualy != -1) {
        for (int i = 0; i < E.numrows; i++)
          editorUpdateRow(&E.row[i]);        
        editorUpdateVisual();
      }
      editorMoveCursor(ARROW_UP);
      break;
      
    case 'm':
      editorSetStatusMessage(E.current_word);
      break;

    case BACKSPACE:
    case ARROW_LEFT:
    case 'h':
      editorMoveCursor(ARROW_LEFT);
      break;
    case ' ':
    case ARROW_RIGHT:
    case 'l':
      editorMoveCursor(ARROW_RIGHT);
      break;

    case '}':
      if (E.cy + 10 >= E.numrows) {
        E.cy = E.numrows-1;
        break;
      }
      E.cy += 10;
      break;

    case '{':
      if (E.cy - 10 < 0) {
        E.cy = 0;
        break;
      }
      E.cy -= 10;
      break;

    case 'H':
    case '0':
      /* Allow '10j' etc */
      if (E.normal_mod > 0) {
        E.normal_mod *= 10;
        break;
      }
      E.cx = 0;
      break;

    case 'L':
    case '$':
      if (E.cy < E.numrows)
        E.cx = E.row[E.cy].size;
      break;

    /* Finding stuff */
    case '/':
      editorFind();
      break;

    /* No match */
    default:
      break;
    }
  } else {
    // INSERT
    switch (c) {
    case '\r':
      editorInsertNewline();
      editorAutoIndent();
      break;

    case '.':
    case ' ':
      resetCurrentWord();
      editorInsertChar(c);
      break;

    case 27:
      if (E.vim) {
        E.normal = 1;
        editorSetStatusMessage("  NORMAL");
        E.cx--;
      }
      break;
    case CTRL_KEY('q'):
      if (E.dirty && quit_times > 0) {
        editorSetStatusMessage("Warning: File has unsaved changes. "
                               "Press Ctrl-Q %d more times to quit.",
                               quit_times);
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

    case CTRL_KEY('f'):
      if (E.row[E.cy].chars[E.cx] == ' ') {
        E.cx++;
      }
      while (E.row[E.cy].chars[E.cx] != ' ') {
        E.cx++;
      }
      editorScroll();
      break;

    case CTRL_KEY('b'):
      if (E.row[E.cy].chars[E.cx] == ' ') {
        E.cx--;
      }
      while (E.row[E.cy].chars[E.cx] != ' ') {
        E.cx--;
      }
      editorScroll();
      break;

    case CTRL_KEY('p'):
      resetCurrentWord();
      editorAutoComplete();
      break;


    case CTRL_KEY('s'):
      if (E.prefix) {
        E.prefix = 0;
        editorSave();
        break;
      }
      editorFind();
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
    case DEL_KEY:
      resetCurrentWord();
      if (c == DEL_KEY || c == CTRL_KEY('d'))
        editorMoveCursor(ARROW_RIGHT);
      editorDeleteChar();
      break;

    case PAGE_UP:
    case PAGE_DOWN: {
      if (c == PAGE_UP) {
        E.cy = E.rowoff;
      } else if (c == PAGE_DOWN) {
        E.cy = E.rowoff + E.screenrows - 1;
        if (E.cy > E.numrows)
          E.cy = E.numrows;
      }
      int times = E.screenrows;
      while (times--)
        editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
    } break;
    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
      editorMoveCursor(c);
      break;
    case CTRL_KEY('g'):
      break;
    default:
      editorInsertChar(c);
      break;
    }
    quit_times = RCC_QUIT_TIMES;
  }
  // highlight matchng parens
  /*if (E.row[E.cy].chars[E.cx] == ')') {
    editorSetStatusMessage("(");
    int line = E.cy;
    for (int i = E.cx; i >= 0; i--) {
      if (E.row[line].chars[i] == '(') {
        editorUpdateRow(&E.row[line]);
        break;
      }
    }
  }*/
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
  E.paste[0] = '\0';
  E.statusmsg_time = 0;
  E.linenum_indent = 6;
  E.indent = 0;

  /* Editor modes */
  E.normal = 1;
  E.normal_mod = 0;
  E.replace_char = 0;

  /* delete modes */
  E.deletemode = 0;
  E.find_mode = 0;
  E.vim = 1;

  /* Visual mode */
  E.visual_line_start = -1;
  E.visual_line_end = -1;

  E.visualx = -1;
  E.visualy = -1;

  /* Current word */
  E.current_word[0] = '\0';
  E.current_word_len = 0;

  E.syntax = NULL;
  E.prefix = 0;
  E.mx = 0;
  E.my = 0;
  E.mroff = 0;
  E.mcoff = 0;

  if (getWindowSize(&E.raw_screenrows, &E.raw_screencols) == -1)
    die("getWindowSize");
  E.screenrows = E.raw_screenrows - 2;
  E.screencols = E.raw_screencols;
}

int main(int argc, char *argv[]) {
  enableRawMode();
  parseInitFile();
  initEditor();
  if (argc >= 2) {
    editorOpen(argv[1]);
  }

  while (1) {
    editorRefreshScreen();
    editorProcessKeypress();
    /*if (E.auto_complete) {
      editorAutoComplete();
    }*/
  }
  return 0;
}
