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
#include <signal.h>

#include "battleships.h"

/* === Constants === */

/* === Macros === */

/* === Global Variables === */

static volatile sig_atomic_t quit = 0;

static struct ship player1_ship;
static struct ship player2_ship;

static int player1_hits = 0;
static int player2_hits = 0;
static int game_finished;

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

static void signal_handler(int sig);

/* === Implementations === */

static void free_resources(void) {
	/* unmap shared memory */
	if (munmap(shared, sizeof *shared) == -1) {
		bail_out(errno, "munmap failed");
	}
	/* remove shared memory object */
	if (shm_unlink(SHM_NAME) == -1) {
		bail_out(errno, "shm unlink failed");
	}
	post_sem(new_game);
	post_sem(player1);
	post_sem(player1);
	post_sem(player2);

	sem_close(server);
	sem_close(player1);
	sem_close(player2);
	sem_close(new_game);
	sem_unlink(SEM_1); 
	sem_unlink(SEM_2);
	sem_unlink(SEM_3);
	sem_unlink(SEM_4);
}

static void bail_out(int exitcode, const char *fmt, ...) {
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

	exit(exitcode);
}

static void setup_signal_handler(void) {
    /* setup signal handlers */
		fprintf(stdout, "setup signal handler\n");

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
}

static int is_hit(int player, int shot) {
		struct ship *enemy_ship;		


		if (player == PLAYER1) {
			enemy_ship = &player2_ship;
		} else {
			enemy_ship = &player1_ship;		
		}
	
		int hit;
		if (shot == enemy_ship->a || shot == enemy_ship->b || shot == enemy_ship->c) {
			hit = 1;		
		} else {
			hit = 0;		
		}
		fprintf(stdout, "Got shot %d ship at %d, %d, %d\n", shot, enemy_ship->a, enemy_ship->b, enemy_ship->c); 

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

	if (shared->state == STATE_GIVEN_UP) {
		game_finished = 1;
		fprintf(stdout, "Player %d has given up\n", player_nr);
		post_sem(other_player_sem);
	} else {
		int hit = is_hit(player_nr, shared->player_shot);
		fprintf(stdout, "Player %d attack was: %d\n", player_nr, hit);
		shared->was_hit = hit;

		if (has_won(player_nr, hit)) {
			game_finished = 1;
			shared->state = STATE_GAME_WON;
			fprintf(stdout, "Player %d won the game\n", player_nr);
			post_sem(player_sem);
			wait_sem(server);
			shared->state = STATE_GAME_LOST;
			post_sem(other_player_sem);
		} else {
			post_sem(player_sem);		
		}
	}
}

void signal_handler(int sig) {
	quit = 1;
}

/**
 * @brief Program entry point
 * @param argc The argument counter
 * @param argv The argument vector
 * @return EXIT_SUCCESS on success and an error code if a problem occured
 */
int main(int argc, char **argv) {
	// TODO get opts trotzdem verwenden?!
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

	while(!quit) {	
		game_finished = 0;
		shared->state = STATE_GAME;
		fprintf(stdout, "Setting up game field 0 waiting for players.\n");

		wait_sem(server);
		wait_sem(server);

		fprintf(stdout, "Got two players!\n");

		shared->player = PLAYER1;
		post_sem(player1);
		wait_sem(server);

		fprintf(stdout, "Player 1 ready.\n");	
		player1_ship = shared->player_ship;

		shared->player = PLAYER2;
		post_sem(player1);
		wait_sem(server);

		fprintf(stdout, "Player 2 ready.\n");
		player2_ship = shared->player_ship;
		fprintf(stdout, "Game startet.\n");

		post_sem(player1);	
		while(game_finished == 0) {
			calc_result(PLAYER1, player1, player2);
			if (game_finished == 1) {
				break;
			}
			calc_result(PLAYER2, player2, player1);
		}	

		wait_sem(server);
		fprintf(stdout, "Game finished.\n");
		post_sem(new_game);
		post_sem(new_game);
	}

	return EXIT_SUCCESS;
}
