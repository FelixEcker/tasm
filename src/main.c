// t(heft)asm ; main.c
//----------------------------------------
// Copyright (c) 2023, Marie Eckert
// Licensed under the BSD 3-Clause License

#include <assembler.h>
#include <log.h>

#include <argp.h>
#include <stdio.h>

#define AUTHOR "Marie Eckert"

const char *argp_program_version = "tasm 0.0.0";
const char *argp_program_bug_address =
    "https://github.com/FelixEcker/tasm/issues";
const char description[] = "Assembler for the Theft fantasy cpu\n"
                           "Author: " AUTHOR;
const char args_doc[] = "";

static struct argp_option options[] = {
    {"in", 'i', "FILE", 0, "Specify the input assembly source"},
    {"out", 'o', "FILE", 0, "Specify the output filename"},
    {"format", 'f', "rom/tef", 0, "Specify the output format (default=rom)"},
    {"search-dirs", 's', "DIRßCOTRY", 0, "Specify a colon seperated list of "
      "directories to search through for included files"},
    {0, 0, 0, 0}};

struct arguments {
  char *in;
  char *out;
  char *format;
  char *search_dirs;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  struct arguments *args = state->input;
  switch (key) {
  case 'i':
    args->in = arg;
    break;
  case 'o':
    args->out = arg;
    break;
  case 'f':
    args->format = arg;
    break;
  case 's':
    args->search_dirs = arg;
    break;
  default:
    return ARGP_ERR_UNKNOWN;
  }

  return 0;
}

static struct argp argp = {options, parse_opt, args_doc, description};

int main(int argc, char **argv) {
  struct arguments args;
  args.in = NULL;
  args.out = "asm.out";
  args.format = TASM_OUT_ROM;

  argp_parse(&argp, argc, argv, 0, 0, &args);
  
  printf("%s by " AUTHOR "\n\n", argp_program_version);

  if (args.in == NULL) {
    log_err("Missing assembler input file!\n");
    return 1;
  }

  return asm_write_file(args.in, args.out, args.format);
}
