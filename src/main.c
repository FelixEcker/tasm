/* t(heft)asm: main.c ; author: Marie Eckert
 *
 * Copyright (c) 2023, Marie Eckert
 */

#include <assembler.h>
#include <log.h>

#include <stdio.h>
#include <argp.h>

const char *argp_program_version = "mariebuild 0.0.0";
const char *argp_program_bug_address =
  "https://github.com/FelixEcker/tasm/issues";
const char description[] =
  "Assembler for the Theft fantasy cpu\n"
  "Author: Marie Eckert";
const char args_doc[] = "";

static struct argp_option options[] = {
  {"in", 'i', "FILE", 0, "Specify the input assembly source"}
, {"out", 'o', "FILE", 0, "Specify the output filename"}
, {"format", 'f', "rom/tef", 0, "Specify the output format (default=rom)"}
, {0, 0, 0, 0}
};

struct arguments {
  char *in;
  char *out;
  char *format;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  struct arguments *args = state->input;
  switch (key) {
  case 'i': args->in = arg; break;
  case 'o': args->out = arg; break;
  case 'f': args->format = arg; break;
  default: return ARGP_ERR_UNKNOWN;
  }

  return 0;
}

static struct argp argp = { options, parse_opt, args_doc, description };

int main(int argc, char **argv) {
  struct arguments args;
  args.in = NULL;
  args.out = "asm.out";
  args.format = TASM_OUT_ROM;
  
  argp_parse(&argp, argc, argv, 0, 0, &args);

  if (args.in == NULL) {
    log_err("Missing assembler input file!\n");
    return 1;
  }

  return asm_write_file(args.in, args.out, args.format);
}
