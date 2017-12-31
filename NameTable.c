#include<stdio.h>
#include<string.h>
#include<stdlib.h>

typedef struct ID_entry {
char *name_ptr;
int len;
struct ID_entry *next;
}ID_entry;

#define IDT_SIZE 113
static ID_entry *IDtable[IDT_SIZE];

static int hash(char *);

char *IDentry(char *sp, int len)
{
	int hval = hash(sp);
	ID_entry *np;


	for (np = IDtable[hval]; np != NULL; np = np->next)
		if((np->len)==len && strcmp(np->name_ptr, sp)==0)
			return np->name_ptr;
	np = (ID_entry *)malloc(sizeof(ID_entry));
	np->name_ptr = (char *)malloc(len + 1);
	np->len = len;
	np->next = IDtable[hval];
	IDtable[hval] = np;
	return strcpy(np->name_ptr, sp);
}

static int hash(char *sp)
{
	unsigned h, g;

	for(h=0 ; *sp != '\0' ; sp++)
	{
		h=(h<<4) + (unsigned)(*sp);
		if((g=h&0xf0000000)!=0)
			h = (h^g >> 24) ^ g;
	}
	return(h%IDT_SIZE);
}
