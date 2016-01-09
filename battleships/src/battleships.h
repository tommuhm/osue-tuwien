#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include <semaphore.h>
#include <fcntl.h>

#include <sys/mman.h>
#include <unistd.h>

#define SHM_NAME "/myshm"
#define MAX_DATA (50)
#define PERMISSION (0600)

#define PLAYER1 (1)
#define PLAYER2 (2)

#define STATE_GAME (1)
#define STATE_GIVEN_UP (4)
#define STATE_GAME_WON (5)
#define STATE_GAME_LOST (6)

#define FIELD_MISS (1)
#define FIELD_HIT (2)
#define FIELD_SHOT_FIRED (3)

#define SEM_1 "/sem_1"
#define SEM_2 "/sem_2"
#define SEM_3 "/sem_3"
#define SEM_4 "/sem_4"

/* === Macros === */

/**
 * @brief print debug messages if the debug flag is set 
 */
#ifdef ENDEBUG
#define DEBUG(...) do { fprintf(stderr, __VA_ARGS__); } while(0)
#else
#define DEBUG(...)
#endif

/* Length of an array */
#define COUNT_OF(x) (sizeof(x)/sizeof(x[0]))

/* === Prototypes === */

/** free_resources
 * @brief free allocated resources
 */
static void free_resources(void);

/** program exit point for errors
 * @brief free resources and terminate program on program error
 * @param exitcode exit code
 * @param fmt format string
 */
static void bail_out(int exitcode, const char *fmt, ...);

struct battleships *shared;

sem_t *server;
sem_t *player1;
sem_t *player2;
sem_t *new_game;

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

struct ship {
	unsigned int a;
	unsigned int b;
	unsigned int c;
};

struct battleships {
	unsigned int player;
	unsigned int state;
	unsigned int round;
	unsigned int player_shot;
	unsigned int was_hit;
	struct ship player_ship;
};
