#include <debug_utils.h>

#include <assembler.h>

#include <stdio.h>

void debug_print_ast(asm_tree_t ast) {
#ifndef DEBUG_UTILS_MUTE
  printf("symbols: %zu\nbranches: %zu\n{\n", ast.symbol_count, ast.branch_count);
  for (size_t s = 0; s < ast.symbol_count; s++) {
    printf("  symbol {\n");
    printf("    name: %s;\n", ast.symbols[s].name);
    printf("    value: %s;\n", ast.symbols[s].value);
    printf("  }\n");
  }
  
  printf("  base {\n");

  for (size_t s = 0; s < ast.branch_count; s++) {
    printf("    branch {\n");
    for (size_t e = 0; e < ast.branches[s].exp_count; e++) {
      asm_exp_t exp = ast.branches[s].asm_exp[e];
      printf("      expr {\n");
      printf("        line: %u;\n", exp.line);
      printf("        type: %d;\n", exp.type);
      printf("        inst: %d;\n", exp.inst);
      printf("        dire: %d;\n", exp.directive);
      for (size_t p = 0; p < exp.parameter_count; p++) {
        printf("        param {\n");
        printf("          value: %s;\n", exp.parameters[p]);
        printf("        }\n");
      }
      printf("      }\n");
    }
  }

  printf("  }\n");
  printf("}\n");
#endif
}
