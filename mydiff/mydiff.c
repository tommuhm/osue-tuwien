/** 
 *  mydiff
 *
 *  @author Thomas Muhm
 *
 *  @brief compares 2 files and prints the number of differences for each line
 *
 *  @details 
 *    compares 2 files line by line and print die number of different characters for each line
 *    compares only to the end of the shorter line and to the end of the shorter file    
 *
 *  @date 17.10.2015
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// max line length (buffer size)
const int SIZE = 1024;

// command name - used for usage output
const char *COMMAND = "mydiff";

/**
  * closes open file descriptors and exits the program
  */
static void bail_out(FILE *file1, FILE *file, int exit_code, char *argv[]);

/**
  * print formated error code for filname
  */
static void print_error_code(char *filename, int errorno);

/** 
 * mydiff file1 file2
 *
 * compares file1 and file2 line by line
 * prints a message for each line where differenzen where found
 *
 */
int main(int argc, char *argv[]) {

  // line buffer for each file
  char b1[SIZE];
  char b2[SIZE];
  
  // usage check
  if (argc != 3) {
    (void) fprintf(stderr, "Usage: %s file1 file2\n", COMMAND);
    exit(EXIT_FAILURE);
  }

  // try to open files for read and exit on error
  FILE *file1, *file2;
  if ((file1 = fopen(argv[1], "r")) == NULL) {
    (void) fprintf(stderr, "%s: Datei %s existiert nicht!\n", COMMAND, argv[1]);
    exit(EXIT_FAILURE);
  }
  
  if ((file2 = fopen(argv[2], "r")) == NULL) {
    (void) fprintf(stderr, "%s: Datei %s existiert nicht!\n", COMMAND, argv[2]);
    if (fclose(file1) != 0) {
      (void) fprintf(stderr, "%s: %s - %s\n", COMMAND, argv[2], strerror(errno));
    } 
    exit(EXIT_FAILURE);
    }

  int line_count = 0;
  while (fgets(b1, SIZE, file1) != NULL && fgets(b2, SIZE, file2) != NULL) {
    int error_count = 0;

    line_count++;
    // count erros until null or new line symbol
    for (int i = 0; i < SIZE && b1[i] != '\0' && b2[i] != '\0' && b1[i] != '\n' && b2[i] != '\n'; i++) {
      if (b1[i] != b2[i]) {
        error_count++;
      }
    }
    if (error_count > 0) {
      (void) printf("Zeile: %i Zeichen: %i\n", line_count, error_count);
    }
  }
  
  // check if fgets got an error
  if (ferror(file1) != 0) {
    print_error_code(argv[1], errno);
    bail_out(file1, file2, EXIT_FAILURE, argv);
  }
  if (ferror(file2) != 0) {
    print_error_code(argv[2], errno);
    bail_out(file1, file2, EXIT_FAILURE, argv);
  }

  bail_out(file1, file2, EXIT_SUCCESS, argv);
}

static void bail_out(FILE *file1, FILE *file2, int exit_code, char *argv[]) {
  errno = 0;
  if (file1 != NULL && fclose(file1) != 0) {
    print_error_code(argv[1], errno);
    file1 = NULL;
  }
  if (file2 != NULL && fclose(file2) != 0) {
    print_error_code(argv[2], errno);
    file2 = NULL;
  }
  
  if (exit_code != 0 || errno != 0) {
    exit(EXIT_FAILURE);
  } else {
    exit(EXIT_SUCCESS);
  }
}

static void print_error_code(char *filename, int errorno) {
  (void) fprintf(stderr, "%s: %s - %s\n", COMMAND, filename, strerror(errno));
}

