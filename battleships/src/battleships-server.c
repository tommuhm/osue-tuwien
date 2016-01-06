/**
 * @file battleships-server.c
 *
 * @author Thomas Muhm 1326486
 * 
 * @brief battleships server
 *
 * @details battleships server 
 *
 * @date 04.01.2016
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include <semaphore.h>
#include <fcntl.h>

#include <sys/mman.h>
#include <unistd.h>

#include "battleships.h"

/* === Constants === */


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
static const char *progname = "battleship-server"; /* default name */

/* === Type Definitions === */

/* === Prototypes === */

/**
 *
 */
static void create_semaphores(void);

/**
 *
 */
static void create_shared_memory(void);

/* === Implementations === */

static void free_resources(void) {
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

static void create_semaphores(void) {
	player_ready = sem_open(SEM_1, O_CREAT | O_EXCL, PERMISSION, 0);
	if (player_ready == SEM_FAILED) {
		bail_out(errno, "could not create semaphore s1");
	}
	round_client = sem_open(SEM_2, O_CREAT | O_EXCL, PERMISSION, 0);
	if (round_client == SEM_FAILED) {
		bail_out(errno, "could not create semaphore s2");
	}	
	round_server = sem_open(SEM_3, O_CREAT | O_EXCL, PERMISSION, 2);
	if (round_server == SEM_FAILED) {
		bail_out(errno, "could not create semaphore s3");
	}
	new_game = sem_open(SEM_4, O_CREAT | O_EXCL, PERMISSION, 2);
	if (new_game == SEM_FAILED) {
		bail_out(errno, "could not create semaphore s4");
	}
}

static void create_shared_memory(void) {
	/* create and/or open shared memory object */
	int shmfd = shm_open(SHM_NAME, O_RDWR | O_CREAT, PERMISSION);
	if (shmfd == -1) {
 		/* error */
		bail_out(errno, "shm_open failed");
	}

	if (ftruncate(shmfd, sizeof *shared) == -1) {
		/* error */
		bail_out(errno, "ftruncate failed");
	}
	
	/* map shared memory object */
	shared = mmap(NULL, sizeof *shared, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
	if (shared == MAP_FAILED) { 
		bail_out(errno, "mmap failed");
		/* error */
	}

	if (close(shmfd) == -1) {
		/* error */
		bail_out(errno, "could not close shared memory file descriptor");
	}

	shmfd = -1; // TODO do i need this, copied from uli
}

/**
 * @brief Program entry point
 * @param argc The argument counter
 * @param argv The argument vector
 * @return EXIT_SUCCESS on success and an error code if a problem occured
 */
int main(int argc, char **argv) {
	// TODO singal handlers
	// TODO get opts trotzdem verwenden?!
	if(argc > 0) {
		progname = argv[0];
	}
	if (argc != 1) {
		bail_out(EXIT_FAILURE, "Usage: %s ", progname);		
	}

	create_semaphores();
	create_shared_memory();

	// waiting for two players


//	while(true) {	
		fprintf("setting up game field");
		wait_sem(player_ready);
		fprintf(stdout, "player 1 ready");
		wait_sem(player_ready);
		fprintf(stdout, "player 2 ready");
	
		post_sem(round_client);
		fprintf(stdout, "game startet");
	
		for (int i = 0; i < 5; i++) {
			wait_sem(server_round);
		
			fprintf(stdout, "server doing stuff for player 1");
		
			post_sem(server_response);
		
		
			wait_sem(server_round);
		
			fprintf(stdout, "server doing stuff for player 2");
		
			post_sem(server_response);
		}
		fprintf(stdout, "server finished");
//	}

	// client 1




//	for(int i = 0; i < 3; ++i) {
//		semWait(s1);
		/* critical section entry ... */
//		shared->data[0] = 23;
//		fprintf(stdout, "critical: data = %d\n", shared->data[0]);
		/* critical section exit ... */
//		semPost(s2);
//	}



	// TODO close stuff to free method
	sem_close(new_game);
	sem_close(player_ready); 
	sem_close(round_client);
	sem_close(round_server);
	sem_unlink(SEM_1); 
	sem_unlink(SEM_2);
	sem_unlink(SEM_3);
	sem_unlink(SEM_4);

	/* unmap shared memory */
	if (munmap(shared, sizeof *shared) == -1) {
		/* error */
		bail_out(errno, "munmap failed");
	}

	/* remove shared memory object */
	if (shm_unlink(SHM_NAME) == -1) {
		/* error */
		bail_out(errno, "shm unlink failed");
	}
	// TODO END


	free_resources();

	return EXIT_SUCCESS;
}
