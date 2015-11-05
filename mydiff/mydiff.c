/** 
 *  mydiff
 *
 *  @author Thomas Muhm 1326486
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

static FILE *file1, *file2;

/**
  * bail_out
  * @brief closes open resources and exits the program
  * @param exit_code programm exits with this code
  */
static void bail_out(int exit_code);

/** 
  * close_resources
  * @brief closes all open resources
  */
static void close_resources(void);

/**
 * main
 * @brief Program entry point
 * @param argc The argument counter
 * @param argv The argument vector
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on error
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
  if ((file1 = fopen(argv[1], "r")) == NULL) {
    (void) fprintf(stderr, "%s: Datei %s existiert nicht!\n", COMMAND, argv[1]);
    bail_out(EXIT_FAILURE);
  }
  
  if ((file2 = fopen(argv[2], "r")) == NULL) {
    (void) fprintf(stderr, "%s: Datei %s existiert nicht!\n", COMMAND, argv[2]);
    bail_out(EXIT_FAILURE);
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
  if (ferror(file1) != 0 || ferror(file2) != 0) {
    bail_out(EXIT_FAILURE);
  }

  bail_out(EXIT_SUCCESS);
}

static void bail_out(int exit_code) {
  close_resources();
  exit(exit_code);
}

static void close_resources(void) {
  if (file1 != NULL) {
    fclose(file1);
    file1 = NULL;
  } 
  if (file2 != NULL) {
    fclose(file2);
    file2 = NULL;
  }
}
