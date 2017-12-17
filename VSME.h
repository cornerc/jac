/* 仮想スタックマシンのインターフェース */

typedef enum { NOP, CASSGN, ASSGN, DASSGN, ADD, DADD, ADDI, SUB, DSUB, SUBI, MUL, DMUL, MULI, DIV, DDIV, DIVI, 
	MOD, DUMMY, MODI, CSIGN, DSIGN, COMP, DCOMP, COMPI, COPY, CPUSH, PUSH, DPUSH, PUSHI, CRVAL, RVAL, DRVAL, 
	REMOVE, CPOP, POP, DPOP, SETFR, INCFR,
	DECFR, INTDBL, DBLINT, JUMP, BLT, BLE, BEQ, BNE, BGE, BGT, CALL, RET, HALT, MCOPY, MALLOC, MFREE, 
	INPUT, OUTPUT, OUTSTR } OP;

#define FP 0x01

#define ISEG_SIZE 2000
#define DSEG_SIZE 5000

#define CBSZ sizeof(char)
#define IBSZ sizeof(int)
#define DBSZ sizeof(double)


typedef struct{
	unsigned char Op, Reg;
	int Addr;
}INSTR;

#define MAX_DSEG (DSEG_SIZE * DBSZ)

void SetPC(int N);
int PC(void);
int StartVSM(int StartAddr, int TraceSW);

void SetI(OP OPcode, int Flag, int Addr);
void Bpatch(int Loc, int Addr);
void WriteDseg(int Loc, char *Data, int Bytes);
void DumpIseg(int first, int last);
void ExecReport(void);

//表示の際、変換指定が整数か浮動小数点か判断する
int IntOrFloat(char *str);

#define Cout(OPcode, Addr) SetI(OPcode, 0, Addr)
#define Pout(OPcode) SetI(OPcode, 0, 0)

extern int DebugSW;
