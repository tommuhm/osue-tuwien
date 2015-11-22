/**
 * @file dsort.c
 *
 * @author Thomas Muhm 1326486
 * 
 * @brief dsort prints all duplicated output from command1 and command2 in a sorted order
 *
 * @details dsort executes and saves the ouput of command1 and command2 in a list, 
						the list will then be sorted and all duplicated lines will be printed. 
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

/**
 * @brief max length for the strings in string_list 
 */
#define LINE_SIZE (1024)

/* === Macros === */

/**
 * @brief print debug messages if the debug flag is set 
 */
#ifdef ENDEBUG
#define DEBUG(...) do { fprintf(stderr, __VA_ARGS__); } while(0)
#else
#define DEBUG(...)
#endif

/* === Global Variables === */

/** Name of the program */
static const char *progname = "dsort"; /* default name */

/** list for command output */
static struct string_list cmd_out;

/* === Type Definitions === */

/** struct for a list of strings */
struct string_list { 
	/** current items in the list */
	char **items;		
	/** size of the list */
	int size;      
	/** current item count of the list */ 
	int item_count; 
};

/* === Prototypes === */

/** free_resources
 * @brief free allocated resources
 */
static void free_resources(void);

/** compare two strings
 * @brief compare string function for qsort - uses stcmp
 * @param p1 pointer to the first string
 * @param p2 pointer to the second string
 * @return 0 if strings are equal, != 0 otherwise
 */
static int str_cmp_p(const void *p1, const void *p2);

/** program exit point for errors
 * @brief free resources and terminate program on program error
 * @param exitcode exit code
 * @param fmt format string
 */
static void bail_out(int exitcode, const char *fmt, ...);

/** wait for child to finish
 * @brief wait for child process to finish
 * @param child_pid The process id of the child
 */
static void wait_for_child(pid_t child_pid);

/** pipe stdout from command to cmd_out
 * @brief run command in child process and pipe child:stdout to parent:cmd_out
 * @param command The command to execute
 */
static void pipe_from_command(char *command);

/** pipe cmd_out to stdin of command
 * @brief run command in child process and pipe parent:cmd_out to child:stdin 
 * @param command The command to execute
 */
static void pipe_to_command(char *command);

/* === Implementations === */

static void free_resources(void) {
	for (int i = 0; i < cmd_out.item_count; i++) {
		free(cmd_out.items[i]);
	}
	free(&cmd_out.items[0]);
	cmd_out.size = 0;
	cmd_out.items = NULL;
}

static int str_cmp_p(const void *p1, const void *p2) {
	/* The actual arguments to this function are "pointers to
		pointers to char", but strcmp(3) arguments are "pointers
		to char", hence the following cast plus dereference */
	return strcmp(* (char * const *) p1, * (char * const *) p2);
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

static void wait_for_child(pid_t child_pid) {
	DEBUG("parent: waiting for child\n");
	pid_t pid;
	int status;
	while ((pid = wait(&status)) != child_pid ) {
		if (pid != -1) continue; /* other child */
		if (errno == EINTR) continue; /* interrupted */
		bail_out(errno, "parent: can‘t wait");
	}
	if (WEXITSTATUS(status) == EXIT_SUCCESS) {
		DEBUG("parent: child exit successfully\n"); 
	} else {
		DEBUG("parent: child had an error: %s\n", strerror(WEXITSTATUS(status)));	
	}
}

static void pipe_from_command(char *command) {
	DEBUG("pipe_from_command %s\n", command);
	// create arg list for command
	char *cmd[] = { "bash", "-c", command, (char *) 0};
	// create pipe 
	int cmd_pipe[2];
	if (pipe(cmd_pipe) != 0) {
		bail_out(errno, "can't create pipe");	
	}
	// run commmand in child process
	pid_t child_pid;
	switch (child_pid = fork()) {
		case -1: 
			bail_out(errno, "can't fork");
			break;
		case 0:
			// child (rewire pipes and execute command)
			if (close(cmd_pipe[0]) != 0) {
				bail_out(errno, "child: can't close read pipe");			
			}
			if (fflush(stdout) != 0) {
				bail_out(errno, "child: could not flush stdout");			
			}
			if (dup2(cmd_pipe[1], fileno(stdout)) == -1) {
				bail_out(errno, "child: can't rewire write pipe");			
			}
			if (close(cmd_pipe[1]) != 0) {
				bail_out(errno, "child: can't close fd after rewire");
			}							
			(void) execv("/bin/bash", cmd);
			bail_out(errno, "child: can't exec");
			break;
		default:
			// parent (read from pipe into cmd output)
			if (close(cmd_pipe[1]) != 0) {
				bail_out(errno, "parent: can't close write pipe");			
			}

			char buffer[LINE_SIZE];
			bzero(&buffer, sizeof buffer);
			FILE *read_stream;
			if ((read_stream = fdopen(cmd_pipe[0], "r")) == NULL) {
				bail_out(errno, "parent: could not open read pipe");			
			}

			while(fgets(buffer, sizeof buffer, read_stream) != NULL) {
				if (cmd_out.items == NULL) {
					cmd_out.size = 8;
					cmd_out.items = malloc(cmd_out.size * sizeof (char *));
				} else if (cmd_out.size == cmd_out.item_count) {
					cmd_out.size *= 2;
					cmd_out.items = realloc(cmd_out.items, cmd_out.size * sizeof  (char *));
				}
				cmd_out.items[cmd_out.item_count] = strdup(buffer);
				cmd_out.item_count++;
			}

			if (fclose(read_stream) != 0) {
				bail_out(errno, "parent: could not close read stream");
			}

			wait_for_child(child_pid);
			break;
		}
}

static void pipe_to_command(char *command) {
	DEBUG("pipe_to_command %s\n", command);
	// create arg list for command
	char *cmd[] = { "bash", "-c", command, (char *) 0};
	// create pipe 
	int cmd_pipe[2];
	if (pipe(cmd_pipe) != 0) {
		bail_out(errno, "can't create pipe");	
	}
	// run commmand in child process
	pid_t child_pid;
	switch (child_pid = fork()) {
		case -1: 
			bail_out(errno, "can't fork");
			break;
		case 0:
			// child (rewire pipes and execute command)
			if (close(cmd_pipe[1]) != 0) {
				bail_out(errno, "child: can't close write pipe");			
			}
			if (fflush(stdout)) {
				bail_out(errno, "child: could not flush stdout");
			}
			if (dup2(cmd_pipe[0], fileno(stdin)) == -1) {
				bail_out(errno, "child: can't rewire read pipe");			
			}
			if (close(cmd_pipe[0]) != 0) {
				bail_out(errno, "child: can't close fd after rewire");
			}							
			(void) execv("/bin/bash", cmd);
			bail_out(errno, "child: can't exec");
			break;
		default:
			// parent (write cmd output to pipe)
			if (close(cmd_pipe[0]) != 0) {
				bail_out(errno, "parent: can't close read pipe");			
			}
		
			FILE *write_stream;
			if ((write_stream = fdopen(cmd_pipe[1], "w")) == NULL) {
				bail_out(errno, "parent: could not open read pipe");			
			}

			for (int i =0; i < cmd_out.item_count; i++) {
				if (fputs(cmd_out.items[i], write_stream) == EOF) {
					bail_out(EXIT_FAILURE, "fputs failed");
				}
			}

			if (fclose(write_stream) != 0) {
				bail_out(errno, "parent: could not close write stream");
			}
			
			wait_for_child(child_pid);
			break;
	}
}

/**
 * @brief Program entry point
 * @param argc The argument counter
 * @param argv The argument vector
 * @return EXIT_SUCCESS on success and an error code if a problem occured
 */
int main(int argc, char **argv) {

	if(argc > 0) {
		progname = argv[0];
	}
	if (argc != 3) {
		bail_out(EXIT_FAILURE, "Usage: %s \"command1\" \"command2\"", progname);		
	}

	bzero(&cmd_out, sizeof cmd_out);
	pipe_from_command(argv[1]);
	pipe_from_command(argv[2]);

	// sort cmd output list
	qsort(cmd_out.items, cmd_out.item_count, sizeof(char *), str_cmp_p);

	DEBUG("### sorted command output ###\n");
	for (int i = 0; i < cmd_out.item_count; i++) {
		DEBUG("%d: %s", i, cmd_out.items[i]);
	}
	DEBUG("###\n");

	pipe_to_command("uniq -d");

	free_resources();

	return EXIT_SUCCESS;
}
