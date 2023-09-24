#include <assembler.h>

#include <log.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

/*asm_res_t asm_asm(char *src, asm_ctx_t ctx) {
  asm_res_t res;
  res.line = 1;
  res.status = TASM_OK;
  res.result_size = 0;
  res.result = malloc(TASM_RESULT_ALLOC);

  return res;
}*/

int asm_write_file(char *src_fl, char *out_fl, char *format) {
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

  while ((read = getline(&line, &len, file)) != -1) {
    linenum++;
    printf("%s", line);
  }

  return 0;
//  return res.status;
}
