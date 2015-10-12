#include <stdio.h>
#include <stdlib.h>
//#include <sys/socket.h>
//#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <limits.h>

#define READ_BYTES (1)
#define WRITE_BYTES (2)
#define BUFFER_BYTES (2)

/* === Macros === */

#ifdef ENDDEBUG
#define DEBUG(...) do { fprintf(stderr, __VA_ARGS__); } while(0)
#else
#define DEBUG(...)
#endif

static const char *progname = "client";

static int sockfd = -1;

struct opts {
  char *hostname;
  long int port;
};

void usage(void);

static void parse_args(int argc, char **argv, struct opts *options);

static void bail_out(int exitcode, const char *fmt, ...);

static uint8_t *read_from_server(int sockfd, uint8_t *buffer, size_t n);

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
    return -1;
  }

  if (ai == NULL) {
    (void) fprintf(stderr, "Could not resolve host %s.\n", options.hostname);
    return -1;
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
    return -1;
  }
  
 
  // read
  uint16_t request;
  static uint8_t buffer[BUFFER_BYTES];
  
  
  buffer[0] = 255;
  buffer[1] = 255;
  buffer[2] = 255;
  if (send(sockfd, buffer, WRITE_BYTES, 0) < 0) {
    bail_out(EXIT_FAILURE, "write to server");
  }
  
  
  while (1) {
  }

//  if (read_from_server(sockfd, &buffer[0], READ_BYTES) == NULL) {
  //  //if (quit) break; /* caught signal */
    //bail_out(EXIT_FAILURE, "read_from_server");
//  }
//  request = (buffer[1] << 8) | buffer[0];
  
    DEBUG("Round %d: Received 0x%x\n", round, request); 
 
  
  
  
  
  freeaddrinfo(ai);

  return 0;
}

static void *write_to_server(int sockfd, uint16_t *buffer) {
  send(sockfd, buffer, sizeof(buffer), 0);
}

static uint8_t *read_from_server(int sockfd, uint8_t *buffer, size_t n) {

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

void usage(void) {
  (void) fprintf(stderr, "Usage %s <server-hostname> <server-port>\n", progname); 
  exit(EXIT_FAILURE);
}
