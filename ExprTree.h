enum { AND = 100, OR, NOT, INC, DEC, AELM, SUBL, ARGL, CONS, IDS };

typedef struct Node {
	unsigned char Op, type, etc;
	struct Node *left, *right;
}Node, *NP;

typedef struct {
	double Dcons;
	int addr;
} DCdesc;

extern DCdesc DCtbl[];

#define INTV(P) (int)((P)->left)
#define SYMP(P) (STP)((P)->left)
#define MakeC(X, T, D) { (X) = AllocNode(CONS, (NP)(D), NULL); (X)->type = T; }
#define INTCHK(X) { if((X->type) != INT && (X->type) != CHAR) yyerror("Non-integer type expression"); }
#define VAR_NODE(X, Y) { X = MakeL(Y); if((SYMP(X))->class != VAR) yyerror("Non-variable name"); }

NP AllocNode(int Op, NP left, NP right);
NP MakeL(char *Name);
NP MakeN(int Op, NP left, NP right);
NP TypeConv(NP tree, Dtype T);
int Wcons(double value);
void WriteTree(Node *root);
void FreeSubT(NP tree);