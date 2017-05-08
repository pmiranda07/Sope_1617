#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>


char* gen_fifo = "/tmp/entrada";
char* rej_fifo = "/tmp/rejeitados";

typedef struct{
  int id;
  char gender;
  int duration;
  int denials;

} Request;
