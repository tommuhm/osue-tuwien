/**
 * @file battleships-server.c
 *
 * @author Thomas Muhm 1326486
 * 
 * @brief battleships server which handles manages one game between 2 clients
 *
 * @details battleships server which communications via semaphores and shared memory with multiple clients, there can only be one active game between 2 clients
 *
 * @date 04.01.2016
 * 
 */
#include "battleships.h"

/* === Global Variables === */

/** the location for the ship from player1 */
static struct ship player1_ship;

/** the location for the ship from player1 */
static struct ship player2_ship;

/** number of hits for player1 */
static int player1_hits = 0;

/** number of hits for player2 */
static int player2_hits = 0;

/** flag which indicate that the game has finished */
static int game_finished;

/** Name of the program */
static const char *progname = "battleships-server"; /* default name */

/* === Prototypes === */

/**	create_semaphores
 *	@brief tires to create the semaphores for the server-client communication
 */
static void create_semaphores(void);

/**	create_shared_memory
 *	@brief tires to create the shared memory
 */
static void create_shared_memory(void);

/**	is_hit
 *	@brief check if the shot of player hit the enemey ship
 */
static int is_hit(int player, int shot);

/**	has_won
 *	@brief calc the hit_count for player and check if he has won
 */
static int has_won(int player, int is_hit);

/**	calc_result
 *	@brief calc result from user player_nr and start next turn
 */
static void calc_result(int player_nr, sem_t *player_sem, sem_t *other_player_sem);

/**	wait_for_turn
 *	@brief wait for the turn of the server and lock the memory if it begins
 */
static void wait_for_turn();

/**	turn_finished
 *	@brief free memory and send post to the next semaphore
 */
static void turn_finished(sem_t *next);

/* === Implementations === */

void free_resources(void) {
	/* unmap shared memory */
	if (munmap(shared, sizeof *shared) == -1) {
		print_error(errno, "munmap failed");
	}
	/* remove shared memory object */
	if (shm_unlink(SHM_NAME) == -1) {
		print_error(errno, "shm unlink failed");
	}

	if (sem_close(server) == -1) {
		print_error(errno, "close server sem failed");
	}
	if (sem_close(player1) == -1) {
		print_error(errno, "close player1 sem failed");
	}	
	if (sem_close(player2) == -1) {
		print_error(errno, "close player2 sem failed");
	}
	if (sem_close(new_game) == -1) {
		print_error(errno, "close new_game sem failed");
	}
	if (sem_close(memory) == -1) {
		print_error(errno, "close memory sem failed");
 	}

	if (sem_unlink(SEM_1) == -1) {
		print_error(errno, "unlink sem1 failed");
	}
	if (sem_unlink(SEM_2) == -1) {
		print_error(errno, "unlink sem2 failed");
	}
	if (sem_unlink(SEM_3) == -1) {
		print_error(errno, "unlink sem3 failed");
	}
	if (sem_unlink(SEM_4) == -1) {
		print_error(errno, "unlink sem4 failed");
	}
	if (sem_unlink(SEM_5) == -1) {
		print_error(errno, "unlink sem5 failed");
	}
}

void print_error(int exitcode, const char *fmt, ...) {
	va_list ap;

	(void) fprintf(stderr, "%s: ", progname);
	if (fmt != NULL) {
		va_start(ap, fmt);
		(void) vfprintf(stderr, fmt, ap);
		va_end(ap);
	}
	if (exitcode == errno && errno != 0) {
		(void) fprintf(stderr, ": %s", strerror(errno));
	}
	(void) fprintf(stderr, "\n");
}

void bail_out(int exitcode, const char *fmt, ...) {
	print_error(exitcode, fmt);
	exit(exitcode);
}

void setup_signal_handler(void) {
    /* setup signal handlers */
    const int signals[] = {SIGINT, SIGTERM};
    struct sigaction s;

    s.sa_handler = signal_handler;
    s.sa_flags   = 0;
    if(sigfillset(&s.sa_mask) < 0) {
        bail_out(EXIT_FAILURE, "sigfillset");
    }
    for(int i = 0; i < COUNT_OF(signals); i++) {
        if (sigaction(signals[i], &s, NULL) < 0) {
            bail_out(EXIT_FAILURE, "sigaction");
        }
    }
}

void signal_handler(int sig) {
	quit = 1;
}

void wait_sem(sem_t *sem) {
	if (sem_wait(sem) == -1) {
		bail_out(errno, "sem_wait failed");
	}
}

void post_sem(sem_t *sem) {
	if (sem_post(sem) == -1) {
		bail_out(errno, "sem_post failed");
	}
}

/*
static void wait_for_turn() {
	wait_sem(server);
	wait_sem(memory);
}

static void turn_finished(sem_t *next) {
	post_sem(memory);
	post_sem(next);
}*/

static void create_semaphores(void) {
	server = sem_open(SEM_1, O_CREAT | O_EXCL, PERMISSION, 0);
	if (server == SEM_FAILED) {
		bail_out(errno, "could not create semaphore s1");
	}
	player1 = sem_open(SEM_2, O_CREAT | O_EXCL, PERMISSION, 0);
	if (player1 == SEM_FAILED) {
		bail_out(errno, "could not create semaphore s2");
	}
	player2 = sem_open(SEM_3, O_CREAT | O_EXCL, PERMISSION, 0);
	if (player2 == SEM_FAILED) {
		bail_out(errno, "could not create semaphore s3");
	}
	new_game = sem_open(SEM_4, O_CREAT | O_EXCL, PERMISSION, 0);
	if (new_game == SEM_FAILED) {
		bail_out(errno, "could not create semaphore s4");
	}
	memory = sem_open(SEM_5, O_CREAT | O_EXCL, PERMISSION, 1);
	if (memory == SEM_FAILED) {
		bail_out(errno, "could not create semaphore s5");
	}
}

static void create_shared_memory(void) {
	/* create and/or open shared memory object */
	int shmfd = shm_open(SHM_NAME, O_RDWR | O_CREAT, PERMISSION);
	if (shmfd == -1) {
		bail_out(errno, "shm_open failed");
	}
	if (ftruncate(shmfd, sizeof *shared) == -1) {
		bail_out(errno, "ftruncate failed");
	}
	/* map shared memory object */
	shared = mmap(NULL, sizeof *shared, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
	if (shared == MAP_FAILED) { 
		bail_out(errno, "mmap failed");
	}
	if (close(shmfd) == -1) {
		bail_out(errno, "could not close shared memory file descriptor");
	}
}

static int is_hit(int player, int shot) {
		struct ship *enemy_ship;		
		if (player == PLAYER1) {
			enemy_ship = &player2_ship;
		} else {
			enemy_ship = &player1_ship;		
		}
		int hit = (shot == enemy_ship->a || shot == enemy_ship->b || shot == enemy_ship->c);
		(void) fprintf(stdout, "Got shot %d ship at %d, %d, %d\n", shot, enemy_ship->a, enemy_ship->b, enemy_ship->c); 
		return hit;
}

static int has_won(int player, int is_hit) {
	int *hits;
	if (player == PLAYER1) {
		hits = &player1_hits;
	} else {
		hits = &player2_hits;		
	}
	*hits += is_hit;
	if (*hits == 3) {
		return 1;	
	} else {
		return 0;
	}
}

static void calc_result(int player_nr, sem_t *player_sem, sem_t *other_player_sem) {
	wait_sem(server);

	int state = shared->state;
	sem_t *next_player = other_player_sem;
	if (state == STATE_CLIENT_DISCONNECTED) {
		game_finished = 1;
		(void) fprintf(stdout, "Player %d has disconnected\n", player_nr);
	}
	if (state == STATE_GIVEN_UP) {
		game_finished = 1;
		(void) fprintf(stdout, "Player %d has given up\n", player_nr);
	} else {
		int hit = is_hit(player_nr, shared->player_shot);
		shared->was_hit = hit;
		(void) fprintf(stdout, "Player %d attack was: %d\n", player_nr, hit);

		if (has_won(player_nr, hit)) {
			game_finished = 1;
			shared->state = STATE_GAME_WON;
			(void) fprintf(stdout, "Player %d won the game\n", player_nr);

			post_sem(player_sem);
			wait_sem(server);

			shared->state = STATE_GAME_LOST;
		} else {
			next_player = player_sem;
		}
	}

	post_sem(next_player);
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
	if (argc != 1) {
		bail_out(EXIT_FAILURE, "Usage: %s ", progname);		
	}
	if (atexit(free_resources) != 0) {
		bail_out(errno, "atexit failed");
	}
	setup_signal_handler();
	create_semaphores();
	create_shared_memory();

	while(quit == 0) {
		post_sem(new_game);
		post_sem(new_game);

		game_finished = 0;
		shared->state = STATE_GAME;

		(void) fprintf(stdout, "Setting up game field 0 waiting for players.\n");

		// wait for two players
		wait_sem(server);
		wait_sem(server);
		if (shared->state == STATE_CLIENT_DISCONNECTED) {
			(void) fprintf(stdout, "client has been disconnected - starting new game\n");
			post_sem(player2);	// notify other player
			wait_sem(server);
			continue;						// start new game
		}

		(void) fprintf(stdout, "Got two players!\n");

		/* get player 1 ship */
		shared->player = PLAYER1;
		post_sem(player2);
		wait_sem(server);
		if (shared->state == STATE_CLIENT_DISCONNECTED) {
			(void) fprintf(stdout, "client has been disconnected - starting new game\n");
			post_sem(player2);	// notify other player
			wait_sem(server);
			continue;						// start new game
		}
		(void) fprintf(stdout, "Player 1 ready.\n");	
		player1_ship = shared->player_ship;

		/* get player 2 ship */
		shared->player = PLAYER2;
		post_sem(player2);
		wait_sem(server);	
		fprintf(stdout, "check disconnect\n");
		if (shared->state == STATE_CLIENT_DISCONNECTED) {
			(void) fprintf(stdout, "client has been disconnected - starting new game\n");
			post_sem(player2);	// notify other player
			wait_sem(server);
			continue;						// start new game
		}
		(void) fprintf(stdout, "Player 2 ready.\n");
		player2_ship = shared->player_ship;

		/* run game turn based */
		(void) fprintf(stdout, "Game startet.\n");
		post_sem(player1);

		while(game_finished == 0) {
			calc_result(PLAYER1, player1, player2);
			if (game_finished == 1) {
				break;
			}
			calc_result(PLAYER2, player2, player1);
		}	

		/* game end */
		wait_sem(server);
		(void) fprintf(stdout, "Game finished.\n");
	}

	return EXIT_SUCCESS;
}
