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
	new_game = sem_open(SEM_1, O_CREAT | O_EXCL, PERMISSION, 2);
	if (new_game == SEM_FAILED) {
		bail_out(errno, "could not create semaphore s1");
	}
	player_ready = sem_open(SEM_2, O_CREAT | O_EXCL, PERMISSION, 0);
	if (player_ready == SEM_FAILED) {
		bail_out(errno, "could not create semaphore s2");
	}
	client_round = sem_open(SEM_3, O_CREAT | O_EXCL, PERMISSION, 0);
	if (client_round == SEM_FAILED) {
		bail_out(errno, "could not create semaphore s3");
	}	
	server_round = sem_open(SEM_4, O_CREAT | O_EXCL, PERMISSION, 2);
	if (server_round == SEM_FAILED) {
		bail_out(errno, "could not create semaphore s4");
	}
	server_response = sem_open(SEM_5, O_CREAT | O_EXCL, PERMISSION, 2);
	if (server_response == SEM_FAILED) {
		bail_out(errno, "could not create semaphore s5");
	}
	player1 = sem_open(SEM_6, O_CREAT | O_EXCL, PERMISSION, 2);
	if (player1 == SEM_FAILED) {
		bail_out(errno, "could not create semaphore s6");
	}
	player2 = sem_open(SEM_7, O_CREAT | O_EXCL, PERMISSION, 2);
	if (player2 == SEM_FAILED) {
		bail_out(errno, "could not create semaphore s7");
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
		fprintf(stdout, "setting up game field\n");

		struct ship player1ship;
		struct ship player2ship;

		wait_sem(player_ready);
		shared->player = PLAYER1;
		post_sem(client_round);
		wait_sem(server_round);
		fprintf(stdout, "player 1 ready\n");
		
		player1ship = shared->playership;

		wait_sem(player_ready);
		shared->player = PLAYER2;
		post_sem(client_round);
		wait_sem(server_round);
		fprintf(stdout, "player 2 ready\n");

		player2ship = shared->playership;


		shared->state	= STATE_INIT;

		post_sem(player1);
		wait_sem(server_round);
		
		fprintf(stdout, "got ship from client 1\n");

		post_sem(player2);
		wait_sem(server_round);

		fprintf(stdout, "got ship from client 2\n");

		shared->state = STATE_PLAYING;
		post_sem(player1);

		fprintf(stdout, "game startet\n");
	
		for (int i = 0; i < 5; i++) {
			wait_sem(server_round);
			shared->round = i;
			
			

			fprintf(stdout, "got first attack of player1\n");

			post_sem(player1);
			wait_sem(server_round);
			
			fprintf(stdout, "got first attack of player2\n");
			
			post_sem(player2);
		}			

		wait_sem(server_round);
		fprintf(stdout, "5 rounds finished\n");
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
	sem_close(client_round);
	sem_close(server_round);
	sem_close(server_response);
	sem_unlink(SEM_1); 
	sem_unlink(SEM_2);
	sem_unlink(SEM_3);
	sem_unlink(SEM_4);
	sem_unlink(SEM_5);

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
