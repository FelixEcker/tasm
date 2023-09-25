#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <stddef.h>
#include <stdint.h>

#define TASM_OUT_ROM "rom"
#define TASM_OUT_TEF "tef"

typedef enum err_t {
  TASM_OK = 0,
  TASM_INVALID_INSTRUCTION,
  TASM_INVALID_PARAMETER,
  TASM_MISSING_PARAMETER,
  TASM_INVALID_DIRECTIVE,
  TASM_DIRECTIVE_MISSING_PARAMETER,
  TASM_STRING_NOT_CLOSED,
} err_t;

//-- Assembly Keywords / Tokens / Values --//

#define TASM_CHAR_COMMENT          ';'
#define TASM_CHAR_DIRECTIVE_PREFIX '.'
#define TASM_CHAR_STRING_CONT      '"'
#define TASM_CHAR_ESCAPE           '\\'
#define TASM_CHAR_DECIMAL_POSTFIX  't'
#define TASM_CHAR_BINARY_POSTFIX   'b'

#define TASM_STR_ADDRESS_PREFIX    "$"
#define TASM_STR_VALUE_PREFIX      "$#"

typedef enum directive_t {
  DIR_INCLUDE,
  DIR_NULLPAD,
  DIR_BYTE,
  DIR_BYTES,
  DIR_PADDING,
  DIR_TEXT,
  DIR_SYMBOLS,
} directive_t;

typedef enum inst_t {
  INST_LD = 0x00,
  INST_ST = 0x01,
  INST_BRN = 0x02,
  INST_BEQ = 0x02,
  INST_BNE = 0x02,
  INST_CMP = 0x03,
  INST_CAL = 0x04,
  INST_RTS = 0x05,
  INST_RTI = 0x06,
  INST_INT = 0x07,
  INST_DIN = 0x08,
  INST_EIN = 0x09,
  INST_OR = 0x0a,
  INST_AND = 0x0b,
  INST_INC = 0x0c,
  INST_DEC = 0x0d,
  INST_ADD = 0x0e,
  INST_SUB = 0x0f,
  INST_SHR = 0x19,
  INST_SHL = 0x29,
  INST_NOP = 0x39,
} inst_t;

//typedef enum inst_sizes_t {
//} inst_sizes_t;

//-- Assembler Tree Datatypes --//

typedef enum exp_type_t {
  EXP_DIRECTIVE = 0,
  EXP_INSTRUCTION = 1,
} exp_type_t;

typedef struct asm_exp_t {
  uint32_t line;
  exp_type_t type;
  inst_t inst;
  directive_t directive;
  size_t parameter_count;
  char **parameters;
} asm_exp_t;

typedef struct asm_tree_branch_t {
  size_t exp_count;
  asm_exp_t *asm_exp;
  char *file;
} asm_tree_branch_t;

typedef struct asm_tree_t {
  size_t branch_count;
  asm_tree_branch_t *branches;
} asm_tree_t;

typedef struct asm_res_t {
  uint32_t line;
  err_t status;
  size_t result_size;
  uint8_t *result;
} asm_res_t;

//-- Functions --//

char *asm_errname(err_t err);

err_t asm_parse_exp(asm_tree_branch_t *branch, char *keyword, char **params);

err_t asm_parse_line(asm_tree_branch_t *branch, char *line);

err_t asm_parse_file(char *src_fl, asm_tree_t *ast);

err_t asm_write_file(char *src_fl, char *out_fl, char *format);

#endif
