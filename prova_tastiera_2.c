#include <stdio.h>

#include <string.h>     /* strerror() */
#include <errno.h>      /* errno */

#include <fcntl.h>      /* open() */
#include <unistd.h>     /* close() */
#include <sys/ioctl.h>  /* ioctl() */

#include <linux/input.h>    /* EVIOCGVERSION ++ */

#define BUFFER_SIZE 16
#define NESSUN_ERRORE 0
#define ERRORE_ARGOMENTI 1

int main(int argc, char *argv[])
{
    int file_descriptor, n_bytes_letti;
    unsigned i;

    /* A few examples of information to gather */
    unsigned version;
    unsigned short id[4];                   /* or use struct input_id */
    char name[256] = "N/A";

    struct input_event buffer[BUFFER_SIZE]; /* Read up to N events ata time */

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
    /* Error check here as well. */
    ioctl (file_descriptor, EVIOCGVERSION, &version);
    ioctl (file_descriptor, EVIOCGID, id); 
    ioctl (file_descriptor, EVIOCGNAME(sizeof(name)), name);

    printf (
        "Name      : %s\n"
        "Version   : %d.%d.%d\n"
        "ID        : Bus=%04x Vendor=%04x Product=%04x Version=%04x\n"
        "----------\n"
        ,
        name,

        version >> 16,
        (version >> 8) & 0xff,
        version & 0xff,

        id[ID_BUS],
        id[ID_VENDOR],
        id[ID_PRODUCT],
        id[ID_VERSION]
    );

    /* Loop. Read event file and parse result. */
    for (;;) {
        n_bytes_letti = read (file_descriptor, buffer, sizeof(struct input_event) * BUFFER_SIZE);

        if (n_bytes_letti < (int) sizeof(struct input_event)) {
            fprintf(stderr,
                "ERR %d:\n"
                "Reading of `%s' failed\n"
                "%s\n",
                errno, argv[1], strerror(errno)
            );
            break;
        }

        /* Implement code to translate type, code and value */
        for (i = 0; i < n_bytes_letti / sizeof(struct input_event); ++i) {
	    if (buffer[i].type == EV_KEY && buffer[i].value == 1) {
		    printf (
		        "%ld.%06ld: "
		        "type=%02x "
		        "code=%02x "
		        "value=%02x\n",
		        buffer[i].time.tv_sec,
		        buffer[i].time.tv_usec,
		        buffer[i].type,
		        buffer[i].code,
		        buffer[i].value
		    );
	    }
        }
    }

    close (file_descriptor);

    return NESSUN_ERRORE;
}

