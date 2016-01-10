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
/** location for shared memory shm_name */
#define SHM_BATTLESHIPS "/battleships_shm"

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

/** indicates a miss on the game field */
#define FIELD_MISS (1)

/** indicates a hit on the game field */
#define FIELD_HIT (2)

/** indicates if a postion has been fired on (during wait for server response) */
#define FIELD_SHOT_FIRED (3)

/** location of semaphore 1 */
#define SEM_SERVER "/battleships_server_sem"

/** location of semaphore 2 */
#define SEM_PLAYER1 "/battleships_player1_sem"

/** location of semaphore 3 */
#define SEM_PLAYER2 "/battleships_player2_sem"

/** location of semaphore 4 */
#define SEM_NEW_GAME "/battleships_new-game_sem"

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


