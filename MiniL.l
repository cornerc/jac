%{
	#include "SymTable.h"
	#include "ExprTree.h"
	#include "y.tab.h"
	#include "VSME.h"

	char *IDentry(char *Name, int Len), *StrScan(void);
	int NumOrder(char *str);
%}


ws (" "|"　"|"\t"|"\n")
sym [[\]]
comma ","|、 
equal "="|＝
s_block "{"|「
e_block "}"|」
s_bracket "("|（
e_bracket ")"|）
period ";"|。
ampersand "＆"|&
exclamation "！"|!
digit (0|1|2|3|4|5|6|7|8|9|０|１|２|３|４|５|６|７|８|９)
id (あ|い|う|え|お|主)+
real_num {digit}+"."{digit}*((E|e)("+"|"-")?{digit}+)?
string \"([^\"\\\n]|\\.)*[\"\n]
other .

%%

{ws}+		;
{equal} {return EQUAL;}
{comma} {return COMMA;}
{s_block} {return SBLOCK;}
{e_block} {return EBLOCK;}
{s_bracket} {return SBRACKET;}
{e_bracket} {return EBRACKET;}
{period} { return PERIOD; }
{ampersand} { return AMPERSAND; }
{exclamation} { return EXCLAMATION; }
{sym} {return(yytext[0]);}
"＋" {yylval.Int = ADD; return ADDOP;}
"ー" {yylval.Int = SUB; return ADDOP;}
"＊" {yylval.Int = MUL; return MULOP;}
"/" {yylval.Int = DIV; return MULOP;}
"％" {yylval.Int = MOD; return MULOP;}
"＋＋" {yylval.Int = INC; return PPMM;}
"ーー" {yylval.Int = DEC; return PPMM;}
"｜｜" {return LOR;}
"＆＆" {return LAND;}
"＜" {yylval.Int = BLT; return RELOP;}
"＜＝" {yylval.Int = BLE; return RELOP;}
"＝＝" {yylval.Int = BEQ; return RELOP;}
"！＝" {yylval.Int = BNE; return RELOP;}
"＞＝" {yylval.Int = BGE; return RELOP;}
"＞" {yylval.Int = BGT; return RELOP;}
"'"."'" {yylval.Int = yytext[1]; return CNUM;}
"'\\n'" {yylval.Int = '\n'; return CNUM;}
"文字" {yylval.Int = CHAR; return TYPE;}
"do" {return DO;}
"浮動小数" {yylval.Int = DBL; return TYPE;}
"その他" {return ELSE;}
"回数反復" {return FOR;}
"もし" {return IF;}
"整数" {yylval.Int = INT; return TYPE;}
"読込" {return READ;}
"返す" {return RETURN;}
"動的" {return STATIC;}
"型無"  {yylval.Int = VOID; return TYPE;}
"表示" {return WRITE;}
"条件反復" {return WHILE;}
{digit}+ 	{ yylval.Int =  NumOrder(yytext); return NUM; }
{real_num} 	{ sscanf(yytext, "%lf", &yylval.Dbl); return RNUM; }
{id} 		{ /*printf("yytext::%s\n", yytext);*/ yylval.Name = IDentry(yytext, yyleng); return ID; }
{string} 	{ yylval.Name = StrScan(); return STRING; }
{other} 	{ fprintf(stderr, "Illegal char '%c' ignored\n", yytext[0]); }

%%

char *StrScan(void)
{
	char *p, *q;

	if(yytext[yyleng-1] != '\"')
		yyerror("Unterminated string liternal");
	for(p = yytext+1, q = yytext; p < yytext+(yyleng-1); *(q++) = *(p++))
		if(*p == '\\'){
			if(*(++p) == 'n') *p = '\n';
			else if (*p == 't') *p = '\t';
			}
		*q = '\0';
		yyleng = q - yytext;
		return yytext;
}

int NumOrder(char *str)
{

	char str2[1000]="";//上限があるのは数式処理において致命的。
	char *ch;

	for(ch = str; *ch != '\0'; ch++)
	{
		if(*ch==0xffffffef && *(ch+1)==0xffffffbc){
			switch(*(ch+2)){
				case 0xffffff90:
					strcat(str2, "0");
					break;
				case 0xffffff91:
					strcat(str2, "1");
					break;
				case 0xffffff92:
					strcat(str2, "2");
					break;
				case 0xffffff93:
					strcat(str2, "3");
					break;
				case 0xffffff94:
					strcat(str2, "4");
					break;
				case 0xffffff95:
					strcat(str2, "5");
					break;
				case 0xffffff96:
					strcat(str2, "6");
					break;
				case 0xffffff97:
					strcat(str2, "7");
					break;
				case 0xffffff98:
					strcat(str2, "8");
					break;
				case 0xffffff99:
					strcat(str2, "9");
					break;
			}
			ch += 2;
		} else {
			sprintf(str2, "%s%c", str2, *ch);//strcat()は文字型には使えない。
		}
	}

	return atoi(str2);

}
