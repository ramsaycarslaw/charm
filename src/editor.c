// Copyright (C) 2021 Ramsay Carslaw
#include <ctype.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include "../include/buffer.h"
#include "../include/editor.h"
#include "../include/init.h"

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
    if (cur_rx > rx)
      return cx;
  }
  return cx;
}

/* Draw stuff to the screen */
void editorDrawRows(struct abuf *ab) {
  int y;

  /* Change the backgeound color
  char normal_bg[10];
  sprintf(normal_bg, "\x1b[48;5;%dm", 7);
  abAppend(ab, normal_bg, 10);
  */

  for (y = 0; y < E.screenrows; y++) {

    int filerow = y + E.rowoff;

    char normal_bg[12];
    sprintf(normal_bg, "\x1b[48;5;%dm", colors.backgroundColor);
    // draw line number
    char format[16];
    char linenum[E.linenum_indent + 1];
    memset(linenum, ' ', E.linenum_indent);
    snprintf(format, 4, "%%%dd", E.linenum_indent - 1);
    if (filerow < E.numrows) {
      snprintf(linenum, E.linenum_indent + 1, format, filerow + 1);
    }

    // Get user linenumber colors
    // orig: 244,234
    char linenum_buffer[12];
    sprintf(linenum_buffer, "\x1b[38;5;%dm", colors.linenumColor);

    char linenumbg_buffer[12];
    sprintf(linenumbg_buffer, "\x1b[48;5;%dm", colors.linenumBGColor);

    abAppend(ab, linenum_buffer, 11);
    abAppend(ab, linenumbg_buffer, 11);
    abAppend(ab, linenum, E.linenum_indent);
    abAppend(ab, "\x1b[49m", 5);
    abAppend(ab, "\x1b[39m", 5);
    abAppend(ab, " ", 1);

    if (filerow >= E.numrows) {
      if (E.numrows == 0 && y == E.screenrows / 3) {
        abAppend(ab, " ", 1);
      } else {
        abAppend(ab, " ", 1);
      }
    } else {
      int len = E.row[filerow].rsize - E.coloff;
      if (len < 0)
        len = 0;
      if (len > E.screencols)
        len = E.screencols;
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
            abAppend(ab, normal_bg, 10);
            abAppend(ab, buf, clen);
          }
        } else if (hl[j] == HL_NORMAL) {
          if (current_color != -1) {
            /* Change the color of normal text */
            char normal_text[12];

            sprintf(normal_text, "\x1b[38;5;%dm", colors.normalColor);
            abAppend(ab, normal_bg, 10);
            abAppend(ab, normal_text, strlen(normal_text));

            current_color = -1;
          }
          abAppend(ab, &c[j], 1);
        } else {
          int color = editorSyntaxToColor(hl[j]);
          if (color != current_color) {
            current_color = color;
            char buf[16];
            int clen = 0;
            /* If its visual mode we change the background */
            if (color == colors.visualColor) {
              clen = snprintf(buf, sizeof(buf), "\x1b[48;5;%dm", color);
            } else {
              clen = snprintf(buf, sizeof(buf), "\x1b[38;5;%dm", color);
            }
            abAppend(ab, buf, clen);
          }
          abAppend(ab, &c[j], 1);
        }
      }
      abAppend(ab, "\x1b[39m", 5);
      abAppend(ab, normal_bg, 10);
    }
    abAppend(ab, "\x1b[K", 3);
    abAppend(ab, "\r\n", 2);
  }
}

void editorDrawStatusBar(struct abuf *ab) {
  char status_bar[14];
  sprintf(status_bar, "\x1b[48;5;%d;7m", colors.statusColor);
  char status_bar_fg[14];
  sprintf(status_bar_fg, "\x1b[38;5;%d;7m", 244);
  abAppend(ab, status_bar_fg, 14);
  char status[80], rstatus[80];
  int len = snprintf(status, sizeof(status), "%.20s - %d lines %s",
                     E.filename ? E.filename : "[No Name]", E.numrows,
                     E.dirty ? "(modified)" : "");

  float percent = (E.numrows == 0) ? 0 : 100*((E.cy + 1) / E.numrows);

  int rlen =
      snprintf(rstatus, sizeof(rstatus), "%s | %d,%d %.1f%%",
               E.syntax ? E.syntax->filetype : "text", E.cy + 1, E.cx, percent);
  if (len > E.raw_screencols)
    len = E.raw_screencols;
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
  if (msglen > E.raw_screencols)
    msglen = E.raw_screencols;
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
  // snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1,// (E.rx -
  // E.coloff) + 1 + E.linenum_indent); snprintf(buf, sizeof(buf),
  // "\x1b[%d;%dH", (E.cy - E.rowoff) + 1, E.cx + 1);
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy - E.rowoff + 1,
           E.rx - E.coloff + 1 + E.linenum_indent);
  abAppend(&ab, buf, strlen(buf));
  abAppend(&ab, "\x1b[?25h", 6);
  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);
}

void editorSetStatusMessage(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(E.statusmessage, sizeof(E.statusmessage), fmt, ap);
  va_end(ap);
  E.statusmsg_time = time(NULL);
}

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
