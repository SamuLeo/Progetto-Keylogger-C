#include <stdio.h>
#include <stdlib.h>

void main () {
	//int vettore [10000000];
	
	char *p = malloc (2000000000 * sizeof(char) );
	int i;
	for (i = 0; i < 2000000000; i++)
		p[i] = (char)i;
	fflush (stdin);
	getchar();
}
