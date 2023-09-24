#include <assembler.h>

#include <log.h>

#include <stdlib.h>
#include <stdio.h>

asm_res_t asm_asm(char *src, asm_ctx_t ctx) {
  asm_res_t res;
  res.line = 1;
  res.status = TASM_OK;
  res.result_size = 0;
  res.result = malloc(TASM_RESULT_ALLOC);

  return res;
}

int asm_write_file(char *src_fl, char *out_fl, char *format) {
  FILE *file;


  asm_res_t res = asm_asm("test");

  return res.status;
}
