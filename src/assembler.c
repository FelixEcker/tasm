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

err_t asm_parse_line(asm_tree_branch_t *branch, char *line) {
  char *linecpy = strdup(line);

  // Trim whitespace
  char *clean_line = trim_whitespace(linecpy);
  if (clean_line[0] == TASM_CHAR_COMMENT)
    goto parse_line_cleanup;

  printf("%s\n", clean_line);
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
