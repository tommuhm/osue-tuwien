#include <stdio.h>
#include <stdlib.h>

const int SIZE = 256;

char *command = "mydiff";

void usage(void);

int main(int argc, char *argv[]) {

	// buffer
	char b1[SIZE];
	char b2[SIZE];
	
	if (argc != 3) {
		usage();
	}

	FILE *file1, *file2;
	if ((file1 = fopen(argv[1], "r")) == NULL) {
		(void) fprintf(stderr, "%s: Datei %s existiert nicht!\n", command, argv[1]);
		exit(EXIT_FAILURE);
	}
	
	if ((file2 = fopen(argv[2], "r")) == NULL) {
		(void) fprintf(stderr, "%s: Datei %s existiert nicht!\n", command, argv[2]);
		exit(EXIT_FAILURE);
	}
 
  int line_count = 0;
  while (fgets(b1, SIZE, file1) != NULL && fgets(b2, SIZE, file2)) {
  		int error_count = 0;
  		
  		line_count++;
  	  	for (int i = 0; i < SIZE && b1[i] != '\0' && b2[i] != '\0' && b1[i] != '\n' && b2[i] != '\n'; i++) {
  			if (b1[i] != b2[i]) {
  				error_count++;
  			}
  		}
  		if (error_count > 0) {
  			(void) printf("Zeile: %i Zeichen: %i\n", line_count, error_count);
  		}
  } 
 
	fclose(file1);
	fclose(file2);

	exit(EXIT_SUCCESS);
}

void usage(void) {
	(void) fprintf(stderr, "Usage: %s file1 file2\n", command);
	exit(EXIT_FAILURE);
}

