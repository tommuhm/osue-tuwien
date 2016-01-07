
#define MAX_CLIENTS 64
#define MAX_WORDSIZE 128
#define MAX_LISTSIZE 128
#define MAX_TRIES 9
#define ALPHABET 26

#define STATE_ALIVE (1)
#define STATE_OPEN (0)
#define STATE_DEAD (-1)

#define GAME_WIN (1)
#define GAME_NONE (0)
#define GAME_LOST (-1)

#define SHM_NAME "/myshm"
#define MAX_DATA (50)
#define PERMISSION (0600)


#define PLAYER1 (1)
#define PLAYER2 (2)

#define STATE_INIT (1)
#define STATE_PLAYING (2)


#define SEM_1 "/sem_1"
#define SEM_2 "/sem_2"
#define SEM_3 "/sem_3"
#define SEM_4 "/sem_4"
#define SEM_5 "/sem_5"
#define SEM_6 "/sem_6"
#define SEM_7 "/sem_7"

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

sem_t *new_game;
sem_t *player_ready;
sem_t *client_round;
sem_t *server_round;
sem_t *server_response;

sem_t *player1;
sem_t *player2;


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


struct battleships {
	unsigned int player;
	unsigned int state;
	unsigned int round;
	unsigned int shoot;
	struct ship *playership;
};

struct ship {
	unsigned int a;
	unsigned int b;
	unsigned int c;
};

struct myhangman {
	unsigned int state;
	unsigned int tries;
	unsigned int tip;
	char tips[ALPHABET];
	char word[MAX_WORDSIZE];
	int wordnr;
	int usedWords[MAX_LISTSIZE];
	int winState;
	
	int wins;
	int losses;
	int end;
};

// TODO ALL REFACTOR!!
