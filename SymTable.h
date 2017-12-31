typedef enum {NEWSYM, FUNC, F_PROT, VAR} Class;
typedef enum {VOID, CHAR, INT, DBL, ARY=0x10, C_ARY,} Dtype;

typedef struct STentry{
	unsigned char type, class, attrib, deflvl, dim, Nparam;
	char *name;
	int loc, *dlist;
	struct STentry *h_chain;
}STentry, *STP;

#define SALLOC 0x80
#define PARAM 0x40
#define BYREF 0x20
#define DUPFID 0x10

#define ALLOCNUM(C, T) AllocCons((char *)(&C), T, 1)

extern int ByteW[], AStable[];

void OpenBlock(void);
void CloseBlock(void);

STP MakeFuncEntry(char *Fname);
void FuncDef(STP Funcp, Dtype T);
void Prototype(STP Funcp, Dtype T);
void EndFdecl(STP Funcp);
STP VarDecl(char *Name, int Dim);
void MemAlloc(STP Symp, Dtype T, int Param);
STP SymRef(char *Name);
int AllocCons(char *cp, int bl, int Nobj);

void UndefCheck(void);
void PrintSymEntry(STP Symp);