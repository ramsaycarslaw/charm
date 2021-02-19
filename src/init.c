// Copyright (C) 2021 ramsaycarslaw

#include "../include/init.h"

// adjust theme here using ANSI 3/4 bit colors
int editorSyntaxToColor(int hl) {
  switch (hl) {
  case HL_MLCOMMENT:
  case HL_COMMENT:
    return colors.commentColor;
  case HL_FUNC:
    return colors.funcColor;
  case HL_KEYWORD1:
    return colors.keyword1Color;
  case HL_KEYWORD2:
    return colors.keyword2Color;
  case HL_STRING:
    return colors.stringColor;
  case HL_NUMBER:
    return colors.numberColor;
  case HL_MATCH:
    return colors.matchColor;
  default:
    return colors.normalColor;
  }
}

// read a file from a given path
static char *readFile(const char *path) {
  FILE *file = fopen(path, "rb");
  if (file == NULL) {
    fprintf(stderr, "Could not open file \"%s\".\n", path);
    exit(74);
  }

  fseek(file, 0L, SEEK_END);
  size_t fileSize = ftell(file);
  rewind(file);

  char *buffer = (char *)malloc(fileSize + 1);
  if (buffer == NULL) {
    fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
    exit(74);
  }
  size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
  if (bytesRead < fileSize) {
    fprintf(stderr, "Could not read file \"%s\".\n", path);
    exit(74);
  }
  buffer[bytesRead] = '\0';

  fclose(file);
  return buffer;
}

// Run an MT file
static void runFile(const char *path) {
  // int len = (int) strlen(path);

  char *source = readFile(path);

  interpret(source);
  free(source);
}

// parse the init file
int parseInitFile() {
  initVM();

  // init file for now
  runFile("/Users/ramsaycarslaw/.charm.mt");

  /* Get the colors from the init file */
  colors.normalColor =
      AS_NUMBER(interpret("print ((normalColor == nil) ? 7 :  normalColor);"));
  colors.commentColor = AS_NUMBER(interpret("print commentColor;"));
  colors.funcColor = AS_NUMBER(interpret("print funcColor;"));
  colors.keyword1Color = AS_NUMBER(interpret("print keyword1Color;"));
  colors.keyword2Color = AS_NUMBER(interpret("print keyword2Color;"));
  colors.stringColor = AS_NUMBER(interpret("print stringColor;"));
  colors.numberColor = AS_NUMBER(interpret("print numberColor;"));
  colors.backgroundColor = AS_NUMBER(interpret("print backgroundColor;"));
  colors.matchColor = AS_NUMBER(interpret("print matchColor;"));

  /* Window colors */
  colors.statusColor = AS_NUMBER(interpret("print statusColor;"));
  colors.linenumColor = AS_NUMBER(
      interpret("print ((linenumColor == nil) ? 244 : linenumColor);"));
  colors.linenumBGColor = AS_NUMBER(interpret("print linenumBGColor;"));

  freeVM();
  return 0;
}
