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
  ast->branch_count++;
  if (ast->branch_count == 1) {
    ast->branches = malloc(sizeof(asm_tree_branch_t));
  } else {
    ast->branches =
        realloc(ast->branches, sizeof(asm_tree_branch_t) * ast->branch_count);
  }

  err_t err;
  while ((read = getline(&line, &len, file)) != -1) {
    linenum++;
    err = asm_parse_line(&ast->branches[ast->branch_count - 1], line);
    if (err != TASM_OK)
      if (_handle_err(err, src_fl, line, linenum) == 0)
        goto parse_file_cleanup;
  }

parse_file_cleanup:
  free(line);
  fclose(file);
  return err;
}

err_t asm_write_file(char *src_fl, char *out_fl, char *format) {
  log_inf("Assembling \"%s\"\n", src_fl);
  log_inf("Step 1: Parsing Sources\n");
  asm_tree_t ast;
  ast.branch_count = 0;

  err_t err = asm_parse_file(src_fl, &ast);
  if (err != TASM_OK)
    goto asm_write_file_cleanup;

asm_write_file_cleanup:
  for (int i = 0; i < ast.branch_count; i++) {
    for (int j = 0; j < ast.branches[i].exp_count; j++) {
      size_t param_count = ast.branches[i].asm_exp[j].parameter_count;
      for (int k = 0; k < param_count; k++)
        free(ast.branches[i].asm_exp[j].parameters[k]);
      free(ast.branches[i].asm_exp[j].parameters);
    }
    free(ast.branches[i].asm_exp);
  }
  free(ast.branches);
  return err;
}

//-- Utilities --//

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
