#include "request.h"

int id=1;
int duracaoMax;
Request* requests[];
int gen_M, gen_F, rej_M, rej_F, del_M, del_F;
char* msg;


void* request_gen(void* arg){

  int numr = *(int*) arg;

  for (int i = 0; i < numr; i++){
    Request* request = malloc(sizeof(Request));

    request->id = id++;
    request->gender = (rand() % 2) ? 'M' : 'F';
    request->duration = rand() % duracaoMax + 1;
    request->denials = 0;

    requests[i] = request;

  }
  pthread_exit(NULL);
}

void* rejLis(void* arg){

  int fifo;
  Request* req;

  while ((fifo = open(rej_fifo, O_RDONLY)) == -1){
    if (errno == ENOENT) printf("No rejected pipe available! Retrying...\n");
    sleep(2);
  }

  while(read(fifo, req, sizeof(Request)) != 0){
    printf("REJECTED\nID: %d\nGender: %c\nDuration: %d\nDenials: %d\n", req->id, req->gender, req->duration, req->denials);
  }

  write(fifo, req, sizeof(*req));
  pthread_exit(NULL);
}


int main(int argc, char* argv[]){

  if(argc != 3){
    printf("Sintax must be: gerador <numero de pedidos> <max.utilizacao>\n");
    exit(-1);
  }

  char pid[32], gerStr[64] = "/tmp/ger.";
  sprintf(pid, "%d", getpid());
  strcat(gerStr, pid);
  msg = (char*) malloc(strlen(gerStr) + 1);
  strcpy(msg, gerStr);

  int nrequests = atoi(argv[1]);
  duracaoMax = atoi(argv[2]);

  int fifo;
  time_t t;
  srand((unsigned) time(&t));

    pthread_t gen_tid, req_tid;

    pthread_create(&gen_tid, NULL, request_gen, (void*) &nrequests);
    pthread_create(&req_tid, NULL, rejLis, NULL);

    while ((fifo = open(gen_fifo, O_WRONLY | O_NONBLOCK)) == -1){
      if (errno != ENXIO){
        perror("Error opening fifo for write");
        exit(-1);
      }
    }

   for(int i = 0; i < nrequests; i++){
       Request* req = requests[i];
       printf("ID: %d\nGender: %s\nDuration: %d\nDenials: %d\n", req->id, &req->gender, req->duration, req->denials);
       write(fifo, req, sizeof(*req));
   }

     unlink(gen_fifo);


}
