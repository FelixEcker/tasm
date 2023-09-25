#include <assembler.h>

#include <log.h>

#include <butter/strutils.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t _handle_err(err_t err, char *line, uint32_t linenum) {

  return 0;
}

static uint8_t _parse_str_tok(char **dest, char *tok) {
  if (dest == NULL) {
    log_wrn("internal: _parse_str_tok received NULL-poiting dest param!\n");
    return 0;
  }

  char *tokcpy = strdup(tok);

  size_t len = strlen(tokcpy);
  uint8_t is_end = 0;
  if (len == 1) {
    is_end = tokcpy[0] == TASM_CHAR_STRING_CONT;
  } else if (len == 0) {
    is_end = 1;
  } else {
    is_end = (tokcpy[len-1] == TASM_CHAR_STRING_CONT
        && tokcpy[len-2] != TASM_CHAR_ESCAPE);
  }

  if (is_end) {
    tokcpy[len-1] = 0;
  }

  char *fmt_tok = convert_escape_sequences(tokcpy);
  // Assemble new string
  char whitespace[] = " ";
  char *new = malloc(strlen(dest[0]) + strlen(fmt_tok) + 2);
  strcpy(new, dest[0]);
  strcpy(new + strlen(dest[0]), whitespace);
  strcpy(new + strlen(dest[0]) + 1, fmt_tok);

  // Swap pointers, free old string
  free(dest[0]);
  dest[0] = new;

  // cleanup
  free(fmt_tok);
  free(tokcpy);

  return !is_end;
}

err_t asm_parse_line(asm_tree_branch_t *branch, char *line) {
  char *linecpy = strdup(line);

  // Trim whitespace
  char *clean_line = trim_whitespace(linecpy);
  if (clean_line[0] == TASM_CHAR_COMMENT)
    goto parse_line_cleanup;

  char *pstrtok;
  char *tok = strtok_r(clean_line, " ", &pstrtok);

  char *keyword = NULL;
  size_t parameter_count = 0;
  char **parameters = NULL;

  uint8_t ix = 0;
  uint8_t in_string = 0;
  while (tok != NULL) {
    if (ix == 0) {
      keyword = strdup(tok);
      goto parse_line_next_tok;
    }

    if (in_string) {
      char *strptr = parameters[parameter_count - 1];
      in_string = _parse_str_tok(&strptr, tok);
      parameters[parameter_count - 1] = strptr;
      
      if (!in_string)
        printf("%s\n", parameters[parameter_count - 1]);
      goto parse_line_next_tok;
    }

    if (parameter_count == 0) {
      parameters = malloc(sizeof(char *));
    } else {
      parameters = realloc(parameters, sizeof(char *) * (parameter_count + 1));
    }

    if (tok[0] == TASM_CHAR_STRING_CONT) {
      in_string = 1;
      parameters[parameter_count] = strdup(tok + 1);
    } else {
      parameters[parameter_count] = strdup(tok);
    }

    parameter_count++;

  parse_line_next_tok:
    tok = strtok_r(NULL, " ", &pstrtok);
    ix++;
  }

parse_line_cleanup:
  free(linecpy);
  return TASM_OK;
}

err_t asm_parse_file(char *src_fl, asm_tree_t *ast) {
  FILE *file;
  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  uint32_t linenum = 0;

  errno = 0;
  file = fopen(src_fl, "r");
  if (file == NULL) {
    log_err("Error opening \"%s\": %s\n", src_fl, strerror(errno));
    return 1;
  }

  log_inf("Parsing \"%s\"\n", src_fl);

  // Create new tree branch for this file

  err_t err;
  while ((read = getline(&line, &len, file)) != -1) {
    linenum++;
    err = asm_parse_line(&ast->branches[ast->branch_count - 1], line);
    if (err != TASM_OK)
      if (_handle_err(err, line, linenum) == 0)
        goto parse_file_cleanup;
  }

parse_file_cleanup:
  fclose(file);
  return err;
}

int asm_write_file(char *src_fl, char *out_fl, char *format) {
  log_inf("Assembling \"%s\"\n", src_fl);
  log_inf("Step 1: Parsing Sources\n");
  asm_tree_t ast;
  err_t parse_err = asm_parse_file(src_fl, &ast);

  return parse_err;
}
