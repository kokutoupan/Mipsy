%{
#include "types.h"

#include "basic_ls.tab.h"
Node *make_ident_node(char *str);

%}

digit   [0-9]
number  {digit}+
alphabet [a-zA-Z]
alphanumeric ({digit}|{alphabet})
string {alphanumeric}+

%%
=             {return ASSIGN;}
;             {return SEMIC;}
==              {return EQ;}
"!="            {return NEQ;}
\<               {return LT;}
\>               {return GT;}
"<="              {return LEQ;}
">="              {return GEQ;}
"%"  { return MOD; }
"&"  { return BITAND; }
"|"  { return BITOR; }
"^"  { return BITXOR; }
"<<" { return LSHIFT; }
">>" { return RSHIFT; }

\(               {return LPR;}
\)               {return RPR;}
\{               {return LCB;}
\}               {return RCB;}

\[               {return LSQ;}
\]               {return RSQ;}

\+               {return PLUS;}
\-               {return MINUS;}
\*               {return MULT;}
\/               {return DIVI;}
,                {return COMMA;}
"$"     { return PTR_DEREF; }
"@"     { return ADDR_OF; }

if              {return IF;}
else            {return ELSE;}
while           {return WHILE;}
define          {return DEFINE;}
array           {return ARRAY;}
func            {return FUNC;}
funccall        {return FUNCCALL;}
return          {return RETURN;}
break           {return BREAK;}
reg       { return REG;}

{number}          {
  yylval.ival = atoi(yytext);
  return NUMBER;}

{alphabet}{string}|{alphabet} {
  char* str = strdup(yytext);
  yylval.n = make_ident_node(str);
  return IDENT;}

[ \t\n]+        ; /* 空白、タブ、改行は明示的に無視する */
"//".*          ; /* コメント行を無視 */

.               { 
    fprintf(stderr, "Lexer Error: Unknown character '%s'\n", yytext); 
}

%%
