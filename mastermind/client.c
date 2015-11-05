/** 
 *  mastermind-client
 *
 *  @author Thomas Muhm
 *
 *  @brief starts a mastermind client which connects to the specified server and tries to solve the secret
 *
 *  @details 
 *    client connects to the specified masermind server 
 *    uses the first steps from the knuth alorithem to solve the secret most of the time in under 8 steps
 *
 *  @date 17.10.2015
 *
 *
 * Copyright (c) 2012-2015 OSUE Team <osue-team@vmars.tuwien.ac.at>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * gcc -std=c99 -Wall -g -pedantic -DENDEBUG \
 *      -D_BSD_SOURCE -D_XOPEN_SOURCE=500 -o server server.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>

#define SLOTS (5)
#define COLORS (8)
#define READ_BYTES (1)  
#define WRITE_BYTES (2)
#define BUFFER_BYTES (2)
#define SHIFT_WIDTH (3)
#define PARITY_BIT (15)

// #define ENDEBUG

/* === Macros === */

#ifdef ENDEBUG
#define DEBUG(...) do { fprintf(stderr, __VA_ARGS__); } while(0)
#else
#define DEBUG(...)
#endif

enum { beige, darkblue, green, orange, red, black, violet, white };

static const char *progname = "client";

static int sockfd = -1;

struct opts {
  char *hostname;
  long int port;
};

static uint16_t generate_guess(void);

static void usage(void);

static void parse_args(int argc, char **argv, struct opts *options);

static void bail_out(int exitcode, const char *fmt, ...);

static uint8_t *read_from_server(int sockfd, uint8_t *buffer, size_t n);

static int write_to_server(int sockfd, uint8_t *buffer, size_t n);

static void print_binary(int guess, int slots);

static void print_buffer(uint8_t *buffer, size_t n);

static void int_to_code(char *code, int guess, int slots);

static void calc_score(uint16_t guess, int *solutions, int n, int red_guess, int white_guess);

static void free_resources(void);

/**
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

  struct addrinfo hints, *ai, *ai_sel;
  int fd, res;
    
  hints.ai_flags = 0;
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_addrlen = 0;
  hints.ai_addr = NULL;
  hints.ai_canonname = NULL;
  hints.ai_next;

  if ((err = getaddrinfo(options.hostname, argv[2], &hints, &ai)) != 0) {
    (void) fprintf(stderr, "ERROR: %s\n", gai_strerror(err));
    bail_out(EXIT_FAILURE, "SHIIIIIT");
  }

  if (ai == NULL) {
    (void) fprintf(stderr, "Could not resolve host %s.\n", options.hostname);
    bail_out(EXIT_FAILURE, "SHIIIIIT");
  }
  
  // TODO test each address?
  ai_sel = ai;
  
  // create socket
  if ((sockfd = socket(ai_sel->ai_family, ai_sel->ai_socktype, ai->ai_protocol)) < 0) {
    (void) fprintf(stderr, "Socket creation failed\n");
    return -1;
  }
  
  if (connect(sockfd, ai_sel->ai_addr, ai_sel->ai_addrlen) < 0) {
    (void) close(sockfd);
    freeaddrinfo(ai);
    (void) fprintf(stderr, "Connection failed.\n");
    bail_out(EXIT_FAILURE, "SHIIIIIT");
  }
  
  // TODO bytes von rechts nach links übertragen
  //uint8_t send_buffer = request;
  
  //for (int i = 15; i >= 0; i--) {
  //  fprintf("%i", (send_buffer >> i) ^ 1);
  //}
  
  
  //if (send(sockfd, buffer, WRITE_BYTES, 0) < 0) {    
  //  bail_out(EXIT_FAILURE, "write to server");
  //}
  

  int solutions_max = pow(COLORS,SLOTS);
  int solutions[solutions_max];
  memset(solutions, 0, sizeof(int) * solutions_max);
//  uint16_t guess = (beige << 4*SHIFT_WIDTH) | (beige << 3*SHIFT_WIDTH) | (darkblue << 2*SHIFT_WIDTH) | (darkblue << SHIFT_WIDTH) | green;

  uint16_t guess = (beige << 4*SHIFT_WIDTH) | (beige << 3*SHIFT_WIDTH) | (darkblue << 2*SHIFT_WIDTH) | (red << SHIFT_WIDTH) | green;
  

  
  int end = 0;
  int rounds = 1;
  while (end != 1) {

    // alg knuth alg 
    solutions[guess] = 1;
    uint8_t parity_bit = 0;
    for (int i = 0; i < SLOTS * SHIFT_WIDTH; i++) {
      parity_bit ^= (guess >> i) & 1;
    }
    guess |= (parity_bit << PARITY_BIT);
    
    //uint16_t request = generate_guess();
    uint8_t buffer[BUFFER_BYTES];
    for (int i = 0; i < 2; i++) {
      buffer[i] = (guess >> i*8);
    }
 
    // alg end 
  
  
    (void) fprintf(stdout, "Calc guess: ");
//    (void) print_buffer(buffer, WRITE_BYTES);
    
    char code[SLOTS];
    int_to_code(code, guess, SLOTS);
    (void) fprintf(stdout, "%s\n", code); 
  
    if (write_to_server(sockfd, buffer, WRITE_BYTES) != 0) {
      bail_out(EXIT_FAILURE, "write_to_server");
    } 
    
    if (read_from_server(sockfd, buffer, READ_BYTES) == NULL) {
      bail_out(EXIT_FAILURE, "read_from_server");
    }
    
    (void) fprintf(stdout, "Server Response: ");
    (void) print_buffer(buffer, READ_BYTES);
    
    uint8_t red = (buffer[0] & 7);
    uint8_t white = (buffer[0] >> 3) & 7;
    uint8_t parity_check = (buffer[0] >> 6) & 1;
    uint8_t status = (buffer[0] >> 7);
    
    (void) fprintf(stdout, "Status - red: %i, white: %i, parity check: %i, end: %i\n", red, white, parity_check, status);
    if (red == 5) {
      end = 1;
      //(void) fprintf(stdout, "yay success - game finished\n");
      (void) fprintf(stdout, "%i\n", rounds);
    }
    
    if (status) {
      end = 1;
      (void) fprintf(stdout, "game lost - no more rounds :(\n");
    }
    
    
    // knuth alg - remove solutions
    calc_score(guess, solutions , solutions_max , red, white);
    //
    
    // next guess
    int active = 0;
    for (int i = solutions_max-1; i >= 0; --i) {
      if (solutions[i] == 0) {
        active++;
        guess = i;
        break;
      }
    }
    //(void) fprintf(stdout, "found %i active solutions\n", active);
    
  //  int j = 0;
  //  for (int i = 0; i < solutions_max; i++) {
  //    if (solutions[i] == 0) {
  //      j++;
  //      if (j == active) {
  //        guess = i;
  //        break;
  //      }
  //    }  
  //  }
    
  //  int i = 0;
   // while (i < active) {
    //  if (solutions[i] == 0) {
      //  i++; 
   //   }
   // }
   // guess = i;
    
    rounds++;
  }
  
  free_resources();
  
  freeaddrinfo(ai);

  return 0;
}

static void calc_score(uint16_t guess, int *solutions, int n, int red_guess, int white_guess) {
  int red, white;
  int colors_left[COLORS];
  int k = 0;

  for (int sol = 0; sol < n; sol++) {
    red = white = 0;
    (void) memset(&colors_left[0], 0, sizeof(colors_left));
    
    
    if (solutions[sol] == 0) {
      // get red und white for sol[i] == guess
      for (int j = 0; j < SLOTS; j++) {
        int guess_color = (guess >> (j * SHIFT_WIDTH)) & 7;
        int sol_color = (sol >> (j * SHIFT_WIDTH)) & 7;
        if (guess_color == sol_color) {
          red++;
        } else {
          colors_left[guess_color]++;
          //colors_left[sol_color]++;
        }
      }
      for (int j = 0; j < SLOTS; j++) {
        int guess_color = (guess >> (j * SHIFT_WIDTH)) & 7;
        int sol_color = (sol >> (j * SHIFT_WIDTH)) & 7;
        // white colors
        if (guess_color != sol_color) {
          //if (colors_left[guess_color] > 0) {
           if (colors_left[sol_color] > 0) {
            white++;
            colors_left[sol_color]--;
//            colors_left[guess_color]--;
          }
        }
      }
      //(void) fprintf(stdout, "gue: ");
      //print_binary(guess, SLOTS);
      //(void) fprintf(stdout, "sol: ");
      //print_binary(sol, SLOTS);
      //(void) fprintf(stdout, "real score: %ir%iw\n", red_guess, white_guess);
      //(void) fprintf(stdout, "calc score: %ir%iw\n", red, white);
      

      if (red != red_guess || white != white_guess) {
        char code[SLOTS];
        int_to_code(code, sol, SLOTS);
        int test = strncmp("bbbgw",code, 4);
        if (sol == 30208) {
          (void) fprintf(stdout, "ADSASDASDASDADSSADASDASDD");
        }
        if (test == 0) {
          (void) fprintf(stdout, "NOOOOOOOOOOOOO SOL ELIMINATED - %s - %i\n", code, sol);
        }
        solutions[sol] = 1;
        k++;      
      }
      

    }   
    
  }
  //TODO (void) fprintf(stdout, "eliminated: %i\n", k);
}

static void int_to_code(char *code, int guess, int n) 
{
  //char code[n];
  for (int i = 0; i < n; i++) {
    switch ((guess >> i*SHIFT_WIDTH) & 7) {
      case beige:
        code[i] = 'b';
        break;
      case darkblue:
        code[i] = 'd';
        break;
      case green:
        code[i] = 'g';
        break;
      case orange:
        code[i] = 'o';
        break;
      case red:
        code[i] = 'r';
        break;
      case black:
        code[i] = 's';
        break;
      case violet:
        code[i] = 'v';
        break;
      case white:
        code[i] = 'w';
        break;
      default:
        bail_out(EXIT_FAILURE, "Unkown Color - not possible!!");
    }
  }
}


static void print_binary(int guess, int n) 
{
  for (int i = (n*3)-1; i >= 0; i--) {
    (void) fprintf(stdout, "%i", ((guess >> i) & 1));
  }
  (void) fprintf(stdout, "\n");
}

static void print_buffer(uint8_t *buffer, size_t n) 
{
  for (int i = 0; i < n; i++) {
      for (int j = 7; j >= 0; j--) {
        (void) fprintf(stdout, "%i", ((buffer[i] >> j) & 1));
      }
  }
  (void) fprintf(stdout, "\n");
}

static int write_to_server(int fd, uint8_t *buffer, size_t n) 
{
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

static uint8_t *read_from_server(int fd, uint8_t *buffer, size_t n) 
{
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

static uint16_t generate_guess(void) {
  int guess[SLOTS];
  uint16_t req = 0;
  int i;
    
  for (i = 0; i < SLOTS; i++) {
    guess[i] = 0x0;
    //if (i==4) {
    //  guess[i] = 0x7;
    //}
    // calc parity bit
    req ^= (guess[i] & 1) ^ ((guess[i] >> 1) & 1) ^ ((guess[i] >> 2) & 1);
  }
  
  for (i = 0; i < SLOTS; i++) {
    req <<= SHIFT_WIDTH;
    req |= guess[i];
  }

  return req;
}

static void parse_args(int argc, char **argv, struct opts *options) {
  char *hostname_arg;
  char *port_arg;
  char *endptr;
 
  if (argc > 0) {
    progname = argv[0];
  }
  
  if (argc != 3) {
    usage();
  }
  
  hostname_arg = argv[1];
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
  
  options->hostname = hostname_arg;
}

static void bail_out(int exitcode, const char *fmt, ...) {
  (void) fprintf(stderr, "%s: %s", progname, fmt);
  exit(exitcode);
}

static void usage(void) {
  (void) fprintf(stderr, "Usage %s <server-hostname> <server-port>\n", progname); 
  exit(EXIT_FAILURE);
}

static void free_resources(void)
{
    /* clean up resources */
    DEBUG("Shutting down client\n");
    if(sockfd >= 0) {
        (void) close(sockfd);
    }
}
