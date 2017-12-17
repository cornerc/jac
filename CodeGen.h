enum { FJ, TJ };
enum { PRE, POST };

void GenFuncEntry(STP FuncP);
void ExprSmnt(NP tree);
int CtrlExpr(NP tree, int jc);
void GenReturn(STP func, NP tree);
void Epilogue(void);