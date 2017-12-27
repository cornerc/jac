#include <stdio.h>
#include "VSME.h"
#include "SymTable.h"
#include "ExprTree.h"
#include "CodeGen.h"

#define OPC(X, T) ((X)+(T)-INT)
#define FPBIT(P) ((P->attrib) & SALLOC ? 0 : FP)


extern char *IDentry(char *Name, int len);
extern int StartP, DebugSW;

static void Cgen(NP tree), PushLval(NP tree),
			GenArg(STP param, NP arg, int pnumber);
static int GenCond(NP tree, int cond, int bpc),
			GenAddrComp(STP Ap, NP subs);

void GenFuncEntry(STP func)
{
	STP pl;

	SetI(PUSH, FP, 0); 
	Cout(INCFR, -1); 
	Pout(RET);
	Cout(DECFR, PC()-2);
	SetI(POP, FP, 0);
	for(pl = func + func->Nparam; pl > func; pl--)
		if((pl->attrib) & BYREF)
			SetI(POP, FP, pl->loc);
		else
			SetI(OPC(POP, pl->type), FP, pl->loc);
}

void ExprStmnt(NP tree)
{
	if(tree == NULL) return;
	if(DebugSW) WriteTree(tree);
	if(tree->Op == ASSGN && (tree->left)->Op == IDS) {
		STP sp;
		sp = SYMP(tree->left);
		if(sp->attrib & BYREF){
			Cgen(tree);
			Pout(REMOVE);
		}else{
			Cgen(tree->right);
			SetI(OPC(POP, sp->type), FPBIT(sp), sp->loc);
		}
	} else{
		Cgen(tree);
		if(tree->type != VOID) Pout(REMOVE);
	}
	FreeSubT(tree);
}

int CtrlExpr(NP tree, int jc)
{
	if(tree == NULL) return -1;
	if(DebugSW) WriteTree(tree);
	INTCHK(tree);
	jc = GenCond(tree, jc, -1);
	FreeSubT(tree);
	return jc;

}

void GenReturn(STP func, NP tree)
{
	if(func->type == VOID){
		if(tree != NULL)
			yyerror("Meaningless return value");
	} else if(tree == NULL){
		Cout(PUSHI, 0);
	} else{
		tree = TypeConv(tree, func->type);
	}
	if(tree){
		if(DebugSW) 
			WriteTree(tree);
		Cgen(tree); 
		FreeSubT(tree);
	}
	Cout(JUMP, (func->loc)-3);	
}

void Epilogue(void)
{
	STP sp;

	UndefCheck();
	StartP = PC();
	Cout(SETFR, MAX_DSEG);
	if(sp = SymRef(IDentry("本処理", strlen("本処理")))){
		if(sp->Nparam > 0)
			yyerror("Parameter list specified at main()");
		Cout(CALL, sp->loc);
	}
	Pout(HALT);
}

static void Cgen(NP tree)
{
	int Nid = tree->Op, T = tree->type, jc, n;
	NP lt = tree->left, rt = tree->right;
	STP sp;

	switch (Nid){
		case ASSGN:
			PushLval(lt);
			Cgen(rt);
			break;
		case ADD: case SUB: case MUL: case DIV: case MOD:
			Cgen(lt);
			if(T == INT && (rt->Op) == CONS){
				Cout(Nid+2, INTV(rt));
				return;
			}
			Cgen(rt);
			break;	
		case INTDBL:
			T = INT;
		case CSIGN: case DBLINT:
			Cgen(lt);
			break;
		case INC: case DEC:
			T = lt->type;
			if(lt->Op == IDS && !((sp = SYMP(lt))->attrib & BYREF)){
				SetI(OPC(PUSH, T), FPBIT(sp), sp->loc);
				if(tree->etc == PRE){
					Cout(Nid == INC ? ADDI : SUBI, 1);
					Pout(COPY);
				} else {
					Pout(COPY);
					Cout(Nid == INC ? ADDI : SUBI, 1);
				}
				SetI(OPC(POP, T), FPBIT(sp), sp->loc);
			} else {
				PushLval(lt);
				Pout(COPY);
				Pout(OPC(RVAL, T));
				Cout(Nid == INC ? ADDI : SUBI, 1);
				Pout(OPC(ASSGN, T));
				if(tree->etc == POST)
					Cout(Nid == INC ? SUBI : ADDI, 1);
			}
			return;
		case AND: case OR: case NOT: case COMP:
			jc = GenCond(tree, Nid == NOT ? TJ : FJ, -1);
			Cout(PUSHI, 1);
			Cout(JUMP, PC()+2);
			Bpatch(jc, PC());
			Cout(PUSHI, 0);
			return;
		case AELM:
			PushLval(tree); 
			Nid = RVAL;
			break;
		case CALL:
			sp = SYMP(lt);
			GenArg(sp + sp->Nparam, rt, sp->Nparam);
			Cout(CALL, sp->loc);
			if(sp->class == F_PROT)
				sp->loc = PC() -1;
			return;
		case INPUT: PushLval(lt); PushLval(rt); break;
		case OUTPUT: PushLval(lt); Cgen(rt); Pout(OUTPUT); return;
		case OUTSTR: PushLval(lt); Pout(OUTSTR); return;
		case CONS:
			n = (int)lt;
			if(T == INT)
				Cout(PUSHI, n);
			else if(T == DBL){
				if(DCtbl[n].addr < 0)
					DCtbl[n].addr = ALLOCNUM(DCtbl[n].Dcons, DBSZ);
				Cout(DPUSH, DCtbl[n].addr);
			}
			return;
		case IDS:
			sp = (STP)lt;
			if(sp->attrib & BYREF){
				SetI(PUSH, FP, sp->loc);
				Pout(OPC(RVAL, T));
			} else {
				SetI(OPC(PUSH, T), FPBIT(sp), sp->loc);
			}
			return;
	}
	Pout(OPC(Nid, T));
}

static void PushLval(NP tree)
{
	int n, m, offset;
	STP sp;

	switch(tree->Op){
		case IDS:
			sp = SYMP(tree);
			if(sp->attrib & BYREF)
				SetI(PUSH, FP, sp->loc);
			else
				SetI(PUSHI, FPBIT(sp), sp->loc);
			return;
		case CONS:
			Cout(PUSHI, INTV(tree));
			return;
		case AELM:
			sp = SYMP(tree->left);
			offset= GenAddrComp(sp, tree->right);
			for(m = ByteW[sp->type], n = tree->etc; n < sp->dim; n++)
				m *= (sp->dlist)[n];
			if(offset >= 0){
				if(sp->attrib & BYREF){
					SetI(PUSHI, FP, sp->loc);
					Cout(ADDI, offset*m);
				} else {
					SetI(PUSHI, FPBIT(sp), sp->loc + offset * m);
				}
			} else {
				if(m > 1) Cout(MULI, m);
				if(sp->attrib & BYREF){
					SetI(PUSH, FP, sp->loc);
					Pout(ADD);
				} else {
					SetI(ADDI, FPBIT(sp), sp->loc);
				}
			}
	}
}

static int GenAddrComp(STP Ap, NP subs)
{
	if(subs->Op == SUBL){
		int m, n;
		if((m = GenAddrComp(Ap, subs->left)) >= 0){
			m *= (Ap->dlist)[subs->etc - 1];
			if((n = GenAddrComp(Ap, subs->right)) >= 0)
				return m + n;
			if(m > 0) Cout(ADDI, m);
		} else {
			Cout(MULI, (Ap->dlist)[subs->etc - 1]);
			Cgen(subs->right);
			Pout(ADD);
		} 
	} else if(subs->Op == CONS){
		return INTV(subs);
	} else{
		Cgen(subs);
	}
	return -1;
}

static void GenArg(STP param, NP arg, int pn)
{
	if(pn <= 0 || arg == NULL) return;
	if(arg->Op == ARGL){
		GenArg(param - 1, arg->left, pn-1);
		arg = arg->right;
	}
	
	if (param->attrib & BYREF){
		if (param->type != ((arg->type)&(~ARY)))
			yyerror("Argument type unmatched to parameter");
		PushLval(arg);
	} else {
		arg = TypeConv(arg, param->type);
		Cgen(arg);
	}
}

static int GenCond(NP tree, int cond, int bpc)
{
	int temp, n , cc;
	NP lt = tree->left, rt = tree->right;
	static int NegC[] = {BGE, BGT, BNE, BEQ, BLT, BLE};

	switch(tree->Op){
		case AND:
			if(cond == TJ){
				temp = GenCond(lt, FJ, -1);
				bpc = GenCond(rt, TJ, bpc);
				Bpatch(temp, PC());
			} else {
				bpc = GenCond(lt, FJ, bpc);
				bpc = GenCond(rt, FJ, bpc);
			}
			return bpc;
		case OR:
			if(cond == TJ){
				bpc = GenCond(lt, TJ, bpc);
				bpc = GenCond(rt, TJ, bpc);
			} else {
				temp = GenCond(lt, TJ, -1);
				bpc = GenCond(rt, FJ, bpc);
				Bpatch(temp, PC());
			}
			return bpc;
		case NOT:
			return GenCond(lt, cond == TJ ? FJ : TJ, bpc);
		case COMP:
			Cgen(lt);
			if(rt->Op == CONS && rt->type == INT){
				if(INTV(rt) != 0) Cout(COMPI, INTV(rt));
			} else {
				Cgen(rt);
				Pout(lt->type == DBL ? DCOMP : COMP);
			}
			cc = tree->etc;
			Cout(cond == TJ ? cc : NegC[cc-BLT], bpc);
			return PC()-1;
		case CONS:
			n = (int)lt;
			if((cond == TJ && n != 0) || (cond == FJ && n == 0)){
				Cout(JUMP, bpc);
				bpc = PC() - 1;
			}
			return bpc;
		default:
			Cgen(tree);
			SetI(JUMP, cond == TJ ? BNE : BEQ, bpc);
			return PC() - 1;
	}
}




