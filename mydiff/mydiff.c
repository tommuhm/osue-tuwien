#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

const int line_length = 256;

void usage(void);

void error_exit(const char *msg);

int main(int argc, char *argv[]) {

  int c;
  int aflag = 0;
  int bflag = 0;

  while((c = getopt(argc, argv, "ab")) != -1) {
    switch(c) {
      case 'a':
        aflag = 1;
        printf("first par set"); 
        break;
      case 'b':
        printf("second par set");
        bflag = 1;
        break;
      case '?':
       printf("unkown option");
       error_exit("test ?");
       break;
      default:
        assert(0);  // imposible
    }
  }

  FILE *file1, *file2;
  //file1 = fopen(argv[1], "r");
  //file2 = fopen(argv[2], "r");

  
 
  //fclose(file1);
  //fclose(file2);



  exit(0);
}

void usage(void) {
}

void error_exit(const char *msg) {
  //(void) printf("mydiff: %s",  msg);
  exit(-1);
}
