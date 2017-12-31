#include <stdio.h>
#include "SymTable.h"
#include "VSME.h"

int ByteW[]={IBSZ, CBSZ, IBSZ, DBSZ};
int GAptr = 0, FAptr = 0;

static int Level = 0, MaxFSZ;
static char msg[50];

#define ALIGN(X, Y) (((X + Y - 1) / Y) * Y)
#define M_ALLOC(Loc, P, Sz, Bs) { Loc = P = ALIGN(P, Bs); P += (Sz); }

#define SYMT_SIZE 200
static STentry SymTab[SYMT_SIZE];
static STP SymTptr = &SymTab[0];

#define BT_SIZE 20
static struct{
	int allocp;
	STP first;
}BLKtbl[BT_SIZE] = {{0, &SymTab[0]}};

#define HT_SIZE 131
static STP Htbl[HT_SIZE];
#define SHF(X) ((unsigned int)(X) % HT_SIZE)

#define AST_SIZE 10
int AStable[AST_SIZE];

extern int SymPrintSW;

void OpenBlock(void)
{
	BLKtbl[++Level].first = SymTptr;
	BLKtbl[Level].allocp = FAptr;
}

void CloseBlock(void)
{
	if(FAptr > MaxFSZ) MaxFSZ = FAptr;
	while(SymTptr > BLKtbl[Level].first){
		if((--SymTptr)->dim > 0) free(SymTptr->dlist);
		Htbl[SHF(SymTptr->name)] = SymTptr->h_chain;
	}
	FAptr = BLKtbl[Level--].allocp;
}

static STP MakeEntry(char *Name, Dtype T, Class C)
{
	int h = SHF(Name);
	STP p;

	if((p=SymTptr++) >= &SymTab[SYMT_SIZE]){
		fprintf(stderr, "Too many symbols declared"); 
		exit(-2);
	}
	p->type = T; p->class = C;
	p->dim = p->Nparam = 0;
	p->deflvl = Level;
	p->attrib = (Level == 0) ? SALLOC : 0;
	p->name = Name;
	p->h_chain = Htbl[h];
	return Htbl[h] = p;
}

static STP LookUp(char *Name)
{
	STP p = Htbl[SHF(Name)];


	for(; p!= NULL && p->name != Name; p = p->h_chain);
	return p;
}

STP MakeFuncEntry(char *Fname)
{
	STP p;

	if((p = LookUp(Fname)) == NULL)
		p = MakeEntry(Fname, VOID, NEWSYM);
	else if (p->class != F_PROT)
		yyerror("The function name already declared");
	if(SymPrintSW){
		printf("\n"); PrintSymEntry(p);
	}
	OpenBlock();
	FAptr = MaxFSZ = IBSZ;
	return p;
}

void Prototype(STP Funcp, Dtype T)
{
	if(Level > 1)
		yyerror("The prototype declaration ignored");
	Funcp->class = F_PROT; 
	Funcp->type = T & ~SALLOC;
	Funcp->loc = -1;
	Funcp->Nparam = SymTptr - BLKtbl[Level].first;
	CloseBlock();
	SymTptr += Funcp->Nparam;
}

void FuncDef(STP Funcp, Dtype T)
{
	int n = SymTptr - BLKtbl[Level].first;

	T &= ~SALLOC;
	if(Funcp->class == F_PROT){
		STP p, q;
		if(Funcp->Nparam != n)
			yyerror("No. of paramater unmatched with the prototype");
		if(Funcp->type == T)
			for(p=Funcp+1, q=BLKtbl[Level].first; q < SymTptr; p++, q++){
				if(p->type != q->type || (p->dim != q->dim) || p->attrib != q->attrib)
					yyerror("paramater specification unmatched");
				p->loc = q->loc;
			}
		else yyerror("Function type unmatched to the prototype");
		Bpatch(Funcp->loc, PC() + 3);
		Funcp->attrib |= DUPFID;
	}else if (Funcp->class == NEWSYM)
		Funcp->type = T;
	else{
		yyerror("The function already declared");
		return;
	}
	Funcp->class = FUNC;
	Funcp->Nparam = n;
	Funcp->loc = PC() + 3;
}

void EndFdecl(STP Funcp)
{
	CloseBlock();
	if((Funcp->attrib & DUPFID) == 0)
		SymTptr += Funcp->Nparam;
	Bpatch(Funcp->loc, ALIGN(MaxFSZ, DBSZ));
	if(SymPrintSW) PrintSymEntry(Funcp);
}

STP VarDecl(char *Name, int Dim)
{
	int i, *dtp;
	STP p = LookUp(Name);

	if(p == NULL || p->deflvl < Level){
		p=MakeEntry(Name, VOID, VAR);
		if((p->dim = Dim) > 0){
			p->dlist = dtp = (int *)malloc(sizeof(int)*Dim);
			for(i=0; i<Dim; i++, dtp++)
				if((*dtp = AStable[i]) == 0)
					yyerror("array size is zero");
		}
		else
			p->dlist = NULL;
	} else yyerror("Duplicated declaration");
	return p;
}

void MemAlloc(STP Symptr, Dtype T, int Param)
{
	int n, bs, sz, *p;

	if(T & SALLOC){
		T &= ~SALLOC; 
		Symptr->attrib |= SALLOC;
	}
	if((Symptr->type = T) == VOID)
		yyerror("Void is used as a type name");
	if(Param & PARAM && Symptr->dim > 0)
		Param |= BYREF;
	if((Symptr->attrib |= Param) & BYREF)
		bs = sz = IBSZ;
	else {
		bs = sz = ByteW[T];
		for(n = Symptr->dim, p=Symptr->dlist; n > 0;n--, p++)
			if((sz *= (*p)) <= 0)
				yyerror("Array size unspecified in the definition");
	}
	if((Symptr->attrib) & SALLOC)
		M_ALLOC(Symptr->loc, GAptr, sz, bs)
	else
		M_ALLOC(Symptr->loc, FAptr, sz, bs)
	if(SymPrintSW) PrintSymEntry(Symptr);
}

int AllocCons(char *ConsP, int Blen, int Nobj)
{
	int m, tsz = Blen*Nobj;

	M_ALLOC(m, GAptr, tsz, Blen)
	WriteDseg(m, ConsP, tsz);
	return m;
}

STP SymRef(char *Name)
{
	STP p;

	if((p=LookUp(Name))==NULL){
		sprintf(msg, "Ref. to undeclared identifier %s", Name);
		yyerror(msg);
		p = MakeEntry(Name, INT, VAR);
	}
	return p;
}

void UndefCheck(void)
{
	STP p;

	for(p=BLKtbl[0].first; p< SymTptr; p++)
		if(p->class == F_PROT && p->loc > 0){
			sprintf(msg, "There's no definition of %s", p->name);
			yyerror(msg);
		}
}

void PrintSymEntry(STP Symp)
{
	static char *SymC[] = {"NewSym", "Func", "ProtF", "Var"},
				*SymD[] = {"void", "char", "int", "double"};

				printf("%*c%-10s", Level*4+2, ' ', Symp->name);
				printf(" %-6s %-6s", SymC[Symp->class], SymD[Symp->type]);
				switch(Symp->class){
					int i;
					case VAR:
					printf((Symp->attrib)&SALLOC ? "%5d  " : "%+5d  ", Symp->loc);
					if((Symp->attrib) & PARAM)
						printf("%s  ", (Symp->attrib) & BYREF ? "by-ref" : "by-val");
					for(i=0;i < Symp-> dim; i++)
						printf((Symp->dlist)[i] < 0 ? "[*]" : "[%d]", Symp->dlist[i]);
					break;
					case FUNC:
					printf("%5d Frame = %2d bytes", Symp->loc, MaxFSZ);
				}
				printf("\n");
}





