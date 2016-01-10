/**
 * @file battleships.h
 *
 * @author Thomas Muhm 1326486
 * 
 * @brief this file includes shared structures and variables for client and server
 *
 * @details this file includes all semaphore variables, shared memory variables and method prototypes which are implemented on the server and the client.
 *
 * @date 04.01.2016
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include <semaphore.h>
#include <fcntl.h>

#include <sys/mman.h>
#include <unistd.h>

/** location for shared memory shm_name */
#define SHM_NAME "/myshm"

/** permission for semaphores and shared memory */
#define PERMISSION (0600)

/** number of player 1 */
#define PLAYER1 (1)

/** number of player 2 */
#define PLAYER2 (2)

/** state for a new game and running game */
#define STATE_GAME (1)

/** state when a user has given up */
#define STATE_GIVEN_UP (2)

/** state for the client when the game has been won */
#define STATE_GAME_WON (3)

/** state for the client when the game has been lost */
#define STATE_GAME_LOST (4)

/** state when a client has been disconnected */
#define STATE_CLIENT_DISCONNECTED (5)

/** indicates a miss on the game field */
#define FIELD_MISS (1)

/** indicates a hit on the game field */
#define FIELD_HIT (2)

/** indicates if a postion has been fired on (during wait for server response) */
#define FIELD_SHOT_FIRED (3)

/** location of semaphore 1 */
#define SEM_1 "/sem_1"

/** location of semaphore 2 */
#define SEM_2 "/sem_2"

/** location of semaphore 3 */
#define SEM_3 "/sem_3"

/** location of semaphore 4 */
#define SEM_4 "/sem_4"

/** location of semaphore 5 */
#define SEM_5 "/sem_5"

/* === Macros === */

/**
 * @brief print debug messages if the debug flag is set 
 */
#ifdef ENDEBUG
#define DEBUG(...) do { fprintf(stderr, __VA_ARGS__); } while(0)
#else
#define DEBUG(...)
#endif

/** Length of an array */
#define COUNT_OF(x) (sizeof(x)/sizeof(x[0]))

/* === Prototypes === */

/** free_resources
 * @brief free allocated resources
 */
static void free_resources(void);

/** print_error
 *  @brief print error message
 *  @param exitcode exit code
 *  @param fmt format string
 */
static void print_error(int exitcode, const char *fmt, ...);

/** bail_out
 *  @brief print error message and terminate program
 *  @param exitcode exit code
 *  @param fmt format string
 */
static void bail_out(int exitcode, const char *fmt, ...);

/** signal_handler
 *  @brief Signal handler
 *  @param sig Signal number catched
 */
static void signal_handler(int sig);

/** setup_signal_handler
 *  @brief setup the signal handler for SIGINT and SIGTERM signals
 */
static void setup_signal_handler(void);

/**	post_sem
 *	@brief post to semaphore and check for error
 *	@param sem to post to
 */
static void post_sem(sem_t *sem);

/**	wait_sem
 *	@brief wait for semaphore and check for error
 *	@param sem to wait for
 */
static void wait_sem(sem_t *sem);

/* === Global Variables === */

/** this flag indicates if a SIGINT or a SIGTERM was caught */
static volatile sig_atomic_t quit = 0;

/** battlefield structure in the shared memory - lock before use with memory semaphore! */
struct battleships *shared;

/** semphore for server turn */
sem_t *server;
/** semaphore for player1 turn */
sem_t *player1;

/** semaphore for player2 turn */
sem_t *player2;

/** semaphore for available game slots - 2 slots on init */
sem_t *new_game;

/** semaphore for memory lock - lock for the shortest possible time! - because the shared memory is also used for shutdown signals! */
sem_t *memory;

/* === Type Definitions === */

/** structure for a ship */
struct ship {
	/** postion of the first element of the ship */
	unsigned int a;
	/** postion of the second element of the ship */
	unsigned int b;
	/** postion of the thirds element of the ship */
	unsigned int c;
};

/** structure for the shared memory */
struct battleships {
	/** player_nr used in the initalised phase to assign semaphores - set by the server */
	unsigned int player;
	/** current state of the game - set by the server */
	unsigned int state;
	/** postion of the last player shot - set by the client */
	unsigned int player_shot;
	/** indicate if the last shot was a hit - set by the server */
	unsigned int was_hit;
	/** player ship created by the user during startup - set by the client */
	struct ship player_ship;
};
