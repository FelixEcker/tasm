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

static reg_t _get_register(char reg_id) {
  switch (reg_id) {
  case 'a':
    return REG_ACC;
  case 'c':
    return REG_C;
  case 'd':
    return REG_D;
  case 'e':
    return REG_E;
  case 'f':
    return REG_F;
  case 'g':
    return REG_G;
  case 'h':
    return REG_H;
  default:
    return REG_INVALID;
  }
}

static size_t _dir_exp_size(asm_exp_t exp) {
  switch (exp.directive) {
  case DIR_BYTES:
  case DIR_PADDING:
  case DIR_NULLPAD:
    if (exp.parameters == NULL || exp.parameters[0] == NULL)
      return 0;

    char *pend;
    return (size_t)strtol(exp.parameters[0], &pend, 0);
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

static size_t _get_inst_param_count(inst_t inst) {
  for (int i = 0; i < INST_COUNT; i++) {
    if (inst_descriptors[i].inst == inst) {
      return inst_descriptors[i].param_count;
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
        if (exp.type == EXP_LABEL)
          continue;
        if (exp.directive != DIR_NULLPAD && exp.directive != DIR_BYTE &&
            exp.directive != DIR_BYTES)
          continue;

        ret += _dir_exp_size(exp);
        continue;
      }

      ret += _get_inst_size(exp.inst);
    }
  }

  return ret;
}

static void _mod_inst_address(uint8_t *inst) {
  switch (inst[0]) {
  case INST_CMP:
  case INST_LD:
    inst[0] = inst[0] | 0b10000000;
    break;
  case INST_OR:
  case INST_AND:
  case INST_INC:
  case INST_DEC:
  case INST_ADD:
  case INST_SUB:
  case INST_SHR:
  case INST_SHL:
    inst[3] = inst[3] | 0b10000000;
    break;
  }
}

static void _mod_inst_register(uint8_t *inst, reg_t reg) {
  switch (inst[0]) {
  case INST_LD:
  case INST_CMP:
    inst[0] = inst[0] | (reg << 4);
    break;
  case INST_OR:
  case INST_AND:
  case INST_INC:
  case INST_DEC:
  case INST_ADD:
  case INST_SUB:
  case INST_SHR:
  case INST_SHL:
    inst[3] = inst[3] | (reg << 4);
    break;
  }
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

err_t asm_parse_symbol(asm_tree_t *ast, char *name, size_t word_count,
                       char **words) {
  if (name == NULL || words == NULL) {
    log_wrn("internal: asm_parse_symbol was passed one or more NULL-values\n");
    return TASM_OK;
  }

  if (ast->symbol_count == 0) {
    ast->symbols = malloc(sizeof(asm_symbol_t));
  } else {
    ast->symbols = realloc(ast->symbols,
                           sizeof(asm_symbol_t) * (ast->symbol_count + 1));
  }

  ast->symbols[ast->symbol_count].name = strdup(name);
  ast->symbols[ast->symbol_count].value = str_from_strarr(words, word_count, ' '); 

  for (size_t i = 0; i < word_count; i++)
    free(words[i]);

  free(words);
  ast->symbol_count++;
  return TASM_OK;
}

err_t asm_parse_exp(asm_tree_t *ast, char *keyword, size_t param_count,
                    char **params, uint32_t line) {
  err_t ret = TASM_OK;

  if (ast->curr_section == DIR_SYMBOLS &&
      keyword[0] != TASM_CHAR_DIRECTIVE_PREFIX)
    return asm_parse_symbol(ast, keyword, param_count, params);

  asm_tree_branch_t *branch = &ast->branches[ast->branch_count - 1];

  if (branch->exp_count == 0) {
    branch->asm_exp = malloc(sizeof(asm_exp_t));
  } else {
    branch->asm_exp =
        realloc(branch->asm_exp, sizeof(asm_exp_t) * (branch->exp_count + 1));
  }

  branch->asm_exp[branch->exp_count].line = line;
  branch->asm_exp[branch->exp_count].parameter_count = param_count;
  branch->asm_exp[branch->exp_count].parameters = params;

  branch->asm_exp[branch->exp_count].type = EXP_INSTRUCTION;
  if (keyword[0] == TASM_CHAR_DIRECTIVE_PREFIX) {
    branch->asm_exp[branch->exp_count].type = EXP_DIRECTIVE;
    branch->asm_exp[branch->exp_count].directive = get_dir(keyword + 1);

    switch (branch->asm_exp[branch->exp_count].directive) {
    case DIR_SYMBOLS:
      ast->curr_section = DIR_SYMBOLS;
      break;
    case DIR_TEXT:
      ast->curr_section = DIR_TEXT;
      break;
    default:
      break;
    }
  } else if (keyword[strlen(keyword) - 1] == TASM_CHAR_LABEL_POSTFIX) {
    branch->asm_exp[branch->exp_count].type = EXP_LABEL;
    branch->asm_exp[branch->exp_count].parameter_count = 1;
    branch->asm_exp[branch->exp_count].parameters = malloc(sizeof(char *));
    branch->asm_exp[branch->exp_count].parameters[0] = strdup(keyword);
  } else {
    branch->asm_exp[branch->exp_count].inst = get_inst(keyword);
  }
  branch->exp_count++;

  return ret;
}

err_t asm_parse_line(asm_tree_t *ast, char *line, uint32_t line_num) {
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

  ret = asm_parse_exp(ast, keyword, parameter_count, parameters, line_num);
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
  ast->branches[branch_ix].file = src_fl;

  err_t err;
  while ((read = getline(&line, &len, file)) != -1) {
    linenum++;
    err = asm_parse_line(ast, line, linenum);
    if (err != TASM_OK)
      if (_handle_err(err, src_fl, line, linenum) == 0)
        goto parse_file_cleanup;
  }

  size_t exp_count = ast->branches[branch_ix].exp_count;
  for (size_t i = 0; i < exp_count; i++) {
    if (ast->branches[branch_ix].asm_exp[i].type != EXP_DIRECTIVE)
      continue;
    if (ast->branches[branch_ix].asm_exp[i].directive != DIR_INCLUDE)
      continue;

    err = _do_dir_include(ast, ast->branches[branch_ix].asm_exp[i].parameters,
                          ast->branches[branch_ix].asm_exp[i].parameter_count);

    if (err != TASM_OK)
      break;
  }

parse_file_cleanup:
  free(line);
  fclose(file);
  return err;
}

err_t asm_resolve_labels(asm_tree_t *ast) {
  err_t ret = TASM_OK;

  size_t offset = 0;
  for (size_t b = 0; b < ast->branch_count; b++) {
    asm_tree_branch_t *branch = &ast->branches[b];

    for (size_t e = 0; e < branch->exp_count; e++) {
      asm_exp_t *exp = &branch->asm_exp[e];
      if (exp->type != EXP_LABEL) {
        if (exp->type == EXP_DIRECTIVE) {
          offset += _dir_exp_size(*exp);
          continue;
        }

        offset += _get_inst_size(exp->inst);
        continue;
      }

      exp->lbl_position = offset;
    }
  }

  return ret;
}

err_t asm_replace_symbols(asm_tree_t *ast) {
  err_t ret = TASM_OK;

  return ret;
}

err_t asm_translate_parameters(asm_tree_t *ast, char **params, size_t count,
                               uint8_t *dest) {
  err_t ret = TASM_OK;

  for (size_t p = 0; p < count; p++) {
    if (params[p] == NULL) {
      log_wrn("internal: asm_translate_parameters received a null-pointer "
              "param!\n");
      continue;
    }

    switch (params[p][0]) {
    case TASM_CHAR_CHAR_CONT:
      if (strlen(params[p]) != 3)
        return TASM_INVALID_PARAMETER_FORMAT;

      dest[1] = params[p][1];
      break;
    case TASM_CHAR_ADDRESS_PREFIX:
      if (strlen(params[p]) < 2)
        return TASM_INVALID_PARAMETER_FORMAT;

      size_t offset = 1;
      if (params[p][1] == TASM_CHAR_VALUE_PREFIX)
        offset++;

      size_t len = strlen(params[p] + offset);
      unsigned long value = 0;

      char lchar = params[p][strlen(params[p] - 1)];
      switch (lchar) {
      case TASM_CHAR_DECIMAL_POSTFIX: {
        char *cpy = strdup(params[p] + offset);
        cpy[len - 1] = 0;
        value = strtoul(params[p] + offset, NULL, 10);
        free(cpy);
        break;
      }
      case TASM_CHAR_BINARY_POSTFIX: {
        char *cpy = strdup(params[p] + offset);
        cpy[len - 1] = 0;
        value = strtoul(params[p] + offset, NULL, 2);
        free(cpy);
        break;
      }
      default:
        value = strtoul(params[p] + offset, NULL, 16);
      }

      if (params[p][1] == TASM_CHAR_VALUE_PREFIX)
        _mod_inst_address(dest);
      dest[1] = (value & 0xff00) >> 8;
      dest[2] = value & 0xff;
      break;
    case 'a':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'g':
    case 'h':
      if (strlen(params[p]) == 1) {
        reg_t reg = _get_register(params[p][0]);
        if (reg == REG_INVALID)
          return TASM_INVALID_REGISTER;
        _mod_inst_register(dest, reg);
        break;
      }
    default:
      // TODO: Replace with labels
      return TASM_INVALID_TYPE;
    }
  }

  return ret;
}

err_t asm_translate_tree(asm_tree_t *ast, uint8_t **dest_ptr, size_t *size) {
  err_t ret = TASM_OK;

  size_t calcd_size = _precalc_size(ast);
  log_inf("Precalculated Size: 0x%x bytes\n", calcd_size);

  log_inf("Resolving label positions...\n");
  ret = asm_resolve_labels(ast);
  if (ret != TASM_OK)
    return ret;

  dest_ptr[0] = malloc(calcd_size);
  size[0] = calcd_size;

  log_inf("Replacing Symbol usages...\n");
  ret = asm_replace_symbols(ast);
  if (ret != TASM_OK)
    return ret;

  log_inf("Translating tree...\n");

  size_t wi = 0;
  asm_tree_branch_t *branch;
  asm_exp_t *exp;
  for (size_t b = 0; b < ast->branch_count; b++) {
    branch = &ast->branches[b];

    for (size_t e = 0; e < branch->exp_count; e++) {
      exp = &branch->asm_exp[e];
      if (exp->type != EXP_INSTRUCTION)
        continue;

      if (exp->parameter_count != _get_inst_param_count(exp->inst)) {
        ret = TASM_INVALID_PARAMETER;
        goto asm_translate_tree_exit;
      }

      size_t cur_size = _get_inst_size(exp->inst);
      uint8_t *translated = malloc(cur_size);
      translated[0] = exp->inst;
      ret = asm_translate_parameters(ast, exp->parameters, exp->parameter_count,
                                     translated + 1);

      if (ret != TASM_OK) {
        free(translated);
        goto asm_translate_tree_exit;
      }

      memcpy(dest_ptr[0] + wi, translated, cur_size);
      free(translated);
    }
  }

asm_translate_tree_exit:
  if (ret != TASM_OK) {
    _handle_err(ret, branch->file, "?", exp->line);
  }
  return ret;
}

err_t asm_write_file(char *src_fl, char *out_fl, char *format) {
  log_inf("Assembling \"%s\"\n", src_fl);
  log_inf("Step 1: Parsing Sources\n");
  asm_tree_t ast;
  ast.branch_count = 0;
  ast.symbol_count = 0;
  ast.curr_section = DIR_INVALID;

  err_t err = asm_parse_file(src_fl, &ast);
  if (err != TASM_OK)
    goto asm_write_file_cleanup;

  log_inf("Step 2: Translating Parsed Sources\n");

  size_t size;
  uint8_t *bin;
  err = asm_translate_tree(&ast, &bin, &size);
  if (err != TASM_OK) {
    if (size > 0)
      free(bin);
    goto asm_write_file_cleanup;
  }

  log_inf("Step 3: Writing 0x%x bytes to \"%s\"\n", size, out_fl);

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

  for (int i = 0; i < ast.symbol_count; i++) {
    free(ast.symbols[i].name);
    free(ast.symbols[i].value);
  }
  free(ast.symbols);
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
  case TASM_INVALID_PARAMETER_FORMAT:
    return "Invalid format for Parameter";
  case TASM_INVALID_TYPE:
    return "Invalid type for Parameter";
  case TASM_INVALID_REGISTER:
    return "Invalid Register identifier";
  default:
    return "Unknown Error";
  }
}
