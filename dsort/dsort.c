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
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <strings.h>


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

	// create arg list for command
	char *cmd[] = { "bash", "-c", command, (char *) 0};
	
	// create pipe 
	int command_pipe[2];
	if (pipe(command_pipe) != 0) {
		bail_out(errno, "can't create pipe");	
	}

	// run child - own functions for child and parent behavior?
	pid_t pid, child_pid;
	int status;
	switch (child_pid = fork()) {
		case -1: 
			bail_out(errno, "can't fork");
			break;
		case 0:
			(void) printf("child: child here - exec command\n");
			(void) printf("child: closing read pipe\n");
			if (close(command_pipe[0]) != 0) {
				bail_out(errno, "child: can't close write pipe");			
			}
			(void) printf("child: rewire write pipe to stdout\n");
			(void) fflush(stdout); // TODO error?
			if (dup2(command_pipe[1], fileno(stdout)) == -1) {
				bail_out(errno, "child: can't rewire wire pipe");			
			}
			if (close(command_pipe[1]) != 0) {
				bail_out(errno, "child: can't close fd after rewire");
			}							
			(void) execv("/bin/bash", cmd);
			bail_out(errno, "child: can't exec");
			break;
		default:
			(void) printf("parent: closing write pipe\n");
			if (close(command_pipe[1]) != 0) {
				bail_out(errno, "parent: can't close write pipe");			
			}
			(void) printf("parent: read from pipe\n");
			char buffer[LINE_SIZE];
			bzero(&buffer, sizeof buffer);
			
			FILE *read_pipe;
			if ((read_pipe = fdopen(command_pipe[0], "r")) == NULL) {
				bail_out(errno, "parent: could not open read pipe");			
			}

			char *output_array[LINE_SIZE];
			printf("array size %li\n", sizeof(output_array));
printf("array size %li\n", sizeof(*output_array));			
int array_size = 0;
			while(fgets(buffer, sizeof buffer, read_pipe) != NULL) {
				if (output_array == NULL) {
			//		(void) printf("init array with malloc size: %li %li \n", (sizeof *output_array), sizeof output_array);
			//		output_array = malloc(sizeof *output_array);
			//		free(output_array);
				}


				(void) printf("parent: pipe read: %s\n", buffer);	
			}

			(void) printf("parent: parent here - wait for child to die?\n"); 
			while ((pid = wait(&status)) != child_pid ) {
				if (pid != -1) continue; /* other child */
				if (errno == EINTR) continue; /* interrupted */
				bail_out(errno, "parent: can‘t wait");
			}
			if (WEXITSTATUS(status) == EXIT_SUCCESS) {
				(void) printf("parent: child exit successfully\n"); 
			} else {
				(void) printf("parent: child had an error: %s\n", strerror(WEXITSTATUS(status)));	
			}
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

	// 2 commands von argv laden - done

	// 2 forks fuer kindprozze - done

	// bourne shell in kindprozze aufrufen mit "-c" "command" - done	

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
