%{ 
	#include <stdio.h>
	#include <string.h>
	#include "VSME.h"
	#include "SymTable.h"
	#include "ExprTree.h"
	#include "CodeGen.h"

	STP FuncP;
	extern int yyleng;
%}

%union{
	NP NodeP;
	int Int;
	double Dbl;
	char *Name;
	STP SymP;
}

%token DO ELSE FOR IF LAND LOR READ RETURN STATIC WRITE WHILE 
%token <Int> ADDOP MULOP PPMM RELOP TYPE NUM CNUM
%token <Dbl> RNUM
%token <Name> ID STRING
%token SBLOCK EBLOCK SBRACKET EBRACKET PERIOD EQUAL COMMA AMPERSAND EXCLAMATION
 

%type <Int> type_spec decl dim_list if_part 
%type <SymP> f_head declatr
%type <NodeP> expr opt_expr cast_expr primary sub_list arg_list

%right EQUAL
%left LOR
%left LAND
%left RELOP
%left ADDOP
%left MULOP
%right UM PPMM EXCLAMATION
%left POSOP

%%

program   : glbl_def { Epilogue(); }
		  ;

glbl_def  :
		  | glbl_def decl PERIOD
		  | glbl_def func_def
		  | glbl_def error PERIOD { yyerrok; }
		  ;

decl      : type_spec declatr { MemAlloc($2, $1, 0); }
		  | type_spec f_head  { Prototype($2, $1); }
		  | decl COMMA declatr  { MemAlloc($3, $1, 0); }
		  | decl COMMA f_head   { Prototype($3, $1); }
		  ;

type_spec : TYPE 
		  | STATIC			{ $$ = SALLOC | INT; }
		  | STATIC TYPE 	{ $$ = SALLOC | $2; }
		  ;

declatr   : ID dim_list { $$ = VarDecl($1, $2); }
		  ;

dim_list  :  						{ $$ = 0; }
		  | dim_list '[' ']' 		{ AStable[$1] = -1; $$ = $1 + 1; }
		  | dim_list '[' NUM ']' 	{ AStable[$1] = $3; $$ = $1 + 1; }
		  ;

f_head    : ID 						{ $<SymP>$ = MakeFuncEntry($1); }
			SBRACKET p_list EBRACKET 			{ $$ = $<SymP>2; }
		  ;

p_list    :
		  | p_decl
		  | p_list COMMA p_decl
		  ;

p_decl    : TYPE declatr { MemAlloc($2, $1, PARAM); }
		  | TYPE AMPERSAND declatr { MemAlloc($3, $1, PARAM | BYREF); }
		  ;

func_def  : type_spec f_head SBLOCK { FuncDef($2, $1); FuncP = $2; GenFuncEntry($2); }
		    decl_list
		    st_list EBLOCK { GenReturn($2, NULL); EndFdecl($2); }
		  ;

block 	  : SBLOCK { OpenBlock(); }
			decl_list st_list
			EBLOCK { CloseBlock(); }
		  ;

decl_list :
		  | decl_list decl PERIOD
		  ;

st_list   : stmnt
		  | st_list stmnt
		  ;

stmnt     : block
		  | PERIOD
		  | expr PERIOD             { ExprStmnt($1); }
		  | if_part		         { Bpatch($1, PC()); }
		  | if_part ELSE         { $<Int>$ = PC(); Cout(JUMP, -1); Bpatch($1, PC()); }
		    stmnt                { Bpatch($<Int>3, PC()); }
		  | WHILE                { $<Int>$ = PC(); }
		  	SBRACKET expr EBRACKET         { $<Int>$ = CtrlExpr($4, FJ); }
		  	stmnt                { Cout(JUMP, $<Int>2); Bpatch($<Int>6, PC()); }
		  | FOR SBRACKET opt_expr PERIOD { ExprStmnt($3); $<Int>$ = PC(); }
		  	opt_expr PERIOD         { $<Int>$ = CtrlExpr($6, FJ); }
		  	opt_expr EBRACKET
		  	stmnt 				 { ExprStmnt($9); Cout(JUMP, $<Int>5); Bpatch($<Int>8, PC()); }
		  | DO                   { $<Int>$ = PC(); }
		  	stmnt WHILE
		  	SBRACKET expr EBRACKET PERIOD     { Bpatch(CtrlExpr($6, TJ), $<Int>2); }
		  | RETURN PERIOD           { GenReturn(FuncP, NULL); }
		  | RETURN expr PERIOD      { GenReturn(FuncP, $2); }
		  | error                { yyerrok; }
		  ;

if_part   : IF SBRACKET expr EBRACKET      { $<Int>$ = CtrlExpr($3, FJ); }
		  	stmnt                { $$ = $<Int>5; }
		  ;

opt_expr  :                      { $$ = NULL; }
		  | expr
		  ;

expr      : primary EQUAL expr     { $$ = MakeN(ASSGN, $1, $3); }
		  | expr LOR expr        { $$ = MakeN(OR, $1, $3); } 
		  | expr LAND expr       { $$ = MakeN(AND, $1, $3); } 
		  | expr RELOP expr      { $$ = MakeN(COMP, $1, $3); $$->etc = $2; } 
		  | expr ADDOP expr      { $$ = MakeN($2, $1, $3); } 
		  | expr MULOP expr      { $$ = MakeN($2, $1, $3); } 
		  | cast_expr
		  ;

cast_expr : primary
		  | SBRACKET TYPE EBRACKET cast_expr { $$ = TypeConv($4, $2); }
		  ;

primary   : ADDOP primary %prec UM           { $$ = ($1 == SUB)? MakeN(CSIGN, $2, NULL): $2; }
		  | PPMM primary                     { $$ = MakeN($1, $2, NULL); $$->etc = PRE; }
		  | primary PPMM %prec POSOP         { $$ = MakeN($2, $1, NULL); $$->etc = POST; }
		  | EXCLAMATION primary                      { $$ = MakeN(NOT, $2, NULL); }
		  | READ SBRACKET primary COMMA primary EBRACKET { $$ = MakeN(INPUT, $3, $5); }
		  | WRITE SBRACKET primary EBRACKET            { $$ = MakeN(OUTSTR, $3, NULL); }
		  | WRITE SBRACKET primary COMMA expr EBRACKET   { $$ = MakeN(OUTPUT, $3, $5); }
		  | ID SBRACKET arg_list EBRACKET              { $$ = MakeN(CALL, MakeL($1), $3); }
		  | SBRACKET expr EBRACKET                     { $$ = $2; }
		  | ID sub_list                      { $$ = MakeN(AELM, MakeL($1), $2); }
		  | ID                               { VAR_NODE($$, $1) }
		  | STRING                           { MakeC($$, C_ARY, AllocCons($1, CBSZ, yyleng + 1)); }
		  | NUM                              { MakeC($$, INT, $1); }
		  | CNUM                             { MakeC($$, INT, $1); }
		  | RNUM                             { MakeC($$, DBL, Wcons($1)); }
		  ;

sub_list  : '[' expr ']'                     { $$ = $2; }
		  | sub_list '[' expr ']'            { $$ = MakeN(SUBL, $1, $3); }
		  ;

arg_list  :                                  { $$ = NULL; }
		  | expr
		  | arg_list COMMA expr                { $$ = MakeN(ARGL, $1, $3); }
		  ;
%%








