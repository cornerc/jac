#include <stdio.h>
#include <stdlib.h>
#include "SymTable.h"
#include "VSME.h"
#include "ExprTree.h"

#define NUMCHK(X) { if((X)->type < CHAR || (X)->type > DBL) yyerror("Illegal type of expression"); }
#define FMTCHK(X) { if((X)->type != C_ARY) yyerror("Illegal format specification"); }
#define LHSCHK(X) { if((X)->Op == IDS || (X)->Op == AELM) NUMCHK(X) else yyerror("Illegal type of expression"); }


#define DCT_SIZE 50
DCdesc DCtbl[DCT_SIZE];
static int NoOfDC = 0;

static Dtype ToSameType(NP *D1, NP *D2);
static NP ToDouble(NP np);

NP AllocNode(int Nid, NP lt, NP rt)
{
	NP np = (NP)malloc(sizeof(Node));

	np->Op = Nid;
	np->left = lt;
	np->right = rt;
	return np;
}

void FreeSubT(NP np)
{
	if(np != NULL){
		if(np->Op != CONS && np->Op != IDS){
			FreeSubT(np->left); FreeSubT(np->right);
		}
		free(np);
	}
}

NP MakeL(char *Name)
{
	STP sp = SymRef(Name);
	NP np;

	np = AllocNode(IDS, (NP)sp, NULL);
	np->type = sp->type;
	if(sp->class == VAR && sp->dim > 0)
		np->type |= ARY;
	return np;
}

NP MakeN(int Nid, NP lt, NP rt)
{
	NP np = AllocNode(Nid, lt, rt);
	STP sp;

	np->type = lt->type;
	switch(Nid){
		case ASSGN:
			LHSCHK(lt); np->right = TypeConv(rt, lt->type); break;
		case ADD: case SUB: case MUL: case DIV:
			np->type = ToSameType(&np->left, &np->right); break;
		case CSIGN: 
			NUMCHK(lt);
			if(lt->Op == CONS){
				if(lt->type == INT)
					lt->left = (NP)(-INTV(lt));
				else if(lt->type == DBL)
					lt->left = (NP)Wcons(-DCtbl[INTV(lt)].Dcons);
				free(np);
				return lt;
			}
			break;
		case INC: case DEC: 
			LHSCHK(lt);
		case NOT: case MOD: case AND: case OR:
			if(rt != NULL) INTCHK(rt);
			INTCHK(lt); 
			np->type = INT; 
			break;
		case COMP:
			ToSameType(&np->left, &np->right);
			np->type = INT;
			break;
		case AELM: 
			if(rt->Op != SUBL) INTCHK(rt);
			if(sp = SYMP(lt)){
				np->etc = (rt->Op==SUBL)? rt->etc : 1;
				if(np->etc == sp->dim)
					np->type &= (~ARY);
				else if (np->etc > sp->dim)
					yyerror("Too many subscripts or non-array");
			}
			break;
		case SUBL: 
			INTCHK(rt);
			if(lt->Op != SUBL) INTCHK(lt);
		case ARGL:
			np->etc = (lt->Op==Nid)? lt->etc+1: 2;
			break;
		case CALL:
			if(sp = SYMP(lt)){
				int n;
				n = (rt == NULL)? 0 : (rt->Op == ARGL)? rt->etc: 1;
				if(sp->Nparam != n) yyerror("Number of arguments unmatched");
				if(sp->class != FUNC && sp->class != F_PROT) yyerror("Trying to call non-function");
			}
			break;
		case INPUT:
			FMTCHK(lt);
			LHSCHK(rt);
			np->type = INT; break;
		case OUTPUT: 
			NUMCHK(rt);
		case OUTSTR: 
			FMTCHK(lt); 
			np->type = VOID;
	}
	return np;
}

NP TypeConv(NP ep, Dtype To)
{
	Dtype T = ep->type;

	if(T < CHAR || T > DBL)
		yyerror("Illegal type conversion");
	switch(To){
		case CHAR: case INT:
			if(T == DBL){
				if(ep->Op == CONS)
					ep->left = (NP)(int)DCtbl[INTV(ep)].Dcons;
				else
					ep = AllocNode(DBLINT, ep, NULL);
				ep->type = INT;
			}
			break;
		case DBL:
			return (T==INT || T==CHAR)? ToDouble(ep): ep;
		default: yyerror("Illegal casting");

	}
	return ep;
}

static Dtype ToSameType(NP *D1, NP *D2)
{
	Dtype T1, T2;

	if((T1 = (*D1)->type) == CHAR) T1 = INT;
	if((T2 = (*D2)->type) == CHAR) T2 = INT;
	if(T1 == VOID || T2 == VOID || T1 & ARY || T2 & ARY)
		yyerror("Incompatible type of binary operation");
	else if (T1 == INT && T2 == DBL)
		*D1 = ToDouble(*D1);
	else if (T1 == DBL && T2 == INT)
		*D2 = ToDouble(*D2);
	return T1 > T2 ? T1 : T2;
}

static NP ToDouble(NP np)
{
	if(np->Op == CONS)
		np->left = (NP)Wcons((double)INTV(np));
	else
		np = AllocNode(INTDBL, np, NULL);
	np->type = DBL;
	return np;
}

int Wcons(double C)
{
	int i;

	DCtbl[NoOfDC].Dcons = C;
	for(i = 0; DCtbl[i].Dcons != C; i++);
		if(i >= NoOfDC)
			DCtbl[NoOfDC++].addr = -1;
		return i;
}

static int TempC;

void WriteTree(NP root)
{
	extern int yylineno;

	TempC = 0;
	printf("\nLine %d\n", yylineno);
	WriteNode(root); printf("\n");
}

int WriteNode(NP np)
{
	int opc, t1, t2;
	char *ops;
	STP sp;
	static char *SymD[] = {"void", "char", "int", "double"},
				*Ccode[] = {"and", "or", "not", "++", "--", "[ ]", "subs", "arg"};
	extern char *Scode[];

	if(np == NULL) return -1;
	switch(opc = np->Op){
		case IDS:
			sp = SYMP(np);
			printf("%5d %-10s ", TempC, "id");
			printf("%-10s%-12s\n", sp->name, SymD[sp->type]);
			break; 
		case CONS: 
			printf("%5d  ", TempC);
			if(np->type == INT)
				printf("%-8s %d\n", "Int", INTV(np));
			else if(np->type == DBL)
				printf("%-8s %f\n", "Double", DCtbl[INTV(np)].Dcons);
			else if(np->type == C_ARY)
				printf("%-8s loc. %6d\n", "String", INTV(np));
			break;
		default:
			t1 = WriteNode(np->left);
			t2 = WriteNode(np->right);
			ops = (opc < AND) ? Scode[opc] : Ccode[opc-AND];
			printf("%5d %-8s (%2d)", TempC, ops, t1);
			if(t2 >= 0) printf(" %5c (%2d)\n", ' ', t2);
			printf("\n");
	}
	return TempC++;
}





