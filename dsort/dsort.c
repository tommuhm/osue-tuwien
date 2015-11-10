/**
 * @file dsort.c
 *
 * @author Thomas Muhm 1326486
 * 
 * @brief TODO
 *
 * @details TODO
 *
 * @date 10.11.2015
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>


/* === Constants === */

#define LINE_SIZE (1024)


/* === Macros === */

#ifdef ENDEBUG
#define DEBUG(...) do { fprintf(stderr, __VA_ARGS__); } while(0)
#else
#define DEBUG(...)
#endif

/* Length of an array */
#define COUNT_OF(x) (sizeof(x)/sizeof(x[0]))


/* === Global Variables === */

/* Name of the program */
static const char *progname = "dsort"; /* default name */


/* === Prototypes === */

/**
 * free_resources
 * @brief free allocated resources
 */
static void free_resources(void);

/**
 * bail_out  - TODO FREE RESOURCES
 * @brief terminate program on program error
 * @param exitcode exit code
 * @param fmt format string
 */
static void bail_out(int exitcode, const char *fmt, ...);

/**
 *  TODO create pipes, create fork, run command in fork
 *
 */
static void run_command(char *command);


/* === Implementations === */

static void free_resources(void) {
    /* clean up resources */
		(void) printf("cleaning up - TODO\n");
}


static void bail_out(int exitcode, const char *fmt, ...) {
    va_list ap;

    (void) fprintf(stderr, "%s: ", progname);
    if (fmt != NULL) {
        va_start(ap, fmt);
        (void) vfprintf(stderr, fmt, ap);
        va_end(ap);
    }
    if (errno != 0) {
        (void) fprintf(stderr, ": %s", strerror(errno));
    }
    (void) fprintf(stderr, "\n");

    free_resources();
    exit(exitcode);
}

static void run_command(char *command) {
	DEBUG("run command %s\n", command);
	pid_t pid;
	switch (pid = fork()) {
		case -1: 
			bail_out(errno, "can't fork");
			break;
		case 0:
			(void) printf("child here - exec command\n");
			exit(EXIT_SUCCESS);			
			break;
		default:
			(void) printf("parent here - wait for child to die?\n"); 
			break;
	}
}

int main(int argc, char **argv) {

	if(argc > 0) {
		progname = argv[0];
	}
	if (argc != 3) {
		bail_out(EXIT_FAILURE, "Usage: %s \"command1\" \"command2\"", progname);		
	}

	// TODO richtig parsen alles zwischen hochcomma wird zu einem string !!! is eh automatisch oder?

	run_command(argv[1]);

	run_command(argv[2]);

	// 2 commands von argv laden

	// 2 forks fuer kindprozze

	// bourne shell in kindprozze aufrufen mit "-c" "command"	

	// mit pipes die ausgabe in ein array speichern
	// ausgabe zeilenweise lesen (fgets?) size 1023echte zeichen=> SIZE=1024 fuer \0 or \n!!

	// popen darf net verwendet werden, aber evt code davon anschaun? :)

	// array und strings von array müssen dynamisch alloziert erden - auf saubere freigabe achten !!


	// parent darf erst sterben wenn kinder tot sind

	// array sortieren ? (man 3 qsort)

	// forken und "uniq -d" starten - sortierte array mit pipes auf stdin von dem kindprozess spielen


	// alles muss wieder freigegeben werden!!, auch im fehlerfall!!!

	return 0;
}
