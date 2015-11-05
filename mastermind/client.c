/** 
 *  mastermind: client
 *
 *  @author Thomas Muhm 1326486
 *
 *  @brief starts a mastermind client which connects to the specified server and tries to solve the secret
 *
 *  @details 
 *    client connects to the specified masermind server 
 *    uses the first steps from the knuth alorithem to solve the secret most of the time in under 8 steps
 *
 *  @date 17.10.2015
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <assert.h>

/* === Constants === */

#define SLOTS (5)
#define COLORS (8)
#define READ_BYTES (1)  
#define WRITE_BYTES (2)
#define BUFFER_BYTES (2)
#define SHIFT_WIDTH (3)
#define PARITY_BIT (15)

#define PARITY_ERR_BIT (6)
#define GAME_LOST_ERR_BIT (7)

#define EXIT_PARITY_ERROR (2)
#define EXIT_GAME_LOST (3)
#define EXIT_MULTIPLE_ERRORS (4)

/* === Macros === */

#ifdef ENDEBUG
#define DEBUG(...) do { fprintf(stderr, __VA_ARGS__); } while(0)
#else
#define DEBUG(...)
#endif

/* Length of an array */
#define COUNT_OF(x) (sizeof(x)/sizeof(x[0]))

/* === Global Variables === */

/* Name of the program */
static const char *progname = "client";

/* File descriptor for the socket */
static int sockfd = -1;

static struct addrinfo *ai;

/* This variable is set upon receipt of a signal */
volatile sig_atomic_t quit = 0;

enum { beige = 0, darkblue, green, orange, red, black, violet, white };

/* === Type Definitions === */
struct opts {
  char *hostname;
  long int port;
};

/* === Prototypes === */

/**
 * @brief Parse command line options
 * @param argc The argument counter
 * @param argv The argument vector
 * @param options Struct where parsed arguments are stored
 */
static void parse_args(int argc, char **argv, struct opts *options);

/**
 * read_from_server
 * @brief Read message from socket
 *
 * This code *illustrates* one way to deal with partial reads
 *
 * @param sockfd Socket to read from
 * @param buffer Buffer where read data is stored
 * @param n Size to read
 * @return Pointer to buffer on success, else NULL
 */
static uint8_t *read_from_server(int sockfd, uint8_t *buffer, size_t n);

/**
 * write_to_server
 * @brief Write message to socket
 *
 * This code *illustrates* one way to deal with partial writes
 *
 * @param sockfd Socket to write to
 * @param buffer Buffer for data to be sent
 * @param n Size to write
 * @return 0 on success and -1 on error
 */
static int write_to_server(int sockfd, uint8_t *buffer, size_t n);

/**
 * knuth_remove_sol
 * @brief knuth alogrithm - remove impossible solutions
 * @param guess       The last guess
 * @param solutions   Solution array - each index represents a solution
 * @param red_guess   The number of red pins for the last guess
 * @param white_guess The number of red pins for the last guess
 */
static void knuth_remove_sol(uint16_t guess, int *solutions, int n, int red_guess, int white_guess);

/**
 * bail_out
 * @brief terminate program on program error
 * @param exitcode exit code
 * @param fmt format string
 */
static void bail_out(int exitcode, const char *fmt, ...);

/**
 * signal_handler
 * @brief Signal handler
 * @param sig Signal number catched
 */
static void signal_handler(int sig);

/**
 * free_resources
 * @brief free allocated resources
 */
static void free_resources(void);

/**
 * main
 * @brief Program entry point
 * @param argc The argument counter
 * @param argv The argument vector
 * @return EXIT_SUCCESS on success, EXIT_PARITY_ERROR in case of an parity
 * error, EXIT_GAME_LOST in case client needed to many guesses,
 * EXIT_MULTIPLE_ERRORS in case multiple errors occured in one round,
 * EXIT_FAILURE if socket creation failed
 */
int main(int argc, char *argv[]) {

  struct opts options;
  int err;
  
  parse_args(argc, argv, &options);

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

  /* find address infos for speciied hostname and port */
  struct addrinfo hints, *ai_sel;
  memset(&hints, 0, sizeof hints);  

  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM; 
  hints.ai_protocol = IPPROTO_TCP;

  if ((err = getaddrinfo(options.hostname, argv[2], &hints, &ai)) != 0) {
    bail_out(EXIT_FAILURE, "getaddrinfo error: %s", gai_strerror(err));
  }

  if (ai == NULL) {
    bail_out(EXIT_FAILURE, "Could not resolve host %s.", options.hostname);
  }
  ai_sel = ai;
  
  /* create socket and set options */
  if ((sockfd = socket(ai_sel->ai_family, ai_sel->ai_socktype, ai_sel->ai_protocol)) < 0) {
    bail_out(EXIT_FAILURE, "Socket creation failed.", options.hostname);
  }

  /* connect socket to server */
  if (connect(sockfd, ai_sel->ai_addr, ai_sel->ai_addrlen) < 0) {
    bail_out(EXIT_FAILURE, "Could not connect to server");
  }
  
  /* init solution array, this index is the combination, 
   * every index of the array is a combination
   * the content of the array:
   *    0 = possible solution
   *    1 = solution removed
   */
  int solutions_max = pow(COLORS,SLOTS);
  int solutions[solutions_max];
  memset(solutions, 0, sizeof(int) * solutions_max);

  /* starting guess */
  uint16_t guess = (beige << 4*SHIFT_WIDTH) | (orange << 3*SHIFT_WIDTH) | (darkblue << 2*SHIFT_WIDTH) | (red << SHIFT_WIDTH) | green;
  
  int ret = EXIT_SUCCESS;
  int rounds = 0;
  while (true) {
    rounds++;
    /* removee current guess from solutions */
    solutions[guess] = 1;

    /* calculate parity bit for guess */
    uint8_t parity_bit = 0;
    for (int i = 0; i < SLOTS * SHIFT_WIDTH; i++) {
      parity_bit ^= (guess >> i) & 1;
    }
    guess |= (parity_bit << PARITY_BIT);
    
    /* transform int-guess to 1 byte buffer */
    uint8_t buffer[BUFFER_BYTES];
    memset(&buffer, 0, sizeof buffer);  

    for (int i = 0; i < 2; i++) {
      buffer[i] = (guess >> i*8);
    }
 
    /* send guess to server */
    if (write_to_server(sockfd, buffer, WRITE_BYTES) != 0) {
      if (quit) break; /* caught signal */
      bail_out(EXIT_FAILURE, "write_to_server");
    } 
    
    /* receive response from server */
    if (read_from_server(sockfd, buffer, READ_BYTES) == NULL) {
      if (quit) break; /* caught signal */
      bail_out(EXIT_FAILURE, "read_from_server");
    }
    
    /* extract informations from server response and check status */
    uint8_t red = (buffer[0] & 7);
    uint8_t white = (buffer[0] >> 3) & 7;
    uint8_t parity_check = (buffer[0] >> PARITY_ERR_BIT) & 1;
    uint8_t game_lost_check = (buffer[0] >> GAME_LOST_ERR_BIT) & 1; 

    DEBUG("Status - red: %i, white: %i, parity check: %i, end: %i\n", red, white, parity_check, status);
    if (red == SLOTS) {
      (void) fprintf(stdout, "Runden %d\n", rounds);
      break;
    }
    if (game_lost_check == 1) {
      (void) fprintf(stdout, "Game lost\n");
      ret = EXIT_GAME_LOST;
      break;
    }
    if (parity_check == 1) {
      (void) fprintf(stdout, "Parity error\n");
      if (ret == EXIT_GAME_LOST) {
        ret = EXIT_MULTIPLE_ERRORS;
      } else {
        ret = EXIT_PARITY_ERROR;
      }
      break;
    }
    
    // knuth alg - remove non possible solutions
    knuth_remove_sol(guess, solutions, solutions_max, red, white);
        
    // pick next guess
    int active = 0;
    for (int i = solutions_max-1; i >= 0; --i) {
      if (solutions[i] == 0) {
        active++;
        guess = i;
        break;
      }
    }
  }
  
  free_resources();
  return ret;
}

static void knuth_remove_sol(uint16_t guess, int *solutions, int n, int red_guess, int white_guess) {
  int red, white;
  int colors_left[COLORS];
  int k = 0;

  for (int sol = 0; sol < n; sol++) {
    red = white = 0;
    (void) memset(&colors_left[0], 0, sizeof(colors_left));    
    
    if (solutions[sol] == 0) {
      // calc red colors
      for (int j = 0; j < SLOTS; j++) {
        int guess_color = (guess >> (j * SHIFT_WIDTH)) & 7;
        int sol_color = (sol >> (j * SHIFT_WIDTH)) & 7;
        if (guess_color == sol_color) {
          red++;
        } else {
          colors_left[guess_color]++;
        }
      }

      // calc white colors
      for (int j = 0; j < SLOTS; j++) {
        int guess_color = (guess >> (j * SHIFT_WIDTH)) & 7;
        int sol_color = (sol >> (j * SHIFT_WIDTH)) & 7;
        if (guess_color != sol_color) {
           if (colors_left[sol_color] > 0) {
            white++;
            colors_left[sol_color]--;
          }
        }
      }
      
      // delete solution if it has a different score than the guess
      if (red != red_guess || white != white_guess) {
        solutions[sol] = 1;
        k++;      
      }
    }   
    
  }
}

static int write_to_server(int fd, uint8_t *buffer, size_t n) {
  size_t bytes_sent = 0;
  do {
    ssize_t s = send(fd, buffer + bytes_sent, n - bytes_sent, 0);
    if (s <= 0) {
      return -1;
    }
    bytes_sent += s;
  } while (bytes_sent < n);

  if (bytes_sent < n ) {
    return -1;
  }
  return 0;
}

static uint8_t *read_from_server(int fd, uint8_t *buffer, size_t n) {
  size_t bytes_recv = 0;
  do {
    ssize_t r;
    r = recv(fd, buffer + bytes_recv, n - bytes_recv, 0);
    if (r <= 0) {
      return NULL;
    }
    bytes_recv += r;
  } while (bytes_recv < n);
  
  if (bytes_recv < n) {
    return NULL;
  }
  return buffer;
}

static void parse_args(int argc, char **argv, struct opts *options) {
  char *port_arg;
  char *endptr;
 
  if (argc > 0) {
    progname = argv[0];
  }
  
  if (argc != 3) {
    bail_out(EXIT_FAILURE, "Usage %s <server-hostname> <server-port>\n", progname); 
  }
  
  options->hostname = argv[1];
  port_arg = argv[2];
 
  errno = 0; 
  options->port = strtol(port_arg, &endptr, 10);
  if ((errno == ERANGE && 
      (options->port == LONG_MAX || options->port == LONG_MIN)) 
      || (errno != 0 && options->port == 0)) {
    (void) fprintf(stderr, "strtol error\n");
  }
  
  if (endptr == port_arg) {
    (void) fprintf(stderr, "No digits were found\n");
  }
  
  if (options->port < 1 || options->port > 65535) {
    (void) fprintf(stderr, "Usa a valid TCP/IP port range (1-65535)\n");
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
  if (errno != 0) {
      (void) fprintf(stderr, ": %s", strerror(errno));
  }
  (void) fprintf(stderr, "\n");

  free_resources();
  exit(exitcode);
}

static void signal_handler(int sig) {
  quit = 1;
}

static void free_resources(void) {
  /* clean up resources */
  DEBUG("Shutting down client\n");
  if(sockfd >= 0) {
    (void) close(sockfd);
  }
  if (ai != NULL) {
    freeaddrinfo(ai);
  }
}
