/**
 * @file battleships-client.c
 *
 * @author Thomas Muhm 1326486
 * 
 * @brief battleships client
 *
 * @details battleships client 
 *
 * @date 04.01.2016
 * 
 */
#include "battleships.h"

/* === Global Variables === */

static int field[15] = {0};

/** Name of the program */
static const char *progname = "battleship-server"; /* default name */

/* === Prototypes === */

/**
 *
 */
static void open_semaphores(void);

/**
 *
 */
static void open_shared_memory(void);

/**
 *
 */
static void print_border(void);



/* === Implementations === */

static void free_resources(void) {
	sem_close(server); // TODO error handling
	sem_close(player1); // TODO error handling
	sem_close(player2); // TODO error handling
	sem_close(new_game); // TODO error handling
	/* unmap shared memory */
	if (munmap(shared, sizeof *shared) == -1) {
		bail_out(errno, "munmap failed");
	}
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

static void open_semaphores(void) {
	server = sem_open(SEM_1, 0);
	if (server == SEM_FAILED) {
		bail_out(errno, "could not open semaphore s1");
	}	
	player1 = sem_open(SEM_2, 0);
	if (player1 == SEM_FAILED) {
		bail_out(errno, "could not open semaphore s2");
	}	
	player2 = sem_open(SEM_3, 0);
	if (player2 == SEM_FAILED) {
		bail_out(errno, "could not open semaphore s3");
	}	
	new_game = sem_open(SEM_4, 0);
	if (new_game == SEM_FAILED) {
		bail_out(errno, "could not open semaphore s4");
	}	
}

static void open_shared_memory(void) {
	/* create and/or open shared memory object */
	int shmfd = shm_open(SHM_NAME, O_RDWR, PERMISSION);
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

static void check_server_alive(void) {
	int shmfd = shm_open(SHM_NAME, O_RDWR, PERMISSION);
	if (shmfd == -1) {
		post_sem(new_game);
		bail_out(EXIT_FAILURE, "Server is down!");
	} else {
		if (close(shmfd) == -1) {
			bail_out(errno, "could not close shared memory file descriptor");
		}	
	}
}

static int is_in_field(int a) {
	return a >= 0 && a <= 15;
}

static int is_vertical(int a, int b, int c) {
	return (a == b+4 && b == c+4) || (a == b-4 && b == c-4);
}

static int is_horizontal(int a, int b, int c) {
	return (a == b+1 && b == c+1) || (a == b-1 && b == c-1);
}

static int is_diagonal(int a, int b, int c) {
	return (a == b+5 && b == c+5) || (a == b-5 && b == c-5) ||
				 (a == b+3 && b == c+3) || (a == b-3 && b == c-3);
}

static void input_ship(struct ship *ship) {
	int a = -1;
	int b = -1;
 	int c = -1;
	char buffer[100];

	fprintf(stdout, "\nPlace your ship horizontal, vertical or diagonal on the field (eg: 9, 6, 3): ");
	while (fgets(buffer, 100, stdin) != NULL) {
    if (sscanf(buffer, "%d, %d, %d", &a, &b, &c) == 3) {
			if (is_in_field(a) && is_in_field(b) && is_in_field(c)) {
				if (is_vertical(a,b,c) || is_horizontal(a,b,c) || is_diagonal(a,b,c)) {
					break;				
				}			
			}
		}
		fprintf(stdout, "Your input was invalid please try again: ");		
	}
	ship->a = a;
	ship->b = b;
	ship->c = c;
	fprintf(stdout, "Placed ship at: %d, %d, %d\n", a,b,c);
}

static int read_input(void) {
  int input;
	char buffer[100];
	while (fgets(buffer, 100, stdin) != NULL) {
    if (sscanf(buffer, "%d", &input) == 1) {
			if (is_in_field(input) || input == -1) {
				break;			
			} 
  		fprintf (stdout, "Input invalid, please enter a number between -1 and 15: ");
		}
	}
	return input;
}

static int get_valid_input(void) {
	int input;
	int valid = 0;
	fprintf(stdout, "\nEnter a number between 0 and 15 to fire on the enemy ship\n");
	fprintf(stdout, "Enter the number '-1' to give up\n\n");
	fprintf (stdout, "Please enter a Number between -1 and 15: ");
	while (valid == 0) {
		input = read_input();
		if (input < -1 || input > 15) {
	  	fprintf (stdout, "Input invalid, please enter a number between -1 and 15: ");
			continue;
		}		
		if (field[input] == FIELD_HIT || field[input] == FIELD_MISS) {
			fprintf(stdout, "You already fired on this field, please enter a different number: ");
			continue;
		} else {
			valid = 1;
			field[input] = FIELD_SHOT_FIRED;		
		}
	} 
	return input;
}

static void print_border(void) {
	for (int i = 0; i <= 30; ++i) {
		fprintf(stdout, "_");
	}
}

static void print_map(void) {
	print_border();
	fprintf(stdout, "\n\n");
	for (int i = 0; i <= sizeof(field)/sizeof(field[0]); ++i) {
		char print_state[10];
		int state = field[i];

		if (state == FIELD_HIT) {
			strcpy(print_state, "BOOM");
		} else if (state == FIELD_MISS) {
			strcpy(print_state, "MISS");
		} else {
			sprintf(print_state, "%d", i);		
		}
		fprintf(stdout, "%s\t", print_state);

		if (((i+1) % 4) == 0) {
			fprintf(stdout, "\n");
			if (i != 15) {
				fprintf(stdout, "\n");			
			}		
		}
	} 
	print_border();
	fprintf(stdout, "\n");
	return;	
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

	open_semaphores();
	open_shared_memory();

	fprintf(stdout, "Waiting for game slot.\n");
	wait_sem(new_game);
	check_server_alive();
	fprintf(stdout, "Got game slot - Waiting for other player.\n");
	
	post_sem(server);
	int game_finished = 0;	
	wait_sem(player1);
	check_server_alive();

	int playerid = shared->player;
	sem_t *player, *other_player;
	
	struct ship *myship = &shared->player_ship;
	
	fprintf(stdout, "Game started!\n");
	print_map();

	input_ship(myship);	

	if (playerid == PLAYER1) {
		player = player1;
		other_player = player2;
	} else {
		player = player2;	
		other_player = player1;
	}

	post_sem(server);	
	fprintf(stdout, "\nWaiting for other player...\n");
	wait_sem(player);
	check_server_alive();

	while (game_finished == 0) {
		switch (shared->state) {
			case STATE_GIVEN_UP:
				fprintf(stdout, "The other player has given up - You Win!\n");
				game_finished = 1;
				break;
			case STATE_GAME_LOST:
				fprintf(stdout, "\nYou lost the Game.\n");
				game_finished = 1;
				break;
			default:
				;
				int user_input = get_valid_input();
				shared->player_shot = user_input;
				if (user_input == -1) {
					fprintf(stdout, "Giving up...\n");
					shared->state = STATE_GIVEN_UP;		
					game_finished = 1;
					break;
				}

				post_sem(server);
				wait_sem(player);
				check_server_alive();

				if (shared->was_hit == 1) {
					field[user_input] = FIELD_HIT;			
				} else {
					field[user_input] = FIELD_MISS;
				}
				print_map();
				if (field[user_input] == FIELD_HIT) {
					fprintf(stdout, "\nYour shot was a HIT, nice job!\n");
				} else {
					fprintf(stdout, "\nYou missed, better luck next time!\n");
				}

				if (shared->state == STATE_GAME_WON) {
					fprintf(stdout, "\nYou won the Game :)\n");				
					game_finished = 1;
				} else {
					fprintf(stdout, "\nWaiting for other player...\n");
					post_sem(other_player);
					wait_sem(player);
					check_server_alive();
				}
				break;				
		}		
	}
	fprintf(stdout, "\nClient finished.\n");
	post_sem(server);
	return EXIT_SUCCESS;
}
