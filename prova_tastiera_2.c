#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>     /* strerror() */
#include <errno.h>      /* errno */
#include <fcntl.h>      /* open() */
#include <unistd.h>     /* close() */
#include <sys/ioctl.h>  /* ioctl() */

#include <linux/input.h>    /* EVIOCGVERSION ++ */

#define VETT_CONVERSIONE_SIZE 51
#define BUFFER_SIZE 16

#define NESSUN_ERRORE 0
#define ERRORE_ARGOMENTI 1
#define ERRORE_MEMORIA 2
#define MAX_LUNGHEZZA_PAROLA 50

typedef struct nodo_parola
{
	char *parola;
	unsigned int conteggio;
	struct nodo_parola *successiva;
}nodo_parola;

void inizializza (char vettore_conversione_simboli[]);
char codeToLetter (int code, char vettore_conversione_simboli[]);
void contaSimbolo (int code, int vettore_conteggio_simboli[]);
char simboloPiuFrequente (int vettore_conteggio_simboli[], char vettore_conversione_simboli[]);
bool isAlfanumerica (char c);
int totCaratteri(int vettore_conteggio_simboli[]);
void inserisciParola(nodo_parola *testa_lista, char buffer_parola[]);
nodo_parola *creaNodoParola (char *buffer_parola);
void* malloc_con_controllo (unsigned int size);

int main (int argc, char *argv[]) {
	int file_descriptor, fd_fileStat, n_bytes_letti;
	unsigned i;
	struct input_event buffer [BUFFER_SIZE];

	char vettore_conversione_simboli [VETT_CONVERSIONE_SIZE] = {'\0'};
	int vettore_conteggio_simboli [VETT_CONVERSIONE_SIZE] = {0};

	char buffer_parola [MAX_LUNGHEZZA_PAROLA];
	unsigned short iBufferParola = 0;

	nodo_parola *testa_lista = NULL;

	bool continua = true;

	inizializza (vettore_conversione_simboli);

	if (argc < 2) {
		printf(
			"Usage: %s /dev/input/eventN\n"
			"Where X = input device number\n",
			argv[0]
		);
		return ERRORE_ARGOMENTI;
	}

	if ((file_descriptor = open(argv[1], O_RDONLY)) < 0) {
		printf(
			"Error: unable to open `%s'\n",
			argv[1]
		);
	}

	/* Loop. Read event file and parse result. */
	while (continua) {
		n_bytes_letti = read (file_descriptor, buffer, sizeof(struct input_event) * BUFFER_SIZE);

		if (n_bytes_letti < (int) sizeof(struct input_event)) {
			printf (
				"ERR %d:\n"
				"Reading of `%s' failed\n"
				"%s\n",
				errno, argv[1], strerror (errno)
			);
			break;
		}

		/* Implement code to translate type, code and value */
		for (i = 0; i < n_bytes_letti / sizeof(struct input_event); ++i) {
			if (buffer[i].type == EV_KEY && buffer[i].value == 1) {
				char carattere = codeToLetter (buffer[i].code, vettore_conversione_simboli);
				printf (
					"%ld.%06ld: "
					"code=%02x "
					"lettera=%c\n",
					buffer[i].time.tv_sec,
					buffer[i].time.tv_usec,
					buffer[i].code,
					carattere
				);
				// conteggio lettere
				contaSimbolo (buffer[i].code, vettore_conteggio_simboli);

				// conteggio parole
				if (isAlfanumerica (carattere))
				{
					buffer_parola[iBufferParola]=carattere;
					iBufferParola++;
				}
				else
				{
					buffer_parola[iBufferParola]='\0';

					if (strcmp (buffer_parola, "quit") == 0) {
						riepilogo (); //TODO
						continua = false;
						break;
					}

					inserisciParola (testa_lista, buffer_parola);
					iBufferParola = 0;
				}
			}
		}
	}

	close (file_descriptor);

	return NESSUN_ERRORE;
}

void inizializza (char vettore_conversione_simboli[]) {
	vettore_conversione_simboli [2] = '1';
	vettore_conversione_simboli [3] = '2';
	vettore_conversione_simboli [4] = '3';
	vettore_conversione_simboli [5] = '4';
	vettore_conversione_simboli [6] = '5';
	vettore_conversione_simboli [7] = '6';
	vettore_conversione_simboli [8] = '7';
	vettore_conversione_simboli [9] = '8';
	vettore_conversione_simboli [10] = '9';
	vettore_conversione_simboli [11] = '0';
	vettore_conversione_simboli [12] = '-';
	vettore_conversione_simboli [13] = '=';
	vettore_conversione_simboli [15] = '\t';
	vettore_conversione_simboli [16] = 'q';
	vettore_conversione_simboli [17] = 'w';
	vettore_conversione_simboli [18] = 'e';
	vettore_conversione_simboli [19] = 'r';
	vettore_conversione_simboli [20] = 't';
	vettore_conversione_simboli [21] = 'y';
	vettore_conversione_simboli [22] = 'u';
	vettore_conversione_simboli [23] = 'i';
	vettore_conversione_simboli [24] = 'o';
	vettore_conversione_simboli [25] = 'p';
	vettore_conversione_simboli [28] = '\n';
	vettore_conversione_simboli [30] = 'a';
	vettore_conversione_simboli [31] = 's';
	vettore_conversione_simboli [32] = 'd';
	vettore_conversione_simboli [33] = 'f';
	vettore_conversione_simboli [34] = 'g';
	vettore_conversione_simboli [35] = 'h';
	vettore_conversione_simboli [36] = 'j';
	vettore_conversione_simboli [37] = 'k';
	vettore_conversione_simboli [38] = 'l';
	vettore_conversione_simboli [44] = 'z';
	vettore_conversione_simboli [45] = 'x';
	vettore_conversione_simboli [46] = 'c';
	vettore_conversione_simboli [47] = 'v';
	vettore_conversione_simboli [48] = 'b';
	vettore_conversione_simboli [49] = 'n';
	vettore_conversione_simboli [50] = 'm';
}

char codeToLetter (int code, char vettore_conversione_simboli[]) {
	/*for (int i = 0; i < 50; i++)
		printf ("code=%d lettera=%c\n", i, vettore_conversione_simboli[i]);*/
	if (code >= 0 && code < VETT_CONVERSIONE_SIZE)
		return vettore_conversione_simboli [code];
	else
		return '\0';
}

void contaSimbolo (int code, int vettore_conteggio_simboli[]) {
	if (code >= 0 && code < VETT_CONVERSIONE_SIZE)
		vettore_conteggio_simboli [code] ++;
}

char simboloPiuFrequente (int vettore_conteggio_simboli[], char vettore_conversione_simboli[]) {
	char simboloPiuFrequente = vettore_conversione_simboli [0];
	int frequenzaMax = vettore_conteggio_simboli [0];
	for (int i = 1; i < VETT_CONVERSIONE_SIZE; i++) {
		if (vettore_conteggio_simboli [i] > frequenzaMax) {
			frequenzaMax = vettore_conteggio_simboli [i];
			simboloPiuFrequente = vettore_conversione_simboli [i];
		}
	}
	return simboloPiuFrequente;
}

bool isAlfanumerica (char c) {
	return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z');
}

int totCaratteri (int vettore_conteggio_simboli[]){
	int somma = 0;
	for (int i = 0; i < VETT_CONVERSIONE_SIZE; i++) {
		somma += vettore_conteggio_simboli[i];
	}
	return somma;
}

void inserisciParola (nodo_parola *testa_lista, char buffer_parola[]){
	if (testa_lista == NULL) {
		testa_lista = creaNodoParola (buffer_parola);
	}
	nodo_parola temp = *testa_lista;
	while(temp.successiva != NULL) {
		if (strcmp (temp.parola, buffer_parola) == 0) {
			temp.conteggio++;
			return;
		}
	}
	temp.successiva = creaNodoParola (buffer_parola);
}

nodo_parola *creaNodoParola (char *buffer_parola) {
	int lunghezza_parola = strlen (buffer_parola) + 1;
	nodo_parola* nodo = (nodo_parola*)malloc_con_controllo (sizeof (nodo_parola));
	nodo->parola = (char*)malloc_con_controllo (lunghezza_parola);
	strcpy (nodo->parola, buffer_parola);
	nodo->conteggio = 1;
	nodo->successiva = NULL;
	return nodo;
}

void* malloc_con_controllo (unsigned int size)
{
	void *ptr;
	ptr = malloc(size);
	if(ptr == NULL)
	{
		fprintf(stderr, "Errore : memoria heap non allocata");
		exit(ERRORE_MEMORIA);
	}
	return ptr;
}

void riepilogo ()
