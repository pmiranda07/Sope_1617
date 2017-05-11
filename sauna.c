#include "request.h"


int capacidade;
int rec_F, rec_M, rej_F, rej_M, done_F, done_M;
char genderUsing;


void* use(void* arg){
  int duracao = *(int*) arg;

  usleep(duracao);
  pthread_exit(NULL);
}

void* requestReader(void* arg){
  int fifo;
  Request* req;

  while ((fifo = open(gen_fifo, O_RDONLY)) == -1){
    if (errno == ENOENT) printf("No generate pipe available! Retrying...\n");
  }
  while(read(fifo, req, sizeof(Request)) != 0){
    pthread_t ticket_tid;
    int duration = req->duration;

    pthread_create(&ticket_tid, NULL, use, (void*) &duration);
  }

  pthread_exit(NULL);
}

int main(int argc, char* argv[]){

  if(argc != 2){
    printf("Sintax must be: sauna <n.lugares>\n");
    exit(-1);
  }

   capacidade = atoi(argv[1]);
   int fifo;
   Request *req = malloc(sizeof(Request));

   printf("\nCapacity: %d\n\n", capacidade);

    while ((fifo = open(gen_fifo, O_RDONLY)) == -1){
      if (errno == ENOENT) printf("No pipe available! Retrying...\n");
      sleep(1);
    }

    while(read(fifo, req, sizeof(Request)) != 0){
      if (req == NULL) exit(-1);
      printf("ID: %d\nGender: %c\nDuration: %d\nDenials: %d\n", req->id, req->gender, req->duration, req->denials);
    }

    return 0;

}
