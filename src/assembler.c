// t(heft)asm ; assembler.c
//----------------------------------------
// Copyright (c) 2023, Marie Eckert
// Licensed under the BSD 3-Clause License

#include <assembler.h>

#include <log.h>

#include <butter/strutils.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//-- Static Utilities --//

static uint8_t _handle_err(err_t err, char *file, char *line,
                           uint32_t linenum) {
  log_err("Assembly failed!\n");
  log_err("%s\n", asm_errname(err));
  log_err("%s:%lu: %s\n", file, linenum, line);
  return 0;
}

static size_t _dir_exp_size(asm_exp_t exp) {
  switch (exp.directive) {
  case DIR_BYTES:
  case DIR_PADDING:
  case DIR_NULLPAD:
    if (exp.parameters == NULL || exp.parameters[0] == NULL)
      return 0;
    
    char *pend;
    return (size_t) strtol(exp.parameters[0], &pend, 0);
  case DIR_BYTE:
    return 1;
  default:
    return 0;
  }
}

static size_t _get_inst_size(inst_t inst) {
  for (int i = 0; i < INST_COUNT; i++) {
    if (inst_descriptors[i].inst == inst) {
      return inst_descriptors[i].size;
    }
  }

  return 0;
}

static size_t _precalc_size(asm_tree_t *ast) {
  size_t ret = 0;

  for (size_t b = 0; b < ast->branch_count; b++) {
    asm_tree_branch_t branch = ast->branches[b];

    for (size_t e = 0; e < branch.exp_count; e++) {
      asm_exp_t exp = branch.asm_exp[e];
      if (exp.type != EXP_INSTRUCTION) {
        if (exp.directive != DIR_NULLPAD
        &&  exp.directive != DIR_BYTE
        &&  exp.directive != DIR_BYTES)
          continue;
      
        ret += _dir_exp_size(exp);
        continue;
      }

      ret += _get_inst_size(exp.inst);
    }
  }

  return ret;
}

static err_t _do_dir_include(asm_tree_t *ast, char **params,
                             size_t param_count) {
  if (param_count < 1)
    return TASM_DIRECTIVE_MISSING_PARAMETER;

  return asm_parse_file(params[0], ast);
}

static uint8_t _str_closed(char *str) {
  size_t len = strlen(str);

  if (len == 1) {
    return str[0] == TASM_CHAR_STRING_CONT;
  } else if (len == 0) {
    return 1;
  }

  return (str[len - 1] == TASM_CHAR_STRING_CONT &&
          str[len - 2] != TASM_CHAR_ESCAPE);
}

static uint8_t _parse_str_tok(char **dest, char *tok) {
  if (dest == NULL) {
    log_wrn("internal: _parse_str_tok received NULL-poiting dest param!\n");
    return 0;
  }

  char *tokcpy = strdup(tok);

  uint8_t is_end = _str_closed(tokcpy);
  if (is_end) {
    tokcpy[strlen(tokcpy) - 1] = 0;
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

//-- Assembly Funcs --//

err_t asm_parse_exp(asm_tree_branch_t *branch, char *keyword,
                    size_t param_count, char **params) {
  err_t ret = TASM_OK;

  if (branch->exp_count == 0) {
    branch->asm_exp = malloc(sizeof(asm_exp_t));
  } else {
    branch->asm_exp =
        realloc(branch->asm_exp, sizeof(asm_exp_t) * (branch->exp_count + 1));
  }

  branch->asm_exp[branch->exp_count].parameter_count = param_count;
  branch->asm_exp[branch->exp_count].parameters = params;

  branch->asm_exp[branch->exp_count].type = EXP_INSTRUCTION;
  if (keyword[0] == TASM_CHAR_DIRECTIVE_PREFIX) {
    branch->asm_exp[branch->exp_count].type = EXP_DIRECTIVE;
    branch->asm_exp[branch->exp_count].directive = get_dir(keyword + 1);
  } else {
    branch->asm_exp[branch->exp_count].inst = get_inst(keyword);
  }
  branch->exp_count++;

  return ret;
}

err_t asm_parse_line(asm_tree_branch_t *branch, char *line) {
  err_t ret = TASM_OK;
  char *linecpy = strdup(line);

  char *keyword = NULL;

  // Trim whitespace
  char *clean_line = trim_whitespace(linecpy);
  if (clean_line[0] == TASM_CHAR_COMMENT)
    goto parse_line_cleanup;

  char *pstrtok;
  char *tok = strtok_r(clean_line, " ", &pstrtok);
  if (tok == NULL)
    goto parse_line_cleanup;

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
      goto parse_line_next_tok;
    } else if (tok[0] == TASM_CHAR_COMMENT) {
      break;
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
      if (tok[strlen(tok) - 1] == TASM_CHAR_PARAM_SEPERATOR)
        parameters[parameter_count][strlen(tok) - 1] = 0;
    }

    parameter_count++;

  parse_line_next_tok:
    tok = strtok_r(NULL, " ", &pstrtok);
    ix++;
  }

  if (in_string) {
    if (!_str_closed(parameters[parameter_count - 1])) {
      ret = TASM_STRING_NOT_CLOSED;
      goto parse_line_cleanup;
    } else {
      size_t len = strlen(parameters[parameter_count - 1]);
      parameters[parameter_count - 1][len - 1] = 0;
    }
  }

  ret = asm_parse_exp(branch, keyword, parameter_count, parameters);
parse_line_cleanup:
  free(linecpy);
  if (keyword != NULL)
    free(keyword);
  return ret;
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
  size_t branch_ix = ast->branch_count;
  ast->branch_count++;
  if (ast->branch_count == 1) {
    ast->branches = malloc(sizeof(asm_tree_branch_t));
  } else {
    ast->branches =
        realloc(ast->branches, sizeof(asm_tree_branch_t) * ast->branch_count);
  }

  ast->branches[branch_ix].exp_count = 0;

  err_t err;
  while ((read = getline(&line, &len, file)) != -1) {
    linenum++;
    err = asm_parse_line(&ast->branches[branch_ix], line);
    if (err != TASM_OK)
      if (_handle_err(err, src_fl, line, linenum) == 0)
        goto parse_file_cleanup;
  }

  size_t exp_count = ast->branches[branch_ix].exp_count;
  for (size_t i = 0; i < exp_count; i++) {
    if (ast->branches[branch_ix].asm_exp[i].type != EXP_DIRECTIVE)
      continue;
    if (ast->branches[branch_ix].asm_exp[i].directive !=
        DIR_INCLUDE)
      continue;

    err = _do_dir_include(
        ast,
        ast->branches[branch_ix].asm_exp[i].parameters,
        ast->branches[branch_ix].asm_exp[i].parameter_count);

    if (err != TASM_OK)
      break;
  }

parse_file_cleanup:
  free(line);
  fclose(file);
  return err;
}

err_t asm_translate_tree(asm_tree_t *ast, uint8_t **dest_ptr, size_t *size) {
  err_t ret = TASM_OK;

  size_t calcd_size = _precalc_size(ast);
  log_inf("Precalculated Size: 0x%x bytes\n", calcd_size);

  //dest_ptr[0] = malloc(calcd_size);

  return ret;
}

err_t asm_write_file(char *src_fl, char *out_fl, char *format) {
  log_inf("Assembling \"%s\"\n", src_fl);
  log_inf("Step 1: Parsing Sources\n");
  asm_tree_t ast;
  ast.branch_count = 0;

  err_t err = asm_parse_file(src_fl, &ast);
  if (err != TASM_OK)
    goto asm_write_file_cleanup;

  log_inf("Step 2: Translating Parsed Sources\n");
  asm_translate_tree(&ast, NULL, NULL);

asm_write_file_cleanup:
  for (int i = 0; i < ast.branch_count; i++) {
    for (int j = 0; j < ast.branches[i].exp_count; j++) {
      size_t param_count = ast.branches[i].asm_exp[j].parameter_count;
      for (int k = 0; k < param_count; k++) {
        free(ast.branches[i].asm_exp[j].parameters[k]);
      }

      free(ast.branches[i].asm_exp[j].parameters);
    }

    free(ast.branches[i].asm_exp);
  }
  free(ast.branches);
  return err;
}

//-- Utilities --//

struct directive_elem_t {
  char *name;
  directive_t directive;
};

const uint32_t DIRECTIVE_COUNT = 7;
static struct directive_elem_t directives[DIRECTIVE_COUNT] = {
    {.name = "inc", .directive = DIR_INCLUDE},
    {.name = "nullpadding", .directive = DIR_NULLPAD},
    {.name = "byte", .directive = DIR_BYTE},
    {.name = "bytes", .directive = DIR_BYTES},
    {.name = "padding", .directive = DIR_PADDING},
    {.name = "text", .directive = DIR_TEXT},
    {.name = "symbols", .directive = DIR_SYMBOLS},
};

directive_t get_dir(char *str) {
  if (str == NULL)
    return DIR_INVALID;
  char *lower = str_lower(str);

  directive_t dir = DIR_INVALID;
  for (int i = 0; i < DIRECTIVE_COUNT; i++) {
    if (strcmp(directives[i].name, str) == 0) {
      dir = directives[i].directive;
      break;
    }
  }

  free(lower);
  return dir;
}

struct inst_elem_t {
  char *name;
  inst_t inst;
};

inst_descriptor_t inst_descriptors[INST_COUNT] = {
    {.name = "ld", .inst = INST_LD, .size = 3, .param_count = 2},
    {.name = "st", .inst = INST_ST, .size = 3, .param_count = 2},
    {.name = "brn", .inst = INST_BRN, .size = 3, .param_count = 1}, 
    {.name = "beq", .inst = INST_BEQ, .size = 3, .param_count = 1},
    {.name = "bne", .inst = INST_BNE, .size = 3, .param_count = 1},
    {.name = "cmp", .inst = INST_CMP, .size = 3, .param_count = 1},
    {.name = "cal", .inst = INST_CAL, .size = 3, .param_count = 1}, 
    {.name = "rts", .inst = INST_RTS, .size = 1, .param_count = 0},
    {.name = "rti", .inst = INST_RTI, .size = 1, .param_count = 0}, 
    {.name = "int", .inst = INST_INT, .size = 1, .param_count = 0},
    {.name = "din", .inst = INST_DIN, .size = 1, .param_count = 0}, 
    {.name = "ein", .inst = INST_EIN, .size = 1, .param_count = 0},
    {.name = "or", .inst = INST_OR, .size = 4, .param_count = 2},   
    {.name = "and", .inst = INST_AND, .size = 4, .param_count = 2},
    {.name = "inc", .inst = INST_INC, .size = 4, .param_count = 2}, 
    {.name = "dec", .inst = INST_DEC, .size = 4, .param_count = 2},
    {.name = "add", .inst = INST_ADD, .size = 4, .param_count = 2}, 
    {.name = "sub", .inst = INST_SUB, .size = 4, .param_count = 2},
    {.name = "shr", .inst = INST_SHR, .size = 4, .param_count = 2}, 
    {.name = "shl", .inst = INST_SHL, .size = 4, .param_count = 2},
    {.name = "nop", .inst = INST_NOP, .size = 1, .param_count = 0},
};

inst_t get_inst(char *str) {
  if (str == NULL)
    return INST_INVALID;
  char *lower = str_lower(str);

  inst_t inst = INST_INVALID;
  for (int i = 0; i < INST_COUNT; i++) {
    if (strcmp(inst_descriptors[i].name, lower) == 0) {
      inst = inst_descriptors[i].inst;
      break;
    }
  }

  free(lower);
  return inst;
}

char *asm_errname(err_t err) {
  switch (err) {
  case TASM_OK:
    return "OK";
  case TASM_INVALID_INSTRUCTION:
    return "Invalid Instruction";
  case TASM_INVALID_PARAMETER:
    return "Invalid Parameter";
  case TASM_MISSING_PARAMETER:
    return "Missing Parameter";
  case TASM_INVALID_DIRECTIVE:
    return "Invalid Directive";
  case TASM_DIRECTIVE_MISSING_PARAMETER:
    return "Invalid Parameter for Directive";
  case TASM_STRING_NOT_CLOSED:
    return "String is not closed at EOL";
  default:
    return "Unknown Error";
  }
}
