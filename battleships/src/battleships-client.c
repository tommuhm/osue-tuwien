/**
 * @file battleships-client.c
 *
 * @author Thomas Muhm 1326486
 * 
 * @brief battleships client which connects to a game server 
 *
 * @details battleships client which communications via semaphores and shared memory with a server and a other client
 *
 * @date 04.01.2016
 * 
 */
#include "battleships.h"

/* === Global Variables === */

/** 4x4 gamefield, the values indicate a NOT_FIRED, HIT or MISS */ 
static int field[15] = {0};

static int game_running = 0;

/** Name of the program */
static const char *progname = "battleships-client"; /* default name */

/* === Prototypes === */

/**	open_semaphores
 *	@brief tires to open the semaphores which were created by the server
 */
static void open_semaphores(void);

/**	open_shared_memory
 *	@brief tires to open the shared memory which was created by the server
 */
static void open_shared_memory(void);

/**	is_in_field
 *	@brief check if the given value is in the 4x4 gamefield
 *  @return true if in field false otherwise
 */
static int is_in_field(int a);

/**	is_vertical
 *	@brief check if the 3 given points are vertical on a 4x4 field
 *  @return true if vertical false otherwise
 */
static int is_vertical(int a, int b, int c);

/**	is_horizontal
 *	@brief check if the 3 given points are horizontal on a 4x4 field
 *  @return true if horizontal false otherwise
 */
static int is_horizontal(int a, int b, int c);

/**	is_diagonal
 *	@brief check if the 3 given points are diagnoal on a 4x4 field
 *  @return true if diagnoal false otherwise
 */
static int is_diagonal(int a, int b, int c);

/**	input_ship
 *	@brief wait for user input for a ship on the 4x4 field and validate the input
 *  return a valid ship in the game field
 */
static struct ship input_ship();

/**	read_input
 *	@brief read a number from stdin
 *	@return a valid number entered by the user
 */
static int read_input(void);

/**	get_valid_input
 *	@brief get a number from the user and validate if it is in the game field and not already fired
 * 	@return a valid number in the game field which was not already fired
 */
static int get_valid_input(void);

/**	print_map
 *	@brief print the game map with all hits and misses
 */
static void print_map(void);

/**	wait_for_turn
 *	@brief wait for the turn of the specified player semaphore and lock memory if it begins
 */
static void wait_for_turn(sem_t *player);

/**	turn_finished
 *	@brief free memory and send post to the next semaphore
 */
static void turn_finished(sem_t *next);

// TODO
static void set_disconnected(void);

/* === Implementations === */

void free_resources(void) {
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
	/* unmap shared memory */
	if (munmap(shared, sizeof *shared) == -1) {
		print_error(errno, "munmap failed");
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
		if (errno == EINTR) {
			//set_disconnected();
		} else {
			bail_out(errno, "sem_wait failed");
		}
	}
}

void post_sem(sem_t *sem) {
	if (sem_post(sem) == -1) {
		bail_out(errno, "sem_post failed");
	}
}

static void wait_for_turn(sem_t *player) {
	wait_sem(player);
	if (!quit) {
		wait_sem(memory);
	}
}

static void turn_finished(sem_t *next) {
	post_sem(memory);
	post_sem(next);
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
	memory = sem_open(SEM_5, 0);
	if (memory == SEM_FAILED) {
		bail_out(errno, "could not open semaphore s5");
	}	
}

static void open_shared_memory(void) {
	/* create and/or open shared memory object */
	int shmfd = shm_open(SHM_NAME, O_RDWR, PERMISSION);
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

static struct ship input_ship() {
	struct ship ship;
	int a = -1;
	int b = -1;
 	int c = -1;
	char buffer[100];

	(void) fprintf(stdout, "\nPlace your ship horizontal, vertical or diagonal on the field (eg: 9, 6, 3): ");
	while (fgets(buffer, 100, stdin) != NULL) {
    if (sscanf(buffer, "%d, %d, %d", &a, &b, &c) == 3) {
			if (is_in_field(a) && is_in_field(b) && is_in_field(c)) {
				if (is_vertical(a,b,c) || is_horizontal(a,b,c) || is_diagonal(a,b,c)) {
					break;				
				}			
			}
		}
		(void) fprintf(stdout, "Your input was invalid please try again: ");		
	}
	ship.a = a;
	ship.b = b;
	ship.c = c;
	(void) fprintf(stdout, "Placed ship at: %d, %d, %d\n", a,b,c);
	return ship;
}

static int read_input(void) {
  int input;
	char buffer[100];
	while (fgets(buffer, 100, stdin) != NULL) {
    if (sscanf(buffer, "%d", &input) == 1) {
			if (is_in_field(input) || input == -1) {
				break;			
			} 
  		(void) fprintf (stdout, "Input invalid, please enter a number between -1 and 15: ");
		}
	}
	return input;
}

static int get_valid_input(void) {
	int input;
	int valid = 0;
	(void) fprintf(stdout, "\nEnter a number between 0 and 15 to fire on the enemy ship\n");
	(void) fprintf(stdout, "Enter the number '-1' to give up\n\n");
	(void) fprintf (stdout, "Please enter a Number between -1 and 15: ");
	while (valid == 0) {
		input = read_input();
		if (input < -1 || input > 15) {
	  	(void) fprintf (stdout, "Input invalid, please enter a number between -1 and 15: ");
			continue;
		}		
		if (field[input] == FIELD_HIT || field[input] == FIELD_MISS) {
			(void) fprintf(stdout, "You already fired on this field, please enter a different number: ");
			continue;
		} else {
			valid = 1;
			field[input] = FIELD_SHOT_FIRED;		
		}
	} 
	return input;
}

static void print_map(void) {
	for (int i = 0; i <= 30; ++i) {
		(void) fprintf(stdout, "_");
	}
	(void) fprintf(stdout, "\n\n");
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
		(void) fprintf(stdout, "%s\t", print_state);

		if (((i+1) % 4) == 0) {
			(void) fprintf(stdout, "\n");
			if (i != 15) {
				(void) fprintf(stdout, "\n");			
			}		
		}
	} 
	for (int i = 0; i <= 30; ++i) {
		(void) fprintf(stdout, "_");
	}
	(void) fprintf(stdout, "\n");
	return;	
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
	open_semaphores();
	open_shared_memory();

	(void) fprintf(stdout, "Waiting for game slot.\n");
	wait_sem(new_game);
	check_server_alive();
	game_running = 1;	
	(void) fprintf(stdout, "Got game slot - Waiting for other player.\n");
	
	struct ship myship = input_ship(); // TODO evt andere postion

	post_sem(server);
	wait_sem(player2);
	check_server_alive();

	if (quit) {
		return EXIT_SUCCESS;	
	}
	if (shared->state == STATE_CLIENT_DISCONNECTED) {
		game_running = 0;	
		(void) fprintf(stdout, "The other player has been disconnected - You Win!\n");
	}

	int playerid = shared->player;
	sem_t *player, *other_player;
	shared->player_ship = myship;
	
	(void) fprintf(stdout, "Game started!\n");
	print_map();

	if (playerid == PLAYER1) {
		player = player1;
		other_player = player2;
	} else {
		player = player2;	
		other_player = player1;
	}

	post_sem(server);
	(void) fprintf(stdout, "\nWaiting for other player...\n");
	wait_sem(server);

	while (game_running == 1) {
		//check_other_disconnected();
		check_server_alive();
		int state = shared->state;

		switch (state) {
			case STATE_CLIENT_DISCONNECTED:
				(void) fprintf(stdout, "The other player has been disconnected - You Win!\n");
				game_running = 0;				
				break;
			case STATE_GIVEN_UP:
				(void) fprintf(stdout, "The other player has given up - You Win!\n");
				game_running = 0;
				break;
			case STATE_GAME_LOST:
				(void) fprintf(stdout, "\nYou lost the Game.\n");
				game_running = 0;
				break;
			default: ;
				int user_input = get_valid_input();
				
				if (user_input == -1) {
					(void) fprintf(stdout, "Giving up...\n");
					shared->state = STATE_GIVEN_UP;		
					game_running = 0;
					break;
				}
				shared->player_shot = user_input;

				post_sem(server);
				wait_sem(player);
				check_server_alive();

				int was_hit = shared->was_hit;
				state = shared->state;

				if (was_hit == 1) {
					field[user_input] = FIELD_HIT;			
				} else {
					field[user_input] = FIELD_MISS;
				}
				print_map();
				if (field[user_input] == FIELD_HIT) {
					(void) fprintf(stdout, "\nYour shot was a HIT, nice job!\n");
				} else {
					(void) fprintf(stdout, "\nYou missed, better luck next time!\n");
				}

				if (state == STATE_GAME_WON) {
					(void) fprintf(stdout, "\nYou won the Game :)\n");				
					game_running = 0;
				} else {
					(void) fprintf(stdout, "\nWaiting for other player...\n");
					post_sem(other_player);
					wait_sem(player);
				}
				break;				
		}		
	}
	(void) fprintf(stdout, "\nClient finished.\n");
	post_sem(server);
	return EXIT_SUCCESS;
}
