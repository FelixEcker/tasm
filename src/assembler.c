#include <assembler.h>

#include <log.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>


static uint8_t _handle_err(err_t err, char *line, uint32_t linenum) {

  return 0;
}

err_t asm_parse_line(asm_tree_t *ast, char *line, uint32_t linenum) {
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

  log_inf("Assembling \"%s\"\n", src_fl);
  log_inf("Step 1: Parsing Sources\n");
  
  err_t err;
  while ((read = getline(&line, &len, file)) != -1) {
    linenum++;
    err = asm_parse_line(ast, line, linenum);
    if (err != TASM_OK)
      if (_handle_err(err, line, linenum) == 0)
        goto parse_file_cleanup;
  }

parse_file_cleanup:
  fclose(file);
  return err;
}

int asm_write_file(char *src_fl, char *out_fl, char *format) {
  asm_tree_t ast;
  err_t parse_err = asm_parse_file(src_fl, &ast);

  return parse_err;
}
