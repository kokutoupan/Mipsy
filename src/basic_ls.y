%debug
%{
  #include <stdio.h>
  #include "types.h"
  #include "make_ast.h"
  #include "basic_ls.tab.h"
  #include "generate_code.h"
  #include "mips_code.h"
  #include "mips_print.h"
  extern int yylex();
  extern int yyerror(const char* s);

  extern Node *top;
  extern CodeList codeList;

  extern int opt_debug;
%}


%union{
  int ival;
  char* str;
  Node* n;
  CompOp comp;
  ArithOp arith;
}

%token ASSIGN SEMIC  LPR RPR LCB RCB ARRAY COMMA
%token EQ LT GT LEQ GEQ
%token PLUS MINUS MULT DIVI
%token IF ELSE WHILE BREAK DEFINE
%token FUNC FUNCCALL
%right ADDR_OF            /* @a (前置は右結合・優先度低) */
%left  LSQ RSQ PTR_DEREF  /* a[i], p^ (後置は左結合・優先度高) */

%token  <n>IDENT <ival>NUMBER 

%type <comp>Comp
%type <arith>MulDiv <arith>AddSub 
%type <n>Variable <n>Factor <n>Term <n>Formula <n>LeftValue
%type <n>CondSentence <n>LoopSentence  <n>AssignSentence <n>Sentence
%type <n>CondExpr  <n>SentenceSet <n>DeclarationStatement <n>ValDeffineSection  <n>Idents <n>DimList
%type <n>FunctionDef <n>Program <n>Functions
%type <n>Params <n>Args

%%

Program
  : Functions
  {
    top = make_concat_node(NULL,$1,1);
  }
  | ValDeffineSection Functions{
    top = make_concat_node($1,$2,1);
  }

Functions
  : FunctionDef{
    $$ = $1;
  }
  | FunctionDef Functions{
    $$ = make_concat_node($1,$2,1);
  }
  ;


FunctionDef
  :  FUNC IDENT LPR Params RPR LCB ValDeffineSection SentenceSet RCB{
    $$ = make_function_node($2, $7, $8,$4);
    if(opt_debug)
      fprintf(stderr, "ok!, function ast [%s]\n",$2->str);
  }
  | FUNC IDENT LPR Params RPR LCB SentenceSet RCB {
    $$ = make_function_node($2, NULL, $7, $4);
    
    if(opt_debug)
      fprintf(stderr, "ok!, function nolocal ast [%s]\n", $2->str);
  }
  ;

Params
  : /* 空 */ { $$ = NULL; }
  | Idents   { $$ = $1; }  /* 既存のIdentsルール(カンマ区切り)を流用 */
  ;

ValDeffineSection
  :   DeclarationStatement ValDeffineSection
  {
    $$ = make_concat_node($1,$2,2);
  }
  |   DeclarationStatement
  {
    $$ = $1;
  }

;

DeclarationStatement
  : ARRAY IDENT DimList SEMIC
  {
    $$ = make_array_node($2, $3);
  }
  | DEFINE Idents SEMIC
  {
    $$ = make_define_node($2);
  }
  ;

DimList
  : LSQ Formula RSQ
  {
    $$ = $2;   // 単一次元（Formulaノードそのもの）
  }
  | LSQ Formula RSQ DimList
  {
    $$ = make_concat_node($2, $4,1);
  }
;


SentenceSet
  :   Sentence SentenceSet
  {
    $$ = make_concat_node($1,$2,3);
  }
  |   Sentence
  {
    $$ = $1;
  }

;

Sentence
  :   AssignSentence
  {
    $$ = $1;
  }
  |LoopSentence
  {
    $$ = $1;
  }
  |   CondSentence
  {
    $$ = $1;
  }
  | FUNCCALL IDENT LPR Args RPR SEMIC{
      $$ = make_call_node($2, $4);
  }
  | BREAK SEMIC{
      $$ = make_node(ND_BREAK);
  }
  ;

AssignSentence
  :   LeftValue ASSIGN Formula SEMIC
  {

$$ = make_assign_node($1,$3);
  }
  ;

Args
  : /* 空 */ { $$ = NULL; }
  | Formula COMMA Args {
      $$ = make_concat_node($1, $3, ND_ARGS);
  }
  | Formula {
      $$ = $1;
  }
  ;

Formula
  :   Formula AddSub Term 
  {
    $$ = make_arith_node($1,$2,$3);
  }
  |   Term
  {
      $$ = $1;
  }
  ;

Term
  :   Term MulDiv Factor
  {
      $$ = make_arith_node($1,$2,$3);
  }
  |   Factor
  {
    $$ = $1;
  }
  ;

Factor
  :   Variable
  {
  $$ = $1;
  }
  | ADDR_OF Factor  /* @a : アドレス取得 */
  {
      $$ = make_unary_node(ND_ADDR, $2); // 新しいノードタイプ
  }
  |   LPR Formula RPR
  {
    $$ = $2;
  }
  ;

AddSub
  :   PLUS {$$ =OP_ADD;}
  |   MINUS {$$ = OP_SUB;}
  ;

MulDiv
  :   MULT  {$$ = OP_MUL;}
  |   DIVI  {$$ = OP_DIV;}
  ;

Variable
  :   LeftValue{
    $$ = $1;
  }
  |   NUMBER{
  // [TODO] numようにする
    $$ = make_val_id_node($1);
  }
  ;

LeftValue
  : LeftValue LSQ Formula RSQ {
      /* 左再帰で入れ子の ND_REF を作る: REF( REF(IDENT, i), j ) */
      $$ = make_array_ref_node($1, $3);
  }
  | IDENT {
      $$ = $1;
  }
  |
  Factor PTR_DEREF {
      /* ポインタ参照 (p^) */
      $$ = make_unary_node(ND_DEREF, $1);
  }
  ;

LoopSentence
  :   WHILE LPR CondExpr RPR LCB SentenceSet RCB{
    $$ = make_loop_node($3, $6);
  }
  ;

CondSentence
  :   IF LPR CondExpr RPR LCB SentenceSet RCB{  
    $$ = make_if_node($3,$6);
  }
  |   IF LPR CondExpr RPR LCB SentenceSet RCB ELSE LCB SentenceSet RCB{
    $$ = make_if_else_node($3,$6,$10);
  }
  ;

CondExpr
  :   Formula Comp  Formula{
    $$ = make_cond_node($1,$2,$3);
  }
  ;

Comp
  :   EQ  {$$ = OP_EQ;}
  |   LT  {$$ = OP_LT;}
  |   GT  {$$ = OP_GT;}
  |   LEQ {$$ = OP_LEQ;}
  |   GEQ {$$ = OP_GEQ;}
  ;

Idents
  : IDENT COMMA Idents{
    $$ = make_concat_node($1,$3,1);
  }
  | IDENT{
    $$ = $1;
  }
  ;
%%
