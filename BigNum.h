
#define BNSIZE 5
#define ZERO "0"
#define ONE "1"

typedef struct BN{//大きい実数。
	int num[BNSIZE];
	int max;
	int sign;//0:+, 1:-
}BN;

BN InitBN(char *str);
BN CngSignBN(BN a), MaxBN(BN a, BN b), MinBN(BN a, BN b), AbsBN(BN a);
BN AddBN(BN a, BN b), SubBN(BN a, BN b);
BN MulBN(BN a, BN b), DivBN(BN a, BN b), ModBN(BN a, BN b);
int SameBN(BN a, BN b);
void Reverse(char *str), ShowBN(BN a), ShowAllBN(BN a);