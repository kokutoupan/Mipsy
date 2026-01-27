#include "types.h"

#include "basic_ls.tab.h"
#include "generate_code.h"
#include "mips_code.h"
#include "mips_print.h"
#include "print_ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern int yylex();
extern int yyerror(const char *s);
extern FILE *yyin;

extern Node *top;
extern CodeList codeList;

int opt_show_ast = 0;
int opt_optimize = 1;
int opt_debug = 0;
const char *out_file = "out.s";

void usage(const char *prog) {
  fprintf(stderr,
          "Usage: %s [options] input\n"
          "Options:\n"
          "  -a        show AST\n"
          "  -O        enable optimization\n"
          "  -o file   output file\n"
          "  -h        show this help\n"
          "  -d        show debug info\n",
          prog);
}

int main(int argc, char *argv[]) {

  int opt;
  while ((opt = getopt(argc, argv, "aO::o:hd")) != -1) {
    switch (opt) {
    case 'a':
      opt_show_ast = 1;
      break;
    case 'O':
      if (optarg) {
        // -O1, -O2 のように引数がある場合
        opt_optimize = atoi(optarg);
      } else {
        // -O だけの場合はデフォルトでレベル1にする
        opt_optimize = 1;
      }
      break;
    case 'o':
      out_file = optarg;
      break;
    case 'h':
      usage(argv[0]);
      return 0;
    case 'd':
      opt_debug = 1;
      break;
    default:
      usage(argv[0]);
      return 1;
    }
  }

  if (optind >= argc) {
    fprintf(stderr, "input file is required\n");
    return 1;
  }

  const char *input = argv[optind];

  FILE *fp = fopen(input, "r");
  if (fp == NULL) {
    fprintf(stderr, "can't open source:[%s]\n", input);
    return -1;
  }
  // yydebug = 1;

  // initialise
  yyin = fp;
  if (yyparse()) {
    fprintf(stderr, "Error!\n");
    return 1;
  }
  // ASTの表示
  if (opt_show_ast) {
    printf("--- Abstract Syntax Tree ---\n");
    print_ast(top, 0);
    // print_ast_json(top,0);

    printf("\n----------------------------\n");
  }

  // コード生成 (list形式)
  CodeList *cl = generate_code(top);

  // コード最適化(nopの削除)
  if (opt_optimize >= 1) {

    // addressの最適化
    optimize_address(cl);
    optimize_address(cl);

    // nopの削除
    optimize_nop(cl);
  }

  fp = fopen(out_file, "w");

  if (fp == NULL) {
    printf("ファイルを開くことができませんでした。");
    return -1;
  }

  print_code_list(fp, codeList.head);
  fclose(fp);

  return 0;
}
