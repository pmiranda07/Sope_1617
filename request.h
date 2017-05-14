#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <sys/file.h>

char* gen_fifo = "/tmp/entrada";
char* rej_fifo = "/tmp/rejeitados";

typedef struct{
  int id;
  char gender;
  int duration;
  int denials;

} Request;
