
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

#define SEM_PAYERS "/battleships_sem_players"
#define SEM_2 "/sem_2"

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

struct myshm *shared;
sem_t *sem_players;
sem_t *s2;

void semWait(sem_t *sem) {
	if (sem_wait(sem) == -1) {
		bail_out(errno, "sem_wait failed");
	}
}

void semPost(sem_t *sem) {
	if (sem_post(sem) == -1) {
		bail_out(errno, "sem_post failed");
	}
}


struct myshm {
	unsigned int state;
	unsigned int data[MAX_DATA];
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

struct myshare {
	struct myhangman hangmen[MAX_CLIENTS];
};

// TODO ALL REFACTOR!!
